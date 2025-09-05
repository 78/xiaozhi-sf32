/**
  ******************************************************************************
  * @file   bsp_pinmux.c
  * @author Sifli software development team
  ******************************************************************************
*/
/**
 * @attention
 * Copyright (c) 2019 - 2022,  Sifli Technology
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Sifli integrated circuit
 *    in a product or a software update for such product, must reproduce the above
 *    copyright notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Sifli nor the names of its contributors may be used to endorse
 *    or promote products derived from this software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Sifli integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY SIFLI TECHNOLOGY "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SIFLI TECHNOLOGY OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "bsp_board.h"

#if defined(SF32LB52X)
//lei
#define LCDC1_8080_PEN GPIO_A01
#define LCDC1_8080_T_CS GPIO_A02
#define LCDC1_8080_T_DI GPIO_A09
#define LCDC1_8080_T_CLK GPIO_A20
#define LCDC1_8080_T_DO GPIO_A31    
#endif

#ifdef BSP_USING_PSRAM1
/* APS 128p*/
static void board_pinmux_psram_func0()
{
    HAL_PIN_Set(PAD_SA01, MPI1_DIO0, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA02, MPI1_DIO1, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA03, MPI1_DIO2, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA04, MPI1_DIO3, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA05, MPI1_DIO4, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA06, MPI1_DIO5, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA07, MPI1_DIO6, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA08, MPI1_DIO7, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA09, MPI1_DQSDM, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA10, MPI1_CLK,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_SA11, MPI1_CS,   PIN_NOPULL, 1);

    HAL_PIN_Set_Analog(PAD_SA00, 1);
    HAL_PIN_Set_Analog(PAD_SA12, 1);
}

/* APS 1:64p 2:32P, 4:Winbond 32/64/128p*/
static void board_pinmux_psram_func1_2_4(int func)
{
    HAL_PIN_Set(PAD_SA01, MPI1_DIO0, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA02, MPI1_DIO1, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA03, MPI1_DIO2, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA04, MPI1_DIO3, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA08, MPI1_DIO4, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA09, MPI1_DIO5, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA10, MPI1_DIO6, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA11, MPI1_DIO7, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA07, MPI1_CLK,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_SA05, MPI1_CS,   PIN_NOPULL, 1);

#ifdef FPGA
    HAL_PIN_Set(PAD_SA00, MPI1_DM, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA06, MPI1_CLKB, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_SA12, MPI1_DQSDM, PIN_PULLDOWN, 1);
#else
    switch (func)
    {
    case 1:             // APS 64P XCELLA
        HAL_PIN_Set(PAD_SA12, MPI1_DQSDM, PIN_PULLDOWN, 1);
        HAL_PIN_Set_Analog(PAD_SA00, 1);
        HAL_PIN_Set_Analog(PAD_SA06, 1);
        break;
    case 2:             // APS 32P LEGACY
        HAL_PIN_Set(PAD_SA00, MPI1_DM, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA12, MPI1_DQS, PIN_PULLDOWN, 1);
        HAL_PIN_Set(PAD_SA06, MPI1_CLKB, PIN_NOPULL, 1);
        break;
    case 4:             // Winbond 32/64/128p
        //HAL_PIN_Set(PAD_SA06, MPI1_CLKB, PIN_NOPULL, 1);
        HAL_PIN_Set(PAD_SA12, MPI1_DQSDM, PIN_NOPULL, 1);
        HAL_PIN_Set_Analog(PAD_SA00, 1);
        HAL_PIN_Set_Analog(PAD_SA06, 1);
        break;
    }
#endif
}


/* APS 16p*/
static void board_pinmux_psram_func3()
{
    HAL_PIN_Set(PAD_SA09, MPI1_CLK, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_SA08, MPI1_CS,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_SA05, MPI1_DIO0, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA07, MPI1_DIO1, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_SA06, MPI1_DIO2, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_SA10, MPI1_DIO3, PIN_PULLUP, 1);

    HAL_PIN_Set_Analog(PAD_SA00, 1);
    HAL_PIN_Set_Analog(PAD_SA01, 1);
    HAL_PIN_Set_Analog(PAD_SA02, 1);
    HAL_PIN_Set_Analog(PAD_SA03, 1);
    HAL_PIN_Set_Analog(PAD_SA04, 1);
    HAL_PIN_Set_Analog(PAD_SA11, 1);
    HAL_PIN_Set_Analog(PAD_SA12, 1);
}

