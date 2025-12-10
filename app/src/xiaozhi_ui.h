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
void ui_switch_to_xiaozhi_screen(void);
void xiaozhi_ui_task(void *args);
void ui_sleep_callback(lv_timer_t *timer);
void ui_switch_to_xiaozhi_screen(void);
void xiaozhi_ui_tts_output(char *string);
void xiaozhi_ui_update_standby_emoji(char *string);
void show_shutdown(void);
void show_sleep_countdown_and_sleep(void);
void xiaozhi_ui_update_charge_status(uint8_t is_charging);
float get_scale_factor(void);

extern const lv_image_dsc_t * angry[57];
extern const lv_image_dsc_t * loving[48];
extern const lv_image_dsc_t * kissy[57];
extern const lv_image_dsc_t * cool[60];
extern const lv_image_dsc_t * crying[26];
extern const lv_image_dsc_t * embarrassed[57];
extern const lv_image_dsc_t * funny[57];
extern const lv_image_dsc_t * neutral[57];
extern const lv_image_dsc_t * sleepy[57];
extern const lv_image_dsc_t * surprised[57];
extern const lv_image_dsc_t * thinking[57];





extern const lv_image_dsc_t angry_1;
extern const lv_image_dsc_t angry_2;
extern const lv_image_dsc_t angry_3;
extern const lv_image_dsc_t angry_4;
extern const lv_image_dsc_t angry_5;
extern const lv_image_dsc_t angry_6;
extern const lv_image_dsc_t angry_7;
extern const lv_image_dsc_t angry_8;
extern const lv_image_dsc_t angry_9;
extern const lv_image_dsc_t angry_10;
extern const lv_image_dsc_t angry_11;
extern const lv_image_dsc_t angry_12;
extern const lv_image_dsc_t angry_13;
extern const lv_image_dsc_t angry_14;
extern const lv_image_dsc_t angry_15;
extern const lv_image_dsc_t angry_16;
extern const lv_image_dsc_t angry_17;
extern const lv_image_dsc_t angry_18;
extern const lv_image_dsc_t angry_19;
extern const lv_image_dsc_t angry_20;
extern const lv_image_dsc_t angry_21;
extern const lv_image_dsc_t angry_22;
extern const lv_image_dsc_t angry_23;
extern const lv_image_dsc_t angry_24;
extern const lv_image_dsc_t angry_25;
extern const lv_image_dsc_t angry_26;
extern const lv_image_dsc_t angry_27;
extern const lv_image_dsc_t angry_28;
extern const lv_image_dsc_t angry_29;
extern const lv_image_dsc_t angry_30;
extern const lv_image_dsc_t angry_31;
extern const lv_image_dsc_t angry_32;
extern const lv_image_dsc_t angry_33;
extern const lv_image_dsc_t angry_34;
extern const lv_image_dsc_t angry_35;
extern const lv_image_dsc_t angry_36;
extern const lv_image_dsc_t angry_37;
extern const lv_image_dsc_t angry_38;
extern const lv_image_dsc_t angry_39;
extern const lv_image_dsc_t angry_40;
extern const lv_image_dsc_t angry_41;
extern const lv_image_dsc_t angry_42;
extern const lv_image_dsc_t angry_43;
extern const lv_image_dsc_t angry_44;
extern const lv_image_dsc_t angry_45;
extern const lv_image_dsc_t angry_46;
extern const lv_image_dsc_t angry_47;
extern const lv_image_dsc_t angry_48;
extern const lv_image_dsc_t angry_49;
extern const lv_image_dsc_t angry_50;
extern const lv_image_dsc_t angry_51;
extern const lv_image_dsc_t angry_52;
extern const lv_image_dsc_t angry_53;
extern const lv_image_dsc_t angry_54;
extern const lv_image_dsc_t angry_55;
extern const lv_image_dsc_t angry_56;
extern const lv_image_dsc_t angry_57;


