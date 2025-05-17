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
#include "drivers/touch/touch_sensor.h"
#include "board/board.h"
#include "drivers/exti.h"
#include "drivers/gpio.h"
#include "drivers/i2c.h"
#include "drivers/rtc.h"
#include "drivers/ioexp.h"
#include "kernel/events.h"
#include "kernel/util/delay.h"
#include "os/tick.h"
#include "services/common/touch/touch.h"
#include "system/logging.h"
#include "system/passert.h"
#include "util/attributes.h"
#include "util/net.h"
#include <stdint.h>


////////////////////////////////////////////////////////////////////////////////
#define TOUCH_EVENT_UP      (0x01)
#define TOUCH_EVENT_DOWN    (0x02)
#define TOUCH_EVENT_MOVE    (0x03)
typedef struct touch_message {
    GPoint point;
    uint8_t event;
}touch_message_t ;


////////////////////////////////////////////////////////////////////////////////
static bool s_callback_scheduled = false;

static void cst816x_data_write(uint8_t reg, const uint8_t *data, uint32_t len) {
  i2c_use(BOARD_CONFIG_TOUCH->i2c);
  i2c_write_register_block(BOARD_CONFIG_TOUCH->i2c, reg, len, data);
  i2c_release(BOARD_CONFIG_TOUCH->i2c);
}

static void cst816x_data_read(uint8_t reg, uint8_t *p_data, uint32_t len) {
  i2c_use(BOARD_CONFIG_TOUCH->i2c);
  i2c_read_register_block(BOARD_CONFIG_TOUCH->i2c, reg, len, p_data);
  i2c_release(BOARD_CONFIG_TOUCH->i2c);
}

static void cst816x_register_write(uint8_t reg, uint8_t data)
{
  i2c_use(BOARD_CONFIG_TOUCH->i2c);
  i2c_write_register_block(BOARD_CONFIG_TOUCH->i2c, reg, sizeof(data), &data);
  i2c_release(BOARD_CONFIG_TOUCH->i2c);
}

static uint8_t cst816x_register_read(uint8_t reg)
{
  uint8_t data = 0;
  i2c_use(BOARD_CONFIG_TOUCH->i2c);
  i2c_read_register_block(BOARD_CONFIG_TOUCH->i2c, reg, sizeof(data), &data);
  i2c_release(BOARD_CONFIG_TOUCH->i2c);
  return data;
}

static void cst816x_read_point(touch_message_t* p_msg) {
    uint8_t rbuf[5] = {0};
    cst816x_data_read(0x02, rbuf, 5);

    uint8_t press = rbuf[0] & 0x0F;
    if (press == 0x01) {
        p_msg->event = TOUCH_EVENT_DOWN;
    } else {
        p_msg->event = TOUCH_EVENT_UP;
    }
    p_msg->point.x = (((uint16_t)(rbuf[1] & 0x0F)) << 8) | rbuf[2];
    p_msg->point.y = (((uint16_t)(rbuf[3] & 0X0F)) << 8) | rbuf[4];
}


static void touch_cst816x_int_callback_function(void *context) {
  s_callback_scheduled = false;

  touch_message_t msg;
  uint64_t now_ms = ticks_to_milliseconds(rtc_get_ticks());
  memset(&msg, 0, sizeof(msg));
  cst816x_read_point(&msg);
  if (msg.event == TOUCH_EVENT_DOWN) {
    touch_handle_update(0, TouchState_FingerDown, &msg.point, 0, now_ms);
  } else if (msg.event == TOUCH_EVENT_UP) {
    touch_handle_update(0, TouchState_FingerUp, NULL, 0, now_ms);
  } else {
    
  }
}

// @brief int function
static void touch_cst816x_int_handle(bool *should_context_switch) {
  if (s_callback_scheduled) {
    return;
  }
  PebbleEvent e = {
    .type = PEBBLE_CALLBACK_EVENT,
    .callback.callback = touch_cst816x_int_callback_function
  };
  *should_context_switch = event_put_isr(&e);
  s_callback_scheduled = true;
}

// @brief Initialization
void touch_sensor_init(void) {
#if 0
  gpio_input_init_pull_up_down(&BOARD_CONFIG_TOUCH->int_gpio, GPIO_PuPd_UP);

  // reset 
  // 拉高 1ms
  ioexp_pin_set(IOEXP_CH02, IOEXP_HIGH);
  delay_us(1*1000);
  // 拉低 10ms
  ioexp_pin_set(IOEXP_CH02, IOEXP_LOW);
  delay_us(10*1000);
  // 拉高
  ioexp_pin_set(IOEXP_CH02, IOEXP_HIGH);
  delay_us(50*1000);

  // check
  uint8_t whoami;
  for (int i=0; i<5; ++i) {
    whoami = cst816x_register_read(0xA7);
    PBL_LOG(LOG_LEVEL_ALWAYS, "cst816x read chipid. chipid=0x%02x", whoami);
    if (whoami == 0x11) {
      break;
    }
    delay_us(10*1000);
  }
  
  // initialize exti
  exti_configure_pin(BOARD_CONFIG_TOUCH->int_exti, ExtiTrigger_Falling, touch_cst816x_int_handle);
  exti_enable(BOARD_CONFIG_TOUCH->int_exti);
  PBL_LOG(LOG_LEVEL_DEBUG, "Initialized CST816 touch controller");
#endif
}

