#include "string.h"
#include "board.h"
#include "drv_io.h"
#include "drv_lcd.h"
#include "bf0_pin_const.h"
#include "log.h"
#include <bf0_hal_lcdc.h>
#include <bf0_hal_gpio.h>
#include <bf0_hal_pinmux.h>
#include "rtdef.h"





#ifdef ROW_OFFSET_PLUS
    #define ROW_OFFSET  (ROW_OFFSET_PLUS)
#else
    #define ROW_OFFSET  (0)
#endif

/**
  * @brief st7789_dbi chip IDs
  */
#define THE_LCD_ID                  0x9327

/**
  * @brief  st7789_dbi Size
  */
#define  THE_LCD_PIXEL_WIDTH    ((uint16_t)240)
#define  THE_LCD_PIXEL_HEIGHT   ((uint16_t)320)

/**
  * @brief  st7789_dbi Timing
  */
/* Timing configuration  (Typical configuration from st7789_dbi datasheet)
  HSYNC=10 (9+1)
  HBP=20 (29-10+1)
  ActiveW=240 (269-20-10+1)
  HFP=10 (279-240-20-10+1)

  VSYNC=2 (1+1)
  VBP=2 (3-2+1)
  ActiveH=320 (323-2-2+1)
  VFP=4 (327-320-2-2+1)
*/
#define  st7789_dbi_HSYNC            ((uint32_t)9)   /* Horizontal synchronization */
#define  st7789_dbi_HBP              ((uint32_t)29)    /* Horizontal back porch      */
#define  st7789_dbi_HFP              ((uint32_t)2)    /* Horizontal front porch     */
#define  st7789_dbi_VSYNC            ((uint32_t)1)   /* Vertical synchronization   */
#define  st7789_dbi_VBP              ((uint32_t)3)    /* Vertical back porch        */
#define  st7789_dbi_VFP              ((uint32_t)2)    /* Vertical front porch       */

/**
  * @brief  st7789_dbi Registers
  */
#define REG_POWER_MODE           0x0A
// #define REG_SLEEP_IN           0x10
// #define REG_SLEEP_OUT          0x11
// #define REG_DISPLAY_OFF        0x28
// #define REG_DISPLAY_ON         0x29
#define REG_WRITE_RAM          0x3C
#define REG_READ_RAM           0x3E
#define REG_CASET              0x2A //column address set
#define REG_RASET              0x2B //row address set
#define REG_COLOR_MODE         0x3A

#define REG_WBRIGHT            0x51 /* Write brightness*/







static bool te_enabled = true;

void lcd_sync_control(bool en)
{
    // te_enabled = en;
}

//#define DEBUG

#ifdef DEBUG
    #define DEBUG_PRINTF(...)   LOG_I(__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

static void LCD_WriteReg(LCDC_HandleTypeDef *hlcdc, uint16_t LCD_Reg, uint8_t *Parameters, uint32_t NbParameters);
static uint32_t LCD_ReadData(LCDC_HandleTypeDef *hlcdc, uint16_t RegValue, uint8_t ReadSize);






static LCDC_InitTypeDef lcdc_int_cfg =
{
    .lcd_itf = LCDC_INTF_DBI_8BIT_B,
    .freq = 12000000,
    .color_mode = LCDC_PIXEL_FORMAT_RGB565,

    .cfg = {
        .dbi = {
            .syn_mode = HAL_LCDC_SYNC_DISABLE,//HAL_LCDC_SYNC_VER, //HAL_LCDC_SYNC_DISABLE,
            .vsyn_polarity = 0,
            .vsyn_delay_us = 0,
            .hsyn_num = 0,
        },
    },

};








// #define CS_PA_x_PIN  31
#define CS_PA_x_PIN  3
// #define LCD_BL_EN_PIN (96 + 3)       // GPIO_B03
#define LCD_BL_EN_PIN 26      // GPIO_A26 VCC SW

#define LCDC1_8080_PEN      GPIO_A1
#define LCDC1_8080_T_CS     GPIO_A2
#define LCDC1_8080_T_DI     GPIO_A9
#define LCDC1_8080_T_CLK    GPIO_A20
#define LCDC1_8080_T_DO     GPIO_A31   

