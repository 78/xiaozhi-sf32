#ifndef XIAOZHI_SCREEN_H
#define XIAOZHI_SCREEN_H

#include <rtthread.h>
#include "lv_tiny_ttf.h"
#include "string.h"
#include "xiaozhi_client_public.h"
#include "bf0_pm.h"
#include "gui_app_pm.h"
#include "drv_gpio.h"
#include "lv_timer.h"
#include "lv_display.h"
#include "lv_obj_pos.h"

void show_shutdown(void);
void show_low_battery_shutdown(void);
void show_sleep_countdown_and_sleep(void);
void show_low_battery_warning(void);
void show_startup_animation(void);
#endif // XIAOZHI_SCREEN_H