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
#include "ble_connection_manager.h"
#include "bt_connection_manager.h"
#include "bt_env.h"
#include "ulog.h"
#include "drv_gpio.h"
/* Common functions for RT-Thread based platform
 * -----------------------------------------------*/
/**
 * @brief  Initialize board default configuration.
 * @param  None
 * @retval None
 */
/* User code start from here
 * --------------------------------------------------------*/

#define BT_APP_READY 0
#define BT_APP_CONNECT_PAN 1
#define BT_APP_CONNECT_PAN_SUCCESS 2
#define WEBSOC_RECONNECT 4
#define KEEP_FIRST_PAN_RECONNECT 5
#define XZ_CONFIG_UPDATE        6
#define BT_APP_PHONE_DISCONNECTED 7    // 手机主动断开
#define BT_APP_ABNORMAL_DISCONNECT 8   // 异常断开
#define BT_APP_RECONNECT_TIMEOUT 9    // 重连超时
#define BT_APP_RECONNECT 10 // 重连
#define UPDATE_REAL_WEATHER_AND_TIME 11
#define PAN_TIMER_MS 3000
#define MQTT_RECONNECT 12

#define MAX_RECONNECT_ATTEMPTS 30  // 30次尝试，每次1秒，共30秒
#define XIAOZHI_UI_THREAD_STACK_SIZE (6144)
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

bt_app_t g_bt_app_env;
rt_mailbox_t g_bt_app_mb;
BOOL g_pan_connected = FALSE;
BOOL first_pan_connected = FALSE;
int first_reconnect_attempts = 0;
uint8_t Initiate_disconnection_flag = 0;//蓝牙主动断开标志
rt_mailbox_t g_battery_mb;
bool g_skip_startup = true; 
bool low_battery_shutdown_triggered = true;

static rt_timer_t s_reconnect_timer = NULL;
static rt_timer_t s_sleep_timer = NULL;
static int reconnect_attempts = 0;
static uint8_t g_sleep_enter_flag = 0;    // 进入睡眠标志位
// UI线程和battery线程控制块
static struct rt_thread xiaozhi_ui_thread;
static struct rt_thread battery_thread;


//ui线程
#if defined(__CC_ARM) || defined(__CLANG_ARM)
L2_RET_BSS_SECT_BEGIN(xiaozhi_ui_thread_stack) //6000地址
static uint32_t xiaozhi_ui_thread_stack[XIAOZHI_UI_THREAD_STACK_SIZE / sizeof(uint32_t)];
L2_RET_BSS_SECT_END
#else
static uint32_t
    xiaozhi_ui_thread_stack[XIAOZHI_UI_THREAD_STACK_SIZE / sizeof(uint32_t)] L2_RET_BSS_SECT(xiaozhi_ui_thread_stack);
#endif
//battery线程
#if defined(__CC_ARM) || defined(__CLANG_ARM)
L2_RET_BSS_SECT_BEGIN(battery_thread_stack) //6000地址
static uint32_t battery_thread_stack[BATTERY_THREAD_STACK_SIZE / sizeof(uint32_t)];
L2_RET_BSS_SECT_END
#else
static uint32_t
    battery_thread_stack[BATTERY_THREAD_STACK_SIZE / sizeof(uint32_t)] L2_RET_BSS_SECT(battery_thread_stack);
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
        rt_uint32_t battery_level =
            rt_adc_read((rt_adc_device_t)battery_device, read_arg.channel);
        rt_adc_disable((rt_adc_device_t)battery_device, read_arg.channel);

        // 获取到的是电池电压，单位是mV
        // 假设电池电压范围是3.6V到4.2V，对应的电量范围是0%到100%
        uint32_t battery_percentage = 0;
        if (battery_level < 36000)
        {
            battery_percentage = 0; // 小于3.6V，电量为0
        }
        else if (battery_level > 42000)
        {
            battery_percentage = 100; // 大于4.2V，电量为100
        }
        else
        {
            // 线性插值计算电量百分比
            battery_percentage = ((battery_level - 36000) * 100) / (42000 - 36000);
        }

        rt_mb_send(g_battery_mb, battery_percentage);
        current_status = rt_pin_read(CHARGE_DETECT_PIN);
        rt_kprintf("battery_percentage: %d, current_status: %d \n", battery_percentage, current_status);
        //当电量低于阈值并且当前没有处于充电中并的时候
        if (battery_percentage < LOW_BATTERY_THRESHOLD && low_battery_shutdown_triggered && !current_status) 
        {
            lv_obj_t *now_screen = lv_screen_active();
            rt_kprintf("now_screen address: %p, sleep_screen address: %p, standby_screen address: %p\n", 
               now_screen, sleep_screen, standby_screen);
            low_battery_shutdown_triggered = false;
            rt_kprintf("Low battery ,shutdown\n");
            // 发送消息到UI线程显示低电量关机页面
            if (g_ui_task_mb != RT_NULL) 
            {
                if(now_screen == sleep_screen && now_screen != NULL)
                {
                    gui_pm_fsm(GUI_PM_ACTION_WAKEUP); // 唤醒设备
                }
                
                rt_thread_mdelay(100);
                rt_mb_send(g_ui_task_mb, UI_EVENT_LOW_BATTERY_SHUTDOWN);
            }
        }
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
    #define NAND_MTD_NAME "root"
