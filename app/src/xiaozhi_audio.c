/**
  ******************************************************************************
  * @file   wheather.c
  * @author Sifli software development team
  ******************************************************************************
*/
/**
 * @attention
 * Copyright (c) 2024 - 2025,  Sifli Technology
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Sifli integrated circuit
 *    in a product or a software update for such product, must reproduce the above
 *    copyright notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Sifli nor the names of its contributors may be used to endorse
 *    or promote products derived from this software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Sifli integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY SIFLI TECHNOLOGY "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SIFLI TECHNOLOGY OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <rtthread.h>
#include "lwip/api.h"
#include "lwip/dns.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/apps/mqtt.h"
#include "lwip/udp.h"
#include "lwip/tcpip.h"
#include "xiaozhi.h"
#include "bf0_hal.h"
#include "button.h"
#include "os_adaptor.h"
#include "opus_types.h"
#include "opus_multistream.h"
#include "os_support.h"
#include "audio_server.h"
#include "mem_section.h"

#undef LOG_TAG
#define LOG_TAG           "xz"
#define DBG_TAG           "xz"
#define DBG_LVL           LOG_LVL_INFO
#include "log.h"

#define XZ_THREAD_NAME          "xiaozhi"
#define XZ_OPUS_STACK_SIZE      220000
#define XZ_EVENT_MIC_RX         (1 << 0)
#define XZ_EVENT_SPK_TX         (1 << 1)
#define XZ_EVENT_DOWNLINK       (1 << 2)

#define XZ_MIC_FRAME_LEN        (320 * 6) //60ms for 16k samplerate
#define XZ_SPK_FRAME_LEN        (480 * 6) //60ms for 24k samplerate, speaker frame len

#define XZ_EVENT_ALL            (XZ_EVENT_MIC_RX | XZ_EVENT_SPK_TX | XZ_EVENT_DOWNLINK)

#define XZ_DOWNLINK_QUEUE_NUM   64

typedef struct
{
    rt_slist_t  node;
    uint8_t     *data;
    uint16_t    data_len;
    uint16_t    size;
} xz_decode_queue_t;

typedef struct
{
    struct rt_thread        thread;
    uint8_t                 encode_in[XZ_MIC_FRAME_LEN];
    uint8_t                 encode_out[XZ_MIC_FRAME_LEN];
    bool                    inited;
    bool                    is_rx_enable;
    bool                    is_tx_enable;
    bool                    is_exit;
    uint8_t                 is_websocket; //0: mqtt, 1: websocket
    rt_event_t              event;
    OpusEncoder             *encoder;
    OpusDecoder             *decoder;
    audio_client_t          speaker;
    audio_client_t          mic;
    uint32_t                mic_rx_count;
    struct rt_ringbuffer    *rb_opus_encode_input;
    xz_decode_queue_t       downlink_queue[XZ_DOWNLINK_QUEUE_NUM];
    uint16_t                downlink_decode_out[XZ_SPK_FRAME_LEN / 2];
    rt_slist_t              downlink_decode_busy;
    rt_slist_t              downlink_decode_idle;
} xz_audio_t;

struct udp_pcb *udp_pcb;
static xz_audio_t xz_audio;

#if defined(__CC_ARM) || defined(__CLANG_ARM)
    L2_RET_BSS_SECT_BEGIN(g_xz_opus_stack)
    static  uint32_t g_xz_opus_stack[XZ_OPUS_STACK_SIZE / sizeof(uint32_t)];
    L2_RET_BSS_SECT_END
#else
    static  uint32_t g_xz_opus_stack[XZ_OPUS_STACK_SIZE / sizeof(uint32_t)] L2_RET_BSS_SECT(g_xz_opus_stack);
#endif


static void xz_opus_thread_entry(void *p);
void xz_speaker_open(xz_audio_t *thiz);
void xz_speaker_close(xz_audio_t *thiz);
void xz_mic_open(xz_audio_t *thiz);
void xz_mic_close(xz_audio_t *thiz);
static void audio_write_and_wait(xz_audio_t *thiz, uint8_t *data, uint32_t data_len);

extern void xiaozhi_ui_chat_status(char *string);
extern void xz_audio_send_using_websocket(uint8_t *data, int len);//发送音频数据

void xz_audio_send(uint8_t *data, int len)
{
    uint32_t nonce[4];
    uint8_t *p_nonce = (uint8_t *) & (nonce[0]);

    memcpy(p_nonce, &(g_xz_context.nonce[0]), sizeof(nonce));
    *(uint16_t *)(p_nonce + 2) = htons(len);
    *(uint32_t *)(p_nonce + 12) = htonl(++g_xz_context.local_sequence);
    // Encrypt data
    HAL_AES_init((uint32_t *) & (g_xz_context.key[0]), 16, (uint32_t *) & (nonce[0]), AES_MODE_CTR);
    struct pbuf *pbuf = pbuf_alloc(PBUF_TRANSPORT, len + sizeof(nonce) + 32, PBUF_RAM);
    if (pbuf && g_xz_context.port)
    {
        uint8_t *payload = (uint8_t *)pbuf->payload;
        memcpy(payload, nonce, sizeof(nonce));
        payload += sizeof(nonce);
        HAL_AES_run(AES_ENC, data, payload, len);
        LOCK_TCPIP_CORE();
        udp_sendto(udp_pcb, pbuf, &(g_xz_context.udp_addr), g_xz_context.port);
        UNLOCK_TCPIP_CORE();
        pbuf_free(pbuf);
    }
}

void xz_udp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                 const ip_addr_t *addr, u16_t port)
{
    if (memcmp(addr, &(g_xz_context.udp_addr), sizeof(ip_addr_t)) == 0 && port == g_xz_context.port)
    {
        uint8_t *data = (uint8_t *)p->payload;
        uint32_t nonce[4];
        uint32_t size = 0;
        uint32_t sequence = ntohl(*(uint32_t *)&data[12]);

        if (p->len < sizeof(nonce))
        {
            rt_kprintf("Invalid audio packet size: %u\n", p->len);
            goto end;
        }
        if (data[0] != 0x01)
        {
            rt_kprintf("Invalid audio packet type: %x", data[0]);
            goto end;
        }
        if (sequence < g_xz_context.remote_sequence)
        {
            rt_kprintf("Received audio packet with old sequence: %lu, expected: %lu\n", sequence, g_xz_context.remote_sequence);
            goto end;
        }
        if (sequence !=  g_xz_context.remote_sequence + 1)
        {
            rt_kprintf("Received audio packet with wrong sequence: %lu, expected: %lu\n", sequence,  g_xz_context.remote_sequence + 1);
        }
        else
        {
            g_xz_context.remote_sequence = sequence;
        }
        memcpy(&(nonce[0]), data, sizeof(nonce));
        data += sizeof(nonce);
        size = p->len - sizeof(nonce);
        xz_audio_downlink(data, size, &nonce[0], 1);
end:
        pbuf_free(p);
    }
}

static int mic_callback(audio_server_callback_cmt_t cmd, void *callback_userdata, uint32_t reserved)
{
    //this was called every 10ms
    xz_audio_t *thiz = &xz_audio;

    if (thiz->is_rx_enable && cmd == as_callback_cmd_data_coming)
    {
        //data lengh is 320 bytes, which is 10ms for 16k samplerate
        audio_server_coming_data_t *p = (audio_server_coming_data_t *)reserved;
        rt_ringbuffer_put(thiz->rb_opus_encode_input, p->data, p->data_len);
        thiz->mic_rx_count += 320;

        if (thiz->mic_rx_count >= XZ_MIC_FRAME_LEN)
        {
            thiz->mic_rx_count = 0;
            rt_event_send(thiz->event, XZ_EVENT_MIC_RX);
        }
    }
    return 0;
}

void xz_mic(int on)
{
    // Sample rate is 16K, each packet should be 60ms
    xz_audio_t *thiz = &xz_audio;
    if (on)
    {
        xz_mic_open(thiz);
    }
    else
    {
        xz_mic_close(thiz);
    }

}

void xz_speaker(int on)
{
    // Sample rate is g_xz_context.sample_rate(typical 24K),
    // each packet should be g_xz_context.frame_duration(typical 60ms)
    xz_audio_t *thiz = &xz_audio;
    if (on)
    {
        xz_speaker_open(thiz);
    }
    else
    {
        xz_speaker_close(thiz);
    }

}

#if 0
static void xz_button_event_handler(int32_t pin, button_action_t action)
{
    rt_kprintf("button(%d) %d:", pin, action);

    if (g_state == kDeviceStateUnknown)//goodby唤醒
        {
            xiaozhi_ui_chat_status("\u5524\u9192\u4e2d...");
            mqtt_hello(&g_xz_context);
            if (action == BUTTON_PRESSED)
            {
                rt_kprintf("pressed\r\n");
                if (g_state == kDeviceStateSpeaking)
                    mqtt_speak_abort(&g_xz_context, kAbortReasonWakeWordDetected);
                mqtt_listen_start(&g_xz_context, kListeningModeAutoStop);
                xiaozhi_ui_chat_status("\u8046\u542c\u4e2d...");
                xz_mic(1);
            }
            else if (action == BUTTON_RELEASED)
            {
                rt_kprintf("released\r\n");
                xiaozhi_ui_chat_status("\u5f85\u547d\u4e2d...");
                xz_mic(0);
                mqtt_listen_stop(&g_xz_context);
            }   
        }
        else
        {
            if (action == BUTTON_PRESSED)
            {
                rt_kprintf("pressed\r\n");
                if (g_state == kDeviceStateSpeaking)
                    mqtt_speak_abort(&g_xz_context, kAbortReasonWakeWordDetected);
                mqtt_listen_start(&g_xz_context, kListeningModeAutoStop);
                xiaozhi_ui_chat_status("\u8046\u542c\u4e2d...");
                xz_mic(1);
            }
            else if (action == BUTTON_RELEASED)
            {
                rt_kprintf("released\r\n");
                xiaozhi_ui_chat_status("\u5f85\u547d\u4e2d...");
                xz_mic(0);
                mqtt_listen_stop(&g_xz_context);
            }   
        }
}


void xz_button_init(void)
{
    static int initialized = 0;

    if (initialized == 0)
    {
        button_cfg_t cfg;
        cfg.pin = BSP_KEY2_PIN;

        cfg.active_state = BSP_KEY2_ACTIVE_HIGH;
        cfg.mode = PIN_MODE_INPUT;
        cfg.button_handler = xz_button_event_handler;
        int32_t id = button_init(&cfg);
        RT_ASSERT(id >= 0);
        RT_ASSERT(SF_EOK == button_enable(id));
        initialized = 1;
    }
}


void xz_audio_init()
{
    rt_kprintf("xz_audio_init\n");

    if (udp_pcb)
    {
        udp_remove(udp_pcb);
        udp_pcb = NULL;
    }
    audio_server_set_private_volume(AUDIO_TYPE_LOCAL_MUSIC, 6);
    xz_audio_decoder_encoder_open(0);

    xz_button_init();
    udp_pcb = udp_new();
    g_xz_context.local_sequence = 0;
    g_xz_context.remote_sequence = 0;
    udp_recv(udp_pcb, xz_udp_recv, NULL);
}
#endif
static void audio_write_and_wait(xz_audio_t *thiz, uint8_t *data, uint32_t data_len)
{
    int ret;
    while (!thiz->is_exit)
    {
        ret = audio_write(thiz->speaker, data, data_len);
        if (ret)
        {
            break;
        }

        rt_thread_mdelay(10);
    }
}
static void xz_opus_thread_entry(void *p)
{
    int err;
    xz_audio_t *thiz = &xz_audio;
    thiz->decoder = opus_decoder_create(24000, 1, &err);
    RT_ASSERT(thiz->decoder);

    thiz->encoder = opus_encoder_create(16000, 1, OPUS_APPLICATION_VOIP, &err);
    RT_ASSERT(thiz->encoder);
    opus_encoder_ctl(thiz->encoder, OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_60_MS));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_VBR(1));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_VBR_CONSTRAINT(1));

    opus_encoder_ctl(thiz->encoder, OPUS_SET_BITRATE(16000));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_COMPLEXITY(0));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_LSB_DEPTH(16));

    opus_encoder_ctl(thiz->encoder, OPUS_SET_DTX(0));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_INBAND_FEC(0));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_PACKET_LOSS_PERC(0));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_PREDICTION_DISABLED(0));

    opus_encoder_ctl(thiz->encoder, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_WIDEBAND));
    opus_encoder_ctl(thiz->encoder, OPUS_SET_BANDWIDTH(OPUS_AUTO));

    while (!thiz->is_exit)
    {
        rt_uint32_t evt = 0;
        rt_event_recv(thiz->event, XZ_EVENT_ALL, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &evt);
        if ((evt & XZ_EVENT_MIC_RX) && thiz->is_rx_enable)
        {

            rt_ringbuffer_get(thiz->rb_opus_encode_input, (uint8_t *)&thiz->encode_in[0], XZ_MIC_FRAME_LEN);

            opus_int32 len = opus_encode(thiz->encoder, (const opus_int16 *)thiz->encode_in, XZ_MIC_FRAME_LEN / 2, thiz->encode_out, XZ_MIC_FRAME_LEN);
            if (len < 0 || len > XZ_MIC_FRAME_LEN)
            {
                LOG_I("opus_encode len=%d samrate=%d", len, g_xz_context.sample_rate);
                RT_ASSERT(0);
            }

            // xz_audio_send(thiz->encode_out, len);
            xz_audio_send_using_websocket(thiz->encode_out, len);//发送音频数据

            if (rt_ringbuffer_data_len(thiz->rb_opus_encode_input) >= XZ_MIC_FRAME_LEN)
            {
                rt_event_send(thiz->event, XZ_EVENT_MIC_RX);
            }
        }

        if (evt & XZ_EVENT_SPK_TX)
        {
        }

        if ((evt & XZ_EVENT_DOWNLINK) && thiz->is_tx_enable)
        {
            rt_slist_t *decode;
            rt_enter_critical();
            decode = rt_slist_first(&thiz->downlink_decode_busy);
            rt_exit_critical();
            RT_ASSERT(decode);
            xz_decode_queue_t *queue = rt_container_of(decode, xz_decode_queue_t, node);
            opus_int32 res = opus_decode(thiz->decoder,
                                         (const uint8_t *)queue->data,
                                         queue->data_len,
                                         (opus_int16 *)&thiz->downlink_decode_out[0],
                                         XZ_SPK_FRAME_LEN, 0);

            if (res != XZ_SPK_FRAME_LEN / 2)
            {
                LOG_I("decode out samples=%d\n", res);
                RT_ASSERT(0);
            }

            audio_write_and_wait(thiz, (uint8_t *)thiz->downlink_decode_out, XZ_SPK_FRAME_LEN);

            uint8_t need_decode_gain = 0;
            rt_enter_critical();
            rt_slist_remove(&thiz->downlink_decode_busy, decode);
            rt_slist_append(&thiz->downlink_decode_idle, decode);
            if (rt_slist_first(&thiz->downlink_decode_busy))
            {
                need_decode_gain = true;
            }
            rt_exit_critical();

            if (need_decode_gain)
                rt_event_send(thiz->event, XZ_EVENT_DOWNLINK);

        }

    }
    if (thiz->encoder)
        opus_encoder_destroy(thiz->encoder);
    if (thiz->decoder)
        opus_decoder_destroy(thiz->decoder);

    rt_kprintf("---xz thread exit---\r\n");
}

void xz_mic_open(xz_audio_t *thiz)
{
    if (!thiz->mic)
    {
        LOG_I("mic on");
        while (1)
        {
            uint8_t buf[128];
            int len = rt_ringbuffer_get(thiz->rb_opus_encode_input, (uint8_t *)&buf[0], sizeof(buf));
            if (len == 0)
            {
                break;
            }
        }

        audio_parameter_t pa = {0};
        pa.write_bits_per_sample = 16;
        pa.write_channnel_num = 1;
        pa.write_samplerate = 24000;
        pa.read_bits_per_sample = 16;
        pa.read_channnel_num = 1;
        pa.read_samplerate = 16000;
        pa.read_cache_size = 0;
        pa.write_cache_size = 0;
        thiz->mic = audio_open(AUDIO_TYPE_LOCAL_RECORD, AUDIO_RX, &pa, mic_callback, NULL);
        RT_ASSERT(thiz->mic);
        thiz->is_rx_enable = 1;
    }
}

void xz_mic_close(xz_audio_t *thiz)
{
    if (thiz->mic)
    {
        LOG_I("mic off");
        audio_close(thiz->mic);
        thiz->mic = NULL;
        thiz->is_rx_enable = 0;
    }
}

void xz_speaker_open(xz_audio_t *thiz)
{
    if (!thiz->speaker)
    {
        LOG_I("speaker on");
        xiaozhi_ui_chat_status("\u8bb2\u8bdd\u4e2d...");
        audio_parameter_t pa = {0};
        pa.write_bits_per_sample = 16;
        pa.write_channnel_num = 1;
        pa.write_samplerate = 24000;
        pa.read_bits_per_sample = 16;
        pa.read_channnel_num = 1;
        pa.read_samplerate = 16000;
        pa.read_cache_size = 0;
        pa.write_cache_size = 16000;
        thiz->speaker = audio_open(AUDIO_TYPE_LOCAL_MUSIC, AUDIO_TX, &pa, NULL, NULL);
        RT_ASSERT(thiz->speaker);
        thiz->is_tx_enable = 1;
    }
}
void xz_speaker_close(xz_audio_t *thiz)
{
    LOG_I("speaker off");
    xiaozhi_ui_chat_status("\u5f85\u547d\u4e2d...");
    if (thiz->speaker)
    {
        uint32_t cache_time_ms = 150;
        audio_ioctl(thiz->speaker, 1, &cache_time_ms);
        rt_thread_mdelay(cache_time_ms + 20);
        audio_close(thiz->speaker);
        thiz->speaker = NULL;
        thiz->is_tx_enable = 0;
        rt_slist_t *idle;
        rt_enter_critical();
        while (1)
        {
            idle = rt_slist_first(&thiz->downlink_decode_busy);
            if (idle)
            {
                rt_slist_remove(&thiz->downlink_decode_busy, idle);
                rt_slist_append(&thiz->downlink_decode_idle, idle);
            }
            else
            {
                break;
            }
        }
        rt_exit_critical();
    }
}

/**
 * @brief 打开音频解码器和编码器
 * 
 * 该函数负责初始化音频处理模块，包括事件创建、队列初始化、线程创建等
 * 如果模块尚未初始化，则根据参数决定是否使用WebSocket，并准备音频处理线程
 * 
 * @param is_websocket 指示是否使用WebSocket的标志
 */
