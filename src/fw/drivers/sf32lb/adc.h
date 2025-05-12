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


#pragma once

#include "board/board.h"

#include <stdbool.h>
#include <stdint.h>

//! Start using the I2C bus to which \a slave is connected
//! Must be called before any other reads or writes to the slave are performed
//! @param slave    I2C slave reference, which will identify the bus to use


//! Init all adc supported
//! Must be called before any other deviec use adc
//! @param adc_config   must init member: adc, adc_channel, gpio_pin;adc_channel:0~7; if don't need gpio_pin , gpio_pin should config as 0xFF;
extern int adc_init(ADCInputConfig* adc_config);

//! Enable or disable adc and channel, eanble adc will config pinmux
//! Must enable adc before get adc value
//! @param adc_config   must init member: adc, adc_channel, gpio_pin;adc_channel:0~7; if don't need gpio_pin , gpio_pin should config as 0xFF;
extern int adc_enabled(ADCInputConfig* adc_config, bool enabled);

//! Get special adc channel value
//! Must enable adc before get adc value
//! @param adc_config   must init member: adc, adc_channel;adc_channel:0~7;
extern int get_adc_value(ADCInputConfig* adc_config, uint32_t *value);
extern void example_adc();
