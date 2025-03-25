/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <bluetooth/bluetooth_types.h>
#include <bluetooth/responsiveness.h>
#include <host/ble_gap.h>
#include <nimble/ble.h>
#include <stdint.h>

#define BLE_UUID_SWIZZLE_(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) \
  a15, a14, a13, a12, a11, a10, a9, a8, a7, a6, a5, a4, a3, a2, a1, a0
#define BLE_UUID_SWIZZLE(x) BLE_UUID_SWIZZLE_(x)

void nimble_addr_to_pebble_addr(const ble_addr_t *addr, BTDeviceAddress *addr_out);

void pebble_device_to_nimble_addr(const BTDeviceInternal *device, ble_addr_t *addr_out);

void nimble_addr_to_pebble_device(const ble_addr_t *stack_addr, BTDeviceInternal *host_addr);

bool pebble_device_to_nimble_conn_handle(const BTDeviceInternal *device, uint16_t *handle);

void nimble_conn_params_to_pebble(struct ble_gap_conn_desc *desc, BleConnectionParams *params);

void pebble_conn_update_to_nimble(const BleConnectionParamsUpdateReq *req,
                                  struct ble_gap_upd_params *params);

void nimble_uuid_to_pebble(const ble_uuid_any_t *stack_uuid, Uuid *uuid);
