#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "os/mutex.h"
#include "os/tick.h"
#include "drivers/dma.h"
#include "drivers/gpio.h"
#include "drivers/periph_config.h"
#include "system/passert.h"
#include "services/common/analytics/analytics.h"
#include "jdi_definitions.h"
#include "bf0_hal_lcdc.h"
#include "board/board.h"
#include "lcd_definitions.h"
#include "jdi_definitions.h"

static LCDC_InitTypeDef lcdc_int_cfg =
{
    .lcd_itf = LCDC_INTF_JDI_PARALLEL,
    .freq = 60,
    .color_mode = LCDC_PIXEL_FORMAT_RGB565,

    .cfg = {
        .jdi = {
            .bank_col_head = 0,
            .valid_columns = THE_LCD_PIXEL_WIDTH,
            .bank_col_tail = 4,

            .bank_row_head = 0,
            .valid_rows = THE_LCD_PIXEL_HEIGHT,
            .bank_row_tail = 4,
        },
    },

};
/**
  * @brief  Power on the LCD.
  * @param  None
  * @retval None
  */

static void LCD_Init(LCDC_HandleTypeDef *hlcdc)
{
    /* Initialize JDI low level bus layer ----------------------------------*/
    memcpy(&hlcdc->Init, &lcdc_int_cfg, sizeof(LCDC_InitTypeDef));
    HAL_LCDC_Init(hlcdc);

    BSP_LCD_Reset(0);//Reset LCD
    HAL_Delay_us(10);
    BSP_LCD_Reset(1);
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
    //LCD_WriteReg(hlcdc, REG_DISPLAY_ON, (uint8_t *)NULL, 0);
#ifdef SF32LB58X
//HAL_LCDC_JDI_LP_CLK
    hwp_lptim3->ARR = 32768 / lcdc_int_cfg.freq;
    hwp_lptim3->CMP = hwp_lptim3->ARR / 2;
    hwp_lptim3->CR |= LPTIM_CR_ENABLE;
    hwp_lptim3->CR |= LPTIM_CR_CNTSTRT;


    MODIFY_REG(hwp_lpsys_aon->CR1, LPSYS_AON_CR1_PBR_SEL0_Msk, 2 << LPSYS_AON_CR1_PBR_SEL0_Pos);
    MODIFY_REG(hwp_lpsys_aon->CR1, LPSYS_AON_CR1_PBR_SEL1_Msk, 3 << LPSYS_AON_CR1_PBR_SEL1_Pos);

    hwp_rtc->PBR3R |= 3 << RTC_PBR3R_SEL_Pos; //frp
    hwp_rtc->PBR4R |= 2 << RTC_PBR4R_SEL_Pos; //xfrp
    hwp_rtc->PBR5R |= 3 << RTC_PBR5R_SEL_Pos; //vcom
#endif /* SF32LB58X */
}

/**
  * @brief  Disables the Display.
  * @param  None
  * @retval None
  */
static void LCD_DisplayOff(LCDC_HandleTypeDef *hlcdc)
{
    /* Display Off */
    //LCD_WriteReg(hlcdc, REG_DISPLAY_OFF, (uint8_t *)NULL, 0);
}

static void LCD_SetRegion(LCDC_HandleTypeDef *hlcdc, uint16_t Xpos0, uint16_t Ypos0, uint16_t Xpos1, uint16_t Ypos1)
{
    HAL_LCDC_SetROIArea(hlcdc, 0, Ypos0, THE_LCD_PIXEL_WIDTH - 1, Ypos1); //Not support partical columns
}

static void LCD_WriteMultiplePixels(LCDC_HandleTypeDef *hlcdc, const uint8_t *RGBCode, uint16_t Xpos0, uint16_t Ypos0, uint16_t Xpos1, uint16_t Ypos1)
{
    HAL_LCDC_LayerSetData(hlcdc, HAL_LCDC_LAYER_DEFAULT, (uint8_t *)RGBCode, Xpos0, Ypos0, Xpos1, Ypos1);
    HAL_LCDC_SendLayerData_IT(hlcdc);
}

static const LCD_DrvOpsDef JDI_drv =
{
    LCD_Init,
    LCD_ReadID,
    LCD_DisplayOn,
    LCD_DisplayOff,

    LCD_SetRegion,
    NULL,
    LCD_WriteMultiplePixels,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};
LCD_DRIVER_EXPORT2(jdi, THE_LCD_ID, &lcdc_int_cfg,
                   &JDI_drv, 1);


