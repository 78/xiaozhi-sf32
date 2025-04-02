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
#include "lwip/apps/websocket_client.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/apps/mqtt.h"
#include "xiaozhi.h"
#include "bf0_hal.h"
#include "bts2_global.h"
#include "bts2_app_pan.h"
#include <cJSON.h>
#include "button.h"
#include "audio_server.h"

#define MAX_WSOCK_HDR_LEN 512
extern void xiaozhi_ui_update_ble(char *string);
extern void xiaozhi_ui_chat_status(char *string);
extern void xiaozhi_ui_chat_output(char *string);
extern void xiaozhi_ui_update_emoji(char *string);
/**
 * @brief xiaozhi websocket cntext 数据结构体
 */
typedef struct
{
    uint32_t  sample_rate;
    uint32_t frame_duration;
    uint8_t  session_id[12];
    wsock_state_t  clnt;
    rt_sem_t sem;
    uint8_t  is_connected;
} xiaozhi_ws_t;
xiaozhi_context_t g_xz_context;
xiaozhi_ws_t g_xz_ws;
char mac_address_string[20];
enum DeviceState g_state;
static char message[256];
char client_id_string[40];
ALIGN(4) uint8_t g_sha256_result[32] = {0};
extern BOOL g_pan_connected;
const char *mode_str[] =
{
    "auto",
    "manual",
    "realtime"
};

static const char *hello_message = "{"
                                   "\"type\":\"hello\","
                                   "\"version\": 3,"
                                   "\"transport\":\"websocket\","
                                   "\"audio_params\":{"
                                   "\"format\":\"opus\", \"sample_rate\":16000, \"channels\":1, \"frame_duration\":60"
                                   "}}";

/**
 * @brief Do hash , Single calculation, polling mode.
 * @param algo HASH Algorithm type.
 * @param raw_data Input data.
 * @param raw_data_len Input data len.
 * @param result Output data.
 * @param result_len Output data len.
 *
 * @retval none
 */
void hash_run(uint8_t algo, uint8_t *raw_data, uint32_t raw_data_len,
    uint8_t *result, uint32_t result_len)
{
/* Rest hash block. */
HAL_HASH_reset();
/* Initialize AES Hash hardware block. */
HAL_HASH_init(NULL, algo, 0);
/* Do hash. HAL_HASH_run will block until hash finish. */
HAL_HASH_run(raw_data, raw_data_len, 1);
/* Get hash result. */
HAL_HASH_result(result);
}
void hex_2_asc(uint8_t n, char *str)
{
    uint8_t i=(n>>4);
    if (i>=10)
        *str= i+'a'-10;
    else
        *str= i+'0';
    str++, i=n&0xf;
    if (i>=10)
        *str= i+'a'-10;
    else
        *str= i+'0';    
}

char *get_client_id()
{    
    if (client_id_string[0] == '\0') {
        int i,j=0;
        BTS2S_ETHER_ADDR   addr = bt_pan_get_mac_address(NULL);
        hash_run(HASH_ALGO_SHA256, (uint8_t*)&addr, sizeof(addr), g_sha256_result, sizeof(g_sha256_result));
        for (i=0;i<16;i++,j+=2) {
            //12345678-1234-1234-1234-123456789012
            if (i==4||i==6||i==8||i==10) {
                client_id_string[j++]='-';
            }                
            hex_2_asc(g_sha256_result[i],&client_id_string[j]);
        }
        rt_kprintf(client_id_string);
    }
    return (&(client_id_string[0]));    
}
                        
char *get_mac_address()
{
    if (mac_address_string[0] == '\0')
    {
        BTS2S_ETHER_ADDR   addr = bt_pan_get_mac_address(NULL);
        uint8_t *p = (uint8_t *) & (addr);

        rt_snprintf((char *)mac_address_string, 20, "%02x:%02x:%02x:%02x:%02x:%02x",
                    *p, *(p + 1), *(p + 2), *(p + 3), *(p + 4), *(p + 5));
    }
    return (&(mac_address_string[0]));
}                                   

