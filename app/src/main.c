/*
 * SPDX-FileCopyrightText: 2024-2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rtthread.h"
#include "bf0_hal.h"
#include "drv_io.h"
#include "stdio.h"
#include "string.h"
#include "xiaozhi_websocket.h"
#include "./iot/iot_c_api.h"
#include <stdbool.h>
#ifdef BSP_USING_PM
    #include "gui_app_pm.h"
#endif // BSP_USING_PM
#include "xiaozhi_client_public.h"
#include "bf0_pm.h"
#include <drivers/rt_drv_encoder.h>
#include "drv_flash.h"
#include "../weather/weather.h"
#include "lv_timer.h"
#include "lv_display.h"
#include "../board/board_hardware.h"
#include "xiaozhi_ui.h"
#include "xiaozhi_mqtt.h"
#include "xiaozhi_audio.h"
#include "bts2_app_inc.h"
#include "hfp_type_api.h"
#include "ble_connection_manager.h"
#include "bt_connection_manager.h"
#include "bt_env.h"
#include "ulog.h"
#include "drv_gpio.h"
#include "battery_calculator.h"
#include "bt_pan_ota.h"
#include "charge.h"
/* Common functions for RT-Thread based platform
 * -----------------------------------------------*/
/**
 * @brief  Initialize board default configuration.
 * @param  None
 * @retval None
 */
static rt_tick_t s_last_cind_query_tick = 0; // 上次聚合查询触发时间
/* User code start from here
 * --------------------------------------------------------*/

#define BT_APP_READY 0
#define BT_APP_CONNECT_PAN 1
#define BT_APP_CONNECT_PAN_SUCCESS 2
#define WEBSOC_RECONNECT 4
#define KEEP_FIRST_PAN_RECONNECT 5
#define XZ_CONFIG_UPDATE 6
#define BT_APP_PHONE_DISCONNECTED 7  // 手机主动断开
#define BT_APP_ABNORMAL_DISCONNECT 8 // 异常断开
#define BT_APP_RECONNECT_TIMEOUT 9   // 重连超时
#define BT_APP_RECONNECT 10          // 重连
#define UPDATE_REAL_WEATHER_AND_TIME 11
#define PAN_TIMER_MS 3000
#define MQTT_RECONNECT 12

#define MAX_RECONNECT_ATTEMPTS 30 // 30次尝试，每次1秒，共30秒
#define XIAOZHI_UI_THREAD_STACK_SIZE (8192)
#define BATTERY_THREAD_STACK_SIZE (2048)
#define LOW_BATTERY_THRESHOLD 5

extern rt_tick_t last_listen_tick;
extern xiaozhi_ws_t g_xz_ws;
extern rt_mailbox_t g_button_event_mb;
extern lv_obj_t *standby_screen;
extern lv_timer_t *ui_sleep_timer;
extern lv_obj_t *shutdown_screen;
extern lv_obj_t *sleep_screen;
extern rt_mailbox_t g_ui_task_mb;
extern lv_obj_t *call_screen;
extern uint8_t s_talk_with_hfp;


bt_app_t g_bt_app_env;
rt_mailbox_t g_bt_app_mb;
BOOL g_pan_connected = FALSE;
BOOL first_pan_connected = FALSE;
int first_reconnect_attempts = 0;
uint8_t Initiate_disconnection_flag = 0; // 蓝牙主动断开标志
rt_mailbox_t g_battery_mb;
bool g_skip_startup = true;
bool low_battery_shutdown_triggered = true;
bool lowpower_shutdown_state = true;
bool g_low_power_mode = false;

static rt_timer_t s_reconnect_timer = NULL;
static rt_timer_t s_sleep_timer = NULL;
static int reconnect_attempts = 0;
static uint8_t g_sleep_enter_flag = 0; // 进入睡眠标志位
// HFP来电/通话状态跟踪
static uint8_t g_prev_call_status = 0; // 0: 无通话, 1: 通话中
static uint8_t g_prev_callsetup_status = 0; // 0: 无建立, 1: 来电, 2: 去电, 3: 响铃
// UI线程和battery线程控制块
static struct rt_thread xiaozhi_ui_thread;
static struct rt_thread battery_thread;
// 最新版本信息
extern char latest_version[32];

// ui线程
#if defined(__CC_ARM) || defined(__CLANG_ARM)
L2_RET_BSS_SECT_BEGIN(xiaozhi_ui_thread_stack) // 6000地址
static uint32_t
    xiaozhi_ui_thread_stack[XIAOZHI_UI_THREAD_STACK_SIZE / sizeof(uint32_t)];
L2_RET_BSS_SECT_END
#else
static uint32_t xiaozhi_ui_thread_stack
    [XIAOZHI_UI_THREAD_STACK_SIZE / sizeof(uint32_t)] L2_RET_BSS_SECT(
        xiaozhi_ui_thread_stack);
#endif
// battery线程
#if defined(__CC_ARM) || defined(__CLANG_ARM)
L2_RET_BSS_SECT_BEGIN(battery_thread_stack) // 6000地址
static uint32_t
    battery_thread_stack[BATTERY_THREAD_STACK_SIZE / sizeof(uint32_t)];
L2_RET_BSS_SECT_END
#else
static uint32_t battery_thread_stack
    [BATTERY_THREAD_STACK_SIZE / sizeof(uint32_t)] L2_RET_BSS_SECT(
        battery_thread_stack);
#endif

#ifdef BSP_USING_BOARD_SF32LB52_XTY_AI
static rt_timer_t s_pulse_encoder_timer = NULL;
static struct rt_device *s_encoder_device;

