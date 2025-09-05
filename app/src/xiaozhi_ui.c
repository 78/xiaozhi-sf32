#include "rtthread.h"
#include "bf0_hal.h"
#include "drv_io.h"
#include "littlevgl2rtt.h"
#include "lv_tiny_ttf.h"
#include "string.h"
#include "xiaozhi_public.h"
#include "bf0_pm.h"
#include "gui_app_pm.h"
#include "drv_gpio.h"
#include "lv_timer.h"
#include "lv_display.h"
#include "lv_obj_pos.h"
#include "ulog.h"
#include "drv_flash.h"
#include "xiaozhi2.h"
#include "bts2_app_inc.h"
#include "ble_connection_manager.h"
#include "bt_connection_manager.h"
#include "bt_env.h"
#include "./mcp/mcp_api.h"
#define IDLE_TIME_LIMIT  (20000)
#define SHOW_TEXT_LEN 100
#include "lv_seqimg.h"
#include "xiaozhi_ui.h" 
#include "xiaozhi_weather.h"

// 定义UI消息类型
typedef enum {
    UI_MSG_CHAT_STATUS,
    UI_MSG_CHAT_OUTPUT,
    UI_MSG_UPDATE_EMOJI,
    UI_MSG_UPDATE_BLE,
    UI_MSG_TTS_OUTPUT,
    UI_MSG_TTS_SWITCH_PART,
    UI_MSG_BUTTON_PRESSED,
    UI_MSG_BUTTON_RELEASED,
    UI_MSG_TIME_UPDATE,
    UI_MSG_WEATHER_UPDATE,
    UI_MSG_STANDBY_EMOJI,
    UI_MSG_SWITCH_TO_STANDBY,
    UI_MSG_SWITCH_TO_MAIN,
    UI_MSG_UPDATE_WEATHER_AND_TIME,
    UI_MSG_STANDBY_CHAT_OUTPUT,
    UI_MSG_VOLUME_UPDATE,  //更新下拉菜单里面的音量进度条
    UI_MSG_BRIGHTNESS_UPDATE  //更新下拉菜单里面的亮度进度条

} ui_msg_type_t;

// 定义UI消息结构
typedef struct {
    ui_msg_type_t type;
    char *data;
} ui_msg_t;
rt_mq_t ui_msg_queue = RT_NULL;

#define UPDATE_REAL_WEATHER_AND_TIME 11
#define LCD_DEVICE_NAME "lcd"
#define TOUCH_NAME "touch"
rt_mailbox_t g_ui_task_mb =RT_NULL;
// 开机动画相关全局变量
static struct rt_semaphore update_ui_sema;
extern const lv_image_dsc_t startup_logo;  //开机动画图标
static lv_obj_t *g_startup_screen = NULL;
static lv_obj_t *g_startup_img = NULL;
static lv_anim_t g_startup_anim;
static bool g_startup_animation_finished = false;
/*Create style with the new font*/
static lv_style_t style;
static lv_style_t style2;

static lv_style_t style_battery;

static lv_obj_t* volume_slider = NULL;
static lv_obj_t* brightness_lines = NULL;

/*缩放因子*/
static float g_scale = 1.0f;
#define SCALE_DPX(val) LV_DPX((val) * g_scale)
/*字体资源*/
extern const unsigned char xiaozhi_font[];
extern const int xiaozhi_font_size;
extern BOOL g_pan_connected;

/*对话界面ble图片资源*/
extern const lv_image_dsc_t ble; // ble
extern const lv_image_dsc_t ble_close;

/*对话画面*/
lv_obj_t *main_container;
static lv_obj_t *header_row;
static lv_obj_t *spacer;
static lv_obj_t *img_container;

static lv_obj_t *global_label1;
static lv_obj_t *global_label2;

static lv_obj_t *seqimg;
static lv_obj_t *global_img_ble;

lv_font_t *font_medium;




/*待机画面*/
extern const lv_image_dsc_t ble_icon_img; // 蓝牙图标
extern const lv_image_dsc_t ble_icon_img_close;
extern const lv_image_dsc_t network_icon_img; // 网络图标
extern const lv_image_dsc_t network_icon_img_close;
extern const lv_image_dsc_t sunny;// 天气图标
extern const lv_image_dsc_t strip;//天气栏
extern const lv_image_dsc_t funny2; // 表情图标
extern const lv_image_dsc_t sleepy2; // 表情图标
extern const lv_image_dsc_t cool_gif;
extern const lv_image_dsc_t calendar;//日历
extern const lv_image_dsc_t second;
extern const lv_image_dsc_t img_0;  // 数字图片资源
extern const lv_image_dsc_t img_1;
extern const lv_image_dsc_t img_2;
extern const lv_image_dsc_t img_3;
extern const lv_image_dsc_t img_4;
extern const lv_image_dsc_t img_5;
extern const lv_image_dsc_t img_6;
extern const lv_image_dsc_t img_7;
extern const lv_image_dsc_t img_8;
extern const lv_image_dsc_t img_9;

lv_obj_t *hour_tens_img;     // 小时十位数图片
lv_obj_t *hour_units_img;    // 小时个位数图片
lv_obj_t *minute_tens_img;   // 分钟十位数图片
lv_obj_t *minute_units_img;  // 分钟个位数图片
lv_obj_t *temperature_label = NULL;  // 温度标签
lv_obj_t *last_update_label = NULL;  // 上次更新时间标签
lv_obj_t *battery_arc = NULL;           // 电池圆环显示
lv_obj_t *battery_percent_label = NULL; // 电池电量百分比标签
lv_obj_t *bluetooth_icon = NULL;//蓝牙图标
lv_obj_t *img_emoji = NULL;//表情图标
lv_obj_t *network_icon = NULL;//网络图标

lv_obj_t *weather_bgimg;//天气背景图片
lv_obj_t *weather_icon;//天气图标
lv_obj_t *ui_Image_calendar;//日历图标
lv_obj_t *standby_screen = NULL;//待机界面
lv_obj_t *ui_Label_ip = NULL;//地址和温度标签
lv_obj_t *last_time = NULL;//上次更新天气图标
lv_obj_t *ui_Label_year =NULL;//年份
lv_obj_t *ui_Label_day = NULL;//日期
lv_obj_t *ui_Label_second =NULL;//秒
lv_obj_t *ui_Image_second = NULL;//秒的图片
lv_obj_t * ui_Arc2 = NULL;//电池容器
lv_obj_t * ui_Label3 = NULL;

static lv_timer_t* standby_update_timer = NULL;
static rt_timer_t bg_update_timer = NULL;
rt_timer_t update_time_ui_timer = RT_NULL;
rt_timer_t update_weather_ui_timer = RT_NULL;
static rt_timer_t g_split_text_timer = RT_NULL;


#if defined(__CC_ARM) || defined(__CLANG_ARM)
L2_RET_BSS_SECT_BEGIN(g_second_part) //6000地址
static char g_second_part[512];
L2_RET_BSS_SECT_END
#else
static char g_second_part[512] L2_RET_BSS_SECT(g_second_part);
#endif

static lv_obj_t *g_label_for_second_part = NULL;

static lv_obj_t *cont = NULL;

#define CONT_IDLE           0x01
#define CONT_HIDDEN         0x02
#define CONT_DEFAULT_STATUS     (CONT_IDLE | CONT_HIDDEN)
#define USING_TOUCH_SWITCH  1

#define USING_BTN_SWITCH    0
#define ANIM_TIMEOUT        300

static uint8_t cont_status = CONT_DEFAULT_STATUS;
static uint32_t anim_tick = 0;
uint8_t vad_enable = 1;      //0是支持打断，1是不支持打断


#if defined (KWS_ENABLE_DEFAULT) && KWS_ENABLE_DEFAULT
uint8_t aec_enabled = 1;
#else
uint8_t aec_enabled = 0;
#endif
// xiaozhi2
extern rt_mailbox_t g_button_event_mb;
extern xiaozhi_ws_t g_xz_ws;
extern void ws_send_speak_abort(void *ws, char *session_id, int reason);
extern void ws_send_listen_start(void *ws, char *session_id,
                                 enum ListeningMode mode);
extern void ws_send_listen_stop(void *ws, char *session_id);

extern void send_xz_config_msg_to_main(void);
extern void xz_mic_open(xz_audio_t *thiz);
extern void xz_mic_close(xz_audio_t *thiz);
extern void kws_demo();
extern void show_shutdown(void);

extern xz_audio_t xz_audio;
xz_audio_t *thiz = &xz_audio;
extern rt_mailbox_t g_battery_mb;
extern lv_obj_t *shutdown_screen;
extern lv_obj_t *sleep_screen;
// 默认oled电池图标尺寸
#define OUTLINE_W 58
#define OUTLINE_H 33

// LCD_USING_ST7789电池图标尺寸
#define OUTLINE_W_ST7789 40
#define OUTLINE_H_ST7789 20

// 全局变量存储当前电池电量
static int g_battery_level = 60;        // 默认为满电
static lv_obj_t *g_battery_fill = NULL;  // 电池填充对象
static lv_obj_t *g_battery_label = NULL; // 电量标签

// 亮度表需要按照从小到大排序
static const uint16_t brigtness_tb[] = 
{
    LCD_BRIGHTNESS_MIN,
    LCD_BRIGHTNESS_MID,
    LCD_BRIGHTNESS_MAX,
};
#define BRT_TB_SIZE     (sizeof(brigtness_tb)/sizeof(brigtness_tb[0]))