static void board_pinmux_mpi1_none(void)
{
    uint32_t i;

    for (i = 0; i <= 12; i++)
    {
        HAL_PIN_Set_Analog(PAD_SA00 + i, 1);
    }
}
#endif

static void BSP_PIN_Common(void)
{
#ifdef SOC_BF0_HCPU
    // HCPU pins

    uint32_t pid = (hwp_hpsys_cfg->IDR & HPSYS_CFG_IDR_PID_Msk) >> HPSYS_CFG_IDR_PID_Pos;

    pid &= 7;

#ifdef BSP_USING_PSRAM1
    switch (pid)
    {
    case 5: //BOOT_PSRAM_APS_16P:
        board_pinmux_psram_func3();         // 16Mb APM QSPI PSRAM
        break;
    case 4: //BOOT_PSRAM_APS_32P:
        board_pinmux_psram_func1_2_4(2);    // 32Mb APM LEGACY PSRAM
        break;
    case 6: //BOOT_PSRAM_WINBOND:                // Winbond HYPERBUS PSRAM
        board_pinmux_psram_func1_2_4(4);
        break;
    case 3: // BOOT_PSRAM_APS_64P:
        board_pinmux_psram_func1_2_4(1);    // 64Mb APM XCELLA PSRAM
        break;
    case 2: //BOOT_PSRAM_APS_128P:
        board_pinmux_psram_func0();         // 128Mb APM XCELLA PSRAM
        break;
    default:
        board_pinmux_mpi1_none();
        break;
    }
#endif /* BSP_USING_PSRAM1 */

#ifdef BSP_ENABLE_MPI2
    // MPI2
    HAL_PIN_Set(PAD_PA16, MPI2_CLK,  PIN_NOPULL,   1);
    HAL_PIN_Set(PAD_PA12, MPI2_CS,   PIN_NOPULL,   1);
    HAL_PIN_Set(PAD_PA15, MPI2_DIO0, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_PA13, MPI2_DIO1, PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_PA14, MPI2_DIO2, PIN_PULLUP,   1);
    HAL_PIN_Set(PAD_PA17, MPI2_DIO3, PIN_PULLUP, 1);
#elif BSP_USING_SDIO
    HAL_PIN_Set(PAD_PA15, SD1_CMD, PIN_PULLUP, 1);
    HAL_Delay_us(20);
    HAL_PIN_Set(PAD_PA12, SD1_DIO2,  PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA13, SD1_DIO3, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA14, SD1_CLK, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA16, SD1_DIO0, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA17, SD1_DIO1, PIN_PULLUP, 1);
#endif
    HAL_PIN_Set(PAD_PA00, GPIO_A0,  PIN_PULLDOWN, 1);     // #LCD_RESETB
    HAL_PIN_Set(PAD_PA10, GPIO_A10, PIN_PULLUP, 1);     // AUDIO_PA_CTRL

    // UART1 - debug
    HAL_PIN_Set(PAD_PA18, USART1_RXD, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA19, USART1_TXD, PIN_PULLUP, 1);

    // Key1
    HAL_PIN_Set(PAD_PA34, GPIO_A34, PIN_NOPULL, 1);//POW_KEY
    // Key2
    // HAL_PIN_Set(PAD_PA11, GPIO_A11, PIN_NOPULL, 1);

    // HAL_PIN_Set(PAD_PA40, GPIO_A40, PIN_NOPULL, 1);
    // HAL_PIN_Set(PAD_PA38, GPIO_A38, PIN_NOPULL, 1);

    // rgb
    HAL_PIN_Set(PAD_PA32, GPIO_A32, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA33, GPIO_A33, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA44, GPIO_A44, PIN_NOPULL, 1);

    // PA22 #XTAL32K_XI
    // PA23 #XTAL32K_XO

    // USBD
    // HAL_PIN_Set_Analog(PAD_PA35, 1);                    // USB_DP
    // HAL_PIN_Set_Analog(PAD_PA36, 1);                    // USB_DM
    HAL_PIN_Set(PAD_PA35, GPIO_A35,  PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA36, GPIO_A36,  PIN_NOPULL, 1);

    // SPI1(word lib,NO TF card)
    HAL_PIN_Set(PAD_PA24, SPI1_DIO, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA25, SPI1_DI,  PIN_PULLDOWN, 1);
    HAL_PIN_Set(PAD_PA28, SPI1_CLK, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA29, SPI1_CS,  PIN_NOPULL, 1);

//
//     HAL_PIN_Set_DS1(PAD_PA24, 1, 1);
//     HAL_PIN_Set_DS1(PAD_PA25, 1, 1);
//     HAL_PIN_Set_DS1(PAD_PA28, 1, 1);
//     HAL_PIN_Set_DS1(PAD_PA29, 1, 1);
    // GPIOs
    //HAL_PIN_Set(PAD_PA26, GPIO_A26, PIN_NOPULL, 1);
    //HAL_PIN_Set(PAD_PA32, GPIO_A32, PIN_PULLDOWN, 1);   // RGB LED
    //HAL_PIN_Set(PAD_PA38, GPIO_A38, PIN_PULLDOWN, 1);
    //HAL_PIN_Set(PAD_PA44, GPIO_A44, PIN_PULLDOWN, 1);   // VBUS_DET

#ifdef BSP_USING_BOARD_SF32LB52_XTY_AI_THT
    HAL_PIN_Set(PAD_PA38, GPIO_A38, PIN_PULLUP, 1);//DIY KEY
    // HAL_PIN_Set(PAD_PA42, GPIO_A42, PIN_NOPULL, 1); // BL
    // HAL_PIN_Set(PAD_PA21, GPIO_A21, PIN_PULLDOWN, 1);
#endif

    HAL_PIN_Set(PAD_PA11, GPIO_A11, PIN_PULLDOWN, 1); // 漏电
    // HAL_PIN_Set(PAD_PA38, GPIO_A38, PIN_PULLDOWN, 1); // 漏电
    // HAL_PIN_Set(PAD_PA40, GPIO_A40, PIN_PULLDOWN, 1); // 漏电
#endif

}

