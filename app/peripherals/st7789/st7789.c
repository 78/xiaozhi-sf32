/*
 * SPDX-FileCopyrightText: 2019-2022 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include "string.h"
#include "board.h"
#include "drv_io.h"
#include "drv_lcd.h"

#include "log.h"

#ifdef ROW_OFFSET_PLUS
    #define ROW_OFFSET (ROW_OFFSET_PLUS)
#else
    #define ROW_OFFSET (0)
#endif

/**
 * @brief ST7789 chip IDs
 */
#define THE_LCD_ID 0x85

/**
 * @brief  ST7789 Size - 2.0 inch 320x240
 */
#define  THE_LCD_PIXEL_WIDTH    ((uint16_t)320)
#define  THE_LCD_PIXEL_HEIGHT   ((uint16_t)240)

/**
 * @brief  ST7789 Registers
 */
#define REG_LCD_ID 0x04
#define REG_SLEEP_IN 0x10
#define REG_SLEEP_OUT 0x11
#define REG_PARTIAL_DISPLAY 0x12
#define REG_DISPLAY_INVERSION 0x21
#define REG_DISPLAY_OFF 0x28
#define REG_DISPLAY_ON 0x29
#define REG_WRITE_RAM 0x2C
#define REG_READ_RAM 0x2E
#define REG_CASET 0x2A
#define REG_RASET 0x2B

#define REG_TEARING_EFFECT 0x35
#define REG_NORMAL_DISPLAY 0x36
#define REG_IDLE_MODE_OFF 0x38
#define REG_IDLE_MODE_ON 0x39
#define REG_COLOR_MODE 0x3A
#define REG_WBRIGHT 0x51 /* Write brightness*/

#define REG_PORCH_CTRL 0xB2
#define ST7789_FRAME_CTRL 0xB3
#define REG_GATE_CTRL 0xB7
#define REG_VCOM_SET 0xBB
#define REG_LCM_CTRL 0xC0
#define REG_VDV_VRH_EN 0xC2
#define REG_VDV_SET 0xC4

#define REG_FR_CTRL 0xC6
#define REG_POWER_CTRL 0xD0
#define REG_PV_GAMMA_CTRL 0xE0
#define REG_NV_GAMMA_CTRL 0xE1

// #define DEBUG

#ifdef DEBUG
    #define DEBUG_PRINTF(...) LOG_I(__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif
static void LCD_WriteReg(LCDC_HandleTypeDef *hlcdc, uint16_t LCD_Reg,
                         uint8_t *Parameters, uint32_t NbParameters);
static uint32_t LCD_ReadData(LCDC_HandleTypeDef *hlcdc, uint16_t RegValue,
                             uint8_t ReadSize);

static LCDC_InitTypeDef lcdc_int_cfg = {
    .lcd_itf = LCDC_INTF_SPI_DCX_1DATA,
    .freq = 24000000,
    .color_mode = LCDC_PIXEL_FORMAT_RGB565,
    .cfg =
        {
            .spi =
                {

                    .dummy_clock = 1,
                    .syn_mode = HAL_LCDC_SYNC_DISABLE,
                    .vsyn_polarity = 0,
                    .vsyn_delay_us = 1000,
                    .hsyn_num = 0,
                },
        },

};

/**
 * @brief  spi read/write mode
 * @param  enable: false - write spi mode |  true - read spi mode
 * @retval None
 */
static void LCD_ReadMode(LCDC_HandleTypeDef *hlcdc, bool enable)
{
    if (HAL_LCDC_IS_SPI_IF(lcdc_int_cfg.lcd_itf))
    {
        if (enable)
        {
            HAL_LCDC_SetFreq(hlcdc, 4000000); // read mode min cycle 300ns
        }
        else
        {
            HAL_LCDC_SetFreq(hlcdc,
                             lcdc_int_cfg.freq); // Restore normal frequency
        }
    }
}

#define LCD_CMD_MV_BIT                                                         \
    (1 << 5) // Row/Column order, 0: normal mode, 1: reverse mode
#define LCD_CMD_MX_BIT                                                         \
    (1 << 6) // Column address order, 0: left to right, 1: right to left
#define LCD_CMD_MY_BIT                                                         \
    (1 << 7) // Row address order, 0: top to bottom, 1: bottom to top