/*---------------------angry*/

extern const lv_image_dsc_t cool_1;
extern const lv_image_dsc_t cool_2;
extern const lv_image_dsc_t cool_3;
extern const lv_image_dsc_t cool_4;
extern const lv_image_dsc_t cool_5;
extern const lv_image_dsc_t cool_6;
extern const lv_image_dsc_t cool_7;
extern const lv_image_dsc_t cool_8;
extern const lv_image_dsc_t cool_9;
extern const lv_image_dsc_t cool_10;
extern const lv_image_dsc_t cool_11;
extern const lv_image_dsc_t cool_12;
extern const lv_image_dsc_t cool_13;
extern const lv_image_dsc_t cool_14;
extern const lv_image_dsc_t cool_15;
extern const lv_image_dsc_t cool_16;
extern const lv_image_dsc_t cool_17;
extern const lv_image_dsc_t cool_18;
extern const lv_image_dsc_t cool_19;
extern const lv_image_dsc_t cool_20;
extern const lv_image_dsc_t cool_21;
extern const lv_image_dsc_t cool_22;
extern const lv_image_dsc_t cool_23;
extern const lv_image_dsc_t cool_24;
extern const lv_image_dsc_t cool_25;
extern const lv_image_dsc_t cool_26;
extern const lv_image_dsc_t cool_27;
extern const lv_image_dsc_t cool_28;
extern const lv_image_dsc_t cool_29;
extern const lv_image_dsc_t cool_30;
extern const lv_image_dsc_t cool_31;
extern const lv_image_dsc_t cool_32;
extern const lv_image_dsc_t cool_33;
extern const lv_image_dsc_t cool_34;
extern const lv_image_dsc_t cool_35;
extern const lv_image_dsc_t cool_36;
extern const lv_image_dsc_t cool_37;
extern const lv_image_dsc_t cool_38;
extern const lv_image_dsc_t cool_39;
extern const lv_image_dsc_t cool_40;
extern const lv_image_dsc_t cool_41;
extern const lv_image_dsc_t cool_42;
extern const lv_image_dsc_t cool_43;
extern const lv_image_dsc_t cool_44;
extern const lv_image_dsc_t cool_45;
extern const lv_image_dsc_t cool_46;
extern const lv_image_dsc_t cool_47;
extern const lv_image_dsc_t cool_48;
extern const lv_image_dsc_t cool_49;
extern const lv_image_dsc_t cool_50;
extern const lv_image_dsc_t cool_51;
extern const lv_image_dsc_t cool_52;
extern const lv_image_dsc_t cool_53;
extern const lv_image_dsc_t cool_54;
extern const lv_image_dsc_t cool_55;
extern const lv_image_dsc_t cool_56;
extern const lv_image_dsc_t cool_57;
extern const lv_image_dsc_t cool_58;
extern const lv_image_dsc_t cool_59;
extern const lv_image_dsc_t cool_60;





/*---------------------cool*/


extern const lv_image_dsc_t crying_1;
extern const lv_image_dsc_t crying_2;
extern const lv_image_dsc_t crying_3;
extern const lv_image_dsc_t crying_4;
extern const lv_image_dsc_t crying_5;
extern const lv_image_dsc_t crying_6;
extern const lv_image_dsc_t crying_7;
extern const lv_image_dsc_t crying_8;
extern const lv_image_dsc_t crying_9;
extern const lv_image_dsc_t crying_10;
extern const lv_image_dsc_t crying_11;
extern const lv_image_dsc_t crying_12;
extern const lv_image_dsc_t crying_13;
extern const lv_image_dsc_t crying_14;
extern const lv_image_dsc_t crying_15;
extern const lv_image_dsc_t crying_16;
extern const lv_image_dsc_t crying_17;
extern const lv_image_dsc_t crying_18;
extern const lv_image_dsc_t crying_19;
extern const lv_image_dsc_t crying_20;
extern const lv_image_dsc_t crying_21;
extern const lv_image_dsc_t crying_22;
extern const lv_image_dsc_t crying_23;
extern const lv_image_dsc_t crying_24;
extern const lv_image_dsc_t crying_25;
extern const lv_image_dsc_t crying_26;


