#include "rtthread.h"
#include "bf0_hal.h"
#include "drv_io.h"
#include "littlevgl2rtt.h"
#include "lv_tiny_ttf.h"
#include "string.h"
#include "xiaozhi_client_public.h"
#include "bf0_pm.h"
#include "gui_app_pm.h"
#include "drv_gpio.h"
#include "lv_timer.h"
#include "lv_display.h"
#include "lv_obj_pos.h"
#include "ulog.h"
#include "drv_flash.h"
#include "xiaozhi_websocket.h"
#include "bts2_app_inc.h"
#include "ble_connection_manager.h"
#include "bt_connection_manager.h"
#include "bt_env.h"
#include "./mcp/mcp_api.h"
#include "lv_seqimg.h"
#include "xiaozhi_ui.h"
#include "../weather/weather.h"
#include "xiaozhi_audio.h"
#include "../kws/app_recorder_process.h"
#include "../board/board_hardware.h"
#include "xiaozhi_screen.h"

#define SCALE_DPX(val) LV_DPX((val) * get_scale_factor())

lv_obj_t *shutdown_screen = NULL; 
lv_obj_t *sleep_screen = NULL;
lv_obj_t *low_battery_shutdown_screen = NULL;
lv_obj_t *low_battery_warning_screen = NULL;
lv_obj_t *g_startup_screen = NULL;
//休眠页面
static lv_obj_t *sleep_label = NULL;
static int sleep_countdown = 3;
static lv_timer_t *sleep_timer = NULL;
static volatile int g_sleep_countdown_active = 0; // 休眠倒计时标志
//低电开机警告页面
static lv_obj_t *warning_title_label = NULL;
static lv_obj_t *warning_hint_label = NULL;
static lv_timer_t *warning_shutdown_timer = NULL;
static volatile int g_low_battery_warning_active = 0;

//低电量关机页面
static lv_obj_t *low_battery_label = NULL;
static lv_obj_t *low_battery_countdown_label = NULL;
static lv_timer_t *low_battery_shutdown_timer = NULL;
static volatile int g_low_battery_shutdown_active = 0; // 低电量关机倒计时标志
static int low_battery_countdown = 3;
static uint8_t low_shutdown_index = 0;
static uint8_t shutdown_index = 0;
//关机页面
static lv_obj_t *shutdown_label = NULL;
static int shutdown_countdown = 3;
static lv_timer_t *shutdown_timer = NULL;
static volatile int g_shutdown_countdown_active = 0; // 关机倒计时标志
//开机动画页面

static lv_obj_t *g_startup_img = NULL;
static lv_anim_t g_startup_anim;
static bool g_startup_animation_finished = false;
static volatile int g_startup_animation_active = 0;
//外部变量
extern rt_timer_t update_time_ui_timer;
extern rt_timer_t update_weather_ui_timer;
extern uint8_t aec_enabled;
extern const unsigned char xiaozhi_font[];
extern const int xiaozhi_font_size;
extern const lv_image_dsc_t cdian2; 
extern const lv_image_dsc_t startup_logo;  //开机动画图标
extern lv_obj_t *standby_screen;
extern lv_obj_t *cont;
extern const lv_image_dsc_t no_power2;
extern bool low_battery_shutdown_triggered;
extern lv_obj_t *g_screen_before_low_battery;
static void sleep_countdown_cb(lv_timer_t *timer)
{
    
    if (sleep_label && sleep_countdown > 0)
    {
        char num[2] = {0};
        snprintf(num, sizeof(num), "%d", sleep_countdown);
        lv_label_set_text(sleep_label, num);
        lv_obj_center(sleep_label);
        sleep_countdown--;
    }
    else
    {
        // 清理所有LVGL对象
        if (sleep_label) {
            lv_obj_delete(sleep_label);
            sleep_label = NULL;
        }
                if(update_time_ui_timer)
        {
            rt_timer_stop(update_time_ui_timer);//睡眠停止ui更新
        }
        
        if(update_weather_ui_timer)
        {
            rt_timer_stop(update_weather_ui_timer);
        }

        lv_timer_delete(sleep_timer);
        sleep_timer = NULL;
        g_sleep_countdown_active = 0; // 倒计时结束，清除标志
        rt_kprintf("sleep countdown ok\n");  
        if(aec_enabled)
        {
            rt_pm_request(PM_SLEEP_MODE_IDLE);
        }
        else
        {
           rt_pm_release(PM_SLEEP_MODE_IDLE);
        }
        lv_obj_clean(sleep_screen);
        rt_thread_delay(100);
        gui_pm_fsm(GUI_PM_ACTION_SLEEP);
    }
}

