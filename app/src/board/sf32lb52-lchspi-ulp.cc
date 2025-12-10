#include "board_hardware.h"
#include "../mcp/rgbled_mcp.h"
#include "rtthread.h"
#include "drv_io.h"
#include "stdio.h"
#include "string.h"
#include "drivers/rt_drv_pwm.h"
#include "bf0_hal.h"
#include "bf0_pm.h"
#include "xiaozhi_client_public.h"
#include "bts2_global.h"
#include "bts2_app_pan.h"
#include "bt_connection_manager.h"
#include "bt_env.h"
#include <webclient.h>
#include <cJSON.h>
#include <lwip/dns.h>
#include <lwip/sys.h>
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
extern "C" 
{
void open_led()
{
    int index = 0;
    while (RGBLEDTool::is_color_cycling_) 
    {
        uint32_t color = rgb_color_arry[index].color;
        GetRGBLEDController().SetColor(color);
        index = (index + 1) % (sizeof(rgb_color_arry)/sizeof(rgb_color_arry[0]));
        rt_thread_mdelay(500);

    }
}
void close_led()
{
       if (RGBLEDTool::is_color_cycling_)
        {
            RGBLEDTool::is_color_cycling_ = false;
            rt_thread_mdelay(100);
            GetRGBLEDController().SetColor(0x000000);  // 关灯
        }
}
void set_mode_led()
{

}

void set_pinmux()
{
    unsigned int *addr2 = (unsigned int *)0x50003088; // 21
    *addr2 = 0x00000200;
    unsigned int *addr = (unsigned int *)0x500030B0; // 31
    *addr = 0x00000200;

    // senser
    HAL_PIN_Set(PAD_PA30, GPIO_A30, PIN_PULLDOWN, 1);
    BSP_GPIO_Set(30, 0, 1);
    HAL_PIN_Set(PAD_PA39, GPIO_A39, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_PA40, GPIO_A40, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_PA34, GPIO_A34, PIN_NOPULL, 1);
}

HAL_RAM_RET_CODE_SECT(PowerDownCustom, void PowerDownCustom(void))
{
    rt_kprintf("PowerDownCustom\n");

    HAL_PMU_SelectWakeupPin(0, 19); // PA43
    HAL_PMU_EnablePinWakeup(0, 0);

    HAL_PIN_Set(PAD_PA24, GPIO_A24, PIN_PULLDOWN, 1);
    for (uint32_t i = PAD_PA28; i <= PAD_PA44; i++)
    {
        HAL_PIN_Set(i, (pin_function)(i - PAD_PA28 + GPIO_A28), PIN_PULLDOWN, 1);
    }
    hwp_pmuc->PERI_LDO &=  ~(PMUC_PERI_LDO_EN_LDO18 | PMUC_PERI_LDO_EN_VDD33_LDO2 | PMUC_PERI_LDO_EN_VDD33_LDO3);
    hwp_pmuc->WKUP_CNT = 0x000F000F;

    

    rt_hw_interrupt_disable();
    rt_kprintf("PowerDownCustom2\n");
    HAL_PMU_ConfigPeriLdo(PMU_PERI_LDO2_3V3, false, false);
    HAL_PMU_ConfigPeriLdo(PMU_PERI_LDO_1V8, false, false);
    HAL_PMU_EnterHibernate();
    rt_kprintf("PowerDownCustom3\n");
}

void check_poweron_reason(void)
{
    switch (SystemPowerOnModeGet())
    {
    case PM_REBOOT_BOOT:
    case PM_COLD_BOOT:
    {
        // power on as normal
        break;
    }
    case PM_HIBERNATE_BOOT:
    case PM_SHUTDOWN_BOOT:
    {
        if (PMUC_WSR_RTC & pm_get_wakeup_src())
        {
            // RTC唤醒
            NVIC_EnableIRQ(RTC_IRQn);
            // power on as normal
        }
#ifdef BSP_USING_CHARGER
        else if ((PMUC_WSR_PIN0 << (pm_get_charger_pin_wakeup())) & pm_get_wakeup_src())
        {
        }
#endif
        else if (PMUC_WSR_PIN_ALL & pm_get_wakeup_src())
        {
            rt_thread_mdelay(1000); // 延时1秒
            int val = rt_pin_read(BSP_KEY2_PIN);
            rt_kprintf("Power key level after 1s: %d\n", val);
            if (val != KEY2_ACTIVE_LEVEL)
            {
                // 按键已松开，认为是误触发，直接关机
                rt_kprintf("Not long press, shutdown now.\n");
                PowerDownCustom();
                while (1) {};
            }
            else
            {
                // 长按，正常开机
                rt_kprintf("Long press detected, power on as normal.\n");
            }
        }
        else if (0 == pm_get_wakeup_src())
        {
            RT_ASSERT(0);
        }
        break;
    }
    default:
    {
        RT_ASSERT(0);
    }
    }
}

}