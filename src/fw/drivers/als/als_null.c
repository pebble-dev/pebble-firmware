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
#include "drivers/als/als.h"
#include "drivers/gpio.h"
#include "drivers/pmic.h"
#include "system/logging.h"
#include "system/passert.h"
#include "drivers/i2c.h"
#include "kernel/util/delay.h"



//////////////////////////////////////////////////////////////////////
// @brief initialization
void als_init(void) {
}

// @brief deinit
void als_deinit(void) {
}

// @brief print register value
void als_register_dump(void) {
}

// @brief read ambient light  
float als_ambient_light_lux_get(void) {
  return 0;
}


