// xiaozhi_public.c
#include "xiaozhi_public.h"
#include <rtthread.h>
#include "bf0_hal.h"
#include "bts2_global.h"
#include "bts2_app_pan.h"
#include "bt_connection_manager.h"
#include "bt_env.h"
#include <webclient.h>
#include <cJSON.h>
#include "stdio.h"
#include "string.h"
#include <lwip/dns.h>
#include <lwip/sys.h>
#include "xiaozhi2.h"
#include "lwip/api.h"
#include "lwip/dns.h"
#include "lwip/apps/websocket_client.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/apps/mqtt.h"
#include "lwip/tcpip.h"
#include "lv_timer.h"
#include "lv_display.h"
#include "lv_obj_pos.h"
#include "lv_tiny_ttf.h"
#include "lv_obj.h"
#include "lv_label.h"
#include "bf0_sys_cfg.h"
#include "drv_flash.h"
#include "gui_app_pm.h"
#include "bf0_pin_const.h"
#include <bf0_hal_pinmux.h>

static const char *ota_version =
    "{\r\n "
    "\"version\": 2,\r\n"
    "\"flash_size\": 4194304,\r\n"
    "\"psram_size\": 0,\r\n"
    "\"minimum_free_heap_size\": 123456,\r\n"
    "\"mac_address\": \"%s\",\r\n"
    "\"uuid\": \"%s\",\r\n"
    "\"chip_model_name\": \"sf32lb563\",\r\n"
    "\"chip_info\": {\r\n"
    "    \"model\": 1,\r\n"
    "    \"cores\": 2,\r\n"
    "    \"revision\": 0,\r\n"
    "    \"features\": 0\r\n"
    "},\r\n"
    "\"application\": {\r\n"
    "    \"name\": \"my-app\",\r\n"
    "    \"version\": \"1.0.0\",\r\n"
    "    \"compile_time\": \"2021-01-01T00:00:00Z\",\r\n"
    "    \"idf_version\": \"4.2-dev\",\r\n"
    "    \"elf_sha256\": \"\"\r\n"
    "},\r\n"
    "\"partition_table\": [\r\n"
    "    {\r\n"
    "        \"label\": \"app\",\r\n"
    "        \"type\": 1,\r\n"
    "        \"subtype\": 2,\r\n"
    "        \"address\": 10000,\r\n"
    "        \"size\": 100000\r\n"
    "    }\r\n"
    "],\r\n"
    "\"ota\": {\r\n"
    "    \"label\": \"ota_0\"\r\n"
    "},\r\n"
    "\"board\": {\r\n"
    "    \"type\":\"hdk563\",\r\n"
    "    \"mac\": \"%s\"\r\n"
    "}\r\n"
    "}\r\n";

// 公共变量定义
static uint8_t g_en_vad = 1;
static uint8_t g_en_aec = 1;
static uint8_t g_config_change = 0;

volatile int g_kws_force_exit = 0;
volatile int g_kws_running = 0;
volatile uint8_t she_bei_ma = 1;

