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
#include <rtthread.h>
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

char mac_address_string[20];
char client_id_string[40];
ALIGN(4) uint8_t g_sha256_result[32] = {0};
HAL_RAM_RET_CODE_SECT(PowerDownCustom, void PowerDownCustom(void))
{
    rt_kprintf("PowerDownCustom\n");
    HAL_PMU_SelectWakeupPin(0, 19); // PA43
    HAL_PMU_EnablePinWakeup(0, 0);
    HAL_PIN_Set(PAD_PA24, GPIO_A24, PIN_PULLDOWN, 1);
    for (uint32_t i = PAD_PA28; i <= PAD_PA44; i++)
    {
        HAL_PIN_Set(i, i - PAD_PA28 + GPIO_A28, PIN_PULLDOWN, 1);
    }
    hwp_pmuc->PERI_LDO &=  ~(PMUC_PERI_LDO_EN_LDO18 | PMUC_PERI_LDO_EN_VDD33_LDO2 | PMUC_PERI_LDO_EN_VDD33_LDO3);
    hwp_pmuc->WKUP_CNT = 0x000F000F;

    rt_hw_interrupt_disable();
    rt_kprintf("PowerDownCustom2\n");
    HAL_PMU_EnterHibernate();
    rt_kprintf("PowerDownCustom3\n");
}
char *get_mac_address()
{
    if (mac_address_string[0] == '\0')
    {
        BTS2S_ETHER_ADDR addr = bt_pan_get_mac_address(NULL);
        uint8_t *p = (uint8_t *)&(addr);

        rt_snprintf((char *)mac_address_string, 20,
                    "%02x:%02x:%02x:%02x:%02x:%02x", *p, *(p + 1), *(p + 2),
                    *(p + 3), *(p + 4), *(p + 5));
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

void show_sleep_countdown_and_sleep(void)
{
    static lv_obj_t *sleep_screen = NULL;
    if (!sleep_screen) {
        sleep_screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(sleep_screen, lv_color_hex(0x000000), 0);
    }
    lv_obj_clean(sleep_screen);
    lv_screen_load(sleep_screen);

    // 顶部“即将休眠”label
    int tip_font_size = 36;
    lv_font_t *tip_font = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, tip_font_size);
    static lv_style_t style_tip_sleep;
    lv_style_init(&style_tip_sleep);
    lv_style_set_text_font(&style_tip_sleep, tip_font);
    lv_style_set_text_color(&style_tip_sleep, lv_color_hex(0xFFFFFF));
    lv_obj_t *tip_label = lv_label_create(sleep_screen);
    lv_label_set_text(tip_label, "即将休眠");
    lv_obj_add_style(tip_label, &style_tip_sleep, 0);
    lv_obj_align(tip_label, LV_ALIGN_TOP_MID, 0, 20);

    // 中间倒计时数字
    int font_size = 120;
    lv_font_t *big_font = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, font_size);
    static lv_style_t style_big_sleep;
    lv_style_init(&style_big_sleep);
    lv_style_set_text_font(&style_big_sleep, big_font);
    lv_style_set_text_color(&style_big_sleep, lv_color_hex(0xFFFFFF));
    lv_obj_t *label = lv_label_create(sleep_screen);
    lv_obj_add_style(label, &style_big_sleep, 0);
    lv_obj_center(label);

    for (int i = 3; i >= 1; --i)
    {
        char num[2] = {0};
        snprintf(num, sizeof(num), "%d", i);
        lv_label_set_text(label, num);
        lv_obj_center(label);
        lv_timer_handler();
        rt_thread_mdelay(1000);
    }
    rt_kprintf("sleep countdown ok\n");
}