void parse_helLo(const u8_t *data, u16_t len);
void ws_send_listen_start(void *ws, char *session_id, enum ListeningMode mode)
{
    rt_kprintf("listen start\n");
    rt_snprintf(message, 256, "{\"session_id\":\"%s\",\"type\":\"listen\",\"state\":\"start\",\"mode\":\"%s\"}",
                session_id, mode_str[mode]);
    //rt_kputs("\r\n");
    //rt_kputs(message);
    //rt_kputs("\r\n");
    if (g_xz_ws.is_connected == 1) 
    {
        wsock_write((wsock_state_t *)ws, message, strlen(message), OPCODE_TEXT);
    }
    else
    {
        rt_kprintf("websocket is not connected\n");
    }
    
}

void ws_send_listen_stop(void *ws, char *session_id)
{
    rt_kprintf("listen stop\n");
    rt_snprintf(message, 256, "{\"session_id\":\"%s\",\"type\":\"listen\",\"state\":\"stop\"}",
                session_id);
    if (g_xz_ws.is_connected == 1) 
    {
        wsock_write((wsock_state_t *)ws, message, strlen(message), OPCODE_TEXT);
    }
    else
    {
        rt_kprintf("websocket is not connected\n");
    }
}
void ws_send_hello(void *ws)
{
    if (g_xz_ws.is_connected == 1) 
    {
        wsock_write((wsock_state_t *)ws, hello_message, strlen(hello_message), OPCODE_TEXT);
    }
    else
    {
        rt_kprintf("websocket is not connected\n");
    }
}
void xz_audio_send_using_websocket(uint8_t *data, int len)
{
    if (g_xz_ws.is_connected == 1) 
    {   
        err_t err = wsock_write(&g_xz_ws.clnt, data, len, OPCODE_BINARY);
        rt_kprintf("send audio = %d len=%d\n", err, len);
    }
    else
        rt_kprintf("Websocket disconnected\n");
}