void show_sleep_countdown_and_sleep(void)
{
    if (g_sleep_countdown_active) return; // 已经在倒计时，直接返回
    g_sleep_countdown_active = 1;         // 设置标志

    static lv_font_t *g_tip_font = NULL;
    static lv_font_t *g_big_font = NULL;
    
    const int tip_font_size = 36;
    const int big_font_size = 120;

    if (!g_tip_font)
        g_tip_font = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, tip_font_size);
    if (!g_big_font)
        g_big_font = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, big_font_size);

    if (!sleep_screen) {
        sleep_screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(sleep_screen, lv_color_hex(0x000000), 0);
    }
    lv_obj_clean(sleep_screen);
    lv_screen_load(sleep_screen);

    // 顶部“即将休眠”label
    static lv_style_t style_tip_sleep;
    lv_style_init(&style_tip_sleep);
    lv_style_set_text_font(&style_tip_sleep, g_tip_font);
    lv_style_set_text_color(&style_tip_sleep, lv_color_hex(0xFFFFFF));
    lv_obj_t *tip_label = lv_label_create(sleep_screen);
    lv_label_set_text(tip_label, "即将休眠");
    lv_obj_add_style(tip_label, &style_tip_sleep, 0);
    lv_obj_align(tip_label, LV_ALIGN_TOP_MID, 0, 20);

    // 中间倒计时数字
    static lv_style_t style_big_sleep;
    lv_style_init(&style_big_sleep);
    lv_style_set_text_font(&style_big_sleep, g_big_font);
    lv_style_set_text_color(&style_big_sleep, lv_color_hex(0xFFFFFF));
    sleep_label = lv_label_create(sleep_screen);
    lv_obj_add_style(sleep_label, &style_big_sleep, 0);
    lv_obj_center(sleep_label);
    lv_label_set_text(sleep_label, "3"); 

    sleep_countdown = 3;
    if (sleep_timer)
        lv_timer_delete(sleep_timer);
    sleep_timer = lv_timer_create(sleep_countdown_cb, 1000, NULL);

    // 立即显示第一个数字
    sleep_countdown_cb(sleep_timer);
}


