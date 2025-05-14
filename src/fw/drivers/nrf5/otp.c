#include "drivers/otp.h"

#include "board/board.h"
#include "drivers/qspi.h"
#include "drivers/flash/qspi_flash_definitions.h"
#include "kernel/util/delay.h"
#include "system/passert.h"

#define NRF5_COMPATIBLE
#include <mcu.h>
#include <nrfx.h>
#include <nrfx_qspi.h>

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>

static bool prv_prog_security_register_byte(uint32_t index, uint8_t value);
static bool prv_read_security_register_byte(uint32_t index, uint8_t *value);
static bool prv_erase_security_register();
static bool prv_wait_for_flash_not_busy();

#define OTP_SLOT_SIZE (32)
static char otp_shadow[NUM_OTP_SLOTS][OTP_SLOT_SIZE];

char * otp_get_slot(const uint8_t index) {
  PBL_ASSERTN(index < NUM_OTP_SLOTS);

   for (size_t i = 0; i < OTP_SLOT_SIZE; i++) {
    bool success = prv_read_security_register_byte(
      index * OTP_SLOT_SIZE + i, (uint8_t*)&otp_shadow[index][i]);
    PBL_ASSERTN(success);
  }

  return (char*)otp_shadow[index];
}

// otp_get_lock() deliberately unimplemented: no one calls it

bool otp_is_locked(const uint8_t index) {
  uint8_t lock_byte = 0xff;
  bool success = prv_read_security_register_byte(index * OTP_SLOT_SIZE + OTP_SLOT_SIZE - 1, &lock_byte);
  PBL_ASSERTN(success);
  return (lock_byte == 0x00);
}

OtpWriteResult otp_write_slot(const uint8_t index, const char *value) {
  PBL_ASSERTN(index < NUM_OTP_SLOTS);
  PBL_ASSERTN(strlen(value) < OTP_SLOT_SIZE - 1);
  
  if (otp_is_locked(index)) {
    return OtpWriteFailAlreadyWritten;
  }

  for (size_t i = 0; i < strlen(value) + 1; i++) {
    prv_prog_security_register_byte(index * OTP_SLOT_SIZE + i, (uint8_t)value[i]);
    otp_shadow[index][i] = value[i];
  }

  // "lock" flag since QSPI flash OTP does not have the concept of slot locking
  prv_prog_security_register_byte(index * OTP_SLOT_SIZE + OTP_SLOT_SIZE - 1, 0);

  return OtpWriteSuccess;
}

#define SECURITY_REGISTER_ID (0x20)

static bool prv_prog_security_register_byte(uint32_t index, uint8_t value) {
  nrf_qspi_cinstr_conf_t instr = NRFX_QSPI_DEFAULT_CINSTR(0x42, 6);
  instr.io2_level = true;
  instr.io3_level = true;
  instr.wren = true;
  uint8_t out[5] = {0x00, 0x00, SECURITY_REGISTER_ID, 0x00, 0xff};
  out[2] |= (index >> 8) & 0x03;
  out[3] = (index & 0xff);
  out[4] = value;
  nrfx_err_t err = nrfx_qspi_cinstr_xfer(&instr, out, NULL);
  if (err != NRFX_SUCCESS) return false;
  return prv_wait_for_flash_not_busy();
}

static bool prv_read_security_register_byte(uint32_t index, uint8_t *value) {
  nrf_qspi_cinstr_conf_t instr = NRFX_QSPI_DEFAULT_CINSTR(0x48, 7);
  instr.io2_level = true;
  instr.io3_level = true;
  uint8_t out[6] = {0x00, 0x00, SECURITY_REGISTER_ID, 0x00, 0xff, 0xff};
  out[2] |= (index >> 8) & 0x03;
  out[3] = (index & 0xff);
  uint8_t in[6] = {0};
  nrfx_err_t err = nrfx_qspi_cinstr_xfer(&instr, out, in);
  *value = in[5];
  
  return (err == NRFX_SUCCESS);
}

static bool prv_erase_security_register() {
  nrf_qspi_cinstr_conf_t instr = NRFX_QSPI_DEFAULT_CINSTR(0x44, 5);
  instr.io2_level = true;
  instr.io3_level = true;
  instr.wren = true;
  uint8_t out[4] = {0x00, 0x00, SECURITY_REGISTER_ID, 0x00};
  nrfx_err_t err = nrfx_qspi_cinstr_xfer(&instr, out, NULL);
  if (err != NRFX_SUCCESS) return false;
  return prv_wait_for_flash_not_busy();
}

static bool prv_wait_for_flash_not_busy() {
  nrf_qspi_cinstr_conf_t instr = NRFX_QSPI_DEFAULT_CINSTR(
    QSPI_FLASH->state->part->instructions.rdsr1, 2);
  instr.io2_level = true;
  instr.io3_level = true;
  uint8_t sr1 = 0xff;
  int loops = 0;
  while (1) {
    nrfx_err_t err = nrfx_qspi_cinstr_xfer(&instr, NULL, &sr1);
  if (err != NRFX_SUCCESS) return false;
  if (!(sr1 & QSPI_FLASH->state->part->status_bit_masks.busy)) break;
  if (++loops > 1000) {
    PBL_LOG(LOG_LEVEL_ERROR, "Timeout waiting for flash to become not busy");
    return false;
  }
  delay_us(50);
  }
  return true;
}

#if MANUFACTURING_FW
#include "console/prompt.h"

void command_otp_erase(void) {
  if (prv_erase_security_register()) {
    prompt_send_response("OK");
  } else {
    prompt_send_response("Failed");
  }
}

void command_otp_freeze(void) {
  prompt_send_response("not implemented");
}
#endif