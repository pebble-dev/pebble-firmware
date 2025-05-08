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

#include <stdint.h>
#include <stdbool.h>

#include "board/board.h"

void pwm_init(const PwmConfig *pwm, uint32_t resolution, uint32_t frequency);

// Note: pwm peripheral needs to be enabled before the duty cycle can be set
void pwm_set_duty_cycle(const PwmConfig *pwm, uint32_t duty_cycle);

void pwm_enable(const PwmConfig *pwm, bool enable);

extern void example_pwm();