static void low_battery_shutdown_countdown_cb(lv_timer_t *timer)
{   
     // 检查是否正在充电
    if (rt_pin_read(44)) 
    {
        // 清理定时器
        if (low_battery_shutdown_timer) {
            lv_timer_delete(low_battery_shutdown_timer);
            low_battery_shutdown_timer = NULL;
        }
        
        // 清理所有LVGL对象
        if (low_battery_countdown_label) {
            lv_obj_delete(low_battery_countdown_label);
            low_battery_countdown_label = NULL;
        }
        
        if (low_battery_label) {
            lv_obj_delete(low_battery_label);
            low_battery_label = NULL;
        }
        
        g_low_battery_shutdown_active = 0; // 清除标志
        low_shutdown_index = 0; // 重置关机索引
        
        if (low_battery_shutdown_screen) {
            lv_obj_clean(low_battery_shutdown_screen);
        }
        
        rt_kprintf("在充电 不关机了\n");
        low_battery_shutdown_triggered = true;
        if(g_screen_before_low_battery == standby_screen)
        {
            ui_swith_to_standby_screen();
        }
        else
        {
            ui_switch_to_xiaozhi_screen();
        }
        return;
    }
    if(low_shutdown_index == 1)
    {
        lv_timer_delete(low_battery_shutdown_timer);
        low_battery_shutdown_timer = NULL;
        // 执行关机
        PowerDownCustom();
        rt_kprintf("bu gai chu xian\n");  
    }
    if (low_battery_countdown_label && low_battery_countdown > 0)
    {
        char num[2] = {0};
        snprintf(num, sizeof(num), "%d", low_battery_countdown);
        lv_label_set_text(low_battery_countdown_label, num);
        lv_obj_center(low_battery_countdown_label);
        low_battery_countdown--;
    }
    else
    {
        // 清理所有LVGL对象
        if (low_battery_countdown_label) {
            lv_obj_delete(low_battery_countdown_label);
            low_battery_countdown_label = NULL;
        }
        
        if (low_battery_label) {
            lv_obj_delete(low_battery_label);
            low_battery_label = NULL;
        }
        g_low_battery_shutdown_active = 0; // 倒计时结束，清除标志
        rt_kprintf("low battery shutdown countdown ok\n");
        
        if (low_battery_shutdown_screen) {
            lv_obj_clean(low_battery_shutdown_screen);
        }
        
        rt_thread_delay(200);
        low_shutdown_index = 1;
    }
}
void show_low_battery_shutdown(void)
{
    if (g_low_battery_shutdown_active) return; // 已经在倒计时，直接返回
    g_low_battery_shutdown_active = 1;         // 设置标志

    static lv_font_t *g_tip_font = NULL;
    static lv_font_t *g_big_font = NULL;
    const int tip_font_size = 36;
    const int big_font_size = 120;

    if (!g_tip_font)
        g_tip_font = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, tip_font_size);
    if (!g_big_font)
        g_big_font = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, big_font_size);

    if (!low_battery_shutdown_screen) {
        low_battery_shutdown_screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(low_battery_shutdown_screen, lv_color_hex(0x000000), 0);
    }
    
    lv_obj_clean(low_battery_shutdown_screen);
    lv_screen_load(low_battery_shutdown_screen);

    // 顶部"电量低，即将关机"label - 红色文本
    static lv_style_t style_tip_low_battery;
    lv_style_init(&style_tip_low_battery);
    lv_style_set_text_font(&style_tip_low_battery, g_tip_font);
    lv_style_set_text_color(&style_tip_low_battery, lv_color_hex(0xFF0000)); // 红色
    low_battery_label = lv_label_create(low_battery_shutdown_screen);
    lv_label_set_text(low_battery_label, "电量低，即将关机");
    lv_obj_add_style(low_battery_label, &style_tip_low_battery, 0);
    lv_obj_align(low_battery_label, LV_ALIGN_TOP_MID, 0, 20);

    // 中间倒计时数字 - 红色文本
    static lv_style_t style_big_low_battery;
    lv_style_init(&style_big_low_battery);
    lv_style_set_text_font(&style_big_low_battery, g_big_font);
    lv_style_set_text_color(&style_big_low_battery, lv_color_hex(0xFF0000)); // 红色
    low_battery_countdown_label = lv_label_create(low_battery_shutdown_screen);
    lv_obj_add_style(low_battery_countdown_label, &style_big_low_battery, 0);
    lv_obj_center(low_battery_countdown_label);
    lv_label_set_text(low_battery_countdown_label, "3"); 

    low_battery_countdown = 3;
    if (low_battery_shutdown_timer)
        lv_timer_delete(low_battery_shutdown_timer);
    low_battery_shutdown_timer = lv_timer_create(low_battery_shutdown_countdown_cb, 1000, NULL);

    // 立即显示第一个数字
    low_battery_shutdown_countdown_cb(low_battery_shutdown_timer);
}