char mac_address_string[20];
char client_id_string[40];
ALIGN(4) uint8_t g_sha256_result[32] = {0};
extern uint8_t aec_enabled;
HAL_RAM_RET_CODE_SECT(PowerDownCustom, void PowerDownCustom(void))
{
    rt_kprintf("PowerDownCustom\n");
#ifdef BSP_USING_BOARD_SF32LB52_LCD_N16R8
    BSP_LCD_PowerDown();
    HAL_PMU_SelectWakeupPin(0, HAL_HPAON_QueryWakeupPin(hwp_gpio1, BSP_KEY1_PIN)); //select PA34 to wake_pin0
    HAL_PMU_EnablePinWakeup(0, AON_PIN_MODE_HIGH);  //enable wake_pin0
    hwp_pmuc->WKUP_CNT = 0x50005;    //31-16bit:config PIN1 wake CNT , 15-0bit:PIN0 wake CNT
#else
    HAL_PMU_SelectWakeupPin(0, 19); // PA43
    HAL_PMU_EnablePinWakeup(0, 0);
#endif
    HAL_PIN_Set(PAD_PA24, GPIO_A24, PIN_PULLDOWN, 1);
    for (uint32_t i = PAD_PA28; i <= PAD_PA44; i++)
    {
        HAL_PIN_Set(i, i - PAD_PA28 + GPIO_A28, PIN_PULLDOWN, 1);
    }
    hwp_pmuc->PERI_LDO &=  ~(PMUC_PERI_LDO_EN_LDO18 | PMUC_PERI_LDO_EN_VDD33_LDO2 | PMUC_PERI_LDO_EN_VDD33_LDO3);
    hwp_pmuc->WKUP_CNT = 0x000F000F;

    

    rt_hw_interrupt_disable();
    rt_kprintf("PowerDownCustom2\n");
    HAL_PMU_ConfigPeriLdo(PMUC_PERI_LDO_EN_VDD33_LDO2_Pos, false, false);
    HAL_PMU_ConfigPeriLdo(PMU_PERI_LDO_1V8, false, false);
    HAL_PMU_EnterHibernate();
    rt_kprintf("PowerDownCustom3\n");
}
ble_common_update_type_t ble_request_public_address(bd_addr_t *addr)
{
    uint8_t mac[6] = {0};
    int ret = 0;
    int read_len = rt_flash_config_read(FACTORY_CFG_ID_MAC, mac, 6);
    if (read_len == 0)
    {
        // OTP没有内容，用UID生成MAC
        ret = bt_mac_addr_generate_via_uid_v2(addr);
        if (ret != 0)
        {   
            rt_kprintf("uid get mac fail: %d", ret);
            return BLE_UPDATE_NO_UPDATE;
        }
        else
        {
            // 抹掉最后一个字节的bit0和bit1，避免组播地址
            addr->addr[5] &= ~0x03; 
            rt_kprintf("uid get mac ok\n");
            rt_kprintf("UID mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
            addr->addr[5], addr->addr[4], addr->addr[3],
            addr->addr[2], addr->addr[1], addr->addr[0]);
        }
    }
    else
    {
        // OTP有内容，直接用
        memcpy(addr->addr, mac, 6);
        rt_kprintf("MAC read from OTP: %02x:%02x:%02x:%02x:%02x:%02x\n",
            mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
    }

    return BLE_UPDATE_ONCE;
}

char *get_mac_address()
{
    if (mac_address_string[0] == '\0')
    {
        BTS2S_ETHER_ADDR addr = bt_pan_get_mac_address(NULL);
        uint8_t *p = (uint8_t *)&(addr);

        rt_snprintf((char *)mac_address_string, 20,
                    "%02x:%02x:%02x:%02x:%02x:%02x", *(p + 1),*p, *(p + 3),
                    *(p + 2), *(p + 5), *(p + 4));
    }
    return (&(mac_address_string[0]));
}
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
    uint8_t i = (n >> 4);
    if (i >= 10)
        *str = i + 'a' - 10;
    else
        *str = i + '0';
    str++, i = n & 0xf;
    if (i >= 10)
        *str = i + 'a' - 10;
    else
        *str = i + '0';
}
char *get_client_id()
{
    if (client_id_string[0] == '\0')
    {
        int i, j = 0;
        BTS2S_ETHER_ADDR addr = bt_pan_get_mac_address(NULL);
        hash_run(HASH_ALGO_SHA256, (uint8_t *)&addr, sizeof(addr),
                 g_sha256_result, sizeof(g_sha256_result));
        for (i = 0; i < 16; i++, j += 2)
        {
            // 12345678-1234-1234-1234-123456789012
            if (i == 4 || i == 6 || i == 8 || i == 10)
            {
                client_id_string[j++] = '-';
            }
            hex_2_asc(g_sha256_result[i], &client_id_string[j]);
        }
        rt_kprintf(client_id_string);
    }
    return (&(client_id_string[0]));
}