void st7789_dbi_CS_HOLD_LOW(void)
{
    GPIO_TypeDef *gpio = hwp_gpio1;
    GPIO_InitTypeDef GPIO_InitStruct;

    // set sensor pin to output mode
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pin = CS_PA_x_PIN;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(gpio, &GPIO_InitStruct);

    // set sensor pin to high == power on sensor board
    HAL_GPIO_WritePin(gpio, CS_PA_x_PIN, (GPIO_PinState)0);


    HAL_PIN_Set(PAD_PA00 + CS_PA_x_PIN, GPIO_A0 + CS_PA_x_PIN, PIN_PULLDOWN, 1);
}

void BSP_GPIO_Set_BL(int pin, int val)
{
    GPIO_TypeDef *gpio = NULL;
    if (pin < 96) gpio = hwp_gpio1;
    else
    {
        gpio = hwp_gpio2;
        pin -= 96;
    }
    GPIO_InitTypeDef GPIO_InitStruct;

    // set sensor pin to output mode
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(gpio, &GPIO_InitStruct);

    // set sensor pin to high == power on sensor board
    HAL_GPIO_WritePin(gpio, pin, (GPIO_PinState)val);
}


void st7789_dbi_CS_RELEASE(void)
{
    HAL_PIN_Set(PAD_PA00 + CS_PA_x_PIN, LCDC1_8080_CS, PIN_NOPULL, 1);
}
/**
  * @brief  spi read/write mode
  * @param  enable: false - write spi mode |  true - read spi mode
  * @retval None
  */
static void LCD_ReadMode(LCDC_HandleTypeDef *hlcdc, bool enable)
{
    if (HAL_LCDC_IS_DBI_IF(lcdc_int_cfg.lcd_itf))
    {
        if (enable)
        {
            HAL_LCDC_SetFreq(hlcdc, lcdc_int_cfg.freq / 3); //read mode min cycle 300ns
        }
        else
        {
            HAL_LCDC_SetFreq(hlcdc, lcdc_int_cfg.freq); //Restore normal frequency
        }

    }
}

/**
  * @brief  Power on the LCD.
  * @param  None
  * @retval None
  */
