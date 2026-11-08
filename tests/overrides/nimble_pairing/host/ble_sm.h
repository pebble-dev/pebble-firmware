/* SPDX-FileCopyrightText: 2026 Geert van Dijk */
/* SPDX-License-Identifier: Apache-2.0 */

// The parts of NimBLE's host/ble_sm.h that pairing_confirm.c uses, without the rest of the host.

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define BLE_SM_IOACT_NONE 0
#define BLE_SM_IOACT_OOB 1
#define BLE_SM_IOACT_INPUT 2
#define BLE_SM_IOACT_DISP 3
#define BLE_SM_IOACT_NUMCMP 4
#define BLE_SM_IOACT_OOB_SC 5

struct ble_sm_io {
  uint8_t action;
  union {
    uint32_t passkey;
    uint8_t oob[16];
    uint8_t numcmp_accept;
  };
};

int ble_sm_inject_io(uint16_t conn_handle, struct ble_sm_io *pkey);