/*---------------------crying*/


extern const lv_image_dsc_t embarrassed_1;
extern const lv_image_dsc_t embarrassed_2;
extern const lv_image_dsc_t embarrassed_3;
extern const lv_image_dsc_t embarrassed_4;
extern const lv_image_dsc_t embarrassed_5;
extern const lv_image_dsc_t embarrassed_6;
extern const lv_image_dsc_t embarrassed_7;
extern const lv_image_dsc_t embarrassed_8;
extern const lv_image_dsc_t embarrassed_9;
extern const lv_image_dsc_t embarrassed_10;
extern const lv_image_dsc_t embarrassed_11;
extern const lv_image_dsc_t embarrassed_12;
extern const lv_image_dsc_t embarrassed_13;
extern const lv_image_dsc_t embarrassed_14;
extern const lv_image_dsc_t embarrassed_15;
extern const lv_image_dsc_t embarrassed_16;
extern const lv_image_dsc_t embarrassed_17;
extern const lv_image_dsc_t embarrassed_18;
extern const lv_image_dsc_t embarrassed_19;
extern const lv_image_dsc_t embarrassed_20;
extern const lv_image_dsc_t embarrassed_21;
extern const lv_image_dsc_t embarrassed_22;
extern const lv_image_dsc_t embarrassed_23;
extern const lv_image_dsc_t embarrassed_24;
extern const lv_image_dsc_t embarrassed_25;
extern const lv_image_dsc_t embarrassed_26;
extern const lv_image_dsc_t embarrassed_27;
extern const lv_image_dsc_t embarrassed_28;
extern const lv_image_dsc_t embarrassed_29;
extern const lv_image_dsc_t embarrassed_30;
extern const lv_image_dsc_t embarrassed_31;
extern const lv_image_dsc_t embarrassed_32;
extern const lv_image_dsc_t embarrassed_33;
extern const lv_image_dsc_t embarrassed_34;
extern const lv_image_dsc_t embarrassed_35;
extern const lv_image_dsc_t embarrassed_36;
extern const lv_image_dsc_t embarrassed_37;
extern const lv_image_dsc_t embarrassed_38;
extern const lv_image_dsc_t embarrassed_39;
extern const lv_image_dsc_t embarrassed_40;
extern const lv_image_dsc_t embarrassed_41;
extern const lv_image_dsc_t embarrassed_42;
extern const lv_image_dsc_t embarrassed_43;
extern const lv_image_dsc_t embarrassed_44;
extern const lv_image_dsc_t embarrassed_45;
extern const lv_image_dsc_t embarrassed_46;
extern const lv_image_dsc_t embarrassed_47;
extern const lv_image_dsc_t embarrassed_48;
extern const lv_image_dsc_t embarrassed_49;
extern const lv_image_dsc_t embarrassed_50;
extern const lv_image_dsc_t embarrassed_51;
extern const lv_image_dsc_t embarrassed_52;
extern const lv_image_dsc_t embarrassed_53;
extern const lv_image_dsc_t embarrassed_54;
extern const lv_image_dsc_t embarrassed_55;
extern const lv_image_dsc_t embarrassed_56;
extern const lv_image_dsc_t embarrassed_57;







/*---------------------embarrassed*/

