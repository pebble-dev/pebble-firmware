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
#include "console/prompt.h"
#include "drivers/exti.h"
#include "drivers/gpio.h"
#include "drivers/i2c.h"
#include "drivers/mag.h"
#include "drivers/periph_config.h"
//#include "kernel/events.h"
#include "system/logging.h"
#include "os/mutex.h"
#include "system/passert.h"
#include "kernel/util/sleep.h"
#include <mcu.h>
#include <inttypes.h>
#include <stdint.h>
#include "mmc5603.h"
#include "services/common/new_timer/new_timer.h"


//////////////////////////////////////////////////////////////////////
#define MMC5603_REG_DATA            0x00
#define MMC5603_REG_XL              0x00
#define MMC5603_REG_XH              0x01
#define MMC5603_REG_YL              0x02
#define MMC5603_REG_YH              0x03
#define MMC5603_REG_ZL              0x04
#define MMC5603_REG_ZH              0x05
#define MMC5603_REG_STATUS1         0x18
#define MMC5603_REG_STATUS0         0x19
#define MMC5603_REG_ODR             0x1A
#define MMC5603_REG_CTRL0           0x1B
#define MMC5603_REG_CTRL1           0x1C
#define MMC5603_REG_CTRL2           0x1D
#define MMC5603_REG_X_THD           0x1E
#define MMC5603_REG_Y_THD           0x1F
#define MMC5603_REG_Z_THD           0x20
#define MMC5603_REG_ST_X_VAL        0x27
#define MMC5603_REG_ST_Y_VAL        0x28
#define MMC5603_REG_ST_Z_VAL        0x29
#define MMC5603_REG_WHOAMI          0x39

#undef  __BIT
#define __BIT(x)                    (1uL<<(x))



//////////////////////////////////////////////////////////////////////
static PebbleMutex *s_mag_mutex;
static bool s_initialized = false;
static int s_use_refcount = 0;

static uint8_t s_freq_hz = 5;
static TimerID s_event_timer_id = 0;

static bool mmc5603_read(uint8_t reg_addr, uint8_t data_len, uint8_t *data) {
  return i2c_read_register_block(I2C_MMC5603, reg_addr, data_len, data);
}

static bool mmc5603_write(uint8_t reg_addr, uint8_t data) {
  return i2c_write_register_block(I2C_MMC5603, reg_addr, 1, &data);
}


//! Move the mag into standby mode, which is a low power mode where we're not actively sampling
//! the sensor or firing interrupts.
static bool prv_enter_standby_mode(void) {
  // Ask to enter standby mode
  if (!mmc5603_write(MMC5603_REG_CTRL2, 0x00)) {
    return false;
  }
  return true;
#if 0
  // Wait for the sysmod register to read that we're now in standby mode. This can take up to
  // 1/ODR to respond. Since we only support speeds as slow as 5Hz, that means we may be waiting
  // for up to 200ms for this part to become ready.
  const int NUM_ATTEMPTS = 300; // 200ms + some padding for safety
  for (int i = 0; i < NUM_ATTEMPTS; ++i) {
    uint8_t sysmod = 0;
    if (!mmc5603_read(SYSMOD_REG, 1, &sysmod)) {
      return false;
    }

    if (sysmod == 0) {
      // We're done and we're now in standby!
      return true;
    }

    // Wait at least 1ms before asking again
    psleep(2);
  }
  return false;
#endif
}

// Ask the compass for a 8-bit value that's programmed into the IC at the
// factory. Useful as a sanity check to make sure everything came up properly.
bool mmc5603_check_whoami(void) {
  static const uint8_t COMPASS_WHOAMI_BYTE = 0x10;
  uint8_t whoami = 0;

  PBL_LOG(LOG_LEVEL_ALWAYS, "mmc5603_check_whoami");
  mag_use();
  mmc5603_read(MMC5603_REG_WHOAMI, 1, &whoami);
  PBL_LOG(LOG_LEVEL_ALWAYS, "whoami=%d", whoami);
  mag_release();
  PBL_LOG(LOG_LEVEL_ALWAYS, "Read mmc5603 whoami byte 0x%x, expecting 0x%x", whoami, COMPASS_WHOAMI_BYTE);
  return (whoami == COMPASS_WHOAMI_BYTE);
}

void mmc5603_init(void) {
  if (s_initialized) {
    return;
  }
  
#if 0
  PBL_LOG(LOG_LEVEL_ALWAYS, "mmc5603_init");
  s_mag_mutex = mutex_create();
  s_initialized = true;

  if (!mmc5603_check_whoami()) {
    PBL_LOG(LOG_LEVEL_ALWAYS, "Failed to query Mag");
    return;
  }

  // configure mmc5603
  mmc5603_write(MMC5603_REG_CTRL1, 0);
  mmc5603_write(MMC5603_REG_ODR, s_freq_hz); 
  mmc5603_write(MMC5603_REG_CTRL0, __BIT(7)|__BIT(5));
  mmc5603_write(MMC5603_REG_CTRL2, 0);
#endif
}

