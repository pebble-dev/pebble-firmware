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

#include <stdint.h>

void gpio_use(GPIO_TypeDef* GPIOx) {

}

void gpio_release(GPIO_TypeDef* GPIOx) {
}

void gpio_output_init(const OutputConfig *pin_config, GPIOOType_TypeDef otype,
                      GPIOSpeed_TypeDef speed) {
}

void gpio_output_set(const OutputConfig *pin_config, bool asserted) {
}

void gpio_af_init(const AfConfig *af_config, GPIOOType_TypeDef otype,
                  GPIOSpeed_TypeDef speed, GPIOPuPd_TypeDef pupd) {
}

void gpio_af_configure_low_power(const AfConfig *af_config) {
}

void gpio_af_configure_fixed_output(const AfConfig *af_config, bool asserted) {
}

void gpio_input_init(const InputConfig *input_config) {
}

void gpio_input_init_pull_up_down(const InputConfig *input_config, GPIOPuPd_TypeDef pupd) {
  
}

bool gpio_input_read(const InputConfig *input_config) {
  
  return false;
}

void gpio_analog_init(const InputConfig *input_config) {
}
