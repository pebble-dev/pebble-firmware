#include "drivers/watchdog.h"

#include "util/bitset.h"
#include "system/logging.h"

#define NRF5_COMPATIBLE
#include <mcu.h>

#include <nrfx.h>
#include <helpers/nrfx_reset_reason.h>
#include <hal/nrf_wdt.h>

#include <inttypes.h>

void watchdog_init(void) {
  nrf_wdt_reload_request_enable(NRF_WDT, NRF_WDT_RR0);
  /* WDT expiration: 8s */
  nrf_wdt_reload_value_set(NRF_WDT, 32768 * 8);
}

void watchdog_start(void) {
  nrf_wdt_task_trigger(NRF_WDT, NRF_WDT_TASK_START);
}

void watchdog_feed(void) {
  nrf_wdt_reload_request_set(NRF_WDT, NRF_WDT_RR0);
}

bool watchdog_check_reset_flag(void) {
  return (nrfx_reset_reason_get() & NRFX_RESET_REASON_DOG_MASK) != 0;
}

McuRebootReason watchdog_clear_reset_flag(void) {
  uint32_t reason = nrfx_reset_reason_get();
  nrfx_reset_reason_clear(0xFFFFFFFF);

  McuRebootReason mcu_reboot_reason = {
    .brown_out_reset = 0,
    .pin_reset = (reason & NRFX_RESET_REASON_RESETPIN_MASK) != 0,
    .power_on_reset = (reason & NRFX_RESET_REASON_VBUS_MASK) != 0,
    .software_reset = (reason & NRFX_RESET_REASON_SREQ_MASK) != 0,
    .independent_watchdog_reset = (reason & NRFX_RESET_REASON_DOG_MASK) != 0,
    .window_watchdog_reset = 0,
    .low_power_manager_reset = 0,
  };

  return mcu_reboot_reason;
}