void BSP_PIN_LCD(void)
{
    // LCDC1 - SPI
    // HAL_PIN_Set(PAD_PA03, LCDC1_SPI_CS, PIN_NOPULL, 1);
    // HAL_PIN_Set(PAD_PA04, LCDC1_SPI_CLK, PIN_NOPULL, 1);
    // HAL_PIN_Set(PAD_PA05, LCDC1_SPI_DIO0, PIN_NOPULL, 1);
    // HAL_PIN_Set(PAD_PA06, LCDC1_SPI_DIO1, PIN_NOPULL, 1);
    //LCDC1_DBI_8BITS


    HAL_PIN_Set(PAD_PA01, LCDC1_8080_PEN, PIN_PULLUP, 1);
    HAL_PIN_Set(PAD_PA02, LCDC1_8080_T_CS, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA09, LCDC1_8080_T_DI, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA20, LCDC1_8080_T_CLK, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA31, LCDC1_8080_T_DO, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA03, LCDC1_8080_CS, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA04, LCDC1_8080_WR, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA05, LCDC1_8080_RD, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA06, LCDC1_8080_DC, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA07, LCDC1_8080_DIO0, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA08, LCDC1_8080_DIO1, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA37, LCDC1_8080_DIO2, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA39, LCDC1_8080_DIO3, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA40, LCDC1_8080_DIO4, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA41, LCDC1_8080_DIO5, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA42, LCDC1_8080_DIO6, PIN_NOPULL, 1);
    HAL_PIN_Set(PAD_PA43, LCDC1_8080_DIO7, PIN_NOPULL, 1);

}

void BSP_PIN_Init(void)
{
    BSP_PIN_Common();

    BSP_PIN_LCD();

}


/************************ (C) COPYRIGHT Sifli Technology *******END OF FILE****/