/**
 * @brief  Power on the LCD.
 * @param  None
 * @retval None
 */
static void LCD_Init(LCDC_HandleTypeDef *hlcdc)
{
    uint8_t parameter[14];

    rt_kprintf("DBG:%s %d", __FUNCTION__, __LINE__);
    /* Initialize ST7789 low level bus layer
     * ----------------------------------*/
    memcpy(&hlcdc->Init, &lcdc_int_cfg, sizeof(LCDC_InitTypeDef));
    HAL_LCDC_Init(hlcdc);

    BSP_LCD_Reset(0); // Reset LCD
    HAL_Delay_us(10);
    BSP_LCD_Reset(1);

    /* Sleep In Command */
    LCD_WriteReg(hlcdc, REG_SLEEP_IN, (uint8_t *)NULL, 0);
    /* Wait for 10ms */
    LCD_DRIVER_DELAY_MS(10);

    /* SW Reset Command */
    LCD_WriteReg(hlcdc, 0x01, (uint8_t *)NULL, 0);
    /* Wait for 200ms */
    LCD_DRIVER_DELAY_MS(200);

    /* Sleep Out Command */
    LCD_WriteReg(hlcdc, REG_SLEEP_OUT, (uint8_t *)NULL, 0);
    /* Wait for 120ms */
    LCD_DRIVER_DELAY_MS(120);

    /* Normal display for Driver Down side */
    parameter[0] = 0x00;
    parameter[0] = (LCD_CMD_MV_BIT | LCD_CMD_MX_BIT);
    LCD_WriteReg(hlcdc, REG_NORMAL_DISPLAY, parameter, 1);

    /* Color mode 16bits/pixel */
    parameter[0] = 0x55;
    LCD_WriteReg(hlcdc, REG_COLOR_MODE, parameter, 1);

    /* Display inversion On */
    LCD_WriteReg(hlcdc, REG_DISPLAY_INVERSION, (uint8_t *)NULL, 0);

    /* Set Column address CASET */
    parameter[0] = 0x00;
    parameter[1] = 0x00;
    parameter[2] = (THE_LCD_PIXEL_WIDTH - 1) >> 8;
    parameter[3] = (THE_LCD_PIXEL_WIDTH - 1) & 0xFF;
    LCD_WriteReg(hlcdc, REG_CASET, parameter, 4);
    
    /* Set Row address RASET */
    parameter[0] = 0x00;
    parameter[1] = 0x00;
    parameter[2] = (THE_LCD_PIXEL_HEIGHT - 1) >> 8;
    parameter[3] = (THE_LCD_PIXEL_HEIGHT - 1) & 0xFF;
    LCD_WriteReg(hlcdc, REG_RASET, parameter, 4);

#if 0

    /*--------------- ST7789 Frame rate setting -------------------------------*/
    /* PORCH control setting */
    parameter[0] = 0x0C;
    parameter[1] = 0x0C;
    parameter[2] = 0x00;
    parameter[3] = 0x33;
    parameter[4] = 0x33;
    LCD_WriteReg(hlcdc, REG_PORCH_CTRL, parameter, 5);

    /* GATE control setting */
    parameter[0] = 0x35;
    LCD_WriteReg(hlcdc, REG_GATE_CTRL, parameter, 1);

    /*--------------- ST7789 Power setting ------------------------------------*/
    /* VCOM setting */
    parameter[0] = 0x1F;
    LCD_WriteReg(hlcdc, REG_VCOM_SET, parameter, 1);

    /* LCM Control setting */
    parameter[0] = 0x2C;
    LCD_WriteReg(hlcdc, REG_LCM_CTRL, parameter, 1);

    /* VDV and VRH Command Enable */
    parameter[0] = 0x01;
    parameter[1] = 0xC3;
    LCD_WriteReg(hlcdc, REG_VDV_VRH_EN, parameter, 2);

    /* VDV Set */
    parameter[0] = 0x20;
    LCD_WriteReg(hlcdc, REG_VDV_SET, parameter, 1);

    /* Frame Rate Control in normal mode */
    parameter[0] = 0x0F;
    LCD_WriteReg(hlcdc, REG_FR_CTRL, parameter, 1);

    /* Power Control */
    parameter[0] = 0xA4;
    parameter[1] = 0xA1;
    LCD_WriteReg(hlcdc, REG_POWER_CTRL, parameter, 1);

#endif

    /*--------------- ST7789 Gamma setting
     * ------------------------------------*/
    /* Positive Voltage Gamma Control */
    parameter[0] = 0xD0;
    parameter[1] = 0x08;
    parameter[2] = 0x11;
    parameter[3] = 0x08;
    parameter[4] = 0x0C;
    parameter[5] = 0x15;
    parameter[6] = 0x39;
    parameter[7] = 0x33;
    parameter[8] = 0x50;
    parameter[9] = 0x36;
    parameter[10] = 0x13;
    parameter[11] = 0x14;
    parameter[12] = 0x29;
    parameter[13] = 0x2D;
    LCD_WriteReg(hlcdc, REG_PV_GAMMA_CTRL, parameter, 14);

    /* Negative Voltage Gamma Control */
    parameter[0] = 0xD0;
    parameter[1] = 0x08;
    parameter[2] = 0x10;
    parameter[3] = 0x08;
    parameter[4] = 0x06;
    parameter[5] = 0x06;
    parameter[6] = 0x39;
    parameter[7] = 0x44;
    parameter[8] = 0x51;
    parameter[9] = 0x0B;
    parameter[10] = 0x16;
    parameter[11] = 0x14;
    parameter[12] = 0x2F;
    parameter[13] = 0x31;
    LCD_WriteReg(hlcdc, REG_NV_GAMMA_CTRL, parameter, 14);

#if 0
    /* Test pattern - clear screen to black */
    LCD_WriteReg(hlcdc, REG_WRITE_RAM, (uint8_t *)NULL, 0);

    for (uint32_t i = 0; i < THE_LCD_PIXEL_HEIGHT; i++)
    {
        for (uint32_t j = 0; j < THE_LCD_PIXEL_WIDTH; j++)
        {
            LCD_IO_WriteData16(0x0000);
        }
    }
#endif

    /* Set frame control*/
    parameter[0] = 0x01;
    parameter[1] = 0x0F;
    parameter[2] = 0x0F;
    LCD_WriteReg(hlcdc, ST7789_FRAME_CTRL, parameter, 3);

    /* Display ON command */
    LCD_WriteReg(hlcdc, REG_DISPLAY_ON, (uint8_t *)NULL, 0);

    /* Tearing Effect Line On: Option (00h:VSYNC Only, 01h:VSYNC & HSYNC ) */
    parameter[0] = 0x00;
    LCD_WriteReg(hlcdc, REG_TEARING_EFFECT, parameter, 1);
}

