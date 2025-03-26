#include "board/board.h"
#include "drivers/flash/flash_impl.h"
#include "drivers/flash/qspi_flash.h"
#include "drivers/flash/qspi_flash_part_definitions.h"
#include "flash_region/flash_region.h"
#include "system/passert.h"
#include "system/status_codes.h"
#include "util/math.h"

bool flash_check_whoami(void) { return true; }

FlashAddress flash_impl_get_sector_base_address(FlashAddress addr) {
  return (addr & SECTOR_ADDR_MASK);
}

FlashAddress flash_impl_get_subsector_base_address(FlashAddress addr) {
  return (addr & SUBSECTOR_ADDR_MASK);
}

void flash_impl_enable_write_protection(void) {}

status_t flash_impl_write_protect(FlashAddress start_sector, FlashAddress end_sector) {
  return S_SUCCESS;
}

status_t flash_impl_unprotect(void) { return S_SUCCESS; }

status_t flash_impl_init(bool coredump_mode) { return S_SUCCESS; }

status_t flash_impl_get_erase_status(void) { return S_SUCCESS; }

status_t flash_impl_erase_subsector_begin(FlashAddress subsector_addr) { return S_SUCCESS; }
status_t flash_impl_erase_sector_begin(FlashAddress sector_addr) { return S_SUCCESS; }

status_t flash_impl_erase_suspend(FlashAddress sector_addr) { return S_SUCCESS; }

status_t flash_impl_erase_resume(FlashAddress sector_addr) { return S_SUCCESS; }

status_t flash_impl_read_sync(void *buffer_ptr, FlashAddress start_addr, size_t buffer_size) {
  PBL_ASSERT(buffer_size > 0, "flash_impl_read_sync() called with 0 bytes to read");
  return S_SUCCESS;
}

int flash_impl_write_page_begin(const void *buffer, const FlashAddress start_addr, size_t len) {
  return S_SUCCESS;
}

status_t flash_impl_get_write_status(void) { return S_SUCCESS; }

status_t flash_impl_enter_low_power_mode(void) { return S_SUCCESS; }
status_t flash_impl_exit_low_power_mode(void) { return S_SUCCESS; }

status_t flash_impl_set_burst_mode(bool burst_mode) {
  // NYI
  return S_SUCCESS;
}

status_t flash_impl_blank_check_sector(FlashAddress addr) { return S_SUCCESS; }
status_t flash_impl_blank_check_subsector(FlashAddress addr) { return S_SUCCESS; }

uint32_t flash_impl_get_typical_sector_erase_duration_ms(void) { return 150; }

uint32_t flash_impl_get_typical_subsector_erase_duration_ms(void) { return 50; }