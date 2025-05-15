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

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    IOEXP_HIGH,
    IOEXP_LOW,
}IOEXP_STATE_T;

typedef enum {
    IOEXP_CH00,
    IOEXP_CH01,
    IOEXP_CH02,
    IOEXP_CH03,
    IOEXP_CH04,
    IOEXP_CH05,
    IOEXP_CH06,
    IOEXP_CH07,
    IOEXP_CH10,
    IOEXP_CH11,
    IOEXP_CH12,
    IOEXP_CH13,
    IOEXP_CH14,
    IOEXP_CH15,
    IOEXP_CH16,
    IOEXP_CH17,
}IOEXP_CHANNEL_T;

/*initialization*/
void ioexp_init(void);
/*deinit*/
void ioexp_deinit(void);
/*ctrl led brightness level by percentage*/
void ioexp_led_ctrl(uint8_t percent);
/*channel ctrl*/
void ioexp_pin_set(IOEXP_CHANNEL_T channel, IOEXP_STATE_T state);
/*test routing*/
void ioexp_test(void);