/**
 * @brief  Disables the Display.
 * @param  None
 * @retval LCD Register Value.
 */
static uint32_t LCD_ReadID(LCDC_HandleTypeDef *hlcdc)
{
    uint32_t data = 0;

    data = LCD_ReadData(hlcdc, REG_LCD_ID, 3);
    rt_kprintf("DBG:%s %d ID:%0x", __FUNCTION__, __LINE__, data);
    data = 0x8181b3; // 不同厂商的ID可能不一样，因此这里使用固定的ID
    return data;
}

/**
 * @brief  Enables the Display.
 * @param  None
 * @retval None
 */
static void LCD_DisplayOn(LCDC_HandleTypeDef *hlcdc)
{
    rt_kprintf("DBG:%s %d", __FUNCTION__, __LINE__);
    /* Display On */
    LCD_WriteReg(hlcdc, REG_DISPLAY_ON, (uint8_t *)NULL, 0);
}

/**
 * @brief  Disables the Display.
 * @param  None
 * @retval None
 */
static void LCD_DisplayOff(LCDC_HandleTypeDef *hlcdc)
{
    rt_kprintf("DBG:%s %d", __FUNCTION__, __LINE__);
    /* Display Off */
    LCD_WriteReg(hlcdc, REG_DISPLAY_OFF, (uint8_t *)NULL, 0);
}

/**
 * @brief Set LCD & LCDC clip area
 * @param hlcdc LCD controller handle
 * @param Xpos0 - Clip area left coordinate, base on LCD top-left, same as
 * below.
 * @param Ypos0 - Clip area top coordinate
 * @param Xpos1 - Clip area right coordinate
 * @param Ypos1 - Clip area bottom coordinate
 */