static void LCD_Init(LCDC_HandleTypeDef *hlcdc)
{
    uint8_t parameter[15];

    /* Initialize st7789_dbi low level bus layer ----------------------------------*/
    memcpy(&hlcdc->Init, &lcdc_int_cfg, sizeof(LCDC_InitTypeDef));
    if (!te_enabled)
    {
        hlcdc->Init.cfg.spi.syn_mode = HAL_LCDC_SYNC_DISABLE;
    }
    HAL_LCDC_Init(hlcdc);

    BSP_LCD_Reset(1);
    LCD_DRIVER_DELAY_MS(1);
    BSP_LCD_Reset(0); // Reset LCD
    HAL_Delay_us(20);
    BSP_LCD_Reset(1);
    LCD_DRIVER_DELAY_MS(50);

    // remark
    // parameter[0] = 0x20;
    // LCD_WriteReg(hlcdc, 0xE9, parameter, 1);

    /* Sleep Out Command */
    LCD_WriteReg(hlcdc, 0x11, (uint8_t *)NULL, 0);
    /* Wait for 120ms */
    LCD_DRIVER_DELAY_MS(120);

    parameter[0] = 0x40;//A0
    LCD_WriteReg(hlcdc, 0x36, parameter, 1);

    parameter[0] = 0x05;
    LCD_WriteReg(hlcdc, 0x3A, parameter, 1);

    parameter[0] = 0x0C;
    parameter[1] = 0x0C;
    parameter[2] = 0x00;
    parameter[3] = 0x33;
    parameter[4] = 0x33;
    LCD_WriteReg(hlcdc, 0xB2, parameter, 5);

    parameter[0] = 0x56;
    LCD_WriteReg(hlcdc, 0xB7, parameter, 1);

    parameter[0] = 0x20;
    LCD_WriteReg(hlcdc, 0xBB, parameter, 1);

    parameter[0] = 0x20;
    LCD_WriteReg(hlcdc, 0xC0, parameter, 1);

    parameter[0] = 0x01;
    LCD_WriteReg(hlcdc, 0xC2, parameter, 1);

    parameter[0] = 0x0F;
    LCD_WriteReg(hlcdc, 0xC3, parameter, 1);

    parameter[0] = 0x20;
    LCD_WriteReg(hlcdc, 0xC4, parameter, 1);

    parameter[0] = 0x0F;
    LCD_WriteReg(hlcdc, 0xC6, parameter, 1); // 60Hz

    parameter[0] = 0xA4;
    parameter[1] = 0xA1;
    LCD_WriteReg(hlcdc, 0xD0, parameter, 2);

    parameter[0] = 0xA1;
    LCD_WriteReg(hlcdc, 0xD6, parameter, 1);

    parameter[0] = 0xF0;
    parameter[1] = 0x00;
    parameter[2] = 0x06;
    parameter[3] = 0x06;
    parameter[4] = 0x07;
    parameter[5] = 0x05;
    parameter[6] = 0x30;
    parameter[7] = 0x44;
    parameter[8] = 0x48;
    parameter[9] = 0x38;
    parameter[10] = 0x11;
    parameter[11] = 0x10;
    parameter[12] = 0x2E;
    parameter[13] = 0x34;
    LCD_WriteReg(hlcdc, 0xE0, parameter, 14);

    parameter[0] = 0xF0;
    parameter[1] = 0x0A;
    parameter[2] = 0x0E;
    parameter[3] = 0x0D;
    parameter[4] = 0x0B;
    parameter[5] = 0x27;
    parameter[6] = 0x2F;
    parameter[7] = 0x44;
    parameter[8] = 0x47;
    parameter[9] = 0x35;
    parameter[10] = 0x12;
    parameter[11] = 0x12;
    parameter[12] = 0x2C;
    parameter[13] = 0x32;
    LCD_WriteReg(hlcdc, 0xE1, parameter, 14);

    parameter[0] = 0x00;
    LCD_WriteReg(hlcdc, 0x35, parameter, 1);
#if 0
    parameter[0] = 0x00;
    parameter[1] = 0x00;
    parameter[2] = 0x00;
    parameter[3] = 0xEF;
    LCD_WriteReg(hlcdc, 0x2B, parameter, 4);

    parameter[0] = 0x00;
    parameter[1] = 0x00;
    parameter[2] = 0x01;
    parameter[3] = 0x3F;
    LCD_WriteReg(hlcdc, 0x2A, parameter, 4);
#endif
    LCD_WriteReg(hlcdc, 0x20, (uint8_t *)NULL, 0);
    /* clear gram */
    HAL_LCDC_Next_Frame_TE(hlcdc, 0);
    HAL_LCDC_SetROIArea(hlcdc, 0, 0, THE_LCD_PIXEL_WIDTH - 1, THE_LCD_PIXEL_HEIGHT - 1);
    HAL_LCDC_LayerSetFormat(hlcdc, HAL_LCDC_LAYER_DEFAULT, LCDC_PIXEL_FORMAT_RGB565);
    HAL_LCDC_LayerDisable(hlcdc, HAL_LCDC_LAYER_DEFAULT);
    HAL_LCDC_SetBgColor(hlcdc, 0, 0, 0);
    HAL_LCDC_SendLayerData2Reg(hlcdc, REG_WRITE_RAM, 1);
    HAL_LCDC_LayerEnable(hlcdc, HAL_LCDC_LAYER_DEFAULT);
    LCD_WriteReg(hlcdc, 0x29, (uint8_t *)NULL, 0);
}

/**
  * @brief  Disables the Display.
  * @param  None
  * @retval LCD Register Value.
  */
static uint32_t LCD_ReadID(LCDC_HandleTypeDef *hlcdc)
{
    return THE_LCD_ID;
}

/**
  * @brief  Enables the Display.
  * @param  None
  * @retval None
  */
static void LCD_DisplayOn(LCDC_HandleTypeDef *hlcdc)
{
    /* Display On */
    // LCD_WriteReg(hlcdc, REG_DISPLAY_ON, (uint8_t *)NULL, 0);
    BSP_GPIO_Set_BL(LCD_BL_EN_PIN, 1);
}