#define BASE_WIDTH 390
#define BASE_HEIGHT 450
// 文本复制函数
static char* ui_strdup(const char* str) {
    if (str == RT_NULL) return RT_NULL;
    size_t len = strlen(str) + 1;
    char* copy = (char*)rt_malloc(len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}


// 文本释放函数
static void ui_free(char* str) {
    if (str) {
        rt_free(str);
    }
}


/*开机动画*/

// 获取当前屏幕尺寸并计算缩放因子
static float get_scale_factor(void)
{
    lv_disp_t *disp = lv_disp_get_default();
    lv_coord_t scr_width = lv_disp_get_hor_res(disp);
    lv_coord_t scr_height = lv_disp_get_ver_res(disp);

    float scale_x = (float)scr_width / BASE_WIDTH;
    float scale_y = (float)scr_height / BASE_HEIGHT;

    return (scale_x < scale_y) ? scale_x : scale_y;
}
// 开机动画淡入淡出回调
static void startup_fade_anim_cb(void *var, int32_t value)
{
    if (g_startup_img) {
        lv_obj_set_style_img_opa(g_startup_img, (lv_opa_t)value, 0);
    }
}
lv_timer_t *ui_sleep_timer = NULL;
void ui_sleep_callback(lv_timer_t *timer)
{
    rt_kprintf("in dai_ji,so xiu mian");
    if(thiz->vad_enabled)
    {
        thiz->vad_enabled = false;
        rt_kprintf("in PM,so vad_close\n");
        xz_aec_mic_close(thiz);
    }
    if(aec_enabled) 
    {
        kws_demo();
    } 
    show_sleep_countdown_and_sleep();
    lv_timer_delete(ui_sleep_timer);
    ui_sleep_timer = NULL;
}

void ui_update_real_weather_and_time(void);

static void standby_update_callback(lv_timer_t *timer)
{
    ui_update_real_weather_and_time();
    
    // 删除定时器（一次性使用）
    lv_timer_delete(timer);
    standby_update_timer = NULL;
}


// 淡出完成回调
static void startup_fadeout_ready_cb(struct _lv_anim_t* anim)
{
    // 隐藏开机画面
    if (g_startup_screen) {
        lv_obj_add_flag(g_startup_screen, LV_OBJ_FLAG_HIDDEN);
    }
    g_startup_animation_finished = true;
    rt_kprintf("Startup animation completed\n");

        // 开机动画完成后显示待机画面
    if (standby_screen) {
        rt_kprintf("开机->待机");
        lv_screen_load(standby_screen);
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


static void switch_cont_anim(bool hidden);
static void contdown_anim_ready_cb(struct _lv_anim_t* anim)
{
    switch_cont_anim(true);
    cont_status |= CONT_HIDDEN;
    cont_status &= (uint8_t)(~CONT_IDLE);
    LOG_I("%s",__func__);

}

static void countdown_anim(void)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_delete(cont, NULL);
    lv_anim_set_var(&a, cont);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_set_duration(&a, 3000);
    lv_anim_set_ready_cb(&a, contdown_anim_ready_cb);
    lv_anim_start(&a);
    LOG_I("%s",__func__);
}

static void countdown_anim_del(void)
{
    lv_anim_delete(cont, NULL);
}


static void enable_indev(bool enable)
{
    lv_indev_t *i = lv_indev_get_next(NULL);
    while (i)
    {
        if ((lv_indev_get_type(i) != LV_INDEV_TYPE_POINTER))
        {
            if (!enable)
            {
                lv_indev_reset(i, NULL);
            }
            lv_indev_enable(i, enable);
        }
        i = lv_indev_get_next(i);
    }
}

static void switch_cont_anim_ready_cb(struct lv_anim_t* anim)
{
    lv_obj_t* obj = anim->var;
    if(lv_obj_get_y(obj) + lv_obj_get_height(obj) > 0)
    {
        countdown_anim();
    }
    cont_status |= CONT_IDLE;
    anim_tick = 0;
    enable_indev(true);

    LOG_I("%s:status %d",__func__, cont_status);
}

static void switch_cont_anim(bool hidden)
{
    LOG_I("%s:hidden %d",__func__, hidden);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, cont);
    lv_anim_del(cont, NULL);
    enable_indev(false);

    if (hidden)
    {
        lv_anim_set_values(&a, lv_obj_get_y(cont), -lv_obj_get_height(cont));
        lv_anim_set_ready_cb(&a, switch_cont_anim_ready_cb);
    }
    else
    {
        lv_anim_set_values(&a, lv_obj_get_y(cont), 0);
        lv_anim_set_ready_cb(&a, switch_cont_anim_ready_cb);
    }
    lv_anim_set_duration(&a, 200);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);

    lv_anim_start(&a);
    anim_tick = lv_tick_get();
}

static void switch_anim_timeout_check(void)
{
    if(anim_tick && (anim_tick + ANIM_TIMEOUT < lv_tick_get()) && 0 == (cont_status & CONT_IDLE))
    {
        LOG_I("%s:to set hidden %d",__func__, cont_status & CONT_HIDDEN);
        if(cont_status & CONT_HIDDEN)
        {
            switch_cont_anim(true);
        }
        else 
        {
            switch_cont_anim(false);
        }
    }
}


static void header_row_event_handler(struct _lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_SHORT_CLICKED)
    {
        if(0 == (cont_status & CONT_IDLE)) return;
        if(cont_status & CONT_HIDDEN)
        {
            switch_cont_anim(false);
            cont_status &= (uint8_t)~CONT_HIDDEN;
        }
        else 
        {
            switch_cont_anim(true);
            cont_status |= CONT_HIDDEN;
        }
        cont_status &= (uint8_t)(~CONT_IDLE);
    }
}

