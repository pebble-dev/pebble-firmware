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

static UARTDeviceState s_dbg_uart_state;
static UART_HandleTypeDef s_dbg_uart_handle = {.Instance = USART3,
                                               .Init = {
                                                   .WordLength = UART_WORDLENGTH_8B,
                                                   .StopBits = UART_STOPBITS_1,
                                                   .Parity = UART_PARITY_NONE,
                                                   .HwFlowCtl = UART_HWCONTROL_NONE,
                                                   .OverSampling = UART_OVERSAMPLING_16,
                                               }};
static DMA_HandleTypeDef s_dbg_uart_rx_dma_handle = {
    .Instance = DMA1_Channel1,
};
static UARTDevice DBG_UART_DEVICE = {.state = &s_dbg_uart_state,
                                     .tx_gpio = PAD_PA20,
                                     .rx_gpio = PAD_PA27,  // TODO: Setting GPIOs to actual values
                                     .periph = &s_dbg_uart_handle,
                                     .rx_dma = &s_dbg_uart_rx_dma_handle,
                                     .irq_priority = 1};
UARTDevice *const DBG_UART = &DBG_UART_DEVICE;
UARTDevice *const ACCESSORY_UART;  //TODO
IRQ_MAP(USART3, uart_irq_handler, DBG_UART);

void DMAC1_CH1_IRQHandler(void) { HAL_DMA_IRQHandler(&s_dbg_uart_rx_dma_handle); }


void board_early_init(void) {
}

void board_init(void) {
}