void xz_audio_decoder_encoder_open(uint8_t is_websocket)
{
    // 获取音频处理模块的实例
    xz_audio_t *thiz = &xz_audio;

    // 检查模块是否已经初始化，避免重复初始化
    if (!thiz->inited)
    {
        // 根据参数设置是否使用WebSocket
        thiz->is_websocket = is_websocket;

        // 创建事件，用于音频处理中的同步
        thiz->event = rt_event_create("xzaudio", RT_IPC_FLAG_FIFO);
        RT_ASSERT(thiz->event);

        // 初始化空闲和忙状态的解码队列
        rt_slist_init(&thiz->downlink_decode_idle);
        rt_slist_init(&thiz->downlink_decode_busy);

        // 为下行链路解码队列分配内存
        for (int i = 0; i < XZ_DOWNLINK_QUEUE_NUM; i++)
        {
            thiz->downlink_queue[i].size = 256;
            thiz->downlink_queue[i].data = rt_malloc(thiz->downlink_queue[i].size);
            RT_ASSERT(thiz->downlink_queue[i].data);

            // 将新分配的队列节点添加到空闲队列中
            rt_slist_append(&thiz->downlink_decode_idle, &thiz->downlink_queue[i].node);
        }

        // 创建用于编码输入的环形缓冲区
        thiz->rb_opus_encode_input  = rt_ringbuffer_create(XZ_MIC_FRAME_LEN * 2); // two frame
        RT_ASSERT(thiz->rb_opus_encode_input);

        // 初始化音频处理线程
        rt_err_t err;
        err = rt_thread_init(&thiz->thread,
                             XZ_THREAD_NAME,
                             xz_opus_thread_entry,//音频处理线程入口函数
                             NULL,
                             g_xz_opus_stack,
                             sizeof(g_xz_opus_stack),
                             RT_THREAD_PRIORITY_MIDDLE + RT_THREAD_PRIORITY_HIGHER,
                             RT_THREAD_TICK_DEFAULT);

        // 启动音频处理线程
        rt_thread_startup(&thiz->thread);

        // 标记模块已初始化
        thiz->inited = 1;
    }
}

