#include "drivers/vibe.h"

#include "board/board.h"
#include "drivers/gpio.h"
#include "drivers/i2c.h"
#include "drivers/ioexp.h"

#include <string.h>

static bool prv_read_register(uint8_t register_address, uint8_t *result) {
  i2c_use(I2C_AW86225);
  bool rv = i2c_read_register(I2C_AW86225, register_address, result);
  i2c_release(I2C_AW86225);
  return rv;
}

static bool prv_write_register(uint8_t register_address, uint8_t datum) {
  i2c_use(I2C_AW86225);
  bool rv = i2c_write_register(I2C_AW86225, register_address, datum);
  i2c_release(I2C_AW86225);
  return rv;
}

void vibe_init(void) {
  //ioexp_pin_set(IOE_VIBE_RST, IOEXP_HIGH);
  gpio_output_init(&BOARD_CONFIG.vibe_rst, GPIO_OType_PP, GPIO_Speed_2MHz);
  gpio_output_set(&BOARD_CONFIG.vibe_rst, true);
  uint8_t rv;
  bool found = prv_read_register(0x64, &rv);
  if (found) {
    PBL_LOG(LOG_LEVEL_DEBUG, "Found AW86225 with ID register ID:%02x", rv);
  } else {
    PBL_LOG(LOG_LEVEL_ERROR, "Failed to read the AW86225 ID register");
  }

  //ioexp_pin_set(IOE_VIBE_RST, IOEXP_LOW);
  gpio_output_set(&BOARD_CONFIG.vibe_rst, false);
}

void vibe_set_strength(int8_t strength) {
}

void vibe_ctl(bool on) {
}

void vibe_force_off(void) {
}

int8_t vibe_get_braking_strength(void) {
  // We only support the 0..100 range, just ask it to turn off
  return VIBE_STRENGTH_OFF;
}


void command_vibe_ctl(const char *arg) {

}
