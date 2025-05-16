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

#include <string.h>
#include <stdlib.h>

#include "board/board.h"
#include "ioexp.h"
#include "drivers/i2c.h"
#include "drivers/gpio.h"


static bool prv_read_register(uint8_t register_address, uint8_t *result) {
    i2c_use(I2C_AW9527);
    bool rv = i2c_read_register(I2C_AW9527, register_address, result);
    i2c_release(I2C_AW9527);
    return rv;
}

static bool prv_write_register(uint8_t register_address, uint8_t datum) {
    i2c_use(I2C_AW9527);
    bool rv = i2c_write_register(I2C_AW9527, register_address, datum);
    i2c_release(I2C_AW9527);
    return rv;
}

/*initialization*/
void ioexp_init(void) {
    PBL_LOG(LOG_LEVEL_INFO, "ioexp_init aw9527");

    gpio_output_init(&BOARD_CONFIG.ioe_rst, GPIO_OType_PP, GPIO_Speed_2MHz);
    gpio_output_set(&BOARD_CONFIG.ioe_rst, true);

    uint8_t rv;
    bool found = prv_read_register(0x23, &rv);
    if (found) {
        PBL_LOG(LOG_LEVEL_DEBUG, "Found AW9527 with STATUS register %02x", rv);
    } else {
        PBL_LOG(LOG_LEVEL_ERROR, "Failed to read the STATUS register");
    }
}

/*deinit*/
void ioexp_deinit(void) {
    gpio_output_set(&BOARD_CONFIG.ioe_rst, false);
}

/*ctrl led brightness level by percentage*/
void ioexp_led_ctrl(uint8_t percent) {

}

/*channel ctrl*/
void ioexp_pin_set(IOEXP_CHANNEL_T channel, IOEXP_STATE_T state) {
    if (channel >= IOEXP_CH10) {
        uint8_t p1_value;
        prv_read_register(0x03, &p1_value);
        if (state == IOEXP_HIGH) {
            p1_value |= (1<<channel);
        } else {
            p1_value &= ~(1<<channel);
        }
        prv_write_register(0x03, p1_value);
    } else {
        uint8_t p0_value;
        prv_read_register(0x02, &p0_value);
        if (state == IOEXP_HIGH) {
            p0_value |= (1<<channel);
        } else {
            p0_value &= ~(1<<channel);
        }
        prv_write_register(0x02, p0_value);
    }
}
