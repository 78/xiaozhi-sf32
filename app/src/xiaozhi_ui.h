#ifndef _XIAOZHI_UI_H_
#define _XIAOZHI_UI_H_

#include "lv_image_dsc.h"
#include "lv_timer.h"
#include "lv_display.h"
#include "lv_obj_pos.h"


#ifdef __cplusplus
extern "C" {
#endif

void xiaozhi_ui_update_ble(char *string); // ble
void xiaozhi_ui_update_emoji(char *string);
void xiaozhi_ui_chat_status(char *string);
void xiaozhi_ui_chat_output(char *string);
void xiaozhi_ui_standby_chat_output(char *string);
void ui_swith_to_standby_screen(void);
void ui_switch_to_call_screen(void);
void ui_switch_to_xiaozhi_screen(void);
void xiaozhi_ui_task(void *args);
void ui_sleep_callback(lv_timer_t *timer);
void xiaozhi_ui_tts_output(char *string);
void xiaozhi_ui_update_standby_emoji(char *string);
void show_shutdown(void);
void show_sleep_countdown_and_sleep(void);
void xiaozhi_ui_update_charge_status(uint8_t is_charging);
void xiaozhi_ui_update_latest_version(char *version);
void xiaozhi_ui_update_confirm_button_event(bool is_update_button);
void xiaozhi_ui_reinit_audio(void);
float get_scale_factor(void);


#ifdef __cplusplus
}
#endif

#endif // _XIAOZHI_UI_H_