static void LCD_SetRegion(LCDC_HandleTypeDef *hlcdc, uint16_t Xpos0,
                          uint16_t Ypos0, uint16_t Xpos1, uint16_t Ypos1)
{
    uint8_t parameter[4];

    // Set LCDC clip area
    HAL_LCDC_SetROIArea(hlcdc, Xpos0, Ypos0, Xpos1, Ypos1);

    // Set LCD clip area
    Xpos0 += ROW_OFFSET;
    Xpos1 += ROW_OFFSET;

    parameter[0] = (Xpos0) >> 8;
    parameter[1] = (Xpos0) & 0xFF;
    parameter[2] = (Xpos1) >> 8;
    parameter[3] = (Xpos1) & 0xFF;
    LCD_WriteReg(hlcdc, REG_CASET, parameter, 4);

    parameter[0] = (Ypos0) >> 8;
    parameter[1] = (Ypos0) & 0xFF;
    parameter[2] = (Ypos1) >> 8;
    parameter[3] = (Ypos1) & 0xFF;
    LCD_WriteReg(hlcdc, REG_RASET, parameter, 4);
}

/**
 * @brief  Writes pixel.
 * @param  Xpos: specifies the X position.
 * @param  Ypos: specifies the Y position.
 * @param  RGBCode: the RGB pixel color
 * @retval None
 */
static void LCD_WritePixel(LCDC_HandleTypeDef *hlcdc, uint16_t Xpos,
                           uint16_t Ypos, const uint8_t *RGBCode)
{
    uint8_t data = 0;
    /* Set Cursor */
    LCD_SetRegion(hlcdc, Xpos, Ypos, Xpos, Ypos);
    LCD_WriteReg(hlcdc, REG_WRITE_RAM, (uint8_t *)RGBCode, 2);
}

/**
 * @brief Send layer data to LCD
 * @param hlcdc LCD controller handle
 * @param RGBCode - Pointer to layer data
 * @param Xpos0 - Layer data left coordinate, base on LCD top-left, same as
 * below.
 * @param Ypos0 - Layer data top coordinate
 * @param Xpos1 - Layer data right coordinate
 * @param Ypos1 - Layer data bottom coordinate
 */
static void LCD_WriteMultiplePixels(LCDC_HandleTypeDef *hlcdc,
                                    const uint8_t *RGBCode, uint16_t Xpos0,
                                    uint16_t Ypos0, uint16_t Xpos1,
                                    uint16_t Ypos1)
{
    uint32_t size;

    // Set default layer data
    HAL_LCDC_LayerSetData(hlcdc, HAL_LCDC_LAYER_DEFAULT, (uint8_t *)RGBCode,
                          Xpos0, Ypos0, Xpos1, Ypos1);
    // Write datas to LCD register
    HAL_LCDC_SendLayerData2Reg_IT(hlcdc, REG_WRITE_RAM, 1);
}

/**
 * @brief  Writes  to the selected LCD register.
 * @param  LCD_Reg: address of the selected register.
 * @retval None
 */
static void LCD_WriteReg(LCDC_HandleTypeDef *hlcdc, uint16_t LCD_Reg,
                         uint8_t *Parameters, uint32_t NbParameters)
{
    HAL_LCDC_WriteU8Reg(hlcdc, LCD_Reg, Parameters, NbParameters);
}

/**
 * @brief  Reads the selected LCD Register.
 * @param  RegValue: Address of the register to read
 * @param  ReadSize: Number of bytes to read
 * @retval LCD Register Value.
 */
static uint32_t LCD_ReadData(LCDC_HandleTypeDef *hlcdc, uint16_t RegValue,
                             uint8_t ReadSize)
{
    uint32_t rd_data = 0;

    LCD_ReadMode(hlcdc, true);

    HAL_LCDC_ReadU8Reg(hlcdc, RegValue, (uint8_t *)&rd_data, ReadSize);

    LCD_ReadMode(hlcdc, false);

    return rd_data;
}

