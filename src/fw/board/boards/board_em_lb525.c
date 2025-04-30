/*
 * Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "board/board.h"
#include "drivers/sf32lb/uart_definitions.h"
#include "board_em_lb525.h"
#include "drivers/i2c.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "drivers/sf32lb/i2c_hal_definitions.h"
#include "drivers/sf32lb/pwm_hal_definitions.h"
#include "board/board_sf32lb.h"

#define USING_UART1
#ifdef USING_UART1
#define UART_INST USART1
#define UART_TX PAD_PA19
#define UART_RX PAD_PA18
#define UART_DMAREQ DMA_REQUEST_4
#else
#define UART_INST USART3
#define UART_TX PAD_PA20
#define UART_RX PAD_PA27
#define UART_DMAREQ DMA_REQUEST_7
#endif

static UARTDeviceState s_dbg_uart_state;
static UART_HandleTypeDef s_dbg_uart_handle = {.Instance = UART_INST,
                                               .Init = {
                                                   .WordLength = UART_WORDLENGTH_8B,
                                                   .StopBits = UART_STOPBITS_1,
                                                   .Parity = UART_PARITY_NONE,
                                                   .HwFlowCtl = UART_HWCONTROL_NONE,
                                                   .OverSampling = UART_OVERSAMPLING_16,
                                               }};
static DMA_HandleTypeDef s_dbg_uart_rx_dma_handle = {
    .Instance = DMA1_Channel1,
    .Init.Request = UART_DMAREQ,
};
static UARTDevice DBG_UART_DEVICE = {.state = &s_dbg_uart_state,
                                     .tx_gpio = UART_TX,
                                     .rx_gpio = UART_RX,  // TODO: Setting GPIOs to actual values
                                     .periph = &s_dbg_uart_handle,
                                     .rx_dma = &s_dbg_uart_rx_dma_handle,
                                     .irq_priority = 1};
UARTDevice *const DBG_UART = &DBG_UART_DEVICE;
UARTDevice *const ACCESSORY_UART;  //TODO

#ifdef USING_UART1
IRQ_MAP(USART1, uart_irq_handler, DBG_UART);
#else
IRQ_MAP(USART3, uart_irq_handler, DBG_UART);
#endif

void DMAC1_CH1_IRQHandler(void) { HAL_DMA_IRQHandler(&s_dbg_uart_rx_dma_handle); }


I2CBusState i2c1_state;
I2CBus i2c1 = {
            .hal  = &i2c1_hal_obj,
            .state = &i2c1_state,
            .scl_gpio = 
            {
                .gpio_pin = 31,
            },
            .sda_gpio = 
            {
                .gpio_pin = 33,
            },
            .name = "i2c1",
            }; 


PwmState pwm2_1_state =
{
    .channel = 1,
};


PwmConfig pwm2_1 =
{
    .timer =
        {
            .peripheral = hwp_gptim1,
            .init = tim_init,
            .preload = tim_preload,
        },
     .state = &pwm2_1_state,
     .output = 
      {
            .gpio_pin = 26,
      },

};



#define LCD_RESET_PIN           (0)         // GPIO_A00
static void BSP_GPIO_Set(int pin, int val, int is_porta)
{
    GPIO_TypeDef *gpio = (is_porta) ? hwp_gpio1 : hwp_gpio2;
    GPIO_InitTypeDef GPIO_InitStruct;

    // set sensor pin to output mode
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(gpio, &GPIO_InitStruct);

    // set sensor pin to high == power on sensor board
    HAL_GPIO_WritePin(gpio, pin, (GPIO_PinState)val);
}
void BSP_LCD_Reset(uint8_t high1_low0)
{
    BSP_GPIO_Set(LCD_RESET_PIN, high1_low0, 1);
}

void BSP_LCD_PowerDown(void)
{
    // TODO: LCD power down
    BSP_GPIO_Set(LCD_RESET_PIN, 0, 1);
}

void BSP_LCD_PowerUp(void)
{
    // TODO: LCD power up
    HAL_Delay_us(500);      // lcd power on finish ,need 500us
}



void board_early_init(void) {
}

void board_init(void) {

}
