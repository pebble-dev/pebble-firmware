/* SPDX-FileCopyrightText: 2026 Geert van Dijk */
/* SPDX-License-Identifier: Apache-2.0 */

// The parts of NimBLE's host/ble_gap.h that pairing_confirm.c uses, without the rest of the host.

#pragma once

#include <stdint.h>

#define BLE_ERR_REM_USER_CONN_TERM 0x13

struct ble_gap_passkey_params {
  uint8_t action;
  uint32_t numcmp;
};

int ble_gap_terminate(uint16_t conn_handle, uint8_t hci_reason);
