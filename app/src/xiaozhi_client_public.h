#ifndef XIAOZHI_CLIENT_PUBLIC_H
#define XIAOZHI_CLIENT_PUBLIC_H


#include <rtthread.h>
#include "lwip/api.h"
#include "lwip/dns.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/apps/mqtt.h"
#include "lwip/udp.h"
#include "lwip/tcpip.h"
#include "xiaozhi_mqtt.h"
#include "bf0_hal.h"
#include "button.h"
#include "os_adaptor.h"
#include "opus_types.h"
#include "opus_multistream.h"
#include "os_support.h"
#include "audio_server.h"
#include "mem_section.h"
#ifdef PKG_XIAOZHI_USING_AEC
    #include "webrtc/common_audio/vad/include/webrtc_vad.h"
    #include "sifli_resample.h"
#endif
#include "bts2_app_inc.h"
#include "ble_connection_manager.h"
#include "bt_connection_manager.h"
#include "bt_env.h"
#include "cJSON.h"
#include "bt_pan_ota.h"
#define VERSION "V1.4.1"

 #define XZ_DOWNLINK_QUEUE_NUM 128
 #define XZ_MIC_FRAME_LEN (320 * 6) // 60ms for 16k samplerate
 #define XZ_SPK_FRAME_LEN (480 * 6) // 60ms for 24k samplerate, speaker frame len
 
 #define LCD_BRIGHTNESS_MIN     (10)
 #define LCD_BRIGHTNESS_MID     (50)
 #define LCD_BRIGHTNESS_MAX     (100)
 #define LCD_BRIGHTNESS_DEFAULT LCD_BRIGHTNESS_MID
 #define VOL_MIN_LEVEL          (0)
 #define VOL_MAX_LEVEL          (16)
 #define VOL_DEFAULE_LEVEL      (6)
 #define UI_EVENT_SHUTDOWN 1 //正常按键关机
 #define UI_EVENT_LOW_BATTERY_SHUTDOWN 2  //低电量关机
 #define UI_EVENT_LOW_BATTERY_WARNING    3  // 低电开机警告再关机
 #define AUDIO_IOCTL_ENABLE_CPU_LOW_SPEED            4   /* parameter type is uint32_t
                                                              1 low speed
                                                              0 high speed */
#define CHARGE_DETECT_PIN 44

#ifdef BSP_KEY1_ACTIVE_HIGH
#define KEY1_ACTIVE_LEVEL 1
#else
#define KEY1_ACTIVE_LEVEL 0
#endif

#ifdef BSP_KEY2_ACTIVE_HIGH
#define KEY2_ACTIVE_LEVEL 1
#else
#define KEY2_ACTIVE_LEVEL 0
#endif

extern volatile int g_kws_force_exit;
extern volatile int g_kws_running;
extern volatile uint8_t she_bei_ma;


typedef struct
{
    rt_slist_t node;
    uint8_t *data;
    uint16_t data_len;
    uint16_t size;
} xz_decode_queue_t;

typedef struct
{
    struct rt_thread thread;
    uint8_t encode_in[XZ_MIC_FRAME_LEN];
    uint8_t encode_out[XZ_MIC_FRAME_LEN];
    bool inited;
    bool is_rx_enable;
    bool is_tx_enable;
    bool is_exit;
    uint8_t is_websocket; // 0: mqtt, 1: websocket
    rt_event_t event;
    OpusEncoder *encoder;
    OpusDecoder *decoder;
    audio_client_t speaker;
    audio_client_t mic;
#if PKG_XIAOZHI_USING_AEC
    struct rt_ringbuffer    *rb_vad_cache;
    sifli_resample_t *resample;
    VadInst *handle;
    int voice_state;
    uint32_t voice_start_times;
    uint32_t voice_stop_times;
#endif
    uint32_t mic_rx_count;
    struct rt_ringbuffer *rb_opus_encode_input;
    xz_decode_queue_t downlink_queue[XZ_DOWNLINK_QUEUE_NUM];
    uint16_t downlink_decode_out[XZ_SPK_FRAME_LEN / 2];
    rt_slist_t downlink_decode_busy;
    rt_slist_t downlink_decode_idle;
    bool vad_enabled;
} xz_audio_t;

// 可复用函数
char *get_mac_address(void);
void hash_run(uint8_t algo, uint8_t *raw_data, uint32_t raw_data_len,uint8_t *result, uint32_t result_len);
void hex_2_asc(uint8_t n, char *str);
char *get_client_id();
int check_internet_access();
char *get_xiaozhi();
char *my_json_string(cJSON *json, char *key);
void xz_aec_mic_close(xz_audio_t *thiz);
void xz_aec_mic_open(xz_audio_t *thiz);
uint8_t vad_is_enable(void);
void vad_set_enable(uint8_t enable);
uint8_t aec_is_enable(void);
void aec_set_enable(uint8_t enable);
uint8_t xz_get_config_update(void);
void xz_set_config_update(uint8_t en);
ble_common_update_type_t ble_request_public_address(bd_addr_t *addr);
int register_device_with_server(void);
char* build_ota_query_url(const char* chip_id);
void answer_phone();  //接电话
void hung_up_phone(); //挂断电话
#endif // XIAOZHI_PUBLIC_H