extern const lv_image_dsc_t funny_1;
extern const lv_image_dsc_t funny_2;
extern const lv_image_dsc_t funny_3;
extern const lv_image_dsc_t funny_4;
extern const lv_image_dsc_t funny_5;
extern const lv_image_dsc_t funny_6;
extern const lv_image_dsc_t funny_7;
extern const lv_image_dsc_t funny_8;
extern const lv_image_dsc_t funny_9;
extern const lv_image_dsc_t funny_10;
extern const lv_image_dsc_t funny_11;
extern const lv_image_dsc_t funny_12;
extern const lv_image_dsc_t funny_13;
extern const lv_image_dsc_t funny_14;
extern const lv_image_dsc_t funny_15;
extern const lv_image_dsc_t funny_16;
extern const lv_image_dsc_t funny_17;
extern const lv_image_dsc_t funny_18;
extern const lv_image_dsc_t funny_19;
extern const lv_image_dsc_t funny_20;
extern const lv_image_dsc_t funny_21;
extern const lv_image_dsc_t funny_22;
extern const lv_image_dsc_t funny_23;
extern const lv_image_dsc_t funny_24;
extern const lv_image_dsc_t funny_25;
extern const lv_image_dsc_t funny_26;
extern const lv_image_dsc_t funny_27;
extern const lv_image_dsc_t funny_28;
extern const lv_image_dsc_t funny_29;
extern const lv_image_dsc_t funny_30;
extern const lv_image_dsc_t funny_31;
extern const lv_image_dsc_t funny_32;
extern const lv_image_dsc_t funny_33;
extern const lv_image_dsc_t funny_34;
extern const lv_image_dsc_t funny_35;
extern const lv_image_dsc_t funny_36;
extern const lv_image_dsc_t funny_37;
extern const lv_image_dsc_t funny_38;
extern const lv_image_dsc_t funny_39;
extern const lv_image_dsc_t funny_40;
extern const lv_image_dsc_t funny_41;
extern const lv_image_dsc_t funny_42;
extern const lv_image_dsc_t funny_43;
extern const lv_image_dsc_t funny_44;
extern const lv_image_dsc_t funny_45;
extern const lv_image_dsc_t funny_46;
extern const lv_image_dsc_t funny_47;
extern const lv_image_dsc_t funny_48;
extern const lv_image_dsc_t funny_49;
extern const lv_image_dsc_t funny_50;
extern const lv_image_dsc_t funny_51;
extern const lv_image_dsc_t funny_52;
extern const lv_image_dsc_t funny_53;
extern const lv_image_dsc_t funny_54;
extern const lv_image_dsc_t funny_55;
extern const lv_image_dsc_t funny_56;
extern const lv_image_dsc_t funny_57;





/*---------------------funny*/



extern const lv_image_dsc_t kissy_1;
extern const lv_image_dsc_t kissy_2;
extern const lv_image_dsc_t kissy_3;
extern const lv_image_dsc_t kissy_4;
extern const lv_image_dsc_t kissy_5;
extern const lv_image_dsc_t kissy_6;
extern const lv_image_dsc_t kissy_7;
extern const lv_image_dsc_t kissy_8;
extern const lv_image_dsc_t kissy_9;
extern const lv_image_dsc_t kissy_10;
extern const lv_image_dsc_t kissy_11;
extern const lv_image_dsc_t kissy_12;
extern const lv_image_dsc_t kissy_13;
extern const lv_image_dsc_t kissy_14;
extern const lv_image_dsc_t kissy_15;
extern const lv_image_dsc_t kissy_16;
extern const lv_image_dsc_t kissy_17;
extern const lv_image_dsc_t kissy_18;
extern const lv_image_dsc_t kissy_19;
extern const lv_image_dsc_t kissy_20;
extern const lv_image_dsc_t kissy_21;
extern const lv_image_dsc_t kissy_22;
extern const lv_image_dsc_t kissy_23;
extern const lv_image_dsc_t kissy_24;
extern const lv_image_dsc_t kissy_25;
extern const lv_image_dsc_t kissy_26;
extern const lv_image_dsc_t kissy_27;
extern const lv_image_dsc_t kissy_28;
extern const lv_image_dsc_t kissy_29;
extern const lv_image_dsc_t kissy_30;
extern const lv_image_dsc_t kissy_31;
extern const lv_image_dsc_t kissy_32;
extern const lv_image_dsc_t kissy_33;
extern const lv_image_dsc_t kissy_34;
extern const lv_image_dsc_t kissy_35;
extern const lv_image_dsc_t kissy_36;
extern const lv_image_dsc_t kissy_37;
extern const lv_image_dsc_t kissy_38;
extern const lv_image_dsc_t kissy_39;
extern const lv_image_dsc_t kissy_40;
extern const lv_image_dsc_t kissy_41;
extern const lv_image_dsc_t kissy_42;
extern const lv_image_dsc_t kissy_43;
extern const lv_image_dsc_t kissy_44;
extern const lv_image_dsc_t kissy_45;
extern const lv_image_dsc_t kissy_46;
extern const lv_image_dsc_t kissy_47;
extern const lv_image_dsc_t kissy_48;
extern const lv_image_dsc_t kissy_49;
extern const lv_image_dsc_t kissy_50;
extern const lv_image_dsc_t kissy_51;
extern const lv_image_dsc_t kissy_52;
extern const lv_image_dsc_t kissy_53;
extern const lv_image_dsc_t kissy_54;
extern const lv_image_dsc_t kissy_55;
extern const lv_image_dsc_t kissy_56;
extern const lv_image_dsc_t kissy_57;







