#include "nrf_sdh_ble.h"

#include <bluetooth/id.h>
#include <bluetooth/bluetooth_types.h>

#include <string.h>

static char s_device_name[BT_DEVICE_NAME_BUFFER_SIZE];

void bt_driver_id_set_local_device_name(const char device_name[BT_DEVICE_NAME_BUFFER_SIZE]) {
  ble_gap_conn_sec_mode_t sec_mode;
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
  memcpy(s_device_name, device_name, sizeof(s_device_name));
  s_device_name[BT_DEVICE_NAME_BUFFER_SIZE - 1] = 0;
  sd_ble_gap_device_name_set(&sec_mode, (uint8_t *) s_device_name, strlen(s_device_name));
  /* XXX: need to update Extended Inquiry Response data, and advdata, with the new device_name, once advertising config code is written! */
}

/* bt_driver_id_copy_local_identity_address is in init.c, because we grab the identity address on boot */

void bt_driver_set_local_address(bool allow_cycling,
                                 const BTDeviceAddress *pinned_address) {
  /* XXX: allow_cycling should reset the address to the identity address,
   * and then set ble_gap_privacy_params_t->privacy_mode to enable cycling.
   * otherwise, if allow_cycling is off, we should turn off the privacy_mode
   * and set the pinned_address.  TI does not support allow_cycling, but we
   * should, since the Nordic softdevice makes it easy
   */

  if (pinned_address) {
    ble_gap_addr_t addr;
    memcpy(&addr.addr, pinned_address, 6);
    addr.addr_type = BLE_GAP_ADDR_TYPE_PUBLIC;
    sd_ble_gap_addr_set(&addr); /* XXX: check NRF_SUCCESS */
    /* XXX: if we switch to using Peer Manager library, this needs to use pm_id_addr_set */
  }
}

void bt_driver_id_copy_chip_info_string(char *dest, size_t dest_size) {
  /* this is only used by prompt code; it is unique-ish on TI and Dialog but
   * it need not be, really.  on TI, we use
   * bt_local_id_copy_address_mac_string to explode a string into the
   * buffer.  if we have other root identity information, this could be a
   * good place for it
   */
  strncpy(dest, "nRF52840", dest_size);
}

bool bt_driver_id_generate_private_resolvable_address(BTDeviceAddress *address_out) {
  /* XXX: This is obviously wrong, but in this case, the softdevice makes it
   * difficult!  the softdevice can generate private resolvable addresses
   * internally using its own IRK derivation mechanism, but that isn't
   * exposed to us!  so we would have to implement the IRK hash ourselves.
   *
   * but for now, this will get us off the ground.
   */
  bt_driver_id_copy_local_identity_address(address_out);
  return true;
}