/**
  * @brief  Disables the Display.
  * @param  None
  * @retval None
  */
static void LCD_DisplayOff(LCDC_HandleTypeDef *hlcdc)
{
    /* Display Off */
    BSP_GPIO_Set_BL(LCD_BL_EN_PIN, 0);
    // LCD_WriteReg(hlcdc, REG_DISPLAY_OFF, (uint8_t *)NULL, 0);
    // LCD_WriteReg(hlcdc, REG_SLEEP_IN, (uint8_t *)NULL, 0);
    LCD_DRIVER_DELAY_MS(50);
}

static void LCD_SetRegion(LCDC_HandleTypeDef *hlcdc, uint16_t Xpos0, uint16_t Ypos0, uint16_t Xpos1, uint16_t Ypos1)
{
    uint8_t parameter[4];
    parameter[0] = Xpos0>>8;
    parameter[1] = Xpos0 & 0x0f;
    parameter[2] = Xpos1>>8;
    parameter[3] = Xpos1 & 0x0f;
    LCD_WriteReg(hlcdc, REG_CASET, parameter, 4);
    parameter[0] = Ypos0>>8;
    parameter[1] = Ypos0 & 0x0f;
    parameter[2] = Ypos1>>8;
    parameter[3] = Ypos1 & 0x0f;
    LCD_WriteReg(hlcdc, REG_RASET, parameter, 4);
    LCD_WriteReg(hlcdc, 0x2c, (uint8_t *)NULL, 0);
}

/**
  * @brief  Writes pixel.
  * @param  Xpos: specifies the X position.
  * @param  Ypos: specifies the Y position.
  * @param  RGBCode: the RGB pixel color
  * @retval None
  */
static void LCD_WritePixel(LCDC_HandleTypeDef *hlcdc, uint16_t Xpos, uint16_t Ypos, const uint8_t *RGBCode)
{
    uint8_t data = 0;

    /* Set Cursor */
    LCD_SetRegion(hlcdc, Xpos, Ypos, Xpos, Ypos);
    LCD_WriteReg(hlcdc, REG_WRITE_RAM, (uint8_t *)RGBCode, 2);
}

void HAL_LCDC_SendLayerDataCpltCbk(LCDC_HandleTypeDef *lcdc)
{
    st7789_dbi_CS_RELEASE();
}
static void LCD_WriteMultiplePixels(LCDC_HandleTypeDef *hlcdc, const uint8_t *RGBCode, uint16_t Xpos0, uint16_t Ypos0, uint16_t Xpos1, uint16_t Ypos1)
{

    uint32_t v =  LCD_ReadData(hlcdc, REG_POWER_MODE, 2);
    //LOG_I("0x0A=%x", v);
    v =  LCD_ReadData(hlcdc, 0x04, 2);
    //LOG_I("0x04=%x", v);


    st7789_dbi_CS_HOLD_LOW();
    HAL_LCDC_LayerSetData(hlcdc, HAL_LCDC_LAYER_DEFAULT, (uint8_t *)RGBCode, Xpos0, Ypos0, Xpos1, Ypos1);
    HAL_LCDC_SendLayerData2Reg_IT(hlcdc, REG_WRITE_RAM, 1);

}



/**
  * @brief  Writes  to the selected LCD register.
  * @param  LCD_Reg: address of the selected register.
  * @retval None
  */
static void LCD_WriteReg(LCDC_HandleTypeDef *hlcdc, uint16_t LCD_Reg, uint8_t *Parameters, uint32_t NbParameters)
{
    st7789_dbi_CS_HOLD_LOW();
    HAL_LCDC_WriteU8Reg(hlcdc, LCD_Reg, Parameters, NbParameters);
    st7789_dbi_CS_RELEASE();
}


/**
  * @brief  Reads the selected LCD Register.
  * @param  RegValue: Address of the register to read
  * @param  ReadSize: Number of bytes to read
  * @retval LCD Register Value.
  */