static int pulse_encoder_init(void)
{
    s_encoder_device = rt_device_find("encoder1");
    if (s_encoder_device == RT_NULL)
    {
        LOG_E("Failed to find encoder device\n");
        return -RT_ERROR;
    }
    struct rt_encoder_configuration config;
    config.channel = GPT_CHANNEL_ALL;

    rt_err_t result =
        rt_device_control((struct rt_device *)s_encoder_device,
                          PULSE_ENCODER_CMD_ENABLE, (void *)&config); // 使能

    if (result != RT_EOK)
    {
        rt_kprintf("Failed to enable encoder\n");
        return -RT_ERROR;
    }
    return RT_EOK;
}

static void pulse_encoder_timeout_handle(void *parameter)
{
    static int32_t last_count = 0;
    rt_err_t result;
    struct rt_encoder_configuration config_count;
    config_count.get_count = 0;
    result =
        rt_device_control((struct rt_device *)s_encoder_device,
                          PULSE_ENCODER_CMD_GET_COUNT, (void *)&config_count);
    if (result != RT_EOK)
    {
        LOG_E("Failed to get encoder count\n");
        return;
    }

    int32_t current_count = config_count.get_count;
    int32_t delta_count = current_count - last_count;
    last_count = current_count;
    // delta_count
    // 是以4为单位的增量，不能被4整除不算一次脉冲，需要先算出有多少增量
    // 注意正数是顺时针转动，负数是逆时针转动
    if (delta_count % 4 != 0)
    {
        LOG_W("Encoder count is not a multiple of 4, delta_count: %d\n",
              delta_count);
        return;
    }
    int32_t pulses = delta_count / 4; // 每次脉冲是4个单位
    if (pulses != 0)
    {
        LOG_I("Pulse encoder count: %d\n", pulses);
        int current_volume =
            audio_server_get_private_volume(AUDIO_TYPE_LOCAL_MUSIC);
        int new_volume = current_volume + pulses;
        if (new_volume < 0)
        {
            new_volume = 0; // 最小音量为0
        }
        else if (new_volume > 15)
        {
            new_volume = 15; // 最大音量为15
        }

        audio_server_set_private_volume(AUDIO_TYPE_LOCAL_MUSIC, new_volume);
    }
}
#endif
void HAL_MspInit(void)
{
    //__asm("B .");        /*For debugging purpose*/
    BSP_IO_Init();
    set_pinmux();
}

static void battery_level_task(void *parameter)
{
    if (g_battery_mb == NULL)
    {
        rt_kprintf("Failed to create mailbox g_battery_mb\n");
        return;
    }
    uint8_t battery_percentage = 0;
    uint8_t shutdown_battery_percentage = 0;
    // 初始化电池计算器
    battery_calculator_t battery_calc;
    battery_calculator_config_t calc_config = {
        .charging_table = charging_curve_table,
        .charging_table_size = charging_curve_table_size,
        .discharging_table = discharge_curve_table,
        .discharging_table_size = discharge_curve_table_size,
        .charge_filter_threshold = 50,     // 充电滤波阈值
        .discharge_filter_threshold = 30,  // 放电滤波阈值
        .filter_count = 3,                 // 滤波计数阈值
        .secondary_filter_enabled = true,  // 启用二级滤波
        .secondary_filter_weight_pre = 90, // 二级滤波前电压权重
        .secondary_filter_weight_cur = 10  // 二级滤波当前电压权重
    };

    battery_calculator_init(&battery_calc, &calc_config);
    rt_uint8_t current_status;
    while (1)
    {
        rt_device_t battery_device = rt_device_find("bat1");
        rt_adc_cmd_read_arg_t read_arg;
        read_arg.channel = 7; // 电池电量在通道7
        rt_err_t result =
            rt_adc_enable((rt_adc_device_t)battery_device, read_arg.channel);
        if (result != RT_EOK)
        {
            LOG_E("Failed to enable ADC for battery read\n");
            return;
        }
        rt_thread_mdelay(300);
        rt_uint32_t battery_level =
            rt_adc_read((rt_adc_device_t)battery_device, read_arg.channel);
        rt_kprintf("battery_level: %d\n", battery_level);
        rt_adc_disable((rt_adc_device_t)battery_device, read_arg.channel);
        if (g_low_power_mode)
        {
#ifdef BSP_USING_BOARD_SF32LB52_LCHSPI_ULP
            BSP_GPIO_Set(26, 1, 1);
            HAL_PIN_Set(PAD_PA26, GPIO_A26, PIN_PULLUP, 1);
            HAL_PIN_Set(PAD_PA10, I2C2_SCL, PIN_PULLUP, 1);
            HAL_PIN_Set(PAD_PA11, I2C2_SDA, PIN_PULLUP, 1);
            rt_thread_mdelay(10);
#endif
            battery_percentage =
                battery_calculator_get_percent(&battery_calc, battery_level);
#ifdef BSP_USING_BOARD_SF32LB52_LCHSPI_ULP
            BSP_GPIO_Set(26, 0, 1);
            HAL_PIN_Set(PAD_PA26, GPIO_A26, PIN_NOPULL, 1);
            HAL_PIN_Set(PAD_PA10, GPIO_A10, PIN_PULLDOWN, 1);
            HAL_PIN_Set(PAD_PA11, GPIO_A11, PIN_PULLDOWN, 1);
#endif
        }
        else
        {
            battery_percentage =
                battery_calculator_get_percent(&battery_calc, battery_level);
        }

        rt_mb_send(g_battery_mb, battery_percentage);
        current_status = rt_pin_read(CHARGE_DETECT_PIN);
        rt_kprintf("battery_percentage: %d, current_status: %d: %d \n",
                   battery_percentage, current_status);
#ifdef LOW_POWER_NO_SHUTDOWN // 针对没有电池的板子

#else
        // 当电量低于阈值并且当前没有处于充电中并的时候
        if (battery_percentage < LOW_BATTERY_THRESHOLD &&
            low_battery_shutdown_triggered && !current_status)
        {
            lv_obj_t *now_screen = lv_screen_active();
            rt_kprintf("now_screen address: %p, sleep_screen address: %p, "
                       "standby_screen address: %p\n",
                       now_screen, sleep_screen, standby_screen);
            low_battery_shutdown_triggered = false;
            rt_kprintf("Low battery ,shutdown\n");
            // 发送消息到UI线程显示低电量关机页面
            if (g_ui_task_mb != RT_NULL)
            {
                if (now_screen == sleep_screen && now_screen != NULL)
                {
                    lowpower_shutdown_state = false;
                    gui_pm_fsm(GUI_PM_ACTION_WAKEUP); // 唤醒设备
                }

                rt_thread_mdelay(100);
                rt_mb_send(g_ui_task_mb, UI_EVENT_LOW_BATTERY_SHUTDOWN);
            }
        }
#endif /* LOW_POWER_NO_SHUTDOWN */
        rt_thread_mdelay(5000);
    }
}