int mnt_init(void)
{
    // TODO: how to get base address
    register_nand_device(FS_REGION_START_ADDR & (0xFC000000),
                         FS_REGION_START_ADDR -
                             (FS_REGION_START_ADDR & (0xFC000000)),
                         FS_REGION_SIZE, NAND_MTD_NAME);
    if (dfs_mount(NAND_MTD_NAME, "/", "elm", 0, 0) == 0) // fs exist
    {
        rt_kprintf("mount fs on flash to root success\n");
    }
    else
    {
        // auto mkfs, remove it if you want to mkfs manual
        rt_kprintf("mount fs on flash to root fail\n");
        if (dfs_mkfs("elm", NAND_MTD_NAME) == 0)
        {
            rt_kprintf("make elm fs on flash sucess, mount again\n");
            if (dfs_mount(NAND_MTD_NAME, "/", "elm", 0, 0) == 0)
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
    g_sleep_enter_flag = 1;  // 设置进入睡眠标志位
}
static void start_sleep_timer(void)
{
    if (s_sleep_timer == RT_NULL) {
        s_sleep_timer = rt_timer_create("sleep_timer", 
                                        sleep_timer_timeout_handle,
                                        RT_NULL, 
                                        rt_tick_from_millisecond(30000),  // 30秒
                                        RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
    } else {
        rt_timer_stop(s_sleep_timer);
        rt_timer_control(s_sleep_timer, RT_TIMER_CTRL_SET_TIME, (void *)&(rt_tick_t){rt_tick_from_millisecond(30000)});
    }
    
    if (s_sleep_timer != RT_NULL) {
        rt_timer_start(s_sleep_timer);
        rt_kprintf("Sleep timer started, will trigger after 30 seconds\n");
    } else {
        rt_kprintf("Failed to create sleep timer\n");
    }
}
static void reconnect_timeout_handle(void *parameter)
{
    rt_mb_send(g_bt_app_mb, BT_APP_RECONNECT);
}

static void start_reconnect_timer(void)
{
    if (s_reconnect_timer == NULL) {
        s_reconnect_timer = rt_timer_create(
            "reconnect", reconnect_timeout_handle, NULL,
            rt_tick_from_millisecond(10000), // 1秒间隔
            RT_TIMER_FLAG_PERIODIC);
    }
    
    if (s_reconnect_timer) {
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
                (void *)&g_bt_app_env,
                rt_tick_from_millisecond(PAN_TIMER_MS),
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
        if (g_bt_app_env.pan_connect_timer) {
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
            xiaozhi_ui_standby_chat_output("蓝牙断开连接");//待机画面
            lv_obj_t *now_screen = lv_screen_active();
            if (now_screen != standby_screen && now_screen != sleep_screen && now_screen != shutdown_screen)
                {
                    ui_swith_to_standby_screen();
                }
            //  memset(&g_bt_app_env.bd_addr, 0xFF,
            //  sizeof(g_bt_app_env.bd_addr));
                 if (info->res == BT_NOTIFY_COMMON_SCO_DISCONNECTED) 
                {
                
                    LOG_I("Phone actively disconnected, prepare to enter sleep mode after 30 seconds");
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
            xiaozhi_ui_standby_chat_output("PAN连接成功");//待机画面
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
            xiaozhi_ui_standby_chat_output("PAN断开,尝试唤醒键重新连接");//待机画面
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
        rt_kprintf("MAC: %02x-%02x-%02x-%02x-%02x-%02x\n", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
        rt_kprintf("write_mac PASS\n");
    }
    return 0;
}
MSH_CMD_EXPORT(Write_MAC, write mac);

void check_low_power(void)
{
    rt_device_t battery_device = rt_device_find("bat1");
    if (battery_device) {
        rt_adc_cmd_read_arg_t read_arg;
        read_arg.channel = 7;
        rt_adc_enable((rt_adc_device_t)battery_device, read_arg.channel);
        rt_uint32_t battery_level = rt_adc_read((rt_adc_device_t)battery_device, read_arg.channel);
        rt_adc_disable((rt_adc_device_t)battery_device, read_arg.channel);
        
        uint32_t battery_percentage = 0;
        if (battery_level < 36000) {
            battery_percentage = 0;
        } else if (battery_level > 42000) {
            battery_percentage = 100;
        } else {
            battery_percentage = ((battery_level - 36000) * 100) / (42000 - 36000);
        }
        
        // 检查充电状态
        rt_pin_mode(44, PIN_MODE_INPUT);
        uint8_t charge_status = rt_pin_read(44);
        
        rt_kprintf("Boot battery check: %d%%, charging: %d\n", battery_percentage, charge_status);
        
        // 如果低电量且未充电，进入低电量关机流程
        if (battery_percentage < LOW_BATTERY_THRESHOLD && !charge_status) 
        {
            g_skip_startup = false;
            rt_kprintf("电量不足，进入低电量关机流程，当前电量为%d%%\n", battery_percentage);
            xz_set_lcd_brightness(LCD_BRIGHTNESS_DEFAULT);
            rt_err_t result = rt_thread_init(&xiaozhi_ui_thread,
                                             "xz_ui",
                                             xiaozhi_ui_task,
                                             NULL,
                                             &xiaozhi_ui_thread_stack[0],
                                             XIAOZHI_UI_THREAD_STACK_SIZE,
                                             30,
                                             10);
            if (result == RT_EOK) {
                rt_thread_startup(&xiaozhi_ui_thread);
            }
            
            g_ui_task_mb = rt_mb_create("ui_mb", 8, RT_IPC_FLAG_FIFO);
            rt_thread_mdelay(2000);
            if (g_ui_task_mb != RT_NULL) {
                rt_mb_send(g_ui_task_mb, UI_EVENT_LOW_BATTERY_WARNING);
            }
            
            while (1) {
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
    //初始化电池邮箱
    g_battery_mb = rt_mb_create("battery_level", 1, RT_IPC_FLAG_FIFO);
    check_low_power();

    rt_kprintf("Xiaozhi start!!!\n");
    audio_server_set_private_volume(AUDIO_TYPE_LOCAL_MUSIC, VOL_DEFAULE_LEVEL); // 设置音量 
    xz_set_lcd_brightness(LCD_BRIGHTNESS_DEFAULT);
    iot_initialize(); // Initialize iot
    xiaozhi_time_weather_init();// Initialize time and weather
#ifdef XIAOZHI_USING_MQTT
#else
    xz_ws_audio_init(); // 初始化音频
    xz_ws_button_init2();//初始化关机键

#endif
    set_pinmux();
    // Create  xiaozhi UI
    rt_err_t result = rt_thread_init(&xiaozhi_ui_thread,
                                     "xz_ui",
                                     xiaozhi_ui_task,
                                     NULL,
                                     &xiaozhi_ui_thread_stack[0],
                                     XIAOZHI_UI_THREAD_STACK_SIZE,
                                     30,
                                     10);
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

    rt_err_t battery_thread_result = rt_thread_init(&battery_thread,    
                                                    "battery",          
                                                    battery_level_task,
                                                    NULL,              
                                                    &battery_thread_stack[0], 
                                                    BATTERY_THREAD_STACK_SIZE, 
                                                    20,                
                                                    10);               
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
            
            //xiaozhi_ui_chat_output("初始化 请稍等...");
            xiaozhi_ui_standby_chat_output("初始化 请稍等...");
            xiaozhi_ui_update_ble("open");
            xiaozhi_ui_chat_status("初始化...");
            xiaozhi_ui_update_emoji("neutral");

            rt_thread_mdelay(2000);
            // 执行NTP与天气同步
            xiaozhi_time_weather();
            //xiaozhi_ui_chat_output("连接小智中...");
            xiaozhi_ui_standby_chat_output("请按键连接小智...");
            lv_display_trigger_activity(NULL);

#ifdef XIAOZHI_USING_MQTT
            xiaozhi(0,NULL);
            //xz_mqtt_button_init();
#else
            xz_ws_button_init();
            // xiaozhi2(0, NULL); // Start Xiaozhi
#endif
            // 在蓝牙和PAN连接成功后创建睡眠定时器
            if (!ui_sleep_timer && g_pan_connected)
            {
                rt_kprintf("create sleep timer2\n");
                ui_sleep_timer = lv_timer_create(ui_sleep_callback, 40000, NULL);
            }
        }
        else if (value == KEEP_FIRST_PAN_RECONNECT)
        {
            pan_reconnect();// Ensure that the first pan connection
                                         // is successful
        }
#ifdef XIAOZHI_USING_MQTT

#else
        else if (value == WEBSOC_RECONNECT) // Reconnect Xiaozhi websocket
        {

                        xiaozhi2(0,NULL); // 重连小智websocket
       } 
        else if(value == BT_APP_PHONE_DISCONNECTED)
        {
            rt_kprintf("Phone actively disconnected, enter sleep mode after 30 seconds\n");
            Initiate_disconnection_flag = 1;
            start_sleep_timer();
            //睡眠
        }
        else if(value == BT_APP_ABNORMAL_DISCONNECT)
        {
            rt_kprintf("Abnormal disconnection, start reconnect attempts\n");
            rt_thread_mdelay(3000);
            reconnect_attempts = 0;
            start_reconnect_timer();
        }
        else if(value == BT_APP_RECONNECT_TIMEOUT)
        {
            rt_kprintf("Reconnect timeout, enter sleep mode\n");
            g_sleep_enter_flag = 1;
            //睡眠
        }
        else if(value == BT_APP_RECONNECT)
        {
            if (g_bt_app_env.bt_connected) 
            {
                // 已经重新连接成功，停止定时器
                if (s_reconnect_timer) {
                    rt_timer_stop(s_reconnect_timer);
                }
                reconnect_attempts = 0;
                LOG_I("Reconnect successful, stop reconnect timer");
            }
            else
            {
                reconnect_attempts++;
                LOG_I("Reconnect attempt %d/%d", reconnect_attempts, MAX_RECONNECT_ATTEMPTS);
            }

            if (reconnect_attempts <= MAX_RECONNECT_ATTEMPTS) 
            {
                bt_interface_conn_ext((char *)&g_bt_app_env.bd_addr, BT_PROFILE_HID);
            }
            else
            {
                LOG_I("Reconnect timeout, send timeout event");
                rt_mb_send(g_bt_app_mb, BT_APP_RECONNECT_TIMEOUT);
                reconnect_attempts = 0;
                 if (s_reconnect_timer) {
                        rt_timer_stop(s_reconnect_timer);
                    }
            }
        }
        else if(value == UPDATE_REAL_WEATHER_AND_TIME)
        {
            xiaozhi_time_weather();
        }        
        else
        {
            rt_kprintf("WEBSOCKET_DISCONNECT\r\n");
            xiaozhi_ui_chat_output("请重启");
            xiaozhi_ui_standby_chat_output("请重启");//待机画面
        }
#endif
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