static void shutdown_countdown_cb(lv_timer_t *timer)
{
    if(shutdown_index == 1)
    {
        lv_timer_delete(shutdown_timer);
        shutdown_timer = NULL;
        // 执行关机
        PowerDownCustom();
        rt_kprintf("bu gai chu xian\n");  
    }
    if (shutdown_label && shutdown_countdown > 0)
    {
        char num[2] = {0};
        snprintf(num, sizeof(num), "%d", shutdown_countdown);
        lv_label_set_text(shutdown_label, num);
        lv_obj_center(shutdown_label);
        shutdown_countdown--;
    }
    else
    {
        // 清理所有LVGL对象
        if (shutdown_label) {
            lv_obj_delete(shutdown_label);
            shutdown_label = NULL;
        }
        
        g_shutdown_countdown_active = 0; // 倒计时结束，清除标志
        rt_kprintf("shutdown countdown ok\n");
        lv_obj_clean(shutdown_screen);
        rt_thread_delay(200);
        shutdown_index = 1;
    }

}
void show_shutdown(void)
{
    if (g_shutdown_countdown_active) return; // 已经在倒计时，直接返回
    g_shutdown_countdown_active = 1;         // 设置标志

    static lv_font_t *g_tip_font = NULL;
    static lv_font_t *g_big_font = NULL;
    const int tip_font_size = 36;
    const int big_font_size = 120;

    if (!g_tip_font)
        g_tip_font = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, tip_font_size);
    if (!g_big_font)
        g_big_font = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, big_font_size);

    if (!shutdown_screen) {
        shutdown_screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(shutdown_screen, lv_color_hex(0x000000), 0);
    }
    lv_obj_clean(shutdown_screen);
    lv_screen_load(shutdown_screen);

    // 顶部"准备关机"label
    static lv_style_t style_tip_shutdown;
    lv_style_init(&style_tip_shutdown);
    lv_style_set_text_font(&style_tip_shutdown, g_tip_font);
    lv_style_set_text_color(&style_tip_shutdown, lv_color_hex(0xFFFFFF));
    lv_obj_t *tip_label = lv_label_create(shutdown_screen);
    lv_label_set_text(tip_label, "准备关机");
    lv_obj_add_style(tip_label, &style_tip_shutdown, 0);
    lv_obj_align(tip_label, LV_ALIGN_TOP_MID, 0, 20);

    // 中间倒计时数字
    static lv_style_t style_big_shutdown;
    lv_style_init(&style_big_shutdown);
    lv_style_set_text_font(&style_big_shutdown, g_big_font);
    lv_style_set_text_color(&style_big_shutdown, lv_color_hex(0xFFFFFF));
    shutdown_label = lv_label_create(shutdown_screen);
    lv_obj_add_style(shutdown_label, &style_big_shutdown, 0);
    lv_obj_center(shutdown_label);
    lv_label_set_text(shutdown_label, "3"); 

    shutdown_countdown = 3;
    if (shutdown_timer)
        lv_timer_delete(shutdown_timer);
    shutdown_timer = lv_timer_create(shutdown_countdown_cb, 1000, NULL);

    // 立即显示第一个数字
    shutdown_countdown_cb(shutdown_timer);
}


static void low_battery_warning_simple_timer_cb(lv_timer_t *timer)
{
    // 停止定时器
    lv_timer_delete(timer);
    warning_shutdown_timer = NULL;
    
    g_low_battery_warning_active = 0;
    rt_kprintf("Low battery warning timeout, shutting down\n");
    
    // 清理页面
    if (low_battery_warning_screen) {
        lv_obj_clean(low_battery_warning_screen);
    }
    
    rt_thread_mdelay(200);
    PowerDownCustom();
}
void show_low_battery_warning(void)
{
    if (g_low_battery_warning_active) return;
    g_low_battery_warning_active = 1;

    if (!low_battery_warning_screen) {
        low_battery_warning_screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(low_battery_warning_screen, lv_color_hex(0x000000), 0);
    }
    
    lv_obj_clean(low_battery_warning_screen);
    lv_screen_load(low_battery_warning_screen);

    // 使用 no_power2_map 图片
    lv_obj_t *low_power_img = lv_img_create(low_battery_warning_screen);
    lv_img_set_src(low_power_img, &no_power2);
    lv_obj_center(low_power_img); // 居中显示

    rt_kprintf("Low battery warning image displayed, 3 seconds to shutdown\n");

    if (warning_shutdown_timer)
        lv_timer_delete(warning_shutdown_timer);
    warning_shutdown_timer = lv_timer_create(low_battery_warning_simple_timer_cb, 5000, NULL); // 直接5秒后执行
    lv_timer_set_repeat_count(warning_shutdown_timer, 1); // 只执行一次
}





// 开机动画淡入淡出回调
static void startup_fade_anim_cb(void *var, int32_t value)
{
    if (g_startup_img) {
        lv_obj_set_style_img_opa(g_startup_img, (lv_opa_t)value, 0);
    }
}