void bt_app_connect_pan_timeout_handle(void *parameter)
{
    LOG_I("bt_app_connect_pan_timeout_handle %x, %d", g_bt_app_mb,
          g_bt_app_env.bt_connected);
    if ((g_bt_app_mb != NULL) && (g_bt_app_env.bt_connected))
        rt_mb_send(g_bt_app_mb, BT_APP_CONNECT_PAN);
    return;
}

#if defined(BSP_USING_SPI_NAND) && defined(RT_USING_DFS)
    #include "dfs_file.h"
    #include "dfs_posix.h"
    #include "drv_flash.h"
    #define FS_ROOT "root"
int mnt_init(void)
{
    // TODO: how to get base address
    register_mtd_device(FS_REGION_START_ADDR, FS_REGION_SIZE, FS_ROOT);
    if (dfs_mount(FS_ROOT, "/", "elm", 0, 0) == 0) // fs exist
    {
        rt_kprintf("mount fs on flash to root success\n");
    }
    else
    {
        // auto mkfs, remove it if you want to mkfs manual
        rt_kprintf("mount fs on flash to root fail\n");
        if (dfs_mkfs("elm", FS_ROOT) == 0)
        {
            rt_kprintf("make elm fs on flash sucess, mount again\n");
            if (dfs_mount(FS_ROOT, "/", "elm", 0, 0) == 0)
                rt_kprintf("mount fs on flash success\n");
            else
                rt_kprintf("mount to fs on flash fail\n");
        }
        else
            rt_kprintf("dfs_mkfs elm flash fail\n");
    }
    return RT_EOK;
}
INIT_ENV_EXPORT(mnt_init);
#endif
static void sleep_timer_timeout_handle(void *parameter)
{
    rt_kprintf("30 seconds timeout, set sleep enter flag\n");
    g_sleep_enter_flag = 1; // 设置进入睡眠标志位
}
static void start_sleep_timer(void)
{
    if (s_sleep_timer == RT_NULL)
    {
        s_sleep_timer =
            rt_timer_create("sleep_timer", sleep_timer_timeout_handle, RT_NULL,
                            rt_tick_from_millisecond(30000), // 30秒
                            RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
    }
    else
    {
        rt_timer_stop(s_sleep_timer);
        rt_timer_control(s_sleep_timer, RT_TIMER_CTRL_SET_TIME,
                         (void *)&(rt_tick_t){rt_tick_from_millisecond(30000)});
    }

    if (s_sleep_timer != RT_NULL)
    {
        rt_timer_start(s_sleep_timer);
        rt_kprintf("Sleep timer started, will trigger after 30 seconds\n");
    }
    else
    {
        rt_kprintf("Failed to create sleep timer\n");
    }
}
static void reconnect_timeout_handle(void *parameter)
{
    rt_mb_send(g_bt_app_mb, BT_APP_RECONNECT);
}

static void start_reconnect_timer(void)
{
    if (s_reconnect_timer == NULL)
    {
        s_reconnect_timer =
            rt_timer_create("reconnect", reconnect_timeout_handle, NULL,
                            rt_tick_from_millisecond(10000), // 1秒间隔
                            RT_TIMER_FLAG_PERIODIC);
    }

    if (s_reconnect_timer)
    {
        reconnect_attempts = 0;
        rt_timer_start(s_reconnect_timer);
        LOG_I("Start reconnect timer");
    }
}

void pan_reconnect()
{
    static int first_reconnect_attempts = 0;
    const int max_reconnect_attempts = 3;

    LOG_I("Attempting to reconnect PAN, attempt %d",
          first_reconnect_attempts + 1);
    xiaozhi_ui_chat_status("重新连接 PAN...");
    xiaozhi_ui_chat_output("正在重连PAN...");
    xiaozhi_ui_standby_chat_output("正在重连PAN...");
    if (first_reconnect_attempts < max_reconnect_attempts)
    {
        // 使用与主流程中相同的定时器机制来连接PAN
        if (!g_bt_app_env.pan_connect_timer)
            g_bt_app_env.pan_connect_timer = rt_timer_create(
                "connect_pan", bt_app_connect_pan_timeout_handle,
                (void *)&g_bt_app_env, rt_tick_from_millisecond(PAN_TIMER_MS),
                RT_TIMER_FLAG_SOFT_TIMER);
        else
            rt_timer_stop(g_bt_app_env.pan_connect_timer);
        rt_timer_start(g_bt_app_env.pan_connect_timer);

        first_reconnect_attempts++;
    }
    else
    {
        LOG_W("Failed to keep_first_reconnect PAN after %d attempts",
              max_reconnect_attempts);
        xiaozhi_ui_chat_status("无法连接PAN");
        xiaozhi_ui_chat_output("请确保设备开启了共享网络,重新发起连接");
        xiaozhi_ui_update_emoji("thinking");
        xiaozhi_ui_standby_chat_output("请确保设备开启了共享网络,重新发起连接");
        // 重置尝试次数计数器，以便下次需要时重新开始
        first_reconnect_attempts = 0;

        // 停止定时器
        if (g_bt_app_env.pan_connect_timer)
        {
            rt_timer_stop(g_bt_app_env.pan_connect_timer);
        }

        return;
    }
}

static int bt_app_interface_event_handle(uint16_t type, uint16_t event_id,
                                         uint8_t *data, uint16_t data_len)
{
    if (type == BT_NOTIFY_COMMON)
    {
        int pan_conn = 0;

        switch (event_id)
        {
        case BT_NOTIFY_COMMON_BT_STACK_READY:
        {
            rt_mb_send(g_bt_app_mb, BT_APP_READY);
        }
        break;
        case BT_NOTIFY_COMMON_ACL_CONNECTED:
        {
            LOG_I("BT_NOTIFY_COMMON_ACL_CONNECTED\n");
            // 清除蓝牙主动断开标志位
            Initiate_disconnection_flag = 0;
        }
        break;
        case BT_NOTIFY_COMMON_SCO_CONNECTED:
        {
            rt_kprintf("SCO已建立,语音经本设备传输\n");
            if (lv_screen_active() != call_screen &&
                g_prev_call_status) // 保证打电话的时候只能通过 “接听”
                                    // 的行为进行 开始 打电话
            {
                rt_kprintf("重新切换回打电话\n");
                // 打电话
                answer_phone();
            }
        }
        break;
        case BT_NOTIFY_COMMON_SCO_DISCONNECTED:
        {
            rt_kprintf("SCO已断开,语音路径回手机/原通道\n");
            if (lv_screen_active() == call_screen)
            {
                rt_kprintf("切换为外放\n");
                // 挂电话
                hung_up_phone();
            }
        }
        break;
        case BT_NOTIFY_COMMON_ACL_DISCONNECTED:
        {
            bt_notify_device_base_info_t *info =
                (bt_notify_device_base_info_t *)data;
            LOG_I("disconnected(0x%.2x:%.2x:%.2x:%.2x:%.2x:%.2x) res %d",
                  info->mac.addr[5], info->mac.addr[4], info->mac.addr[3],
                  info->mac.addr[2], info->mac.addr[1], info->mac.addr[0],
                  info->res);
            g_bt_app_env.bt_connected = FALSE;
            xiaozhi_ui_chat_output("蓝牙断开连接");
            xiaozhi_ui_standby_chat_output("蓝牙断开连接"); // 待机画面
            lv_obj_t *now_screen = lv_screen_active();
            if (now_screen != standby_screen && now_screen != sleep_screen &&
                now_screen != shutdown_screen)
            {
                ui_swith_to_standby_screen();
            }
            //  memset(&g_bt_app_env.bd_addr, 0xFF,
            //  sizeof(g_bt_app_env.bd_addr));
            if (info->res == BT_NOTIFY_COMMON_SCO_DISCONNECTED)
            {

                LOG_I("Phone actively disconnected, prepare to enter sleep "
                      "mode after 30 seconds");
                rt_mb_send(g_bt_app_mb, BT_APP_PHONE_DISCONNECTED);
            }
            else
            {
                LOG_I("Abnormal disconnection, start reconnect attempts");
                rt_mb_send(g_bt_app_mb, BT_APP_ABNORMAL_DISCONNECT);
            }

            if (g_bt_app_env.pan_connect_timer)
                rt_timer_stop(g_bt_app_env.pan_connect_timer);
        }
        break;
        case BT_NOTIFY_COMMON_ENCRYPTION:
        {
            bt_notify_device_mac_t *mac = (bt_notify_device_mac_t *)data;
            LOG_I("Encryption competed");
            g_bt_app_env.bd_addr = *mac;
            pan_conn = 1;
        }
        break;
        case BT_NOTIFY_COMMON_PAIR_IND:
        {
            bt_notify_device_base_info_t *info =
                (bt_notify_device_base_info_t *)data;
            LOG_I("Pairing completed %d", info->res);
            if (info->res == BTS2_SUCC)
            {
                g_bt_app_env.bd_addr = info->mac;
                pan_conn = 1;
            }
        }
        break;
        case BT_NOTIFY_COMMON_KEY_MISSING:
        {
            bt_notify_device_base_info_t *info =
                (bt_notify_device_base_info_t *)data;
            LOG_I("Key missing %d", info->res);
            memset(&g_bt_app_env.bd_addr, 0xFF, sizeof(g_bt_app_env.bd_addr));
            bt_cm_delete_bonded_devs_and_linkkey(info->mac.addr);
        }
        break;
        default:
            break;
        }

        if (pan_conn)
        {
            LOG_I("bd addr 0x%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
                  g_bt_app_env.bd_addr.addr[5], g_bt_app_env.bd_addr.addr[4],
                  g_bt_app_env.bd_addr.addr[3], g_bt_app_env.bd_addr.addr[2],
                  g_bt_app_env.bd_addr.addr[1], g_bt_app_env.bd_addr.addr[0]);
            g_bt_app_env.bt_connected = TRUE;
            // Trigger PAN connection after PAN_TIMER_MS period to avoid SDP
            // confliction.
            if (!g_bt_app_env.pan_connect_timer)
                g_bt_app_env.pan_connect_timer = rt_timer_create(
                    "connect_pan", bt_app_connect_pan_timeout_handle,
                    (void *)&g_bt_app_env,
                    rt_tick_from_millisecond(PAN_TIMER_MS),
                    RT_TIMER_FLAG_SOFT_TIMER);
            else
                rt_timer_stop(g_bt_app_env.pan_connect_timer);
            rt_timer_start(g_bt_app_env.pan_connect_timer);
        }
    }
    else if (type == BT_NOTIFY_PAN)
    {
        switch (event_id)
        {
        case BT_NOTIFY_PAN_PROFILE_CONNECTED:
        {
            xiaozhi_ui_chat_output("PAN连接成功");
            xiaozhi_ui_standby_chat_output("PAN连接成功"); // 待机画面
            xiaozhi_ui_update_ble("open");
            LOG_I("pan connect successed \n");
            if ((g_bt_app_env.pan_connect_timer))
            {
                rt_timer_stop(g_bt_app_env.pan_connect_timer);
            }
            rt_mb_send(g_bt_app_mb, BT_APP_CONNECT_PAN_SUCCESS);
            g_pan_connected = TRUE; // 更新PAN连接状态
        }
        break;
        case BT_NOTIFY_PAN_PROFILE_DISCONNECTED:
        {
            xiaozhi_ui_chat_status("PAN断开...");
            xiaozhi_ui_chat_output("PAN断开,尝试唤醒键重新连接");
            xiaozhi_ui_standby_chat_output(
                "PAN断开,尝试唤醒键重新连接"); // 待机画面
            xiaozhi_ui_update_ble("close");
            last_listen_tick = 0;
            LOG_I("pan disconnect with remote device\n");
            g_pan_connected = FALSE; // 更新PAN连接状态

            if (first_pan_connected ==
                FALSE) // Check if the pan has ever been connected
            {
                rt_mb_send(g_bt_app_mb, KEEP_FIRST_PAN_RECONNECT);
            }
        }
        break;
        default:
            break;
        }
    }
    else if (type == BT_NOTIFY_HID)
    {
        switch (event_id)
        {
        case BT_NOTIFY_HID_PROFILE_CONNECTED:
        {
            LOG_I("HID connected\n");
            if (!g_pan_connected)
            {
                if (g_bt_app_env.pan_connect_timer)
                {
                    rt_timer_stop(g_bt_app_env.pan_connect_timer);
                }
                bt_interface_conn_ext((char *)&g_bt_app_env.bd_addr,
                                      BT_PROFILE_PAN);
            }
        }
        break;
        case BT_NOTIFY_HID_PROFILE_DISCONNECTED:
        {
            LOG_I("HID disconnected\n");
        }
        break;
        default:
            break;
        }
    }
    else if (type == BT_NOTIFY_HFP_HF)
    {
        rt_kprintf("hfp_type=%d, event_id=%d\n", type, event_id);
        switch (event_id)
        {
        case BT_NOTIFY_HF_PROFILE_CONNECTED: // 0
        {
            bt_notify_profile_state_info_t *profile_info =
                (bt_notify_profile_state_info_t *)data;
            if (data_len >= sizeof(bt_notify_profile_state_info_t))
            {
                rt_kprintf("HFP已连接: %02x:%02x:%02x:%02x:%02x:%02x "
                           "(role=%d,res=%d)\n",
                           profile_info->mac.addr[5], profile_info->mac.addr[4],
                           profile_info->mac.addr[3], profile_info->mac.addr[2],
                           profile_info->mac.addr[1], profile_info->mac.addr[0],
                           profile_info->profile_role, profile_info->res);
            }
            else
            {
                rt_kprintf("HFP已连接\n");
            }

            // 连接建立后查询手机本机号码（AT+CNUM）
            bt_interface_get_ph_num();
            // 连接建立后主动查询一次聚合通话状态
            bt_interface_get_remote_call_status();
        }
        break;
        case BT_NOTIFY_HF_PROFILE_DISCONNECTED: // 1
        {
            rt_kprintf("HFP已断开\n");
        }
        break;
        case BT_NOTIFY_HF_LOCAL_PHONE_NUMBER: // 4  查询本机号码
                                              // 调用bt_interface_get_ph_num
        {
            if (data && data_len > 0)
            {
                char number[64];
                int n = data_len;
                if (n > (int)sizeof(number) - 1)
                    n = sizeof(number) - 1;
                memcpy(number, data, n);
                number[n] = '\0';
                rt_kprintf("本机号码: %s\n", number);
            }
        }
        break;
        case BT_NOTIFY_HF_VOLUME_CHANGE: // 6
        {
            rt_kprintf("HFP音量已改变\n");
        }
        break;
        case BT_NOTIFY_HF_CALL_STATUS_UPDATE: // 7 通话状态更新
        {
            bt_notify_all_call_status *call_status =
                (bt_notify_all_call_status *)data;
            if (data_len >= sizeof(bt_notify_all_call_status))
            {
                LOG_I("call_status=%d, callsetup_status=%d, callheld_status=%d",
                      call_status->call_status, call_status->callsetup_status,
                      call_status->callheld_status);

                // 来电
                if (g_prev_callsetup_status == 0 &&
                    call_status->call_status == 0 &&
                    call_status->callsetup_status != 0)
                {

                    if (call_status->callsetup_status == 1 ||
                        call_status->callsetup_status == 2)
                    {
                        rt_kprintf("来电...\n");
                    }
                    // answer_phone();
                }

                // 接听
                if (g_prev_call_status == 0 && call_status->call_status == 1)
                {

                    answer_phone();
                    rt_kprintf("已接听\n");
                }

                // 结束/挂断：
                // 1) 活跃通话结束
                if (g_prev_call_status == 1 && call_status->call_status == 0 &&
                    call_status->callsetup_status == 0)
                {

                    rt_kprintf("通话结束\n");
                    hung_up_phone();
                }
                // 2) 建立阶段直接结束（未接/取消）
                else if (g_prev_callsetup_status != 0 &&
                         call_status->callsetup_status == 0 &&
                         call_status->call_status == 0)
                {
                    rt_kprintf("已挂断\n");

                    // hung_up_phone();
                }

                g_prev_call_status = call_status->call_status;
                g_prev_callsetup_status = call_status->callsetup_status;
            }
        }
        break;
        case BT_NOTIFY_HF_REMOTE_CALL_INFO_IND: // 5 显示来电号码
        {
            bt_notify_clcc_ind_t *clcc_info = (bt_notify_clcc_ind_t *)data;
            if (data_len >= sizeof(bt_notify_clcc_ind_t) &&
                clcc_info->number_size > 0)
            {
                char buf[64];
                int n = clcc_info->number_size;
                if (n > (int)sizeof(buf) - 1)
                    n = sizeof(buf) - 1;
                memcpy(buf, clcc_info->number, n);
                buf[n] = '\0';
                rt_kprintf("来电号码: %s\n", buf);
            }
        }
        break;
        case BT_NOTIFY_HF_INDICATOR_UPDATE: // 8
        {
            if (data_len >= sizeof(bt_notify_cind_ind_t))
            {
                bt_notify_cind_ind_t *cind = (bt_notify_cind_ind_t *)data;
                if (cind->type == HFP_AG_CIND_CALL_TYPE ||
                    cind->type == HFP_AG_CIND_CALLSETUP_TYPE ||
                    cind->type == HFP_AG_CIND_CALLHELD_TYPE)
                {
                    rt_tick_t now = rt_tick_get();
                    if (s_last_cind_query_tick == 0 ||
                        (now - s_last_cind_query_tick) >=
                            rt_tick_from_millisecond(1000))
                    {
                        bt_interface_get_remote_call_status();
                        s_last_cind_query_tick = now;
                    }
                }
            }
        }
        break;
        default:
            break;
        }
    }

    return 0;
}

uint32_t bt_get_class_of_device()
{
    return (uint32_t)BT_SRVCLS_NETWORK | BT_DEVCLS_PERIPHERAL |
           BT_PERIPHERAL_REMCONTROL;
}

static int32_t Write_MAC(int argc, char **argv)
{
    uint8_t len;
    uint8_t mac[6] = {0};
    char *endptr;

    if (argc < 7)
    {
        rt_kprintf("write_mac FAIL\n");
        return 1;
    }
    for (len = 0; len < 6; len++)
    {
        mac[5 - len] = (uint8_t)(strtoul(argv[1 + len], &endptr, 16) & 0xFF);
        if (endptr == argv[1 + len])
        {
            rt_kprintf("incorrect MAC\n");
            return 2;
        }
    }

    len = rt_flash_config_write(FACTORY_CFG_ID_MAC, (uint8_t *)&mac[0], 6);
    if (len < 6)
    {
        rt_kprintf("write_mac FAIL\n");
    }
    else
    {
        rt_kprintf("MAC: %02x-%02x-%02x-%02x-%02x-%02x\n", mac[5], mac[4],
                   mac[3], mac[2], mac[1], mac[0]);
        rt_kprintf("write_mac PASS\n");
    }
    return 0;
}
MSH_CMD_EXPORT(Write_MAC, write mac);

void check_low_power(void)
{
    rt_device_t battery_device = rt_device_find("bat1");
    if (battery_device)
    {
        rt_adc_cmd_read_arg_t read_arg;
        read_arg.channel = 7;
        rt_adc_enable((rt_adc_device_t)battery_device, read_arg.channel);
        rt_thread_mdelay(300);
        rt_uint32_t battery_level =
            rt_adc_read((rt_adc_device_t)battery_device, read_arg.channel);
        rt_adc_disable((rt_adc_device_t)battery_device, read_arg.channel);

        battery_calculator_t battery_calc;
        battery_calculator_config_t calc_config = {
            .charging_table = charging_curve_table,
            .charging_table_size = charging_curve_table_size,
            .discharging_table = discharge_curve_table,
            .discharging_table_size = discharge_curve_table_size,
            .charge_filter_threshold = 50,    // 充电滤波阈值
            .discharge_filter_threshold = 30, // 放电滤波阈值
            .filter_count = 3                 // 滤波计数阈值
        };
        battery_calculator_init(&battery_calc, &calc_config);

        uint8_t battery_percentage =
            battery_calculator_get_percent(&battery_calc, battery_level);
        rt_uint8_t charge_status = rt_pin_read(CHARGE_DETECT_PIN);
        rt_kprintf("Boot battery check: %d%%, charging: %d\n",
                   battery_percentage, charge_status);

        // 如果低电量且未充电，进入低电量关机流程
        if (battery_percentage < LOW_BATTERY_THRESHOLD && !charge_status)
        {
            g_skip_startup = false;
            rt_kprintf("电量不足，进入低电量关机流程，当前电量为%d%%\n",
                       battery_percentage);
            xz_set_lcd_brightness(LCD_BRIGHTNESS_DEFAULT);
            rt_err_t result =
                rt_thread_init(&xiaozhi_ui_thread, "xz_ui", xiaozhi_ui_task,
                               NULL, &xiaozhi_ui_thread_stack[0],
                               XIAOZHI_UI_THREAD_STACK_SIZE, 30, 10);
            if (result == RT_EOK)
            {
                rt_thread_startup(&xiaozhi_ui_thread);
            }

            g_ui_task_mb = rt_mb_create("ui_mb", 8, RT_IPC_FLAG_FIFO);
            rt_thread_mdelay(2000);
            if (g_ui_task_mb != RT_NULL)
            {
                rt_mb_send(g_ui_task_mb, UI_EVENT_LOW_BATTERY_WARNING);
            }

            while (1)
            {
                rt_thread_mdelay(1000);
            }
        }
        rt_kprintf("电量充足，正常开机\n");
    }
}
int main(void)
{
    check_poweron_reason();
    // 初始化邮箱
    g_button_event_mb = rt_mb_create("btn_evt", 8, RT_IPC_FLAG_FIFO);
    if (g_button_event_mb == NULL)
    {
        rt_kprintf("Failed to create mailbox g_button_event_mb\n");
        return 0;
    }
    // 初始化电池邮箱
    g_battery_mb = rt_mb_create("battery_level", 1, RT_IPC_FLAG_FIFO);
#ifdef LOW_POWER_NO_SHUTDOWN // 针对立创没有电池的板子

#else
    check_low_power();
#endif                             /* LOW_POWER_NO_SHUTDOWN */
    rt_charge_set_cc_current(100); // 设置充电电流
    rt_kprintf("Xiaozhi start!!!\n");
    audio_server_set_private_volume(AUDIO_TYPE_LOCAL_MUSIC,
                                    VOL_DEFAULE_LEVEL); // 设置音量
    xz_set_lcd_brightness(LCD_BRIGHTNESS_DEFAULT);
    iot_initialize();            // Initialize iot
    xiaozhi_time_weather_init(); // Initialize time and weather
    // rt_pm_request(PM_SLEEP_MODE_IDLE);

#ifdef XIAOZHI_USING_MQTT
#else
    xz_ws_audio_init();   // 初始化音频
    xz_ws_button_init2(); // 初始化关机键

#endif
    set_pinmux();
    // Create  xiaozhi UI
    rt_err_t result = rt_thread_init(
        &xiaozhi_ui_thread, "xz_ui", xiaozhi_ui_task, NULL,
        &xiaozhi_ui_thread_stack[0], XIAOZHI_UI_THREAD_STACK_SIZE, 30, 10);
    if (result == RT_EOK)
    {
        rt_thread_startup(&xiaozhi_ui_thread);
    }
    else
    {
        rt_kprintf("Failed to init xiaozhi UI thread\n");
    }
    // Connect BT PAN

    g_bt_app_mb = rt_mb_create("bt_app", 8, RT_IPC_FLAG_FIFO);
#ifdef BSP_BT_CONNECTION_MANAGER
    bt_cm_set_profile_target(BT_CM_HID, BT_LINK_PHONE, 1);
#endif // BSP_BT_CONNECTION_MANAGER

    bt_interface_register_bt_event_notify_callback(
        bt_app_interface_event_handle);

    sifli_ble_enable();

    rt_err_t battery_thread_result = rt_thread_init(
        &battery_thread, "battery", battery_level_task, NULL,
        &battery_thread_stack[0], BATTERY_THREAD_STACK_SIZE, 20, 10);
    if (battery_thread_result == RT_EOK)
    {
        rt_thread_startup(&battery_thread); // 启动
    }
    else
    {
        rt_kprintf("Failed to init battery thread\n");
    }

#ifdef BSP_USING_BOARD_SF32LB52_XTY_AI
    if (pulse_encoder_init() != RT_EOK)
    {
        rt_kprintf("Pulse encoder initialization failed.\n");
        return -RT_ERROR;
    }

    s_pulse_encoder_timer =
        rt_timer_create("pulse_encoder", pulse_encoder_timeout_handle, NULL,
                        rt_tick_from_millisecond(200), RT_TIMER_FLAG_PERIODIC);
    if (s_pulse_encoder_timer)
    {
        rt_kprintf("Pulse encoder timer created successfully.\n");
        rt_timer_start(s_pulse_encoder_timer);
    }
    else
    {
        rt_kprintf("Failed to create pulse encoder timer.\n");
        return -1;
    }
#endif
    while (1)
    {

        uint32_t value;

        // handle pan connect event
        rt_mb_recv(g_bt_app_mb, (rt_uint32_t *)&value, RT_WAITING_FOREVER);

        if (value == BT_APP_CONNECT_PAN)
        {
            if (g_bt_app_env.bt_connected)
            {
                bt_interface_conn_ext((char *)&g_bt_app_env.bd_addr,
                                      BT_PROFILE_PAN);
            }
        }
        else if (value == BT_APP_READY)
        {
            LOG_I("BT/BLE stack and profile ready");

#ifdef BT_NAME_MAC_ENABLE
            char local_name[32];
            bd_addr_t addr;
            ble_get_public_address(&addr);
            sprintf(local_name, "%s-%02x:%02x:%02x:%02x:%02x:%02x",
                    BLUETOOTH_NAME, addr.addr[0], addr.addr[1], addr.addr[2],
                    addr.addr[3], addr.addr[4], addr.addr[5]);
#else
            const char *local_name = BLUETOOTH_NAME;
#endif
            bt_interface_set_local_name(strlen(local_name), (void *)local_name);
        }
        else if (value == BT_APP_CONNECT_PAN_SUCCESS)
        {
            rt_kputs("BT_APP_CONNECT_PAN_SUCCESS\r\n");

            // xiaozhi_ui_chat_output("初始化 请稍等...");
            xiaozhi_ui_standby_chat_output("初始化 请稍等...");
            xiaozhi_ui_update_ble("open");
            xiaozhi_ui_chat_status("初始化...");
            xiaozhi_ui_update_emoji("neutral");

            rt_thread_mdelay(2000);
            // 执行NTP与天气同步
            xiaozhi_time_weather();

            // 先进行设备注册
            int reg_result = register_device_with_server();
            if (reg_result != 0)
            {
                rt_kprintf("Device registration failed, continuing with "
                           "default chip_id\n");
                // 如果注册失败，可以使用默认chip_id或者采取其他措施
            }
            // 获取动态chip_id并构建查询URL
            char *chip_id = get_client_id();
            char *dynamic_ota_url = build_ota_query_url(chip_id);
            rt_kprintf("Dynamic chip_id: %s\n", chip_id);
            rt_kputs(dynamic_ota_url);
            rt_kprintf("\n");
            // 执行版本信息查询
            int result = dfu_pan_query_latest_version(dynamic_ota_url, VERSION,
                                                      latest_version,
                                                      sizeof(latest_version));
            rt_kprintf("OTA query result: %d, latest version: %s\n", result,
                       latest_version);
            // 根据返回值判断是否有更新
            BOOL needs_update = (result > 0) ? RT_TRUE : RT_FALSE;

            if (needs_update)
            {
                // 弹窗提示版本
                xiaozhi_ui_update_latest_version(latest_version);
            }

            xiaozhi_ui_standby_chat_output("请按键连接小智...");
            lv_display_trigger_activity(NULL);

#ifdef XIAOZHI_USING_MQTT
            xiaozhi(0, NULL);
            // xz_mqtt_button_init();
#else
            xz_ws_button_init();
            // xiaozhi2(0, NULL); // Start Xiaozhi
#endif
            // 在蓝牙和PAN连接成功后创建睡眠定时器
            if (!ui_sleep_timer && g_pan_connected)
            {
                rt_kprintf("create sleep timer2\n");
                ui_sleep_timer =
                    lv_timer_create(ui_sleep_callback, 40000, NULL);
            }
        }
        else if (value == KEEP_FIRST_PAN_RECONNECT)
        {
            pan_reconnect(); // Ensure that the first pan connection
                             // is successful
        }
#ifdef XIAOZHI_USING_MQTT

#else
        else if (value == WEBSOC_RECONNECT) // Reconnect Xiaozhi websocket
        {

            xiaozhi2(0, NULL); // 重连小智websocket
        }
        else if (value == BT_APP_PHONE_DISCONNECTED)
        {
            rt_kprintf("Phone actively disconnected, enter sleep mode after 30 "
                       "seconds\n");
            Initiate_disconnection_flag = 1;
            start_sleep_timer();
            // 睡眠
        }
        else if (value == BT_APP_ABNORMAL_DISCONNECT)
        {
            rt_kprintf("Abnormal disconnection, start reconnect attempts\n");
            rt_thread_mdelay(3000);
            reconnect_attempts = 0;
            start_reconnect_timer();
        }
        else if (value == BT_APP_RECONNECT_TIMEOUT)
        {
            rt_kprintf("Reconnect timeout, enter sleep mode\n");
            g_sleep_enter_flag = 1;
            // 睡眠
        }
        else if (value == BT_APP_RECONNECT)
        {
            if (g_bt_app_env.bt_connected)
            {
                // 已经重新连接成功，停止定时器
                if (s_reconnect_timer)
                {
                    rt_timer_stop(s_reconnect_timer);
                }
                reconnect_attempts = 0;
                LOG_I("Reconnect successful, stop reconnect timer");
            }
            else
            {
                reconnect_attempts++;
                LOG_I("Reconnect attempt %d/%d", reconnect_attempts,
                      MAX_RECONNECT_ATTEMPTS);
            }

            if (reconnect_attempts <= MAX_RECONNECT_ATTEMPTS)
            {
                bt_interface_conn_ext((char *)&g_bt_app_env.bd_addr,
                                      BT_PROFILE_HID);
            }
            else
            {
                LOG_I("Reconnect timeout, send timeout event");
                rt_mb_send(g_bt_app_mb, BT_APP_RECONNECT_TIMEOUT);
                reconnect_attempts = 0;
                if (s_reconnect_timer)
                {
                    rt_timer_stop(s_reconnect_timer);
                }
            }
        }
        else if (value == UPDATE_REAL_WEATHER_AND_TIME)
        {
            xiaozhi_time_weather();
        }
        else
        {
            rt_kprintf("WEBSOCKET_DISCONNECT\r\n");
            xiaozhi_ui_chat_output("请重启");
            xiaozhi_ui_standby_chat_output("请重启"); // 待机画面
        }
#endif
        rt_kprintf("main while loop\r\n");
    }
    return 0;
}

static void pan_cmd(int argc, char **argv)
{
    if (strcmp(argv[1], "del_bond") == 0)
    {
#ifdef BSP_BT_CONNECTION_MANAGER
        bt_cm_delete_bonded_devs();
        LOG_D("Delete bond");
#endif // BSP_BT_CONNECTION_MANAGER
    }
    // only valid after connection setup but phone didn't enable pernal hop
    else if (strcmp(argv[1], "conn_pan") == 0)
        bt_app_connect_pan_timeout_handle(NULL);
}
MSH_CMD_EXPORT(pan_cmd, Connect PAN to last paired device);

static void dfu_pan_finish_xz_cmd(int argc, char **argv)
{
    dfu_pan_test_update_flags();
}
MSH_CMD_EXPORT(dfu_pan_finish_xz_cmd, OTA finish verification command);

static void dfu_pan_print_files_xz_cmd(int argc, char **argv)
{
    dfu_pan_print_files();
}
MSH_CMD_EXPORT(dfu_pan_print_files_xz_cmd, Print OTA firmware files status);

static void dfu_pan_check_dynamic_xz_cmd(int argc, char **argv)
{
    LOG_I("Checking for new firmware version with dynamic chip_id...");

    // 先注册设备
    int reg_result = register_device_with_server();
    if (reg_result != 0)
    {
        LOG_W("Device registration failed");
    }

    // 构建动态URL
    char *chip_id = get_client_id();
    char *dynamic_ota_url = build_ota_query_url(chip_id);

    // 查询最新版本
    int result = dfu_pan_query_latest_version(
        dynamic_ota_url, VERSION, latest_version, sizeof(latest_version));

    // 根据返回值判断是否有更新
    BOOL needs_update = (result > 0) ? RT_TRUE : RT_FALSE;

    if (needs_update)
    {
        LOG_I("New firmware version %s available. Type 'go' to start update.",
              latest_version);
    }
    else
    {
        LOG_I("No new firmware version available.");
    }
}
MSH_CMD_EXPORT(dfu_pan_check_dynamic_xz_cmd, Check for new firmware version with dynamic chip_id);