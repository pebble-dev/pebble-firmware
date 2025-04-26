#include "drivers/otp.h"

#include "system/passert.h"

#define NRF5_COMPATIBLE
#include <mcu.h>

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>

#include "nrfx_nvmc.h"

// nRF5's OTP is based on UICR, which is not *really* OTP ...  it can always
// be erased with, oh, say, ERASEUICR.  And it is in fact probable that it
// will get erased with developers doing a `nrf52_recover`.  But it *is* a
// region of flash that is distinct from SPI flash, which could *also* get
// erased, I suppose.
//
// On the other hand, a more pressing problem is that nRF52 only has 128
// bytes of UICR.  That one is more annoying -- that's only 4 slots worth! 
// But the good news, I suppose, is that since the OTP is not actually OTP,
// there is no need to fear -- if you screw it up, you can just reset it and
// try again and program the value you actually wanted.

#define OTP_SLOT_SIZE_BYTES 32

char * otp_get_slot(const uint8_t index) {
  PBL_ASSERTN(index < NUM_OTP_SLOTS);
  return (char *)(NRF_UICR->CUSTOMER) + index * OTP_SLOT_SIZE_BYTES;
}

bool otp_is_locked(const uint8_t index) {
  char *slot = otp_get_slot(index);
  for (int i = 0; i < OTP_SLOT_SIZE_BYTES; i++) {
    if (slot[i] != 0xFF) {
      return true;
    }
  }
  return false;
}

OtpWriteResult otp_write_slot(const uint8_t index, const char *value) {
  // This may interrupt Bluetooth timing, but we don't care, because we
  // should always be in mfg land for this.
  if (otp_is_locked(index)) {
    return OtpWriteFailAlreadyWritten;
  }

  // nrfx_nvmc_words_write needs word-aligned data to write.
  uint32_t otp_data[OTP_SLOT_SIZE_BYTES / 4]; 
  memcpy(otp_data, value, OTP_SLOT_SIZE_BYTES);

  nrfx_nvmc_words_write((uintptr_t)(NRF_UICR->CUSTOMER) + index * OTP_SLOT_SIZE_BYTES, otp_data, OTP_SLOT_SIZE_BYTES / 4);
  while (!nrfx_nvmc_write_done_check())
    ;

  // According to the spec, UICR changes aren't reflected until the system
  // is reset.  In practice, they seem to be, though.
  if (memcmp(otp_get_slot(index), value, OTP_SLOT_SIZE_BYTES)) {
    return OtpWriteFailCorrupt;
  }
  
  return OtpWriteSuccess;
}
