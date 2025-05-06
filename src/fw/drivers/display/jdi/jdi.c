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
#include "jdi_definitions.h"

#define LCDC_SUPPORT_LINE_DONE
DisplayRow  jdi_row;
static GPoint s_disp_offset;

static DisplayContext s_display_context;

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
LCDC_HandleTypeDef lcdc;
static SemaphoreHandle_t s_dma_update_in_progress_semaphore;

void display_init(void) 
{
    /* Initialize JDI low level bus layer ----------------------------------*/
    memcpy(&lcdc.Init, &lcdc_int_cfg, sizeof(LCDC_InitTypeDef));
    HAL_LCDC_Init(&lcdc);

    BSP_LCD_Reset(0);//Reset LCD
    HAL_Delay_us(10);
    BSP_LCD_Reset(1);
}

uint32_t display_baud_rate_change(uint32_t new_frequency_hz) 
{
    HAL_LCDC_SetFreq(&lcdc, new_frequency_hz);
    return new_frequency_hz;
  }
  

#if 0
static uint32_t LCD_ReadID(LCDC_HandleTypeDef *hlcdc)
{
    return THE_LCD_ID;
}


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


static void LCD_DisplayOff(LCDC_HandleTypeDef *hlcdc)
{
    /* Display Off */
    //LCD_WriteReg(hlcdc, REG_DISPLAY_OFF, (uint8_t *)NULL, 0);
}


static void LCD_SetRegion(LCDC_HandleTypeDef *hlcdc, uint16_t Xpos0, uint16_t Ypos0, uint16_t Xpos1, uint16_t Ypos1)
{
    HAL_LCDC_SetROIArea(hlcdc, 0, Ypos0, THE_LCD_PIXEL_WIDTH - 1, Ypos1); //Not support partical columns
}
#endif

void display_clear(void) 
{

}
  
bool display_update_in_progress(void) 
{
    if (xSemaphoreTake(s_dma_update_in_progress_semaphore, 0) == pdPASS) {
    xSemaphoreGive(s_dma_update_in_progress_semaphore);
    return false;
    }

    return true;
}



static void SendLayerDataCpltCbk(LCDC_HandleTypeDef *lcdc)
{

    if(s_display_context.complete)
        s_display_context.complete();
    
    if (lcdc->XferCpltCallback != NULL)
    {
        lcdc->XferCpltCallback = NULL;
    }

}

#ifdef LCDC_SUPPORT_LINE_DONE_IRQ
  static void SendLineDoneCpltCbk(LCDC_HandleTypeDef *lcdc, uint32_t lines)
  {
    if(s_display_context.get_next_row)
        s_display_context.get_next_row(&jdi_row);

  }
#endif /* LCDC_SUPPORT_LINE_DONE_IRQ */

  void display_update(NextRowCallback nrcb, UpdateCompleteCallback uccb) {
    PBL_ASSERTN(nrcb != NULL);
    PBL_ASSERTN(uccb != NULL);
    xSemaphoreTake(s_dma_update_in_progress_semaphore, portMAX_DELAY);

    s_display_context.get_next_row = nrcb;
    s_display_context.complete = uccb;

    lcdc.XferCpltCallback = SendLayerDataCpltCbk;
#ifdef LCDC_SUPPORT_LINE_DONE_IRQ
    lcdc.XferLineCallback = SendLineDoneCpltCbk;
#endif /* LCDC_SUPPORT_LINE_DONE_IRQ */


    uint16_t Ypos0 = jdi_row.address;
    uint16_t Ypos1 = jdi_row.address+1;
    uint16_t Xpos0 = 0;
    uint16_t Xpos1 = THE_LCD_PIXEL_WIDTH - 1; 
    HAL_LCDC_LayerSetData(&lcdc, HAL_LCDC_LAYER_DEFAULT, jdi_row.data, Xpos0, Ypos0, Xpos1, Ypos1);
    HAL_LCDC_SendLayerData_IT(&lcdc);
    s_disp_offset.x = Xpos0;
    s_disp_offset.y = Ypos0;
   xSemaphoreGive(s_dma_update_in_progress_semaphore);

  }
  
  void display_pulse_vcom(void) {
    PBL_ASSERTN(BOARD_CONFIG.lcd_com.gpio != 0);
    gpio_output_set(&BOARD_CONFIG.lcd_com, true);
    // the spec requires at least 1us; this provides ~2 so should be safe
    for (volatile int i = 0; i < 8; i++);
    gpio_output_set(&BOARD_CONFIG.lcd_com, false);

  }
  
  void display_show_splash_screen(void) {
    // The bootloader has already drawn the splash screen for us; nothing to do!
  }
  
  void display_show_panic_screen(uint32_t error_code) 
  {

  
  }
  
  // Stubs for display offset
  void display_set_offset(GPoint offset) {
    s_disp_offset = offset;
    }
  
  GPoint display_get_offset(void) {  
    return s_disp_offset; 
  }
  