// 淡出完成回调
static void startup_fadeout_ready_cb(struct _lv_anim_t* anim)
{
    // 隐藏开机画面
    if (g_startup_screen) {
        lv_obj_add_flag(g_startup_screen, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clean(g_startup_screen);
    }
    
    g_startup_animation_finished = true;
    g_startup_animation_active = 0;
    rt_kprintf("Startup animation completed\n");

    // 开机动画完成后显示待机画面
    lv_obj_t *now_screen = lv_screen_active();
    if (standby_screen && now_screen != low_battery_shutdown_screen && 
        now_screen != low_battery_warning_screen && now_screen != shutdown_screen && 
        now_screen != sleep_screen)
    {
        rt_kprintf("开机->待机");
        lv_screen_load(standby_screen);
        if (cont) {
            lv_obj_set_parent(cont, lv_screen_active());
            lv_obj_move_foreground(cont);
        }
    }
}

// 定时器回调：用于延时后开始淡出动画
static void startup_fadeout_timer_cb(lv_timer_t *timer)
{
    // 停止定时器
    lv_timer_del(timer);
    
    // 开始淡出动画
    lv_anim_init(&g_startup_anim);
    lv_anim_set_var(&g_startup_anim, g_startup_img);
    lv_anim_set_values(&g_startup_anim, 255, 0); // 淡出
    lv_anim_set_time(&g_startup_anim, 800); // 0.8秒淡出
    lv_anim_set_exec_cb(&g_startup_anim, startup_fade_anim_cb);
    lv_anim_set_ready_cb(&g_startup_anim, startup_fadeout_ready_cb);
    lv_anim_start(&g_startup_anim);
    
    rt_kprintf("Starting fadeout animation\n");
}

// 开机动画淡入完成回调
static void startup_anim_ready_cb(struct _lv_anim_t* anim)
{
    // 使用LVGL定时器代替rt_thread_mdelay，避免在动画回调中阻塞
    lv_timer_t *fadeout_timer = lv_timer_create(startup_fadeout_timer_cb, 1500, NULL);
    lv_timer_set_repeat_count(fadeout_timer, 1); // 只执行一次
    
    rt_kprintf("Startup fadein completed, waiting 1.5s before fadeout\n");
}

// 开机动画函数
void show_startup_animation(void)
{
    if (g_startup_animation_active) return; // 已经在播放动画，直接返回
    g_startup_animation_active = 1;         // 设置标志
    
    rt_kprintf("Creating startup animation\n");
    
    // 检查startup_logo是否可用
    if (&startup_logo == NULL) {
        rt_kprintf("Warning: startup_logo not available, skipping animation\n");
        g_startup_animation_finished = true;
        g_startup_animation_active = 0;
        return;
    }

    // 创建全屏启动画面
    if (!g_startup_screen) {
        g_startup_screen = lv_obj_create(NULL);
    } else {
        lv_obj_clean(g_startup_screen);
    }
    
    if (!g_startup_screen) {
        rt_kprintf("Error: Failed to create startup screen\n");
        g_startup_animation_finished = true;
        g_startup_animation_active = 0;
        return;
    }
    
    lv_obj_remove_style_all(g_startup_screen);
    lv_obj_set_size(g_startup_screen, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_set_style_bg_color(g_startup_screen, lv_color_hex(0x000000), 0); // 黑色背景
    lv_obj_set_style_bg_opa(g_startup_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(g_startup_screen, LV_OBJ_FLAG_CLICKABLE);
    
    // 加载启动画面
    lv_screen_load(g_startup_screen);
    
    // 创建图片对象
    g_startup_img = lv_img_create(g_startup_screen);
    if (!g_startup_img) {
        rt_kprintf("Error: Failed to create startup image\n");
        lv_obj_del(g_startup_screen);
        g_startup_screen = NULL;
        g_startup_animation_finished = true;
        g_startup_animation_active = 0;
        return;
    }
    
    lv_img_set_src(g_startup_img, &startup_logo);
    lv_obj_center(g_startup_img); // 居中显示
    lv_obj_set_style_img_opa(g_startup_img, LV_OPA_0, 0); // 初始完全透明
    
    // 设置图片大小 - 针对200×102分辨率的logo优化
    float g_scale = get_scale_factor();
    lv_obj_set_size(g_startup_img, SCALE_DPX(180), SCALE_DPX(92)); // 宽180dp，高92dp
    lv_img_set_zoom(g_startup_img, (int)(LV_SCALE_NONE * g_scale)); // 根据缩放因子缩放
    
    // 确保启动画面在最顶层
    lv_obj_move_foreground(g_startup_screen);
    
    // 开始淡入动画
    lv_anim_init(&g_startup_anim);
    lv_anim_set_var(&g_startup_anim, g_startup_img);
    lv_anim_set_values(&g_startup_anim, 0, 255); // 淡入
    lv_anim_set_time(&g_startup_anim, 800); // 0.8秒淡入
    lv_anim_set_exec_cb(&g_startup_anim, startup_fade_anim_cb);
    lv_anim_set_ready_cb(&g_startup_anim, startup_anim_ready_cb);
    lv_anim_start(&g_startup_anim);
    
    rt_kprintf("Startup animation started\n");
}