// @brief 定时器函数
void mmc5603_timer_handler(void *data)
{
  if (s_use_refcount == 0) {
    return;
  }
  PBL_LOG(LOG_LEVEL_ALWAYS, "--------------->mag event");

#if 0
  PebbleEvent e = {
    .type = PEBBLE_ECOMPASS_SERVICE_EVENT,
  };
  event_put(&e);
#endif
}

void mag_use(void) {
  PBL_ASSERTN(s_initialized);

  PBL_LOG(LOG_LEVEL_ALWAYS, "---111--");
  mutex_lock(s_mag_mutex);
  PBL_LOG(LOG_LEVEL_ALWAYS, "---222--s_use_refcount=%d", s_use_refcount);
  if (s_use_refcount == 0) {
    i2c_use(I2C_MMC5603);
    PBL_LOG(LOG_LEVEL_ALWAYS, "---333--s_event_timer_id=%ld", s_event_timer_id);
    //根据频率启动定时器
    if (s_event_timer_id == 0) {
      s_event_timer_id = new_timer_create();
      PBL_LOG(LOG_LEVEL_ALWAYS, "---444--s_event_timer_id=%ld", s_event_timer_id);
      PBL_ASSERTN(s_event_timer_id);
    }
    uint32_t timeout_ms = 1000 / s_freq_hz;
    PBL_LOG(LOG_LEVEL_ALWAYS, "---555--timeout_ms=%ld", timeout_ms);
    new_timer_start(s_event_timer_id, timeout_ms, mmc5603_timer_handler, NULL, TIMER_START_FLAG_REPEATING);
  }
  ++s_use_refcount;
  PBL_LOG(LOG_LEVEL_ALWAYS, "---666--");
  mutex_unlock(s_mag_mutex);
  PBL_LOG(LOG_LEVEL_ALWAYS, "---777--");
}

// @brief 
void mag_release(void) {
  PBL_ASSERTN(s_initialized && s_use_refcount != 0);

  mutex_lock(s_mag_mutex);
  --s_use_refcount;
  if (s_use_refcount == 0) {
    if (s_event_timer_id) {
      new_timer_stop(s_event_timer_id);
      new_timer_delete(s_event_timer_id);
      s_event_timer_id = 0;
    }
    // 进入低功耗模式
    prv_enter_standby_mode();
    uint8_t raw_data[6];
    mmc5603_read(MMC5603_REG_DATA, sizeof(raw_data), raw_data);
    i2c_release(I2C_MMC5603);
  }
  mutex_unlock(s_mag_mutex);
}

// @brief 转换坐标系
static int16_t align_coord_system(int axis, uint8_t *raw_data) {
  int offset = 2 * BOARD_CONFIG_MAG.mag_config.axes_offsets[axis];
  bool do_invert = BOARD_CONFIG_MAG.mag_config.axes_inverts[axis];
  int16_t mag_field_strength = ((raw_data[offset] << 8) | raw_data[offset + 1]);
  mag_field_strength *= (do_invert ? -1 : 1);
  return (mag_field_strength);
}

// @brief 读取数据
MagReadStatus mag_read_data(MagData *data) {
  mutex_lock(s_mag_mutex);

  if (s_use_refcount == 0) {
    mutex_unlock(s_mag_mutex);
    return (MagReadMagOff);
  }

  MagReadStatus rv = MagReadSuccess;
  uint8_t raw_data[6];
  if (!mmc5603_read(MMC5603_REG_DATA, sizeof(raw_data), raw_data)) {
    rv = MagReadCommunicationFail;
    goto done;
  }

  // map raw data to watch coord system
  data->x = align_coord_system(0, &raw_data[0]);
  data->y = align_coord_system(1, &raw_data[0]);
  data->z = align_coord_system(2, &raw_data[0]);

done:
  mutex_unlock(s_mag_mutex);
  return (rv);
}

// @brief 修改采样率
bool mag_change_sample_rate(MagSampleRate rate) {
  mutex_lock(s_mag_mutex);

  if (s_use_refcount == 0) {
    mutex_unlock(s_mag_mutex);
    return (true);
  }

  bool success = false;
  if (rate == MagSampleRate5Hz) {
    s_freq_hz = 5;
  } else if (rate == MagSampleRate20Hz) {
    s_freq_hz = 20;
  } else {
    goto done;
  }
  
  mmc5603_write(MMC5603_REG_CTRL1, 0);
  mmc5603_write(MMC5603_REG_ODR, s_freq_hz);
  success = true;
done:
  mutex_unlock(s_mag_mutex);
  return (success);
}

// @brief 开始采样
void mag_start_sampling(void) {
  mag_use();
  mmc5603_write(MMC5603_REG_CTRL0, __BIT(7)|__BIT(5));
  mmc5603_write(MMC5603_REG_CTRL2, __BIT(4));
  mag_change_sample_rate(MagSampleRate5Hz);
}