err_t my_wsapp_fn(int code, char *buf, size_t len)
{
    if (code == WS_CONNECT)
    {
        rt_kprintf("websocket connected\n");
        int status = (uint16_t)(uint32_t)buf;
        if (status == 101)  // wss setup success
        {
            rt_sem_release(g_xz_ws.sem);
            g_xz_ws.is_connected = 1;
        }
    }
    else if (code == WS_DISCONNECT)
    {
        rt_kprintf("WebSocket closed\n");
        g_xz_ws.is_connected = 0;
        xiaozhi_ui_chat_status("休眠中...");
        xiaozhi_ui_chat_output("请按键唤醒");
        xiaozhi_ui_update_emoji("sleepy");

        
    }
    else if (code == WS_TEXT)
    {
        rt_kprintf("Got Text:");
        parse_helLo(buf, len);
    }
    else
    {
        // Receive Audio Data
        xz_audio_downlink(buf, len, NULL, 0);
    }
    return 0;
}
void xiaozhi2(int argc, char **argv);
void reconnect_websocket() {

    if (!g_pan_connected) {
        xiaozhi_ui_chat_status("请开启网络共享");
        xiaozhi_ui_chat_output("请在手机上开启网络共享后重新发起连接");
        xiaozhi_ui_update_emoji("embarrassed");
        return;
    }

    err_t result;
    uint32_t retry = 10;
    while (retry-- > 0)
    {
        err_t close_err2 = wsock_close(&g_xz_ws.clnt, WSOCK_RESULT_LOCAL_ABORT, ERR_OK);
        rt_kprintf("close_err2=%d\n", close_err2);
        if (g_xz_ws.sem == NULL)
            g_xz_ws.sem = rt_sem_create("xz_ws", 0, RT_IPC_FLAG_FIFO);
        char *Client_Id = get_client_id();
        wsock_init(&g_xz_ws.clnt, 1, 1, my_wsapp_fn);//初始化websocket,注册回调函数
        result = wsock_connect(&g_xz_ws.clnt, MAX_WSOCK_HDR_LEN, XIAOZHI_HOST, XIAOZHI_WSPATH,
                            LWIP_IANA_PORT_HTTPS, XIAOZHI_TOKEN, NULL,
                            "Protocol-Version: 1\r\nDevice-Id: %s\r\nClient-Id: %s\r\n", get_mac_address(),Client_Id);
        rt_kprintf("Web socket connection %d\r\n", result);
        if (result == 0)
        {
            rt_kprintf("result_g_xz_ws.sem = 0%d\n",g_xz_ws.sem->value);
            
            if (RT_EOK == rt_sem_take(g_xz_ws.sem, 5000))
            {
                rt_kprintf("g_xz_ws.is_connected = %d\n", g_xz_ws.is_connected);
                if (g_xz_ws.is_connected)
                {
                    result = wsock_write(&g_xz_ws.clnt, hello_message, strlen(hello_message), OPCODE_TEXT);
                    rt_kprintf("Web socket write %d\r\n", result);
                    break;
                }
                else
                {
                    rt_kprintf("result = wsock_write_Web socket disconnected\r\n");
                }
            }
            else
            {
                rt_kprintf("Web socket connected timeout\r\n");
            }
        }
        else
        {
            rt_kprintf("Waiting internet ready(%d)... \r\n", retry);
            rt_thread_mdelay(1000);
        }
    }
}
extern rt_mailbox_t g_bt_app_mb;
#define WEBSOCKET_RECONNECT 3
static void xz_button_event_handler(int32_t pin, button_action_t action)//Session key
{
    rt_kprintf("button(%d) %d:", pin, action);
    rt_kprintf("g_state = %d\n", g_state);

    static button_action_t last_action=BUTTON_RELEASED;
    if(last_action==action)
    {
        return;
    }
    last_action=action;
    
    if (!g_xz_ws.is_connected)//唤醒  stop ? goodbye
    {
        rt_kprintf("please button11 attempting to reconnect WebSocket\n\r\n");
        xiaozhi_ui_chat_status("请按唤醒键...");
        xiaozhi_ui_chat_output("请按唤醒键重连！");
        xiaozhi_ui_update_emoji("embarrassed");

    }
    else
    {
        if (action == BUTTON_PRESSED)
        {
            rt_kprintf("pressed\r\n");
            ws_send_listen_start(&g_xz_ws.clnt, g_xz_ws.session_id, kListeningModeAutoStop);//发送开始监听
            xiaozhi_ui_chat_status("\u8046\u542c\u4e2d...");
            xz_mic(1);
        }
        else if (action == BUTTON_RELEASED)
        {
            rt_kprintf("released\r\n");
            xiaozhi_ui_chat_status("\u5f85\u547d\u4e2d...");
            ws_send_listen_stop(&g_xz_ws.clnt, g_xz_ws.session_id);
            xz_mic(0);
        }
    }

}
static void xz_button_event_handler2(int32_t pin, button_action_t action)//Wake up key
{
    rt_kprintf("button(%d) %d:", pin, action);
    
    static button_action_t last_action=BUTTON_RELEASED;
    if(last_action==action)
    {
        return;
    }
    last_action=action;

    if (action == BUTTON_PRESSED)
    {
        rt_kprintf("pressed\r\n");
        if (!g_xz_ws.is_connected)//唤醒重连websocket
        {
            rt_kprintf("handler2 attempting to reconnect WebSocket\n\r\n");
            xiaozhi_ui_chat_status("唤醒中...");
            xiaozhi_ui_chat_output("正在唤醒!");
            xiaozhi_ui_update_emoji("relaxed");
            rt_mb_send(g_bt_app_mb, WEBSOCKET_RECONNECT);
         
        }
    }
    else if (action == BUTTON_RELEASED)
    {
        rt_kprintf("released\r\n");
        
    }
}