/*---------------------kissy*/




extern const lv_image_dsc_t loving_1;
extern const lv_image_dsc_t loving_2;
extern const lv_image_dsc_t loving_3;
extern const lv_image_dsc_t loving_4;
extern const lv_image_dsc_t loving_5;
extern const lv_image_dsc_t loving_6;
extern const lv_image_dsc_t loving_7;
extern const lv_image_dsc_t loving_8;
extern const lv_image_dsc_t loving_9;
extern const lv_image_dsc_t loving_10;
extern const lv_image_dsc_t loving_11;
extern const lv_image_dsc_t loving_12;
extern const lv_image_dsc_t loving_13;
extern const lv_image_dsc_t loving_14;
extern const lv_image_dsc_t loving_15;
extern const lv_image_dsc_t loving_16;
extern const lv_image_dsc_t loving_17;
extern const lv_image_dsc_t loving_18;
extern const lv_image_dsc_t loving_19;
extern const lv_image_dsc_t loving_20;
extern const lv_image_dsc_t loving_21;
extern const lv_image_dsc_t loving_22;
extern const lv_image_dsc_t loving_23;
extern const lv_image_dsc_t loving_24;
extern const lv_image_dsc_t loving_25;
extern const lv_image_dsc_t loving_26;
extern const lv_image_dsc_t loving_27;
extern const lv_image_dsc_t loving_28;
extern const lv_image_dsc_t loving_29;
extern const lv_image_dsc_t loving_30;
extern const lv_image_dsc_t loving_31;
extern const lv_image_dsc_t loving_32;
extern const lv_image_dsc_t loving_33;
extern const lv_image_dsc_t loving_34;
extern const lv_image_dsc_t loving_35;
extern const lv_image_dsc_t loving_36;
extern const lv_image_dsc_t loving_37;
extern const lv_image_dsc_t loving_38;
extern const lv_image_dsc_t loving_39;
extern const lv_image_dsc_t loving_40;
extern const lv_image_dsc_t loving_41;
extern const lv_image_dsc_t loving_42;
extern const lv_image_dsc_t loving_43;
extern const lv_image_dsc_t loving_44;
extern const lv_image_dsc_t loving_45;
extern const lv_image_dsc_t loving_46;
extern const lv_image_dsc_t loving_47;
extern const lv_image_dsc_t loving_48;





/*-----------------loving*/

