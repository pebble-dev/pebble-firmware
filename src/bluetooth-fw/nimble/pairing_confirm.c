/* SPDX-FileCopyrightText: 2025 Google LLC */
/* SPDX-FileCopyrightText: 2026 Geert van Dijk */
/* SPDX-License-Identifier: Apache-2.0 */

#include "nimble_pairing.h"

#include <bluetooth/pairing_confirm.h>
#include <drivers/rng.h>
#include <host/ble_hs.h>
#include <host/ble_sm.h>
#include <stdint.h>
#include <system/logging.h>

// IO action of the pairing currently waiting for the user. Only one pairing runs at a time.
static uint8_t s_pending_action = BLE_SM_IOACT_NONE;

static uint32_t prv_random_passkey(void) {
  uint32_t random = 0;
  if (!rng_rand(&random)) {
    PBL_LOG(LOG_LEVEL_WARNING, "RNG failed, passkey is not random");
  }
  return random % 1000000;
}

uint32_t nimble_pairing_handle_passkey_action(uint16_t conn_handle,
                                              const struct ble_gap_passkey_params *params) {
  s_pending_action = params->action;

  switch (params->action) {
    case BLE_SM_IOACT_NUMCMP:
      return params->numcmp;

    case BLE_SM_IOACT_DISP: {
      const uint32_t passkey = prv_random_passkey();
      struct ble_sm_io io = {
          .action = BLE_SM_IOACT_DISP,
          .passkey = passkey,
      };
      const int rc = ble_sm_inject_io(conn_handle, &io);
      if (rc != 0) {
        PBL_LOG(LOG_LEVEL_ERROR, "Failed to set pairing passkey: rc=0x%04x", (uint16_t)rc);
      }
      return passkey;
    }

    default:
      // The watch advertises DisplayYesNo, so it is never asked to type a passkey or use OOB.
      PBL_LOG(LOG_LEVEL_ERROR, "Unsupported pairing IO action %u", params->action);
      return 0;
  }
}

void bt_driver_pairing_confirm(const PairingUserConfirmationCtx *ctx, bool is_confirmed) {
  uint16_t conn_handle = (uintptr_t)ctx;

  if (s_pending_action == BLE_SM_IOACT_DISP) {
    // The passkey went to the stack when it was shown and the phone does the checking; the watch
    // only needs to act when the user rejects the pairing.
    if (!is_confirmed) {
      ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }
    return;
  }

  struct ble_sm_io key = {
      .action = BLE_SM_IOACT_NUMCMP,
      .numcmp_accept = is_confirmed,
  };
  int rc = ble_sm_inject_io(conn_handle, &key);

  PBL_LOG_D(LOG_DOMAIN_BT, LOG_LEVEL_DEBUG, "ble_sm_inject_io rc=0x%04x", (uint16_t)rc);
}
