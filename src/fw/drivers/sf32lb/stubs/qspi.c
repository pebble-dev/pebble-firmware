#include "drivers/qspi.h"
#include "drivers/qspi_definitions.h"

#include "drivers/flash/qspi_flash.h"
#include "drivers/flash/qspi_flash_definitions.h"

#include "board/board.h"
#include "drivers/dma.h"
#include "drivers/flash/flash_impl.h"
#include "drivers/gpio.h"
#include "drivers/periph_config.h"
#include "flash_region/flash_region.h"
#include "kernel/util/delay.h"
#include "kernel/util/sleep.h"
#include "kernel/util/stop.h"
#include "mcu/cache.h"
#include "system/logging.h"
#include "system/passert.h"
#include "util/math.h"

#include <mcu.h>


status_t flash_impl_set_nvram_erase_status(bool is_subsector,
                                           FlashAddress addr) {
  return S_SUCCESS;
}

status_t flash_impl_clear_nvram_erase_status(void) {
  return S_SUCCESS;
}

status_t flash_impl_get_nvram_erase_status(bool *is_subsector,
                                           FlashAddress *addr) {
  // XXX
  return S_FALSE;
}