static uint32_t LCD_ReadPixel(LCDC_HandleTypeDef *hlcdc, uint16_t Xpos,
                              uint16_t Ypos)
{
    uint8_t parameter[2];
    uint32_t c;
    uint32_t ret_v;

    parameter[0] = 0x66;
    LCD_WriteReg(hlcdc, REG_COLOR_MODE, parameter, 1);

    LCD_SetRegion(hlcdc, Xpos, Ypos, Xpos, Ypos);

    /*
        read ram need 8 dummy cycle, and it's result is 24bit color which format
       is:

        6bit red + 2bit dummy + 6bit green + 2bit dummy + 6bit blue + 2bit dummy

    */
    c = LCD_ReadData(hlcdc, REG_READ_RAM, 4);
    c >>= lcdc_int_cfg.cfg.spi.dummy_clock; // revert fixed dummy cycle

    switch (lcdc_int_cfg.color_mode)
    {
    case LCDC_PIXEL_FORMAT_RGB565:
        parameter[0] = 0x55;
        ret_v = (uint32_t)(((c >> 8) & 0xF800) | ((c >> 5) & 0x7E0) |
                           ((c >> 3) & 0X1F));
        break;

    case LCDC_PIXEL_FORMAT_RGB666:
        parameter[0] = 0x66;
        ret_v = (uint32_t)(((c >> 6) & 0x3F000) | ((c >> 4) & 0xFC0) |
                           ((c >> 2) & 0X3F));
        break;

    case LCDC_PIXEL_FORMAT_RGB888:
        /*
           pretend that st7789v can support RGB888,

           treated as RGB666 actually(6bit R + 2bit dummy + 6bit G + 2bit dummy
           + 6bit B + 2bit dummy )

           lcdc NOT support RGB666
        */
        /*
            st7789 NOT support RGB888：

            fill 2bit dummy bit with 2bit MSB of color
        */
        parameter[0] = 0x66;
        ret_v = (uint32_t)((c & 0xFCFCFC) | ((c >> 6) & 0x030303));
        break;

    default:
        RT_ASSERT(0);
        break;
    }

    rt_kprintf("ST7789_ReadPixel %x -> %x\n", c, ret_v);

    LCD_WriteReg(hlcdc, REG_COLOR_MODE, parameter, 1);

    return ret_v;
}

static void LCD_SetColorMode(LCDC_HandleTypeDef *hlcdc, uint16_t color_mode)
{
    uint8_t parameter[2];

    /*

    Control interface color format
    ‘011’ = 12bit/pixel ‘101’ = 16bit/pixel ‘110’ = 18bit/pixel ‘111’ = 16M
    truncated

    */
    switch (color_mode)
    {
    case RTGRAPHIC_PIXEL_FORMAT_RGB565:
        /* Color mode 16bits/pixel */
        parameter[0] = 0x55;
        lcdc_int_cfg.color_mode = LCDC_PIXEL_FORMAT_RGB565;
        break;

    /*
       pretend that st7789v can support RGB888,

       treated as RGB666 actually(6bit R + 2bit dummy + 6bit G + 2bit dummy +
       6bit B + 2bit dummy )

       lcdc NOT support RGB666
    */
    case RTGRAPHIC_PIXEL_FORMAT_RGB888:
        /* Color mode 18bits/pixel */
        parameter[0] = 0x66;
        lcdc_int_cfg.color_mode = LCDC_PIXEL_FORMAT_RGB888;
        break;

    default:
        RT_ASSERT(0);
        return; // unsupport
        break;
    }

    LCD_WriteReg(hlcdc, REG_COLOR_MODE, parameter, 1);
    HAL_LCDC_SetOutFormat(hlcdc, lcdc_int_cfg.color_mode);
}

#define ST7789_BRIGHTNESS_MAX 127

static void LCD_SetBrightness(LCDC_HandleTypeDef *hlcdc, uint8_t br)
{
    uint8_t bright = (uint8_t)((int)ST7789_BRIGHTNESS_MAX * br / 100);
    LCD_WriteReg(hlcdc, REG_WBRIGHT, &bright, 1);
}

static const LCD_DrvOpsDef ST7789_drv = {LCD_Init,
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
                                         NULL};

LCD_DRIVER_EXPORT2(st7789h2, 0x8181b3, &lcdc_int_cfg, &ST7789_drv, 1);
