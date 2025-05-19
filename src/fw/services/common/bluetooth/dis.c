/*
 * Copyright 2024 Google LLC
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

#include <bluetooth/dis.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "board/board.h"
#include "process_management/pebble_process_info.h"
#include "mfg/mfg_info.h"
#include "mfg/mfg_serials.h"
#include "system/version.h"

_Static_assert(MODEL_NUMBER_LEN  >= MFG_HW_VERSION_SIZE + 1, "Size mismatch");
_Static_assert(MANUFACTURER_LEN  >= sizeof(BT_VENDOR_NAME), "Size mismatch");
_Static_assert(SERIAL_NUMBER_LEN >= MFG_SERIAL_NUMBER_SIZE + 1, "Size mismatch");
_Static_assert(FW_REVISION_LEN   >= sizeof(TINTIN_METADATA.version_tag), "Size mismatch");

static void prv_set_model_number(DisInfo *info) {
  mfg_info_get_hw_version(info->model_number, MODEL_NUMBER_LEN);
}

static void prv_set_manufacturer_name(DisInfo *info) {
  strncpy(info->manufacturer, BT_VENDOR_NAME, MANUFACTURER_LEN);
}

static void prv_set_serial_number(DisInfo *info) {
  mfg_info_get_serialnumber(info->serial_number, SERIAL_NUMBER_LEN);
}

static void prv_set_firmware_revision(DisInfo *info) {
  strncpy(info->fw_revision, (char*)TINTIN_METADATA.version_tag, FW_REVISION_LEN);
}

static void prv_set_software_revision(DisInfo *info) {
  // Fmt: xx.xx\0
  char sdk_version[SW_REVISION_LEN];
  sniprintf(sdk_version, SW_REVISION_LEN, "%2u.%02u",
            PROCESS_INFO_CURRENT_SDK_VERSION_MAJOR,
            PROCESS_INFO_CURRENT_SDK_VERSION_MINOR);
  strncpy(info->sw_revision, sdk_version, SW_REVISION_LEN);
}

void dis_get_info(DisInfo *info) {
  prv_set_model_number(info);
  prv_set_manufacturer_name(info);
  prv_set_serial_number(info);
  prv_set_firmware_revision(info);
  prv_set_software_revision(info);
}