static void xz_button_init(void)//Session key
{
    static int initialized = 0;

    if (initialized == 0)
    {
        button_cfg_t cfg;
        cfg.pin = BSP_KEY1_PIN;

        cfg.active_state = BSP_KEY1_ACTIVE_HIGH;
        cfg.mode = PIN_MODE_INPUT;
        cfg.button_handler = xz_button_event_handler2;//wakeup key
        int32_t id = button_init(&cfg);
        RT_ASSERT(id >= 0);
        RT_ASSERT(SF_EOK == button_enable(id));
        initialized = 1;
    }
}
static void xz_button_init2(void)
{
    static int initialized = 0;

    if (initialized == 0)
    {
        button_cfg_t cfg;
        cfg.pin = BSP_KEY2_PIN;

        cfg.active_state = BSP_KEY2_ACTIVE_HIGH;
        cfg.mode = PIN_MODE_INPUT;
        cfg.button_handler = xz_button_event_handler;//session key
        int32_t id = button_init(&cfg);
        RT_ASSERT(id >= 0);
        RT_ASSERT(SF_EOK == button_enable(id));
        initialized = 1;
    }
}

void xz_ws_audio_init()
{
    rt_kprintf("xz_audio_init\n");
    audio_server_set_private_volume(AUDIO_TYPE_LOCAL_MUSIC, 6);//设置音量
    xz_audio_decoder_encoder_open(0);//打开音频解码器和编码器
    xz_button_init();
    xz_button_init2();
}


static char *my_json_string(cJSON *json, char *key)
{
    cJSON *item  = cJSON_GetObjectItem(json, key);
    char *r = cJSON_Print(item);

    if (r && ((*r) == '\"'))
    {
        r++;
        r[strlen(r) - 1] = '\0';
    }
    return r;
}

void parse_helLo(const u8_t *data, u16_t len)
{
    cJSON *item = NULL;
    cJSON *root = NULL;
    rt_kputs(data);
    rt_kputs("--\r\n");
    root = cJSON_Parse(data);   /*json_data 为MQTT的原始数据*/
    if (!root)
    {
        rt_kprintf("Error before: [%s]\n", cJSON_GetErrorPtr());
        return;
    }

    char *type = cJSON_GetObjectItem(root, "type")->valuestring;
    if (strcmp(type, "hello") == 0)
    {
        char *session_id = cJSON_GetObjectItem(root, "session_id")->valuestring;
        cJSON *audio_param = cJSON_GetObjectItem(root, "audio_params");
        char *sample_rate = cJSON_GetObjectItem(audio_param, "sample_rate")->valuestring;
        char *duration = cJSON_GetObjectItem(audio_param, "duration")->valuestring;
        g_xz_ws.sample_rate = atoi(sample_rate);
        g_xz_ws.frame_duration = atoi(duration);
        strncpy(g_xz_ws.session_id, session_id, 9);
        g_state = kDeviceStateIdle;
        xz_ws_audio_init();//初始化音频
        xiaozhi_ui_chat_status("\u5f85\u547d\u4e2d...");
        xiaozhi_ui_chat_output("Xiaozhi 已连接!");
        xiaozhi_ui_update_emoji("neutral");

    }
    else if (strcmp(type, "goodbye") == 0)
    {
        g_state = kDeviceStateUnknown;
        rt_kprintf("session ended\n");
        xiaozhi_ui_chat_status("disconnected");
        xiaozhi_ui_chat_output("goodbye! 等待唤醒...");
        xiaozhi_ui_update_emoji("sleep");
    }
    else if (strcmp(type, "tts") == 0)
    {
        char *state = cJSON_GetObjectItem(root, "state")->valuestring;

        if (strcmp(state, "start") == 0)
        {
            if (g_state == kDeviceStateIdle || g_state == kDeviceStateListening)
            {
                g_state = kDeviceStateSpeaking;
                xz_speaker(1);//打开扬声器
            }
        }
        else if (strcmp(state, "stop") == 0)
        {
            g_state = kDeviceStateIdle;
            xz_speaker(0);//关闭扬声器
            xiaozhi_ui_chat_status("\u5f85\u547d\u4e2d...");
            xiaozhi_ui_chat_output("Xiaozhi 已停止说话");
        }
        else if (strcmp(state, "sentence_start") == 0)
        {
            char *txt = cJSON_GetObjectItem(root, "text")->valuestring;
            rt_kputs(txt);
            xiaozhi_ui_chat_output(txt);
            xiaozhi_ui_chat_status("\u8bb2\u8bdd\u4e2d...");
        }
    }
    else if (strcmp(type, "llm") == 0)
    {
        rt_kputs(cJSON_GetObjectItem(root, "emotion")->valuestring);
        xiaozhi_ui_update_emoji(cJSON_GetObjectItem(root, "emotion")->valuestring);
        xiaozhi_ui_chat_status("\u8bb2\u8bdd\u4e2d...");
    }
    
    else
    {
        rt_kprintf("Unkown type: %s\n", type);
    }
    cJSON_Delete(root);/*每次调用cJSON_Parse函数后，都要释放内存*/
}