extern const lv_image_dsc_t neutral_1;
extern const lv_image_dsc_t neutral_2;
extern const lv_image_dsc_t neutral_3;
extern const lv_image_dsc_t neutral_4;
extern const lv_image_dsc_t neutral_5;
extern const lv_image_dsc_t neutral_6;
extern const lv_image_dsc_t neutral_7;
extern const lv_image_dsc_t neutral_8;
extern const lv_image_dsc_t neutral_9;
extern const lv_image_dsc_t neutral_10;
extern const lv_image_dsc_t neutral_11;
extern const lv_image_dsc_t neutral_12;
extern const lv_image_dsc_t neutral_13;
extern const lv_image_dsc_t neutral_14;
extern const lv_image_dsc_t neutral_15;
extern const lv_image_dsc_t neutral_16;
extern const lv_image_dsc_t neutral_17;
extern const lv_image_dsc_t neutral_18;
extern const lv_image_dsc_t neutral_19;
extern const lv_image_dsc_t neutral_20;
extern const lv_image_dsc_t neutral_21;
extern const lv_image_dsc_t neutral_22;
extern const lv_image_dsc_t neutral_23;
extern const lv_image_dsc_t neutral_24;
extern const lv_image_dsc_t neutral_25;
extern const lv_image_dsc_t neutral_26;
extern const lv_image_dsc_t neutral_27;
extern const lv_image_dsc_t neutral_28;
extern const lv_image_dsc_t neutral_29;
extern const lv_image_dsc_t neutral_30;
extern const lv_image_dsc_t neutral_31;
extern const lv_image_dsc_t neutral_32;
extern const lv_image_dsc_t neutral_33;
extern const lv_image_dsc_t neutral_34;
extern const lv_image_dsc_t neutral_35;
extern const lv_image_dsc_t neutral_36;
extern const lv_image_dsc_t neutral_37;
extern const lv_image_dsc_t neutral_38;
extern const lv_image_dsc_t neutral_39;
extern const lv_image_dsc_t neutral_40;
extern const lv_image_dsc_t neutral_41;
extern const lv_image_dsc_t neutral_42;
extern const lv_image_dsc_t neutral_43;
extern const lv_image_dsc_t neutral_44;
extern const lv_image_dsc_t neutral_45;
extern const lv_image_dsc_t neutral_46;
extern const lv_image_dsc_t neutral_47;
extern const lv_image_dsc_t neutral_48;
extern const lv_image_dsc_t neutral_49;
extern const lv_image_dsc_t neutral_50;
extern const lv_image_dsc_t neutral_51;
extern const lv_image_dsc_t neutral_52;
extern const lv_image_dsc_t neutral_53;
extern const lv_image_dsc_t neutral_54;
extern const lv_image_dsc_t neutral_55;
extern const lv_image_dsc_t neutral_56;
extern const lv_image_dsc_t neutral_57;





/*---------------------neutra*/