static void svr_found_callback(const char *name, const ip_addr_t *ipaddr,
                               void *callback_arg)
{
    if (ipaddr != NULL)
    {
        rt_kprintf("DNS lookup succeeded, IP: %s\n", ipaddr_ntoa(ipaddr));
    }
}
int check_internet_access()
{
    int r = 0;
    const char *hostname = XIAOZHI_HOST;
    ip_addr_t addr = {0};

    {
        err_t err =
            dns_gethostbyname(hostname, &addr, svr_found_callback, NULL);
        if (err != ERR_OK && err != ERR_INPROGRESS)
        {
            rt_kprintf("Coud not find %s, please check PAN connection\n",
                       hostname);
        }
        else
            r = 1;
    }

    return r;
}
extern BOOL first_pan_connected;
char *get_xiaozhi()
{
    rt_kprintf("gett_xiaozhi\n");
    char *buffer = RT_NULL;
    int resp_status;
    struct webclient_session *session = RT_NULL;
    char *xiaozhi_url = RT_NULL;
    int content_length = -1, bytes_read = 0;
    int content_pos = 0;

    if (check_internet_access() == 0)
        return buffer;

    if (check_internet_access() == 1)
        first_pan_connected = TRUE;

    int size = strlen(ota_version) + sizeof(client_id_string) +
               sizeof(mac_address_string) * 2 + 16;
    char *ota_formatted = rt_malloc(size);
    if (!ota_formatted)
    {
        goto __exit;
    }
    rt_snprintf(ota_formatted, size, ota_version, get_mac_address(),
                get_client_id(), get_mac_address());

    /* 为 weather_url 分配空间 */
    xiaozhi_url = rt_calloc(1, GET_URL_LEN_MAX);
    if (xiaozhi_url == RT_NULL)
    {
        rt_kprintf("No memory for xiaozhi_url!\n");
        goto __exit;
    }
    /* 拼接 GET 网址 */
    rt_snprintf(xiaozhi_url, GET_URL_LEN_MAX, GET_URI, XIAOZHI_HOST);

    /* 创建会话并且设置响应的大小 */
    session = webclient_session_create(GET_HEADER_BUFSZ);
    if (session == RT_NULL)
    {
        rt_kprintf("No memory for get header!\n");
        goto __exit;
    }

    webclient_header_fields_add(session, "Device-Id: %s \r\n",
                                get_mac_address());
    webclient_header_fields_add(session, "Client-Id: %s \r\n", get_client_id());
    webclient_header_fields_add(session, "Content-Type: application/json \r\n");
    webclient_header_fields_add(session, "Content-length: %d \r\n",
                                strlen(ota_formatted));
    // webclient_header_fields_add(session, "X-language:");

    /* 发送 GET 请求使用默认的头部 */
    if ((resp_status = webclient_post(session, xiaozhi_url, ota_formatted,
                                      strlen(ota_formatted))) != 200)
    {
        rt_kprintf("webclient Post request failed, response(%d) error.\n",
                   resp_status);
        // goto __exit;
    }

    /* 分配用于存放接收数据的缓冲 */
    buffer = rt_calloc(1, GET_RESP_BUFSZ);
    if (buffer == RT_NULL)
    {
        rt_kprintf("No memory for data receive buffer!\n");
        goto __exit;
    }

    content_length = webclient_content_length_get(session);
    if (content_length > 0)
    {
        do
        {
            bytes_read =
                webclient_read(session, buffer + content_pos,
                               content_length - content_pos > GET_RESP_BUFSZ
                                   ? GET_RESP_BUFSZ
                                   : content_length - content_pos);
            if (bytes_read <= 0)
            {
                break;
            }
            content_pos += bytes_read;
        } while (content_pos < content_length);
    }
    else
    {
        rt_free(buffer);
        buffer = NULL;
    }
__exit:
    /* 释放网址空间 */
    if (xiaozhi_url != RT_NULL)
    {
        rt_free(xiaozhi_url);
        xiaozhi_url = RT_NULL;
    }

    /* 关闭会话 */
    if (session != RT_NULL)
    {
        LOCK_TCPIP_CORE();
        webclient_close(session);
        UNLOCK_TCPIP_CORE();
    }
    if (ota_formatted)
    {
        rt_free(ota_formatted);
    }
    return buffer;
}
char *my_json_string(cJSON *json, char *key)
{
    cJSON *item = cJSON_GetObjectItem(json, key);
    char *r = cJSON_Print(item);

    if (r && ((*r) == '\"'))
    {
        r++;
        r[strlen(r) - 1] = '\0';
    }
    return r;
}

uint8_t vad_is_enable(void)
{
    return g_en_vad;
}

void vad_set_enable(uint8_t enable)
{
    if(enable != g_en_vad)
    {
        g_en_vad = enable;
        xz_set_config_update(true);
        rt_kprintf("vad_set_enable VAD %d \r\n", g_en_vad);
    }
}

uint8_t aec_is_enable(void)
{
    return g_en_aec;
}

void aec_set_enable(uint8_t enable)
{
    if(enable != g_en_aec)
    {
        g_en_aec = enable;
        xz_set_config_update(true);
        rt_kprintf("vad_set_enable AEC %d \r\n", g_en_aec);
    }
}

uint8_t xz_get_config_update(void)
{
    return g_config_change;
}

void xz_set_config_update(uint8_t en)
{
    g_config_change = en;
}


enum ListeningMode xz_get_mode(void)
{
    return aec_is_enable() ? kListeningModeAlwaysOn : kListeningModeManualStop;
}


void xz_set_lcd_brightness(uint16_t level)
{
    rt_device_t bl_device = rt_device_find("lcd");
    RT_ASSERT(bl_device);

    int ret = rt_device_open(bl_device, RT_DEVICE_OFLAG_RDWR);
    if (ret == RT_EOK || ret == -RT_EBUSY)
    {
        rt_device_control(bl_device, RTGRAPHIC_CTRL_SET_BRIGHTNESS, &level);
    }
    if (bl_device != NULL && ret == RT_EOK)
        rt_device_close(bl_device);
}
extern const unsigned char xiaozhi_font[];
extern const int xiaozhi_font_size;



static lv_obj_t *sleep_label = NULL;
static int sleep_countdown = 3;
static lv_timer_t *sleep_timer = NULL;
static volatile int g_sleep_countdown_active = 0; // 休眠倒计时标志

extern rt_timer_t update_time_ui_timer;
extern rt_timer_t update_weather_ui_timer;
lv_obj_t *sleep_screen = NULL;
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

// 在 xiaozhi_public.c 中添加关机倒计时函数
static lv_obj_t *shutdown_label = NULL;
static int shutdown_countdown = 3;
static lv_timer_t *shutdown_timer = NULL;
static volatile int g_shutdown_countdown_active = 0; // 关机倒计时标志
lv_obj_t *shutdown_screen = NULL;
uint8_t i = 0;
static void shutdown_countdown_cb(lv_timer_t *timer)
{
    if(i == 1)
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
        i = 1;
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