void xiaozhi2(int argc, char **argv)
{
    if (!g_pan_connected) {
        xiaozhi_ui_chat_status("请开启网络共享");
        xiaozhi_ui_chat_output("请在手机上开启网络共享后重新发起连接");
        xiaozhi_ui_update_emoji("embarrassed");
        return;
    }
    err_t err;

    while (1)
    {

        err_t close_err = wsock_close(&g_xz_ws.clnt, WSOCK_RESULT_LOCAL_ABORT, ERR_OK);
        rt_kprintf("close_err:%d\n", close_err);
        if (g_xz_ws.sem == NULL)
            g_xz_ws.sem = rt_sem_create("xz_ws", 0, RT_IPC_FLAG_FIFO);

        
        wsock_init(&g_xz_ws.clnt, 1, 1, my_wsapp_fn);//初始化websocket,注册回调函数
        char *Client_Id = get_client_id();
        err = wsock_connect(&g_xz_ws.clnt, MAX_WSOCK_HDR_LEN, XIAOZHI_HOST, XIAOZHI_WSPATH,
                            LWIP_IANA_PORT_HTTPS, XIAOZHI_TOKEN, NULL,
                            "Protocol-Version: 1\r\nDevice-Id: %s\r\nClient-Id: %s\r\n", get_mac_address(),Client_Id);
        rt_kprintf("Web socket connection %d\r\n", err);
        if (err == 0)
        {
            rt_kprintf("err = 0\n");
            if (RT_EOK == rt_sem_take(g_xz_ws.sem, 5000))
            {
                rt_kprintf("g_xz_ws.is_connected = %d\n", g_xz_ws.is_connected);
                if (g_xz_ws.is_connected)
                {                 
                    err = wsock_write(&g_xz_ws.clnt, hello_message, strlen(hello_message), OPCODE_TEXT);
                    rt_kprintf("Web socket write %d\r\n", err);
                    break;
                }
                else
                {
                    rt_kprintf("err = wsock_write_Web socket disconnected\r\n");
                }
            }
            else
            {
                rt_kprintf("Web socket connected timeout\r\n");
            }
        }
        else
        {
            rt_kprintf("Waiting internet ready... \r\n");
            rt_thread_mdelay(1000);
        }
    }
}
MSH_CMD_EXPORT(xiaozhi2, Get Xiaozhi)



/************************ (C) COPYRIGHT Sifli Technology *******END OF FILE****/