extern const lv_image_dsc_t sleepy_1;
extern const lv_image_dsc_t sleepy_2;
extern const lv_image_dsc_t sleepy_3;
extern const lv_image_dsc_t sleepy_4;
extern const lv_image_dsc_t sleepy_5;
extern const lv_image_dsc_t sleepy_6;
extern const lv_image_dsc_t sleepy_7;
extern const lv_image_dsc_t sleepy_8;
extern const lv_image_dsc_t sleepy_9;
extern const lv_image_dsc_t sleepy_10;
extern const lv_image_dsc_t sleepy_11;
extern const lv_image_dsc_t sleepy_12;
extern const lv_image_dsc_t sleepy_13;
extern const lv_image_dsc_t sleepy_14;
extern const lv_image_dsc_t sleepy_15;
extern const lv_image_dsc_t sleepy_16;
extern const lv_image_dsc_t sleepy_17;
extern const lv_image_dsc_t sleepy_18;
extern const lv_image_dsc_t sleepy_19;
extern const lv_image_dsc_t sleepy_20;
extern const lv_image_dsc_t sleepy_21;
extern const lv_image_dsc_t sleepy_22;
extern const lv_image_dsc_t sleepy_23;
extern const lv_image_dsc_t sleepy_24;
extern const lv_image_dsc_t sleepy_25;
extern const lv_image_dsc_t sleepy_26;
extern const lv_image_dsc_t sleepy_27;
extern const lv_image_dsc_t sleepy_28;
extern const lv_image_dsc_t sleepy_29;
extern const lv_image_dsc_t sleepy_30;
extern const lv_image_dsc_t sleepy_31;
extern const lv_image_dsc_t sleepy_32;
extern const lv_image_dsc_t sleepy_33;
extern const lv_image_dsc_t sleepy_34;
extern const lv_image_dsc_t sleepy_35;
extern const lv_image_dsc_t sleepy_36;
extern const lv_image_dsc_t sleepy_37;
extern const lv_image_dsc_t sleepy_38;
extern const lv_image_dsc_t sleepy_39;
extern const lv_image_dsc_t sleepy_40;
extern const lv_image_dsc_t sleepy_41;
extern const lv_image_dsc_t sleepy_42;
extern const lv_image_dsc_t sleepy_43;
extern const lv_image_dsc_t sleepy_44;
extern const lv_image_dsc_t sleepy_45;
extern const lv_image_dsc_t sleepy_46;
extern const lv_image_dsc_t sleepy_47;
extern const lv_image_dsc_t sleepy_48;
extern const lv_image_dsc_t sleepy_49;
extern const lv_image_dsc_t sleepy_50;
extern const lv_image_dsc_t sleepy_51;
extern const lv_image_dsc_t sleepy_52;
extern const lv_image_dsc_t sleepy_53;
extern const lv_image_dsc_t sleepy_54;
extern const lv_image_dsc_t sleepy_55;
extern const lv_image_dsc_t sleepy_56;
extern const lv_image_dsc_t sleepy_57;






/*---------------------sleepy*/


extern const lv_image_dsc_t surprised_1;
extern const lv_image_dsc_t surprised_2;
extern const lv_image_dsc_t surprised_3;
extern const lv_image_dsc_t surprised_4;
extern const lv_image_dsc_t surprised_5;
extern const lv_image_dsc_t surprised_6;
extern const lv_image_dsc_t surprised_7;
extern const lv_image_dsc_t surprised_8;
extern const lv_image_dsc_t surprised_9;
extern const lv_image_dsc_t surprised_10;
extern const lv_image_dsc_t surprised_11;
extern const lv_image_dsc_t surprised_12;
extern const lv_image_dsc_t surprised_13;
extern const lv_image_dsc_t surprised_14;
extern const lv_image_dsc_t surprised_15;
extern const lv_image_dsc_t surprised_16;
extern const lv_image_dsc_t surprised_17;
extern const lv_image_dsc_t surprised_18;
extern const lv_image_dsc_t surprised_19;
extern const lv_image_dsc_t surprised_20;
extern const lv_image_dsc_t surprised_21;
extern const lv_image_dsc_t surprised_22;
extern const lv_image_dsc_t surprised_23;
extern const lv_image_dsc_t surprised_24;
extern const lv_image_dsc_t surprised_25;
extern const lv_image_dsc_t surprised_26;
extern const lv_image_dsc_t surprised_27;
extern const lv_image_dsc_t surprised_28;
extern const lv_image_dsc_t surprised_29;
extern const lv_image_dsc_t surprised_30;
extern const lv_image_dsc_t surprised_31;
extern const lv_image_dsc_t surprised_32;
extern const lv_image_dsc_t surprised_33;
extern const lv_image_dsc_t surprised_34;
extern const lv_image_dsc_t surprised_35;
extern const lv_image_dsc_t surprised_36;
extern const lv_image_dsc_t surprised_37;
extern const lv_image_dsc_t surprised_38;
extern const lv_image_dsc_t surprised_39;
extern const lv_image_dsc_t surprised_40;
extern const lv_image_dsc_t surprised_41;
extern const lv_image_dsc_t surprised_42;
extern const lv_image_dsc_t surprised_43;
extern const lv_image_dsc_t surprised_44;
extern const lv_image_dsc_t surprised_45;
extern const lv_image_dsc_t surprised_46;
extern const lv_image_dsc_t surprised_47;
extern const lv_image_dsc_t surprised_48;
extern const lv_image_dsc_t surprised_49;
extern const lv_image_dsc_t surprised_50;
extern const lv_image_dsc_t surprised_51;
extern const lv_image_dsc_t surprised_52;
extern const lv_image_dsc_t surprised_53;
extern const lv_image_dsc_t surprised_54;
extern const lv_image_dsc_t surprised_55;
extern const lv_image_dsc_t surprised_56;
extern const lv_image_dsc_t surprised_57;






