#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "board/board.h"
#include "drivers/sf32lb/i2c_hal_definitions.h"
#ifdef HAL_I2C_MODULE_ENABLED
#if defined(I2C1)
extern bf0_i2c_t *const bf0_i2c1;
#endif
#if defined(I2C2)
extern bf0_i2c_t *const bf0_i2c2;
#endif
#if defined(I2C3)
extern bf0_i2c_t *const bf0_i2c3;
#endif
#if defined(I2C4)
extern bf0_i2c_t *const bf0_i2c4;
#endif
#if defined(I2C5)
extern bf0_i2c_t *const bf0_i2c5;
#endif
#if defined(I2C6)
extern bf0_i2c_t *const bf0_i2c6;
#endif

extern void i2c_init();
extern int rt_i2c_configure(bf0_i2c_t *objs, struct rt_i2c_configuration *cfg);
extern int rt_i2c_transfer(bf0_i2c_t *objs, struct rt_i2c_msg msgs[], uint32_t num);
#endif