void xz_audio_decoder_encoder_close(void)
{
    xz_audio_t *thiz = &xz_audio;

    thiz->is_exit = 1;
    while (rt_thread_find(XZ_THREAD_NAME))
    {
        LOG_I("wait thread %s eixt", XZ_THREAD_NAME);
        os_delay(100);
    }

    rt_ringbuffer_destroy(thiz->rb_opus_encode_input);
    rt_event_delete(thiz->event);

    for (int i = 0; i < XZ_DOWNLINK_QUEUE_NUM; i++)
    {
        if (thiz->downlink_queue[i].data)
        {
            rt_free(thiz->downlink_queue[i].data);
            thiz->downlink_queue[i].data = NULL;
        }
    }
}

void xz_audio_downlink(uint8_t *data, uint32_t size, uint32_t *aes_value, uint8_t need_aes)
{
    xz_audio_t *thiz = &xz_audio;
    rt_slist_t *idle;
    rt_enter_critical();
    idle = rt_slist_first(&thiz->downlink_decode_idle);
    rt_exit_critical();
    if (idle)
    {
        xz_decode_queue_t *queue = rt_container_of(idle, xz_decode_queue_t, node);
        if (queue->size < size + 16)
        {
            rt_free(queue->data);
            queue->size = size + 16;
            queue->data = rt_malloc(queue->size);
            RT_ASSERT(queue->data);
        }
        queue->data_len = size;
        if (need_aes)
        {
            HAL_AES_init((uint32_t *) & (g_xz_context.key[0]), 16, aes_value, AES_MODE_CTR);
            HAL_AES_run(AES_DEC, data, queue->data, size);
        }
        else
        {
            memcpy(queue->data, data, size);
        }
        rt_enter_critical();
        rt_slist_remove(&thiz->downlink_decode_idle, idle);
        rt_slist_append(&thiz->downlink_decode_busy, idle);
        rt_exit_critical();

        rt_event_send(thiz->event, XZ_EVENT_DOWNLINK);
    }
    else
    {
        LOG_I("speaker busy\r\n");
    }
}

/************************ (C) COPYRIGHT Sifli Technology *******END OF FILE****/