static lv_obj_t* create_tip_label(lv_obj_t* parent, const char* tips, uint8_t row, uint8_t col)
{
    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_remove_style_all(obj);
    lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, col, 1,
        LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_set_style_bg_opa(obj, 0, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* label = lv_label_create(obj);
    lv_obj_add_style(label, &style, 0);
    lv_label_set_text_fmt(label, tips);
    lv_obj_center(label);
    return label;
}

static lv_obj_t* create_switch(lv_obj_t* parent,lv_event_cb_t cb, uint8_t row, uint8_t col, uint8_t checked)
{
    lv_obj_t* sw = lv_switch_create(parent);
    lv_obj_add_flag(sw, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(sw, cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_grid_cell(sw, LV_GRID_ALIGN_STRETCH, col, 1,
        LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_set_style_radius(sw, 200, 0);    //to avoid memory malloc failed
    if(checked)
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    return sw;
}

static lv_obj_t* create_slider(lv_obj_t* parent, lv_event_cb_t cb, uint8_t row, uint8_t col, int32_t min, int32_t max, uint8_t val)
{
    lv_obj_t* slider = lv_slider_create(parent);
    lv_obj_set_grid_cell(slider, LV_GRID_ALIGN_STRETCH, col, 2,
        LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_add_event_cb(slider, cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, val, LV_ANIM_OFF);
    lv_obj_add_flag(slider, LV_OBJ_FLAG_EVENT_BUBBLE);
    return slider;
}

static lv_obj_t* create_lines(lv_obj_t* parent, lv_event_cb_t cb, uint8_t row, uint8_t col, uint16_t cnt, uint16_t val)
{
#define COL_PAD_ALL_PCT     10

    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_remove_style_all(obj);
    lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, col, 2,
        LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_set_style_bg_opa(obj, 0, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_layout(obj, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj,
        LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);

    lv_obj_t* btn;
    for (uint32_t i = 0; i < cnt; i++)
    {
        btn = lv_btn_create(obj);
        lv_obj_remove_style_all(btn);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        if (brigtness_tb[i] <= val)
            lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_LIGHT_GREEN), 0);
        else
            lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_GREY), 0);
        lv_obj_set_size(btn, LV_PCT((100 - COL_PAD_ALL_PCT)/cnt), LV_PCT(80));
        lv_obj_add_flag(btn, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_add_event_cb(btn, cb, LV_EVENT_SHORT_CLICKED, (void *)i);
        lv_obj_set_ext_click_area(btn, LV_DPX(8));
        lv_obj_set_user_data(btn, obj);
    }
    return obj;
}


static void cont_event_handler(struct lv_event_t* e)
{
    lv_obj_t* cont = lv_event_get_current_target_obj(e);
    lv_event_code_t code = lv_event_get_code(e);
    static uint32_t press_tick = 0;
    static lv_point_t press_pos = {0};

    if (lv_obj_get_y(cont) != 0) return;

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
        uint32_t release_tick = lv_tick_get();
        lv_point_t release_pos;
        lv_indev_get_point(lv_indev_get_act(), &release_pos);
        int32_t dx = release_pos.x - press_pos.x;
        int32_t dy = release_pos.y - press_pos.y;
        if(release_tick - press_tick < 500 &&  dy < 0 && abs(dy) > 50) 
        {
            switch_cont_anim(true);
            cont_status |= CONT_HIDDEN;
            cont_status &= ~CONT_IDLE;
        }
        else
        {
            countdown_anim();
        }
    }
    else if(code == LV_EVENT_PRESSED)
    {
        countdown_anim_del();

        press_tick = lv_tick_get();
        lv_indev_get_point(lv_indev_get_act(), &press_pos);

    }
}

static void vad_switch_event_handler(struct _lv_event_t* e)
{
    lv_obj_t * obj = lv_event_get_current_target(e);
//    vad_set_enable(lv_obj_has_state(obj, LV_STATE_CHECKED));
//    send_xz_config_msg_to_main();
    vad_enable = !vad_enable; // 取反
    rt_kprintf("vad_status: %d\n", vad_enable);
}

static void aec_switch_event_handler(struct _lv_event_t* e)
{
    lv_obj_t * obj = lv_event_get_current_target(e);
//    aec_set_enable(lv_obj_has_state(obj, LV_STATE_CHECKED));
//    send_xz_config_msg_to_main();
    aec_enabled = !aec_enabled; // 取反
    rt_kprintf("aec_status: %d\n", aec_enabled);
}

static void slider_event_handler(struct _lv_event_t* e)
{
    lv_obj_t* slider = lv_event_get_current_target_obj(e);
    audio_server_set_private_volume(AUDIO_TYPE_LOCAL_MUSIC, lv_slider_get_value(slider)); // 设置音量 
}

static void line_event_handler(struct _lv_event_t* e)
{
    uint32_t idx = (uint32_t)lv_event_get_user_data(e);
    lv_obj_t* obj = lv_event_get_current_target_obj(e);
    lv_obj_t* parent = (lv_obj_t *)lv_obj_get_user_data(obj);
    uint32_t cnt = lv_obj_get_child_count(parent);
    lv_obj_t* child;
    uint16_t i = 0;
    while(i < cnt)
    {
         child = lv_obj_get_child(parent, i);
         if (i <= idx)
         {
             lv_obj_set_style_bg_color(child, lv_palette_main(LV_PALETTE_LIGHT_GREEN), 0); 
         }
         else
         {
             lv_obj_set_style_bg_color(child, lv_palette_main(LV_PALETTE_GREY), 0);
         }
         i++;
    }
    rt_kprintf("set brightness %d", brigtness_tb[idx]);
    xz_set_lcd_brightness(brigtness_tb[idx]);
}

lv_obj_t * ui_Image1 = NULL;
lv_obj_t * ui_Image2 = NULL;
lv_obj_t * ui_Image3 = NULL;
lv_obj_t * ui_Image4 = NULL;
lv_obj_t * ui_Image5 = NULL;
lv_obj_t * ui_Image6 = NULL;
lv_obj_t * ui_Image7 = NULL;
lv_obj_t * ui_Container2 = NULL;
lv_obj_t * ui_Container3 = NULL;
lv_obj_t * ui_Image9 = NULL;

rt_err_t xiaozhi_ui_obj_init()
{

        // 获取屏幕分辨率
    lv_coord_t scr_width = lv_disp_get_hor_res(NULL);
    lv_coord_t scr_height = lv_disp_get_ver_res(NULL);
   
    standby_screen = lv_obj_create(NULL);
    lv_obj_clear_flag(standby_screen, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(standby_screen, lv_color_hex(0x000000), 0);//黑色

    img_emoji = lv_img_create(standby_screen);
    LV_IMAGE_DECLARE(sleepy2);
    LV_IMAGE_DECLARE(funny2);
    lv_img_set_src(img_emoji, &sleepy2);//初始化提示小智还未连接
    lv_obj_set_width(img_emoji, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(img_emoji, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(img_emoji, (int)(104 * g_scale));
    lv_obj_set_y(img_emoji, (int)(-123 * g_scale));
    lv_obj_set_align(img_emoji, LV_ALIGN_CENTER);
    lv_obj_add_flag(img_emoji, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(img_emoji, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_img_set_zoom(img_emoji, (int)(LV_SCALE_NONE * g_scale)); // 根据缩放因子缩放

    hour_tens_img = lv_img_create(standby_screen);
    LV_IMAGE_DECLARE(img_1);
    lv_img_set_src(hour_tens_img, &img_1);
    lv_obj_set_width(hour_tens_img, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(hour_tens_img, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(hour_tens_img, (int)(-142 * g_scale));
    lv_obj_set_y(hour_tens_img, (int)(-163 * g_scale));
    lv_obj_set_align(hour_tens_img, LV_ALIGN_CENTER);
    lv_obj_add_flag(hour_tens_img, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(hour_tens_img, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_img_set_zoom(hour_tens_img, (int)(204 * g_scale));

    hour_units_img = lv_img_create(standby_screen);
    LV_IMAGE_DECLARE(img_2);
    lv_img_set_src(hour_units_img, &img_2);
    lv_obj_set_width(hour_units_img, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(hour_units_img, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(hour_units_img, (int)(-73 * g_scale));
    lv_obj_set_y(hour_units_img, (int)(-163 * g_scale));
    lv_obj_set_align(hour_units_img, LV_ALIGN_CENTER);
    lv_obj_add_flag(hour_units_img, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(hour_units_img, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_img_set_zoom(hour_units_img, (int)(204 * g_scale));

    minute_tens_img = lv_img_create(standby_screen);
        LV_IMAGE_DECLARE(img_3);

    lv_img_set_src(minute_tens_img, &img_3);
    lv_obj_set_width(minute_tens_img, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(minute_tens_img, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(minute_tens_img, (int)(-142 * g_scale));
    lv_obj_set_y(minute_tens_img, (int)(-66 * g_scale));
    lv_obj_set_align(minute_tens_img, LV_ALIGN_CENTER);
    lv_obj_add_flag(minute_tens_img, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(minute_tens_img, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_img_set_zoom(minute_tens_img, (int)(204 * g_scale));

    minute_units_img = lv_img_create(standby_screen);
            LV_IMAGE_DECLARE(img_4);

    lv_img_set_src(minute_units_img, &img_4);
    lv_obj_set_width(minute_units_img, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(minute_units_img, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(minute_units_img, (int)(-73 * g_scale));
    lv_obj_set_y(minute_units_img, (int)(-66 * g_scale));
    lv_obj_set_align(minute_units_img, LV_ALIGN_CENTER);
    lv_obj_add_flag(minute_units_img, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(minute_units_img, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_img_set_zoom(minute_units_img, (int)(204 * g_scale));

    bluetooth_icon = lv_img_create(standby_screen);
        LV_IMAGE_DECLARE(ble_icon_img);

    lv_img_set_src(bluetooth_icon, &ble_icon_img);
    lv_obj_set_width(bluetooth_icon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(bluetooth_icon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(bluetooth_icon, (int)(-134 * g_scale));
    lv_obj_set_y(bluetooth_icon, (int)(133 * g_scale));
    lv_obj_set_align(bluetooth_icon, LV_ALIGN_CENTER);
    lv_obj_add_flag(bluetooth_icon, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(bluetooth_icon, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_img_set_zoom(bluetooth_icon, (int)(LV_SCALE_NONE * g_scale));

    network_icon = lv_img_create(standby_screen);
        LV_IMAGE_DECLARE(network_icon_img);

    lv_img_set_src(network_icon, &network_icon_img);
    lv_obj_set_width(network_icon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(network_icon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(network_icon, (int)(0 * g_scale));
    lv_obj_set_y(network_icon, (int)(133 * g_scale));
    lv_obj_set_align(network_icon, LV_ALIGN_CENTER);
    lv_obj_add_flag(network_icon, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(network_icon, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_img_set_zoom(network_icon, (int)(384 * g_scale));

    // 电池环形
    battery_arc = lv_arc_create(standby_screen);
    lv_obj_set_size(battery_arc, (int)(60 * g_scale), (int)(60 * g_scale)); // 设置圆弧大小
    lv_obj_set_x(battery_arc, (int)(134 * g_scale));
    lv_obj_set_y(battery_arc, (int)(133 * g_scale));
    lv_obj_set_align(battery_arc, LV_ALIGN_CENTER);
    lv_arc_set_rotation(battery_arc, 270); // 从顶部开始
    lv_arc_set_bg_angles(battery_arc, 0, 360); // 背景圆环完整
    lv_arc_set_value(battery_arc, g_battery_level); // 全局电量
    lv_obj_remove_style(battery_arc, NULL, LV_PART_KNOB); // 移除旋钮
    lv_obj_set_style_arc_color(battery_arc, lv_color_hex(0x333333), LV_PART_MAIN); // 背景圆环颜色
    lv_obj_set_style_arc_color(battery_arc, lv_color_hex(0x00CC00), LV_PART_INDICATOR); // 电量颜色(绿色)
    lv_obj_set_style_arc_width(battery_arc, (int)(8 * g_scale), LV_PART_MAIN); // 背景圆环宽度
    lv_obj_set_style_arc_width(battery_arc, (int)(6 * g_scale), LV_PART_INDICATOR); // 电量指示宽度
    // 电池电量百分比文本
    battery_percent_label = lv_label_create(battery_arc);
    lv_label_set_text_fmt(battery_percent_label, "%d%%", g_battery_level);
    lv_obj_set_style_text_color(battery_percent_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(battery_percent_label, font_medium, 0);
    lv_obj_align(battery_percent_label, LV_ALIGN_CENTER, 0, 0); // 在圆弧中心


//天气
    weather_bgimg = lv_img_create(standby_screen);
    LV_IMAGE_DECLARE(strip);
    lv_img_set_src(weather_bgimg, &strip);
    lv_obj_set_width(weather_bgimg, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(weather_bgimg, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(weather_bgimg, (int)(70 * g_scale));
    lv_obj_set_y(weather_bgimg, (int)(52 * g_scale));
    lv_obj_set_align(weather_bgimg, LV_ALIGN_CENTER);
    lv_obj_add_flag(weather_bgimg, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(weather_bgimg, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_img_set_zoom(weather_bgimg, (int)(550 * g_scale));

    weather_icon = lv_img_create(standby_screen);
    LV_IMAGE_DECLARE(sunny);

    lv_img_set_src(weather_icon, &sunny);
    lv_obj_set_width(weather_icon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(weather_icon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(weather_icon, (int)(0 * g_scale));
    lv_obj_set_y(weather_icon, (int)(50 * g_scale));
    lv_obj_set_align(weather_icon, LV_ALIGN_CENTER);
    lv_obj_add_flag(weather_icon, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(weather_icon, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_img_set_zoom(weather_icon, (int)(LV_SCALE_NONE * g_scale)); // 根据缩放因子缩放

    ui_Label_ip = lv_label_create(standby_screen);
    lv_obj_set_width(ui_Label_ip, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label_ip, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label_ip, (int)(80 * g_scale));
    lv_obj_set_y(ui_Label_ip, (int)(31 * g_scale));
    lv_obj_set_align(ui_Label_ip, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label_ip, "IP.00°");
    lv_obj_add_style(ui_Label_ip, &style2, 0);


    last_time = lv_label_create(standby_screen);
    lv_obj_set_width(last_time, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(last_time, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(last_time, (int)(80 * g_scale));
    lv_obj_set_y(last_time, (int)(69 * g_scale));
    lv_obj_set_align(last_time, LV_ALIGN_CENTER);
    lv_label_set_text(last_time, "00:00");
    lv_obj_add_style(last_time, &style2, 0);


//日历
    ui_Image_calendar = lv_img_create(standby_screen);
    LV_IMAGE_DECLARE(calendar);
    lv_img_set_src(ui_Image_calendar, &calendar);
    lv_obj_set_width(ui_Image_calendar, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Image_calendar, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Image_calendar, (int)(-107 * g_scale));
    lv_obj_set_y(ui_Image_calendar, (int)(39 * g_scale));
    lv_obj_set_align(ui_Image_calendar, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Image_calendar, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_Image_calendar, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_img_set_zoom(ui_Image_calendar,(int)(320 * g_scale));

    ui_Label_year = lv_label_create(standby_screen);
    lv_obj_set_width(ui_Label_year, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label_year, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label_year, (int)(-109 * g_scale));
    lv_obj_set_y(ui_Label_year, (int)(49 * g_scale));
    lv_obj_set_align(ui_Label_year, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label_year, "2025");
        lv_obj_add_style(ui_Label_year, &style, 0);


     ui_Label_day = lv_label_create(standby_screen);
    lv_obj_set_width(ui_Label_day, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label_day, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label_day, (int)(-109 * g_scale));
    lv_obj_set_y(ui_Label_day, (int)(74 * g_scale));
    lv_obj_set_align(ui_Label_day, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label_day, "0801");
        lv_obj_add_style(ui_Label_day, &style, 0);

//秒
     ui_Label_second = lv_label_create(standby_screen);
    lv_obj_set_width(ui_Label_second, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label_second, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label_second, (int)(-8 * g_scale));
    lv_obj_set_y(ui_Label_second, (int)(-48 * g_scale));
    lv_obj_set_align(ui_Label_second, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label_second, "00");
    lv_obj_add_style(ui_Label_second, &style, 0);

    ui_Image_second = lv_img_create(standby_screen);
    LV_IMAGE_DECLARE(second);
    lv_img_set_src(ui_Image_second, &second);
    lv_obj_set_width(ui_Image_second, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Image_second, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Image_second, (int)(-7 * g_scale));
    lv_obj_set_y(ui_Image_second, (int)(-47 * g_scale));
    lv_obj_set_align(ui_Image_second, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Image_second, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(ui_Image_second, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_img_set_zoom(ui_Image_second,(int)(300 * g_scale));


    ui_Label3 = lv_label_create(standby_screen);
    lv_obj_set_width(ui_Label3, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label3, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label3, (int)(2 * g_scale));
    lv_obj_set_y(ui_Label3, (int)(189 * g_scale));
    lv_obj_set_align(ui_Label3, LV_ALIGN_CENTER);
    lv_obj_add_style(ui_Label3, &style2, 0);
    lv_label_set_text(ui_Label3, "等待连接");

    LV_IMAGE_DECLARE(ble);
    LV_IMAGE_DECLARE(ble_close);



    // 创建主容器 - Flex Column，垂直排列
    main_container = lv_obj_create(lv_screen_active());
    lv_obj_remove_flag(main_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(main_container, scr_width, scr_height);

    // 清除主容器的 padding 和 margin
    lv_obj_set_style_pad_all(main_container, 0, 0);
    lv_obj_set_style_margin_all(main_container, 0, 0);

    lv_obj_set_style_bg_opa(main_container, LV_OPA_0, 0);
    lv_obj_set_style_border_width(main_container, 0, 0);

    lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_container,
                          LV_FLEX_ALIGN_START,   // 主轴靠上对齐
                          LV_FLEX_ALIGN_CENTER,  // 交叉轴居中
                          LV_FLEX_ALIGN_CENTER); // 对齐方式统一
    lv_obj_set_style_pad_gap(main_container, 0, 0);//子元素之间为0

    // 顶部状态栏容器（Flex Row）
    header_row = lv_obj_create(main_container);
    lv_obj_remove_flag(header_row, LV_OBJ_FLAG_SCROLLABLE); // 关闭滚动条
    lv_obj_set_size(header_row, scr_width, SCALE_DPX(40));  // 固定高度为 40dp
#if USING_TOUCH_SWITCH
    lv_obj_add_event_cb(header_row, header_row_event_handler, LV_EVENT_ALL, NULL);
#endif


    // 清除 header_row 的内边距和外边距
    lv_obj_set_style_pad_all(header_row, 0, 0);
    lv_obj_set_style_margin_all(header_row, 0, 0);
    // 设置 header_row 的背景透明和边框宽度为 0
    lv_obj_set_style_bg_opa(header_row, LV_OPA_0, 0);
    lv_obj_set_style_border_width(header_row, 0, 0);
    lv_obj_set_flex_flow(header_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(header_row, lv_color_hex(0x000000), LV_STATE_DEFAULT); // 调试背景色
    lv_obj_set_style_bg_opa(header_row, LV_OPA_30, LV_STATE_DEFAULT);
    // 插入一个空白对象作为左边距
    lv_obj_t *spacer = lv_obj_create(header_row);
    lv_obj_remove_flag(spacer, LV_OBJ_FLAG_SCROLLABLE); // 关闭滚动条
    lv_obj_set_style_bg_opa(spacer, LV_OPA_0, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_set_size(spacer, SCALE_DPX(40), LV_SIZE_CONTENT); // 宽度为 40dp

    // BLE 图标 - 左上角
    global_img_ble = lv_img_create(header_row);
    lv_img_set_src(global_img_ble, &ble);
    lv_obj_set_size(global_img_ble, SCALE_DPX(24), SCALE_DPX(24)); // 24dp 图标
    lv_img_set_zoom(global_img_ble,
                    (int)(LV_SCALE_NONE * g_scale)); // 根据缩放因子缩放

    // Top Label - 居中显示
    global_label1 = lv_label_create(header_row);
    lv_label_set_long_mode(global_label1, LV_LABEL_LONG_WRAP);
    lv_obj_add_style(global_label1, &style, 0);
    lv_obj_set_width(global_label1, LV_PCT(80));
    lv_obj_set_style_text_align(global_label1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align_to(global_label1, header_row, LV_ALIGN_CENTER, 0, 0);

    // 电池图标 - 放在 header_row 容器中，与 BLE 图标对称
    lv_obj_t* battery_outline = lv_obj_create(header_row);
    lv_obj_set_style_border_width(battery_outline, 2, 0);// 边框宽度
    lv_obj_set_style_pad_all(battery_outline, 0, 0);// 内边距
    lv_obj_set_style_radius(battery_outline, 8, 0);// 圆角半径
    lv_obj_clear_flag(battery_outline, LV_OBJ_FLAG_SCROLLABLE);
    #ifdef LCD_USING_ST7789
    lv_obj_set_size(battery_outline, OUTLINE_W_ST7789, OUTLINE_H_ST7789);
    #else// LCD_USING_ST7789
    lv_obj_set_size(battery_outline, OUTLINE_W * g_scale, OUTLINE_H * g_scale);
    rt_kprintf("Battery outline sizedefualt: %d x %d\n", OUTLINE_W * g_scale,
               OUTLINE_H * g_scale);
#endif // defualt
    lv_obj_add_flag(battery_outline, LV_OBJ_FLAG_EVENT_BUBBLE);

/*---------------------------------下滑菜单-----------------*/
#define CONT_W          scr_width
#define CONT_H          scr_height
#define CONT_W_PER(x)   ((CONT_W)*(x)/100)
#define CONT_H_PER(x)   ((CONT_H)*(x)/100)

    static int32_t col_dsc[] = {0, 1, LV_GRID_FR(1), 0, LV_GRID_TEMPLATE_LAST };
    col_dsc[0] = CONT_W_PER(30);
    col_dsc[1] = CONT_W_PER(24);
    col_dsc[3] = CONT_W_PER(10);

    static int32_t row_dsc[] = {0, 0, 0, 0, 0, LV_GRID_TEMPLATE_LAST };
    row_dsc[0] = CONT_H_PER(4);
    row_dsc[1] = row_dsc[2] = CONT_H_PER(12);
    row_dsc[3] = row_dsc[4] = CONT_H_PER(8);

    cont = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(cont);
    lv_obj_set_style_grid_column_dsc_array(cont, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(cont, row_dsc, 0);
    lv_obj_set_size(cont, CONT_W, CONT_H);
    lv_obj_set_style_bg_color(cont, lv_color_make(0X88, 0X88, 0X88), 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, 0);
    lv_obj_set_pos(cont, 0, - CONT_H);
    lv_obj_set_layout(cont, LV_LAYOUT_GRID);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_row(cont, 20, 0);
#if USING_TOUCH_SWITCH
    lv_obj_add_event_cb(cont, cont_event_handler, LV_EVENT_ALL, NULL);
#endif

    create_tip_label(cont, "不打断", 1, 0); //vad
    create_switch(cont, vad_switch_event_handler, 1, 2, 1);
    create_tip_label(cont, "唤醒", 2, 0); //aec
    create_switch(cont, aec_switch_event_handler, 2, 2, 0);
    create_tip_label(cont, "音量", 3, 0);
    volume_slider = create_slider(cont, slider_event_handler, 3, 1, VOL_MIN_LEVEL, VOL_MAX_LEVEL, VOL_DEFAULE_LEVEL);
    create_tip_label(cont, "亮度", 4, 0);
    brightness_lines = create_lines(cont, line_event_handler, 4, 1, BRT_TB_SIZE, LCD_BRIGHTNESS_DEFAULT);




/*------------------电池---------------------*/
    g_battery_fill = lv_obj_create(battery_outline);
    lv_obj_set_style_outline_width(g_battery_fill, 0, 0);
    lv_obj_set_style_outline_pad(g_battery_fill, 0, 0);
    lv_obj_set_style_border_width(g_battery_fill, 0, 0);
    lv_obj_set_style_bg_color(g_battery_fill, lv_color_hex(0x00ff00), 0); // 初始绿色

#ifdef LCD_USING_ST7789
    lv_obj_set_size(g_battery_fill, OUTLINE_W_ST7789 - 4, OUTLINE_H_ST7789 - 4);
#else  // LCD_USING_ST7789
    lv_obj_set_size(g_battery_fill, OUTLINE_W * g_scale - 4,
                    OUTLINE_H * g_scale - 4);
#endif // defualt

    lv_obj_set_style_border_width(g_battery_fill, 0, 0);
    lv_obj_set_style_radius(g_battery_fill, 8, 0);
    lv_obj_align(g_battery_fill, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_clear_flag(g_battery_fill, LV_OBJ_FLAG_SCROLLABLE);

    g_battery_label = lv_label_create(battery_outline);
    // lv_obj_add_style(g_battery_label, &style_battery, 0);
    lv_label_set_text_fmt(g_battery_label, "%d%%", g_battery_level);
    lv_obj_align(g_battery_label, LV_ALIGN_CENTER, 0, 0);

    // 插入右侧空白对象用于对称布局
    lv_obj_t *spacer_right = lv_obj_create(header_row);
    lv_obj_remove_flag(spacer_right, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(spacer_right, LV_OPA_0, 0);
    lv_obj_set_style_border_width(spacer_right, 0, 0);
    lv_obj_set_size(spacer_right, SCALE_DPX(40),
                    LV_SIZE_CONTENT); // 宽度为 40dp

    // ====== 中间 GIF 图片容器 img_container ======
    img_container = lv_obj_create(main_container);
    lv_obj_remove_flag(img_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(img_container, scr_width, scr_height * 0.4); // 高度自适应
    lv_obj_set_style_bg_color(img_container, lv_color_hex(0x000000), LV_STATE_DEFAULT); // 调试用绿色背景
    lv_obj_set_style_bg_opa(img_container, LV_OPA_20, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(img_container, 0, 0);
    lv_obj_set_style_margin_all(img_container, 0, 0);
    lv_obj_set_style_border_width(img_container, 0, 0);

    //gif  Emoji - 居中显示
    seqimg = lv_seqimg_create(img_container);
    lv_seqimg_src_array(seqimg, angry, 57);
    lv_seqimg_set_period(seqimg, 30);          // 每帧间隔 100ms
    lv_obj_align(seqimg, LV_ALIGN_CENTER, 0, 0);
    lv_img_set_zoom(seqimg, (int)(LV_SCALE_NONE) * g_scale);
    lv_seqimg_play(seqimg);                     // 开始播放



    // ====== 底部文本容器 text_container 占 40% 屏幕高度 ======
    
    lv_obj_t *text_container = lv_obj_create(main_container);
    lv_obj_remove_flag(text_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(text_container, scr_width, scr_height * 0.4);
    lv_obj_set_style_bg_color(text_container, lv_color_hex(0x000000), LV_STATE_DEFAULT); // 蓝色调试背景
    lv_obj_set_style_bg_opa(text_container, LV_OPA_20, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(text_container, 0, 0);
    lv_obj_set_style_margin_all(text_container, 0, 0);
    lv_obj_set_style_border_width(text_container, 0, 0);


    // Output Label - 紧贴 emoji 下方
    global_label2 = lv_label_create(text_container);
    lv_label_set_long_mode(global_label2, LV_LABEL_LONG_WRAP);
    lv_obj_add_style(global_label2, &style, 0);
    lv_obj_set_width(global_label2, LV_PCT(90));
    lv_obj_set_style_text_align(global_label2, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(global_label2, LV_ALIGN_TOP_MID, 0, 30);

/*-------------添加开机动画--------------------*/


    rt_kprintf("Creating startup animation\n");
    
    // 检查startup_logo是否可用
    if (&startup_logo == NULL) {
        rt_kprintf("Warning: startup_logo not available, skipping animation\n");
        g_startup_animation_finished = true;
        return RT_ERROR;
    }

    // 创建全屏启动画面
    g_startup_screen = lv_obj_create(lv_screen_active());
    if (!g_startup_screen) {
        rt_kprintf("Error: Failed to create startup screen\n");
        g_startup_animation_finished = true;
        return RT_ERROR;
    }
    
    lv_obj_remove_style_all(g_startup_screen);
    lv_obj_set_size(g_startup_screen, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_set_style_bg_color(g_startup_screen, lv_color_hex(0x000000), 0); // 黑色背景
    lv_obj_set_style_bg_opa(g_startup_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(g_startup_screen, LV_OBJ_FLAG_CLICKABLE);
    
    // 创建图片对象 - 与蓝牙图标创建方式完全相同
    g_startup_img = lv_img_create(g_startup_screen);
    if (!g_startup_img) {
        rt_kprintf("Error: Failed to create startup image\n");
        lv_obj_del(g_startup_screen);
        g_startup_screen = NULL;
        g_startup_animation_finished = true;
        return RT_ERROR;
    }
    
    lv_img_set_src(g_startup_img, &startup_logo);  // 使用相同的显示方式
    lv_obj_center(g_startup_img); // 居中显示
    lv_obj_set_style_img_opa(g_startup_img, LV_OPA_0, 0); // 初始完全透明
    
    // 设置图片大小 - 针对200×102分辨率的logo优化
    // 保持宽高比 200:102 ≈ 1.96:1，在屏幕上显示为合适尺寸
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



    return RT_EOK;
}

// 音量进度条更新函数
void xiaozhi_ui_update_volume(int volume)
{
    if (ui_msg_queue != RT_NULL) {
        ui_msg_t* msg = (ui_msg_t*)rt_malloc(sizeof(ui_msg_t));
        if (msg != RT_NULL) {
            msg->type = UI_MSG_VOLUME_UPDATE;
            msg->data = (char*)rt_malloc(sizeof(int));
            if (msg->data != RT_NULL) {
                *((int*)msg->data) = volume;
                if (rt_mq_send(ui_msg_queue, &msg, sizeof(ui_msg_t*)) != RT_EOK) {
                    LOG_E("Failed to send volume update UI message");
                    rt_free(msg->data);
                    rt_free(msg);
                }
            } else {
                rt_free(msg);
            }
        }
    }
}
//屏幕亮度进度条更新函数
void xiaozhi_ui_update_brightness(int brightness)
{
    if (ui_msg_queue != RT_NULL) {
        ui_msg_t* msg = (ui_msg_t*)rt_malloc(sizeof(ui_msg_t));
        if (msg != RT_NULL) {
            msg->type = UI_MSG_BRIGHTNESS_UPDATE;
            msg->data = (char*)rt_malloc(sizeof(int));
            if (msg->data != RT_NULL) {
                *((int*)msg->data) = brightness;
                if (rt_mq_send(ui_msg_queue, &msg, sizeof(ui_msg_t*)) != RT_EOK) {
                    LOG_E("Failed to send brightness update UI message");
                    rt_free(msg->data);
                    rt_free(msg);
                }
            } else {
                rt_free(msg);
            }
        }
    }
}

void xiaozhi_ui_update_standby_emoji(char *string) // emoji
{
    if(ui_msg_queue != RT_NULL)
    {
        ui_msg_t* msg = (ui_msg_t*)rt_malloc(sizeof(ui_msg_t));
        if(msg != RT_NULL)
        {
            msg->type = UI_MSG_STANDBY_EMOJI;
            msg->data = ui_strdup(string);
            if(rt_mq_send(ui_msg_queue, &msg, sizeof(ui_msg_t*)) != RT_EOK)
            {
                LOG_E("Failed to send standby emoji UI message");
                rt_free(msg->data);
                rt_free(msg);
            }
        }
    }
}
void ui_update_real_weather_and_time(void)
{
            // 异步发送消息更新天气和时间
        if (ui_msg_queue != RT_NULL) {
            ui_msg_t* msg = (ui_msg_t*)rt_malloc(sizeof(ui_msg_t));
            if (msg != RT_NULL) {
                msg->type = UI_MSG_UPDATE_WEATHER_AND_TIME;
                msg->data = RT_NULL;
                if (rt_mq_send(ui_msg_queue, &msg, sizeof(ui_msg_t*)) != RT_EOK) {
                    LOG_E("Failed to send weather/time update message");
                    rt_free(msg);
                }
            }
        }
    
}

void ui_swith_to_standby_screen(void)
{
                  if (ui_msg_queue != RT_NULL) {
                  ui_msg_t* msg = (ui_msg_t*)rt_malloc(sizeof(ui_msg_t));
                  if (msg != RT_NULL) {
                      msg->type = UI_MSG_SWITCH_TO_STANDBY;
                      msg->data = RT_NULL;
                      if (rt_mq_send(ui_msg_queue, &msg, sizeof(ui_msg_t*)) != RT_EOK) {
                          LOG_E("Failed to send switch to standby UI message");
                          rt_free(msg);
                      }
                  }
              }
}

void ui_swith_to_xiaozhi_screen(void)
{
    if (ui_msg_queue != RT_NULL) {
                  ui_msg_t* msg = (ui_msg_t*)rt_malloc(sizeof(ui_msg_t));
                  if (msg != RT_NULL) {
                      msg->type = UI_MSG_SWITCH_TO_MAIN;
                      msg->data = RT_NULL;
                      if (rt_mq_send(ui_msg_queue, &msg, sizeof(ui_msg_t*)) != RT_EOK) {
                          LOG_E("Failed to send switch to MAIN UI message");
                          rt_free(msg);
                      }
                  }
              }
}


extern date_time_t g_current_time ;
extern weather_info_t g_current_weather;


void update_xiaozhi_ui_time(void *parameter)
{

// 更新当前时间信息
    xiaozhi_time_get_current(&g_current_time);

    // 使用消息队列发送更新UI的消息到UI线程
    extern rt_mq_t ui_msg_queue;
    if (ui_msg_queue != RT_NULL) {
        ui_msg_t* msg = (ui_msg_t*)rt_malloc(sizeof(ui_msg_t));
        if (msg != RT_NULL) {
            msg->type = UI_MSG_TIME_UPDATE;  
            msg->data = RT_NULL;  
            
            if (rt_mq_send(ui_msg_queue, &msg, sizeof(ui_msg_t*)) != RT_EOK) {
                LOG_E("Failed to send time update UI message");
                rt_free(msg);
            }
        }
    } else {
        // 如果没有消息队列，回退到直接调用（保持向后兼容）
        time_ui_update_callback();
    }
        
}
void update_xiaozhi_ui_weather(void *parameter)
{
        
        // 使用消息队列发送更新UI的消息到UI线程
        extern rt_mq_t ui_msg_queue;
        if (ui_msg_queue != RT_NULL) {
            ui_msg_t* msg = (ui_msg_t*)rt_malloc(sizeof(ui_msg_t));
            if (msg != RT_NULL) {
                msg->type = UI_MSG_WEATHER_UPDATE;  // 需要定义这个消息类型
                msg->data = RT_NULL;  // 天气更新不需要额外数据
                
                if (rt_mq_send(ui_msg_queue, &msg, sizeof(ui_msg_t*)) != RT_EOK) {
                    LOG_E("Failed to send weather update UI message");
                    rt_free(msg);
                }
            }
        } else {
            // 如果没有消息队列，回退到直接调用（保持向后兼容）
            weather_ui_update_callback();
        }

}

void xiaozhi_ui_chat_status(char *string) // top text
{
    if(ui_msg_queue != RT_NULL)
    {
        ui_msg_t* msg = (ui_msg_t*)rt_malloc(sizeof(ui_msg_t));
        if(msg != RT_NULL)
        {
            msg->type = UI_MSG_CHAT_STATUS;
            msg->data = ui_strdup(string);
            if(rt_mq_send(ui_msg_queue, &msg, sizeof(ui_msg_t*)) != RT_EOK)
            {
                LOG_E("Failed to send UI message");
                rt_free(msg->data);
                rt_free(msg);
            }
        }
    }
}


void xiaozhi_ui_standby_chat_output(char *string)
{
    if(ui_msg_queue != RT_NULL)
    {
        ui_msg_t* msg = (ui_msg_t*)rt_malloc(sizeof(ui_msg_t));
        if(msg != RT_NULL)
        {
            msg->type = UI_MSG_STANDBY_CHAT_OUTPUT;
            msg->data = ui_strdup(string);
            if(rt_mq_send(ui_msg_queue, &msg, sizeof(ui_msg_t*)) != RT_EOK)
            {
                LOG_E("Failed to send standby_chat UI message");
                rt_free(msg->data);
                rt_free(msg);
            }
        }
    }
}

void xiaozhi_ui_chat_output(char *string)
{
    if(ui_msg_queue != RT_NULL)
    {
        ui_msg_t* msg = (ui_msg_t*)rt_malloc(sizeof(ui_msg_t));
        if(msg != RT_NULL)
        {
            msg->type = UI_MSG_CHAT_OUTPUT;
            msg->data = ui_strdup(string);
            if(rt_mq_send(ui_msg_queue, &msg, sizeof(ui_msg_t*)) != RT_EOK)
            {
                LOG_E("Failed to send UI message");
                rt_free(msg->data);
                rt_free(msg);
            }
        }
    }
}

static void switch_to_second_part(void *parameter)
{
     if (ui_msg_queue != RT_NULL) {
        ui_msg_t* msg = (ui_msg_t*)rt_malloc(sizeof(ui_msg_t));
        if (msg != RT_NULL) {
            msg->type = UI_MSG_TTS_SWITCH_PART;
            msg->data = RT_NULL;  // 这个消息不需要数据
            if (rt_mq_send(ui_msg_queue, &msg, sizeof(ui_msg_t*)) != RT_EOK) {
                rt_free(msg);
            }
        }
    }
}
void xiaozhi_ui_tts_output(char *string)
{
     if (ui_msg_queue != RT_NULL) {
        ui_msg_t* msg = (ui_msg_t*)rt_malloc(sizeof(ui_msg_t));
        if (msg != RT_NULL) {
            msg->type = UI_MSG_TTS_OUTPUT;
            msg->data = ui_strdup(string);
            if (rt_mq_send(ui_msg_queue, &msg, sizeof(ui_msg_t*)) != RT_EOK) {
                ui_free(msg->data);
                rt_free(msg);
            }
        }
    }
}

void xiaozhi_ui_update_emoji(char *string) // emoji
{
    if(ui_msg_queue != RT_NULL)
    {
        ui_msg_t* msg = (ui_msg_t*)rt_malloc(sizeof(ui_msg_t));
        if(msg != RT_NULL)
        {
            msg->type = UI_MSG_UPDATE_EMOJI;
            msg->data = ui_strdup(string);
            if(rt_mq_send(ui_msg_queue, &msg, sizeof(ui_msg_t*)) != RT_EOK)
            {
                LOG_E("Failed to send UI message");
                rt_free(msg->data);
                rt_free(msg);
            }
        }
    }
}

void xiaozhi_ui_update_ble(char *string) // ble
{
    if(ui_msg_queue != RT_NULL)
    {
        ui_msg_t* msg = (ui_msg_t*)rt_malloc(sizeof(ui_msg_t));
        if(msg != RT_NULL)
        {
            msg->type = UI_MSG_UPDATE_BLE;
            msg->data = ui_strdup(string);
            if(rt_mq_send(ui_msg_queue, &msg, sizeof(ui_msg_t*)) != RT_EOK)
            {
                LOG_E("Failed to send UI message");
                rt_free(msg->data);
                rt_free(msg);
            }
        }
    }
}
extern const unsigned char droid_sans_fallback_font[];
extern const int droid_sans_fallback_font_size;

static rt_device_t lcd_device;
static void pm_event_handler(gui_pm_event_type_t event)
{
    LOG_I("in pm_event_handle");
    switch (event)
    {
    case GUI_PM_EVT_SUSPEND:
    {
        LOG_I("in GUI_PM_EVT_SUSPEND");
        lv_timer_enable(false);
        break;
    }
    case GUI_PM_EVT_RESUME:
    {
        if(update_time_ui_timer)
        {
            rt_timer_start(update_time_ui_timer);//醒来继续开定时器更新ui
        }
        
        if(update_weather_ui_timer)
        {
            rt_timer_start(update_weather_ui_timer);
        }


        lv_timer_enable(true);
        if(ui_sleep_timer)
        {
            lv_timer_delete(ui_sleep_timer);
            ui_sleep_timer = NULL;
        }
        rt_kprintf("恢复屏幕-> 对话\n");
        ui_swith_to_xiaozhi_screen();
        if (!thiz->vad_enabled)
        {
            thiz->vad_enabled = true;
            xz_aec_mic_open(thiz);
            rt_kprintf("PM resume: mic reopened\n");
            
        }
        break;
    }
    default:
    {
        RT_ASSERT(0);
    }
    }
}
void pm_ui_init()
{

    int8_t wakeup_pin;
    uint16_t gpio_pin;
    GPIO_TypeDef *gpio;

    gpio = GET_GPIO_INSTANCE(BSP_KEY1_PIN);
    gpio_pin = GET_GPIOx_PIN(BSP_KEY1_PIN);

    wakeup_pin = HAL_HPAON_QueryWakeupPin(gpio, gpio_pin);
    RT_ASSERT(wakeup_pin >= 0);

    lcd_device = rt_device_find(LCD_DEVICE_NAME);
    if (lcd_device == RT_NULL)
    {
        LOG_I("lcd_device!=NULL!");
        RT_ASSERT(0);
    }
#ifdef BSP_USING_PM
    pm_enable_pin_wakeup(wakeup_pin, AON_PIN_MODE_DOUBLE_EDGE);
    gui_ctx_init();
    gui_pm_init(lcd_device, pm_event_handler);
#endif
}
void xiaozhi_update_battery_level(int level)
{
    // 确保电量在 0 到 100 之间
    g_battery_level = level;
    //rt_kprintf("Battery level updated: %d\n", g_battery_level);
    if (g_battery_fill)
    {
#ifdef LCD_USING_ST7789
        int width =
            (OUTLINE_W_ST7789 - 4) * g_battery_level / 100; // 计算填充宽度
#else                                                       // LCD_USING_ST7789
        int width =
            (OUTLINE_W * g_scale - 4) * g_battery_level / 100; // 计算填充宽度
#endif

        if (width < 2 && g_battery_level > 0)
            width = 2;                           // 最小宽度为 2
        lv_obj_set_width(g_battery_fill, width); // 根据电量设置填充宽度

        // 更新颜色
        if (g_battery_level <= 20)
        {
            lv_obj_set_style_bg_color(g_battery_fill, lv_color_hex(0xff0000),
                                      0); // 红色
        }
        else
        {
            lv_obj_set_style_bg_color(g_battery_fill, lv_color_hex(0xff00),
                                      0); // 绿色
        }
    }

    if (g_battery_label)
    {
        //rt_kprintf("Updating battery label: %d\n", g_battery_level);
        lv_label_set_text_fmt(g_battery_label, "%d%%",
                              g_battery_level); // 更新电量标签
    }

    // 状态栏的环形电池显示
    if (battery_arc) {
        lv_arc_set_value(battery_arc, g_battery_level); // 更新圆弧电量显示
        
        // 根据电量改变颜色
        if (g_battery_level <= 20) {
            // 低电量红色
            lv_obj_set_style_arc_color(battery_arc, lv_color_hex(0xCC0000), LV_PART_INDICATOR);
        } else if (g_battery_level <= 50) {
            // 中等电量黄色
            lv_obj_set_style_arc_color(battery_arc, lv_color_hex(0xCCCC00), LV_PART_INDICATOR);
        } else {
            // 高电量绿色
            lv_obj_set_style_arc_color(battery_arc, lv_color_hex(0x00CC00), LV_PART_INDICATOR);
        }
    }
    
    // 更新电池电量百分比文本
    if (battery_percent_label) {
        lv_label_set_text_fmt(battery_percent_label, "%d%%", g_battery_level);
    }

}
extern rt_mailbox_t g_bt_app_mb;
extern void kws_demo_stop();
rt_tick_t last_listen_tick = 0;

void xiaozhi_ui_task(void *args)
{
    rt_err_t ret = RT_EOK;
    rt_uint32_t ms;
    static rt_device_t touch_device;
    rt_sem_init(&update_ui_sema, "update_ui", 1, RT_IPC_FLAG_FIFO);
    rt_kprintf("xiaozhi_ui_task start\n");
    //初始化UI消息队列
    ui_msg_queue = rt_mq_create("ui_msg", sizeof(ui_msg_t*), 20, RT_IPC_FLAG_FIFO);
    if(ui_msg_queue == RT_NULL)
    {
        LOG_E("Failed to create UI message queue");
        return;
    }
    // 初始化UI消息邮箱
    if (g_ui_task_mb == RT_NULL) {
        g_ui_task_mb = rt_mb_create("ui_mb", 8, RT_IPC_FLAG_FIFO);
        RT_ASSERT(g_ui_task_mb != RT_NULL);
    }
    /* init littlevGL */
    ret = littlevgl2rtt_init("lcd");
    if (ret != RT_EOK)
    {
        return;
    }


#ifdef BSP_USING_PM
    pm_ui_init();
#endif

    float scale = get_scale_factor();

const int medium_font_size = (int)(25 * scale + 0.5f);    // 秒显示
// 创建不同大小的字体并赋值给全局变量

font_medium = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, medium_font_size);


    g_scale = scale; // 保存全局缩放因子
    rt_kprintf("Scale factor: %.2f\n", scale);
    const int base_font_size = 30;
    const int adjusted_font_size = (int)(base_font_size * scale + 0.5f);

    const int base_font_size_battery = 14;
    const int adjusted_font_size_battery =
        (int)(base_font_size_battery * scale + 0.5f);
    lv_style_init(&style2);
     lv_style_set_text_font(&style2, font_medium);
    lv_style_set_text_align(&style2, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_color(&style2, lv_color_hex(0xFFFFFF));
 

    lv_style_init(&style);
    lv_font_t *font = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size,
                                              adjusted_font_size);
    lv_style_set_text_font(&style, font);
    lv_style_set_text_align(&style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_color(&style, lv_color_hex(0xFFFFFF));
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000),
                              LV_PART_MAIN | LV_STATE_DEFAULT); // SET BG COLOR


    ret = xiaozhi_ui_obj_init();
    if (ret != RT_EOK)
    {
        return;
    }

    xiaozhi_ui_update_ble("close");
    xiaozhi_ui_chat_status("连接中...");
    xiaozhi_ui_chat_output("等待连接...");
    xiaozhi_ui_update_emoji("neutral");


    //每秒更新时间的ui
    if (!update_time_ui_timer) 
    {update_time_ui_timer = rt_timer_create("update_ui_time", update_xiaozhi_ui_time, NULL,
                                    rt_tick_from_millisecond(1000), //每秒更新time的ui
                                    RT_TIMER_FLAG_PERIODIC  | RT_TIMER_FLAG_SOFT_TIMER);
    } else 
    {
        rt_timer_stop(update_time_ui_timer);
    }
    rt_timer_start(update_time_ui_timer);



        //更新天气
    if (!update_weather_ui_timer) 
    {update_weather_ui_timer = rt_timer_create("update_ui_time", update_xiaozhi_ui_weather, NULL,
                                    rt_tick_from_millisecond(1800000), //30分钟
                                    RT_TIMER_FLAG_PERIODIC  | RT_TIMER_FLAG_SOFT_TIMER);
    } 
    else 
    {
        rt_timer_stop(update_weather_ui_timer);
    }
    rt_timer_start(update_weather_ui_timer);


    while (1)
    {
        rt_uint32_t btn_event;
        rt_uint32_t ui_event;

        if (g_kws_force_exit)
        {
            g_kws_force_exit = 0;
            kws_demo_stop();
        }

        // 处理关机事件
        if (rt_mb_recv(g_ui_task_mb, &ui_event, 0) == RT_EOK)
        {
            if (ui_event == UI_EVENT_SHUTDOWN)
            {
               show_shutdown();
            }
        }
        // 处理按钮事件
        if (rt_mb_recv(g_button_event_mb, &btn_event, 0) == RT_EOK)
        {
            rt_kprintf("button event: %d\n", btn_event);
            switch (btn_event)
            {
            case BUTTON_EVENT_PRESSED:
                    ws_send_speak_abort(&g_xz_ws.clnt, g_xz_ws.session_id,kAbortReasonWakeWordDetected);                                           
                    xz_speaker(0); // 关闭扬声器
					rt_kprintf("vad_enabled jjjjjk\n");
#ifdef BSP_USING_PM
                    if(!thiz->vad_enabled)
                    {
                        rt_kprintf("vad_enabled\n");
                        thiz->vad_enabled = true;
                        xz_aec_mic_open(thiz);    
                    }
#endif                                       
                xiaozhi_ui_chat_status("聆听中...");
                break;
                
            case BUTTON_EVENT_RELEASED:
                xiaozhi_ui_chat_status("待命中...");
#if !PKG_XIAOZHI_USING_AEC  
                ws_send_listen_stop(&g_xz_ws.clnt, g_xz_ws.session_id);
                xz_mic(0);
#endif // !PKG_XIAOZHI_USING_AEC                
                break;

            default:
                break;
            }
        }

        rt_uint32_t battery_level;
        if (rt_mb_recv(g_battery_mb, &battery_level, 0) == RT_EOK)
        {
           // rt_kprintf("Battery level received: %d\n", battery_level);
            xiaozhi_update_battery_level(battery_level);
        }
        // 处理UI消息队列中的消息
        ui_msg_t* msg;
        while (rt_mq_recv(ui_msg_queue, &msg, sizeof(ui_msg_t*), 0) == RT_EOK)
        {
            switch (msg->type)
            {
                case UI_MSG_STANDBY_CHAT_OUTPUT:
                    if(msg->data)
                    {
                        lv_label_set_text(ui_Label3, msg->data);    
                    }
                    break;
                case UI_MSG_UPDATE_WEATHER_AND_TIME:
                    LOG_I("update weather and time\n");
                    rt_mb_send(g_bt_app_mb, UPDATE_REAL_WEATHER_AND_TIME);
                    break;
                case UI_MSG_STANDBY_EMOJI:
                    if(msg->data)
                    {
                        if (strcmp(msg->data, "sleepy") == 0)
                        {
                            if (img_emoji) 
                            {
                                lv_img_set_src(img_emoji, &sleepy2); // 使用睡眠表情表示小智未连接
                            }
                        }
                        else if (strcmp(msg->data, "funny") == 0)
                        {
                            if (img_emoji) 
                            {
                                lv_img_set_src(img_emoji, &funny2); // 使用睡眠表情表示小智未连接
                            }
                        }
                    }
                    break;
                case UI_MSG_SWITCH_TO_STANDBY:
                    if (standby_screen) {
                        lv_screen_load(standby_screen);
                        }

                        // mic关闭，开启KWS
                        xz_aec_mic_close(thiz);
                        if(aec_enabled) 
                        {
                            kws_demo();
                        }    

                        if(ui_sleep_timer != NULL)
                        {
                            lv_timer_delete(ui_sleep_timer);
                            ui_sleep_timer = NULL;
                        }
                        if(g_pan_connected)
                        {
                            rt_kprintf("create sleep timer1\n");
                            ui_sleep_timer = lv_timer_create(ui_sleep_callback, 40000, NULL);

                        } 

                        if (standby_update_timer != NULL) {
                            lv_timer_delete(standby_update_timer);
                        }
                        
                        // 创建定时器，稍后执行更新
                        standby_update_timer = lv_timer_create(standby_update_callback, 100, NULL);

                    
                    break;
                    
                case UI_MSG_SWITCH_TO_MAIN:
                    if (main_container) {
                        if(ui_sleep_timer)
                        {
                            lv_timer_delete(ui_sleep_timer);
                            ui_sleep_timer = NULL;
                        }

                        lv_screen_load(lv_obj_get_screen(main_container));
                    }
                    // mic开启，关闭KWS
                    xz_aec_mic_open(thiz);
                    if (g_kws_force_exit)
                    {
                        g_kws_force_exit = 0;
                        kws_demo_stop();
                    }
                    break;
                case UI_MSG_WEATHER_UPDATE:
                    weather_ui_update_callback();
                    break;
                case UI_MSG_TIME_UPDATE:
                    time_ui_update_callback();
                    break;
                case UI_MSG_CHAT_STATUS:
                    if(msg->data)
                    {
                        lv_label_set_text(global_label1, msg->data);
                    }
                    break;
                case UI_MSG_CHAT_OUTPUT:
                    if(msg->data)
                    {
                        lv_label_set_text(global_label2, msg->data);    
                    }
                    break;
                case UI_MSG_VOLUME_UPDATE:
                    if(msg->data) {
                        int volume = *((int*)msg->data);
                        if (volume_slider) {
                            lv_slider_set_value(volume_slider, volume, LV_ANIM_OFF);
                        }
                    }
                    break; 
                case UI_MSG_BRIGHTNESS_UPDATE:
                    if(msg->data) {
                        int brightness = *((int*)msg->data);
                        // 更新亮度显示条
                        if (brightness_lines) {
                            uint32_t cnt = lv_obj_get_child_count(brightness_lines);
                            lv_obj_t* child;
                            uint16_t i = 0;
                            // 根据亮度值更新显示条的颜色
                            while(i < cnt) {
                                child = lv_obj_get_child(brightness_lines, i);
                                if (brigtness_tb[i] <= brightness)
                                    lv_obj_set_style_bg_color(child, lv_palette_main(LV_PALETTE_LIGHT_GREEN), 0);
                                else
                                    lv_obj_set_style_bg_color(child, lv_palette_main(LV_PALETTE_GREY), 0);
                                i++;
                            }
                        }
                    }
                    break;      
                case UI_MSG_UPDATE_EMOJI:
                    if(msg->data)
                    {
                        if (strcmp(msg->data, "neutral") == 0)
                        {
                            lv_seqimg_src_array(seqimg, neutral, sizeof(neutral) / sizeof(neutral[0]));
                        }
                        else if (strcmp(msg->data, "happy") == 0)
                        {
                            lv_seqimg_src_array(seqimg, funny, sizeof(funny) / sizeof(funny[0]));
                        }
                        else if (strcmp(msg->data, "laughing") == 0)
                        {
                            lv_seqimg_src_array(seqimg, funny, sizeof(funny) / sizeof(funny[0]));
                        }
                        else if (strcmp(msg->data, "funny") == 0)
                        {
                            lv_seqimg_src_array(seqimg, funny, sizeof(funny) / sizeof(funny[0]));
                        }
                        else if (strcmp(msg->data, "sad") == 0)
                        {
                            lv_seqimg_src_array(seqimg, crying, sizeof(crying) / sizeof(crying[0]));
                        }
                        else if (strcmp(msg->data, "angry") == 0)
                        {
                            lv_seqimg_src_array(seqimg, angry, sizeof(angry) / sizeof(angry[0]));
                        }
                        else if (strcmp(msg->data, "crying") == 0)
                        {
                            lv_seqimg_src_array(seqimg, crying, sizeof(crying) / sizeof(crying[0]));
                        }
                        else if (strcmp(msg->data, "loving") == 0)
                        {
                            lv_seqimg_src_array(seqimg, loving, sizeof(loving) / sizeof(loving[0]));
                        }
                        else if (strcmp(msg->data, "embarrassed") == 0)
                        {
                            lv_seqimg_src_array(seqimg, embarrassed, sizeof(embarrassed) / sizeof(embarrassed[0]));
                        }
                        else if (strcmp(msg->data, "surprised") == 0)
                        {
                            lv_seqimg_src_array(seqimg, surprised, sizeof(surprised) / sizeof(surprised[0]));
                        }
                        else if (strcmp(msg->data, "shocked") == 0)
                        {
                            lv_seqimg_src_array(seqimg, surprised, sizeof(surprised) / sizeof(surprised[0]));
                        }
                        else if (strcmp(msg->data, "thinking") == 0)
                        {
                            lv_seqimg_src_array(seqimg, thinking, sizeof(thinking) / sizeof(thinking[0]));
                        }
                        else if (strcmp(msg->data, "winking") == 0)
                        {
                            lv_seqimg_src_array(seqimg, loving, sizeof(loving) / sizeof(loving[0]));
                        }
                        else if (strcmp(msg->data, "cool") == 0)
                        {
                            lv_seqimg_src_array(seqimg, cool, sizeof(cool) / sizeof(cool[0]));
                        }
                        else if (strcmp(msg->data, "relaxed") == 0)
                        {
                            lv_seqimg_src_array(seqimg, cool, sizeof(cool) / sizeof(cool[0]));
                        }
                        else if (strcmp(msg->data, "delicious") == 0)
                        {
                            lv_seqimg_src_array(seqimg, loving, sizeof(loving) / sizeof(loving[0]));
                        }
                        else if (strcmp(msg->data, "kissy") == 0)
                        {
                            lv_seqimg_src_array(seqimg, kissy, sizeof(kissy) / sizeof(kissy[0]));
                        }
                        else if (strcmp(msg->data, "confident") == 0)
                        {
                            lv_seqimg_src_array(seqimg, cool, sizeof(cool) / sizeof(cool[0]));
                        }
                        else if (strcmp(msg->data, "sleepy") == 0)
                        {
                            lv_seqimg_src_array(seqimg, sleepy, sizeof(sleepy) / sizeof(sleepy[0]));
                        }
                        else if (strcmp(msg->data, "silly") == 0)
                        {
                            lv_seqimg_src_array(seqimg, thinking, sizeof(thinking) / sizeof(thinking[0]));
                        }
                        else if (strcmp(msg->data, "confused") == 0)
                        {
                            lv_seqimg_src_array(seqimg, thinking, sizeof(thinking) / sizeof(thinking[0]));
                        }
                        else
                        {
                            lv_seqimg_src_array(seqimg, neutral, sizeof(neutral) / sizeof(neutral[0])); // common emoji is neutral
                        }
                    }
                    break;
                case UI_MSG_UPDATE_BLE:
                    if(msg->data)
                    {
                        if (strcmp(msg->data, "open") == 0)
                        {
                            lv_img_set_src(global_img_ble, &ble);
                        }
                        else if (strcmp(msg->data, "close") == 0)
                        {
                            lv_img_set_src(global_img_ble, &ble_close);
                        }
                    }
                    break;
                case UI_MSG_TTS_OUTPUT:
                    if(msg->data)
                    {
                         int len = strlen(msg->data);
                        rt_kprintf("len == %d\n", len);

                        if (len > SHOW_TEXT_LEN) {
                            // 查看 SHOW_TEXT_LEN 是否落在一个多字节字符中间
                            int cut_pos = SHOW_TEXT_LEN;

                            // 向前调整到完整的 UTF-8 字符起点
                            while (cut_pos > 0 &&
                                   ((unsigned char)msg->data[cut_pos] & 0xC0) == 0x80) {
                                cut_pos--;
                            }

                            if (cut_pos == 0) // 找不到合适的截断点，直接截断
                                cut_pos = SHOW_TEXT_LEN;

                            // 截取第一部分
                            char first_part[SHOW_TEXT_LEN + 1];
                            strncpy(first_part, msg->data, cut_pos);
                            first_part[cut_pos] = '\0'; // 确保字符串结束

                            // 剩余部分从 cut_pos 开始
                            strncpy(g_second_part, msg->data + cut_pos, sizeof(g_second_part) - 1);
                            g_second_part[sizeof(g_second_part) - 1] = '\0'; // 确保结尾
                            g_label_for_second_part = global_label2;

                            lv_label_set_text(global_label2, first_part);
#ifdef BSP_USING_PM
                            lv_display_trigger_activity(NULL);
#endif // BSP_USING_PM

                            // 创建定时器
                            if (!g_split_text_timer) {
                                g_split_text_timer = rt_timer_create(
                                    "next_text", switch_to_second_part, NULL,
                                    rt_tick_from_millisecond(6000), // 9秒后显示下一部分
                                    RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
                            } else {
                                rt_timer_stop(g_split_text_timer);
                            }
                            rt_timer_start(g_split_text_timer);
                        } 
                        else 
                        {
                            lv_label_set_text(global_label2, msg->data);
                        }
                    }
                    break;
                case UI_MSG_TTS_SWITCH_PART:
                    if (g_label_for_second_part && strlen(g_second_part) > 0) {
                        int len = strlen(g_second_part);
                        if (len > SHOW_TEXT_LEN) {
                            // 再次分割文本
                            char first_part[SHOW_TEXT_LEN + 1];
                            char remaining[512];

                            // 查找合适的截断点
                            int cut_pos = SHOW_TEXT_LEN;
                            while (cut_pos > 0 &&
                                ((unsigned char)g_second_part[cut_pos] & 0xC0) == 0x80) {
                                cut_pos--;
                            }

                            strncpy(first_part, g_second_part, cut_pos);
                            first_part[cut_pos] = '\0';

                            strncpy(remaining, g_second_part + cut_pos, sizeof(remaining) - 1);
                            remaining[sizeof(remaining) - 1] = '\0';

                            // 显示当前部分
                            lv_label_set_text(g_label_for_second_part, first_part);

                            // 保存剩余部分
                            strncpy(g_second_part, remaining, sizeof(g_second_part) - 1);
                            g_second_part[sizeof(g_second_part) - 1] = '\0';

                            // 重置定时器以显示下一部分
                            rt_timer_control(g_split_text_timer, RT_TIMER_CTRL_SET_TIME,
                                            &(rt_tick_t){rt_tick_from_millisecond(6000)});
                            rt_timer_start(g_split_text_timer);
                        } else {
                            // 最后一部分，直接显示
                            lv_label_set_text(g_label_for_second_part, g_second_part);
                            memset(g_second_part, 0, sizeof(g_second_part));
                            g_label_for_second_part = NULL;
                        }
                        
                    }
                    break;
            }
            // 释放消息内存
            ui_free(msg->data);
            rt_free(msg);
        }
        if (RT_EOK == rt_sem_trytake(&update_ui_sema))
        {
            ms = lv_task_handler();
            switch_anim_timeout_check();

            char *current_text = lv_label_get_text(global_label1);
            lv_obj_t *current_screen = lv_screen_active();
            //rt_kprintf("current_screen: %p, main_container: %p\n", current_screen, main_container);
            //rt_kprintf("inactive_time: %d, limit: %d\n", lv_display_get_inactive_time(NULL), IDLE_TIME_LIMIT);
            if (lv_display_get_inactive_time(NULL) > IDLE_TIME_LIMIT && current_screen != standby_screen && current_screen != g_startup_screen && current_screen != shutdown_screen &&
    current_screen != sleep_screen)
            {

                rt_kprintf("listen_tick\n");
                last_listen_tick= 1;
                lv_display_trigger_activity(NULL);
            }
            // 低功耗判断
            if (g_xz_ws.is_connected == 0 && last_listen_tick > 0 && g_pan_connected && she_bei_ma)
            {
                rt_tick_t now = rt_tick_get();
                rt_tick_t delta = now - last_listen_tick;
                rt_kprintf("last_listen_tick: %d, now: %d, delta: %d\n", last_listen_tick, now, delta);
                if (delta < rt_tick_from_millisecond(12000))
                {
                    LOG_I("Websocket disconnected, entering low power mode");
                    lv_display_trigger_activity(NULL);
                    if(thiz->vad_enabled)
                    {
                        thiz->vad_enabled = false;
                        rt_kprintf("in zudon_PM,so vad_close\n");
                        xz_aec_mic_close(thiz);
                    }
                    bt_interface_wr_link_policy_setting(
                    (unsigned char *)&g_bt_app_env.bd_addr,
                    BT_NOTIFY_LINK_POLICY_SNIFF_MODE | BT_NOTIFY_LINK_POLICY_ROLE_SWITCH); // open role switch
                    MCP_RGBLED_CLOSE(); 
                    show_sleep_countdown_and_sleep();
                    
                    if(aec_enabled) 
                    {
                        kws_demo();
                    }                  
                }
                if (delta > rt_tick_from_millisecond(12000))
                {
                    LOG_I("30s no action \n");
                    lv_display_trigger_activity(NULL);
                    bt_interface_wr_link_policy_setting(
                    (unsigned char *)&g_bt_app_env.bd_addr,
                    BT_NOTIFY_LINK_POLICY_SNIFF_MODE | BT_NOTIFY_LINK_POLICY_ROLE_SWITCH); // open role switch
                    MCP_RGBLED_CLOSE();
                    rt_kprintf("time out,xiu_mian\n");
                    if (standby_screen) 
                    {
                        LOG_I("休眠->待机\n");
                        ui_swith_to_standby_screen();

                    }
                }
                last_listen_tick = 0;
            }

#ifdef BSP_USING_PM
            // if (strcmp(current_text, "聆听中...") == 0)
            // {
            //     lv_display_trigger_activity(NULL);
            // }
            // if (lv_display_get_inactive_time(NULL) > IDLE_TIME_LIMIT && g_pan_connected && she_bei_ma)
            // {
            //         lv_display_trigger_activity(NULL);
            //         LOG_I("30s no action \n");
            //         bt_interface_wr_link_policy_setting(
            //         (unsigned char *)&g_bt_app_env.bd_addr,
            //         BT_NOTIFY_LINK_POLICY_SNIFF_MODE | BT_NOTIFY_LINK_POLICY_ROLE_SWITCH); // open role switch
            //         MCP_RGBLED_CLOSE();
            //         rt_kprintf("time out,xiu_mian\n");
            //         if (standby_screen) 
            //         {
            //             ui_swith_to_standby_screen();

            //         }
                
            
            // }
        
            if (gui_is_force_close())
            {
                LOG_I("in force_close");
                bool lcd_drawing;
                rt_device_control(lcd_device, RTGRAPHIC_CTRL_GET_BUSY,
                                  &lcd_drawing);
                if (!lcd_drawing)
                {
                    LOG_I("no input:%d", lv_display_get_inactive_time(NULL));
                    gui_suspend();
                    LOG_I("ui resume");
                    /* force screen to redraw */
                    lv_obj_invalidate(lv_screen_active());
                    /* reset activity timer */
                    lv_display_trigger_activity(NULL);
                }
            }
#endif // BSP_USING_PM

            rt_thread_mdelay(ms);
            rt_sem_release(&update_ui_sema);
        }
    }
}
