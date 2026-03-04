#ifndef XIAOZHI_WEBSOCKET_H
#define XIAOZHI_WEBSOCKET_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <rtthread.h>
#include "lwip/api.h"
#include "lwip/apps/websocket_client.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/apps/mqtt.h"
#include "xiaozhi_mqtt.h"
#include "bf0_hal.h"
#include "bts2_app_pan.h"
#include <cJSON.h>
#include "button.h"
#include "audio_server.h"
#define WEBSOC_RECONNECT 4

void xiaozhi2(int argc, char **argv);
void xz_ws_button_init(void); // 对话键
void xz_ws_button_init2(void);//关机键
void xz_ws_audio_init(void);
void xz_audio_send_using_websocket(uint8_t *data, int len); // 发送音频数据
void ws_send_speak_abort(void *ws, char *session_id, int reason);
void ws_send_listen_start(void *ws, char *session_id,enum ListeningMode mode);
void ws_send_listen_stop(void *ws, char *session_id);
    /**
     * @brief xiaozhi websocket cntext 数据结构体
     */
    typedef struct
    {
        uint32_t sample_rate;
        uint32_t frame_duration;
        uint8_t session_id[12];
        wsock_state_t clnt;
        rt_sem_t sem;
        uint8_t is_connected;
    } xiaozhi_ws_t;

    typedef enum
    {
        BUTTON_EVENT_NONE,
        BUTTON_EVENT_PRESSED,
        BUTTON_EVENT_RELEASED,
    } button_event_type_t;

#ifdef __cplusplus
}
#endif

#endif // XIAOZHI_WEBSOCKET_H