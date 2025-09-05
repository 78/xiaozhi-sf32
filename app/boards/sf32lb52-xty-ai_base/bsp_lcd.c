#include "bsp_board.h"
#include "bf0_hal_pinmux.h"

#ifdef BSP_USING_LCD
#define LCD_RESET_PIN           (0)         // GPIO_A00
#define LCD_BL_PIN              (26)         // GPIO_A26 power control



/***************************LCD ***********************************/
extern void BSP_PIN_LCD(void);
void BSP_LCD_Reset(uint8_t high1_low0)
{
    BSP_GPIO_Set(LCD_RESET_PIN, high1_low0, 1);
}

void BSP_LCD_PowerDown(void)
{
    // TODO: LCD power down
    // BSP_GPIO_Set(LCD_BL_PIN, 0, 1);
    // BSP_GPIO_Set(32, 1, 1);
    BSP_GPIO_Set(LCD_RESET_PIN, 0, 1);

    BSP_GPIO_Set(LCD_BL_PIN, 0, 1);//power off

    // HAL_PIN_Set(PAD_PA00, GPIO_A0, PIN_PULLDOWN, 1);
    // HAL_PIN_Set(PAD_PA01, GPIO_A1, PIN_PULLDOWN, 1);
    // HAL_PIN_Set(PAD_PA02, GPIO_A2, PIN_PULLDOWN, 1);
    // HAL_PIN_Set(PAD_PA03, GPIO_A3, PIN_PULLDOWN, 1);
    // HAL_PIN_Set(PAD_PA04, GPIO_A4, PIN_PULLDOWN, 1);
    // HAL_PIN_Set(PAD_PA05, GPIO_A5, PIN_PULLDOWN, 1);
    // HAL_PIN_Set(PAD_PA06, GPIO_A6, PIN_PULLDOWN, 1);
    // HAL_PIN_Set(PAD_PA07, GPIO_A7, PIN_PULLDOWN, 1);
    // HAL_PIN_Set(PAD_PA08, GPIO_A8, PIN_PULLDOWN, 1);
}

void BSP_LCD_PowerUp(void)
{
    // TODO: LCD power up
    BSP_GPIO_Set(LCD_BL_PIN, 1, 1);
    BSP_GPIO_Set(LCD_RESET_PIN, 1, 1);
    
    HAL_Delay_us(500);      // lcd power on finish ,need 500us
    BSP_PIN_LCD();//set lcd pinmux

    HAL_PIN_Set(PAD_PA00, GPIO_A0, PIN_NOPULL, 1);
}

#endif