static uint32_t LCD_ReadData(LCDC_HandleTypeDef *hlcdc, uint16_t RegValue, uint8_t ReadSize)
{
    uint32_t rd_data = 0;

    LCD_ReadMode(hlcdc, true);

    st7789_dbi_CS_HOLD_LOW();
    HAL_LCDC_ReadU8Reg(hlcdc, RegValue, (uint8_t *)&rd_data, ReadSize);
    st7789_dbi_CS_RELEASE();
    LCD_ReadMode(hlcdc, false);

    return rd_data;
}



static uint32_t LCD_ReadPixel(LCDC_HandleTypeDef *hlcdc, uint16_t Xpos, uint16_t Ypos)
{
    uint8_t   parameter[2];
    uint32_t c;
    uint32_t ret_v;

    parameter[0] = 0x66;
    LCD_WriteReg(hlcdc, REG_COLOR_MODE, parameter, 1);

    LCD_SetRegion(hlcdc, Xpos, Ypos, Xpos, Ypos);

    /*
        read ram need 7 dummy cycle, and it's result is 24bit color which format is:

        6bit red + 2bit dummy + 6bit green + 2bit dummy + 6bit blue + 2bit dummy

    */
    c =  LCD_ReadData(hlcdc, REG_READ_RAM, 4);
    c <<= 1;
    //c >>= lcdc_int_cfg.dummy_clock; //revert fixed dummy cycle

    switch (lcdc_int_cfg.color_mode)
    {
    case LCDC_PIXEL_FORMAT_RGB565:
        parameter[0] = 0x55;
        ret_v = (uint32_t)(((c >> 8) & 0xF800) | ((c >> 5) & 0x7E0) | ((c >> 3) & 0X1F));
        break;

    case LCDC_PIXEL_FORMAT_RGB666:
        /*
           pretend that st7789_dbi can support RGB666,

           treated as RGB666 actually(6bit R + 2bit dummy + 6bit G + 2bit dummy + 6bit B + 2bit dummy )

           lcdc NOT support RGB666
        */
        parameter[0] = 0x66;
        ret_v = (uint32_t)(((c >> 6) & 0x3F000) | ((c >> 4) & 0xFC0) | ((c >> 2) & 0X3F));
        break;

    default:
        RT_ASSERT(0);
        break;
    }

    //rt_kprintf("st7789_dbi_ReadPixel %x -> %x\n",c, ret_v);


    LCD_WriteReg(hlcdc, REG_COLOR_MODE, parameter, 1);

    return ret_v;
}


static void LCD_SetColorMode(LCDC_HandleTypeDef *hlcdc, uint16_t color_mode)
{
    uint8_t   parameter[2];

    /*

    Control interface color format
    ‘011’ = 12bit/pixel ‘101’ = 16bit/pixel ‘110’ = 18bit/pixel ‘111’ = 16M truncated

    */
    switch (color_mode)
    {
    case RTGRAPHIC_PIXEL_FORMAT_RGB565:
        /* Color mode 16bits/pixel */
        parameter[0] = 0x55;
        lcdc_int_cfg.color_mode = LCDC_PIXEL_FORMAT_RGB565;
        break;
    default:
        return; //unsupport
        break;

    }

    LCD_WriteReg(hlcdc, REG_COLOR_MODE, parameter, 1);
    HAL_LCDC_SetOutFormat(hlcdc, lcdc_int_cfg.color_mode);
}

static void LCD_SetBrightness(LCDC_HandleTypeDef *hlcdc, uint8_t br)
{
    uint8_t bright = (uint8_t)((uint16_t)UINT8_MAX * br / 100);
    LCD_WriteReg(hlcdc, REG_WBRIGHT, &br, 1);
}

/*****************************************************************************/





static const LCD_DrvOpsDef st7789_dbi_drv =
{
    LCD_Init,
    LCD_ReadID,
    LCD_DisplayOn,
    LCD_DisplayOff,

    LCD_SetRegion,
    LCD_WritePixel,
    LCD_WriteMultiplePixels,

    LCD_ReadPixel,

    LCD_SetColorMode,
    LCD_SetBrightness,
    NULL,
    NULL

};

LCD_DRIVER_EXPORT2(st7789_dbi, THE_LCD_ID, &lcdc_int_cfg,
                   &st7789_dbi_drv, 1);






/************************ (C) COPYRIGHT Sifli Technology *******END OF FILE****/