/*---------------------surprised*/


extern const lv_image_dsc_t thinking_1;
extern const lv_image_dsc_t thinking_2;
extern const lv_image_dsc_t thinking_3;
extern const lv_image_dsc_t thinking_4;
extern const lv_image_dsc_t thinking_5;
extern const lv_image_dsc_t thinking_6;
extern const lv_image_dsc_t thinking_7;
extern const lv_image_dsc_t thinking_8;
extern const lv_image_dsc_t thinking_9;
extern const lv_image_dsc_t thinking_10;
extern const lv_image_dsc_t thinking_11;
extern const lv_image_dsc_t thinking_12;
extern const lv_image_dsc_t thinking_13;
extern const lv_image_dsc_t thinking_14;
extern const lv_image_dsc_t thinking_15;
extern const lv_image_dsc_t thinking_16;
extern const lv_image_dsc_t thinking_17;
extern const lv_image_dsc_t thinking_18;
extern const lv_image_dsc_t thinking_19;
extern const lv_image_dsc_t thinking_20;
extern const lv_image_dsc_t thinking_21;
extern const lv_image_dsc_t thinking_22;
extern const lv_image_dsc_t thinking_23;
extern const lv_image_dsc_t thinking_24;
extern const lv_image_dsc_t thinking_25;
extern const lv_image_dsc_t thinking_26;
extern const lv_image_dsc_t thinking_27;
extern const lv_image_dsc_t thinking_28;
extern const lv_image_dsc_t thinking_29;
extern const lv_image_dsc_t thinking_30;
extern const lv_image_dsc_t thinking_31;
extern const lv_image_dsc_t thinking_32;
extern const lv_image_dsc_t thinking_33;
extern const lv_image_dsc_t thinking_34;
extern const lv_image_dsc_t thinking_35;
extern const lv_image_dsc_t thinking_36;
extern const lv_image_dsc_t thinking_37;
extern const lv_image_dsc_t thinking_38;
extern const lv_image_dsc_t thinking_39;
extern const lv_image_dsc_t thinking_40;
extern const lv_image_dsc_t thinking_41;
extern const lv_image_dsc_t thinking_42;
extern const lv_image_dsc_t thinking_43;
extern const lv_image_dsc_t thinking_44;
extern const lv_image_dsc_t thinking_45;
extern const lv_image_dsc_t thinking_46;
extern const lv_image_dsc_t thinking_47;
extern const lv_image_dsc_t thinking_48;
extern const lv_image_dsc_t thinking_49;
extern const lv_image_dsc_t thinking_50;
extern const lv_image_dsc_t thinking_51;
extern const lv_image_dsc_t thinking_52;
extern const lv_image_dsc_t thinking_53;
extern const lv_image_dsc_t thinking_54;
extern const lv_image_dsc_t thinking_55;
extern const lv_image_dsc_t thinking_56;
extern const lv_image_dsc_t thinking_57;



/*---------------------thinking*/

#ifdef __cplusplus
}
#endif

#endif // _XIAOZHI_UI_H_