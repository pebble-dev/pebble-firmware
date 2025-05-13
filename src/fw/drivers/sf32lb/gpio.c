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

#include "drivers/gpio.h"
#include "system/passert.h"
#include "board/board.h"

#include "FreeRTOS.h"

#include <stdint.h>

static RCC_MODULE_TYPE prv_get_gpio_rcc_module(GPIO_TypeDef *GPIOx) {
  if (GPIOx == hwp_gpio1) {
    return RCC_MOD_GPIO1;
  } else if (GPIOx == hwp_gpio2) {
    return RCC_MOD_GPIO2;
  } else {
    WTF;
  }
  return 0;
}

void gpio_use(GPIO_TypeDef *GPIOx) {
  RCC_MODULE_TYPE rcc_module = prv_get_gpio_rcc_module(GPIOx);
  portENTER_CRITICAL();
  HAL_RCC_EnableModule(rcc_module);
  portEXIT_CRITICAL();
}

void gpio_release(GPIO_TypeDef *GPIOx) {
  RCC_MODULE_TYPE rcc_module = prv_get_gpio_rcc_module(GPIOx);
  portENTER_CRITICAL();
  HAL_RCC_DisableModule(rcc_module);
  portEXIT_CRITICAL();
}

void gpio_output_init(const OutputConfig *pin_config, GPIOOType_TypeDef otype,
                      GPIOSpeed_TypeDef speed) {
  (void)speed;
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.Pin = pin_config->gpio_pin;
  if (otype == GPIO_OType_OD) {
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  } else if (otype == GPIO_OType_PP) {
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT;
  } else {
    WTF;
  }
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;

  HAL_GPIO_Init(pin_config->gpio, &GPIO_InitStruct);
}

void gpio_input_init(const InputConfig *pin_config) {
  gpio_use(pin_config->gpio);
  GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.Pin = pin_config->gpio_pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;

  HAL_GPIO_Init(pin_config->gpio, &GPIO_InitStruct);
}

void gpio_output_set(const OutputConfig *pin_config, bool asserted) {
  HAL_GPIO_WritePin(pin_config->gpio, pin_config->gpio_pin, asserted);
}
