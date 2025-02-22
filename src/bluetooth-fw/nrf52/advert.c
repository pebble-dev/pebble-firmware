#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "ble_advdata.h"

#include "system/passert.h"

#include <bluetooth/bt_driver_advert.h>

static uint8_t s_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
static uint8_t s_advdata_buf[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
static uint8_t s_srdata_buf[BLE_GAP_ADV_SET_DATA_SIZE_MAX];
static ble_gap_adv_data_t s_advdata = {
  .adv_data = { .p_data = s_advdata_buf, .len = 0 },
  .scan_rsp_data = { .p_data = s_srdata_buf, .len = 0 },
};

bool bt_driver_advert_client_get_tx_power(int8_t *tx_power) {
  *tx_power = 6; // default power is +6 dBm, I believe?
  return true;
}

void bt_driver_advert_set_advertising_data(const BLEAdData *ad_data) {
  memcpy(s_advdata_buf, ad_data->data, ad_data->ad_data_length);
  s_advdata.adv_data.len = ad_data->ad_data_length;
  
  memcpy(s_srdata_buf, ad_data->data + ad_data->ad_data_length, ad_data->scan_resp_data_length);
  s_advdata.scan_rsp_data.len = ad_data->scan_resp_data_length;
  
  // no need to reconfigure the advertising job if it's not already created
  if (s_adv_handle == BLE_GAP_ADV_SET_HANDLE_NOT_SET)
    return;

  (void) sd_ble_gap_adv_set_configure(&s_adv_handle, &s_advdata, NULL);
}

bool bt_driver_advert_advertising_enable(uint32_t min_interval_ms, uint32_t max_interval_ms,
                                     bool enable_scan_resp) {
  ret_code_t rv;
  ble_gap_adv_params_t advparams;
  
  memset(&advparams, 0, sizeof(advparams));
  advparams.primary_phy = BLE_GAP_PHY_1MBPS;
  advparams.duration = BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED;
  advparams.properties.type = enable_scan_resp ? BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED
                                               : BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
  advparams.p_peer_addr = NULL;
  advparams.filter_policy = BLE_GAP_ADV_FP_ANY;
  advparams.interval = min_interval_ms * 8 / 5 /* why this became a whole number, I cannot rightly explain */;

  rv = sd_ble_gap_adv_set_configure(&s_adv_handle, &s_advdata, &advparams);
  PBL_ASSERTN(rv == NRF_SUCCESS);
  
  rv = sd_ble_gap_adv_start(s_adv_handle, BLE_CONN_CFG_TAG_DEFAULT);
  PBL_ASSERTN(rv == NRF_SUCCESS);

  return true;
}

void bt_driver_advert_advertising_disable(void) {
  (void) sd_ble_gap_adv_stop(s_adv_handle);
}

/* legacy: workarounds for Bluetopia, which we are definitely not */

bool bt_driver_advert_is_connectable(void) {
  return true;
}

bool bt_driver_advert_client_has_cycled(void) {
  return true;
}

void bt_driver_advert_client_set_cycled(bool has_cycled) {
}

bool bt_driver_advert_should_not_cycle(void) {
  return false;
}
