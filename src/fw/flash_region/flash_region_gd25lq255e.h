/*
 * Copyright 2025 Core Devices LLC
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

#define PAGE_SIZE_BYTES (0x100)

#define SECTOR_SIZE_BYTES (0x10000)
#define SECTOR_ADDR_MASK (~(SECTOR_SIZE_BYTES - 1))

#define SUBSECTOR_SIZE_BYTES (0x1000)
#define SUBSECTOR_ADDR_MASK (~(SUBSECTOR_SIZE_BYTES - 1))

// A bit of preprocessor magic to help with automatically calculating flash region addresses
//////////////////////////////////////////////////////////////////////////////

#define FLASH_REGION_DEF(MACRO, arg)                                                      \
  /* Protectable region (512K, lower 1/64) BP4-0=10100 */                                 \
  MACRO(SAFE_FIRMWARE,           0x0080000 /*   512K */, arg) /* 0x0000000 - 0x007FFFF */ \
  /* Non-protectable region (31.5MB) */                                                   \
  MACRO(FIRMWARE_SCRATCH,        0x0100000 /*  1024K */, arg) /* 0x0080000 - 0x017FFFF */ \
  MACRO(SYSTEM_RESOURCES_BANK_0, 0x0100000 /*  1024K */, arg) /* 0x0180000 - 0x027FFFF */ \
  MACRO(SYSTEM_RESOURCES_BANK_1, 0x0100000 /*  1024K */, arg) /* 0x0280000 - 0x037FFFF */ \
  MACRO(FILESYSTEM,              0x1A50000 /* 26944K */, arg) /* 0x0380000 - 0x1DCFFFF */ \
  MACRO(RSVD2,                   0x0200000 /*  2048K */, arg) /* 0x1DD0000 - 0x1FCFFFF */ \
  MACRO(DEBUG_DB,                0x0020000 /*   128K */, arg) /* 0x1FD0000 - 0x1FEFFFF */ \
  MACRO(RSVD3,                   0x000E000 /*    56K */, arg) /* 0x1FF0000 - 0x1FFDFFF */ \
  MACRO(MFG_INFO,                0x0001000 /*     4K */, arg) /* 0x1FFE000 - 0x1FFEFFF */ \
  MACRO(SHARED_PRF_STORAGE,      0x0001000 /*     4K */, arg) /* 0x1FFF000 - 0x1FFFFFF */

#include "flash_region_def_helper.h"

// Flash region _BEGIN and _END addresses
//////////////////////////////////////////////////////////////////////////////

#define FLASH_REGION_FIRMWARE_SCRATCH_BEGIN FLASH_REGION_START_ADDR(FIRMWARE_SCRATCH)
#define FLASH_REGION_FIRMWARE_SCRATCH_END FLASH_REGION_END_ADDR(FIRMWARE_SCRATCH)

#define FLASH_REGION_SYSTEM_RESOURCES_BANK_0_BEGIN FLASH_REGION_START_ADDR(SYSTEM_RESOURCES_BANK_0)
#define FLASH_REGION_SYSTEM_RESOURCES_BANK_0_END FLASH_REGION_END_ADDR(SYSTEM_RESOURCES_BANK_0)

#define FLASH_REGION_SYSTEM_RESOURCES_BANK_1_BEGIN FLASH_REGION_START_ADDR(SYSTEM_RESOURCES_BANK_1)
#define FLASH_REGION_SYSTEM_RESOURCES_BANK_1_END FLASH_REGION_END_ADDR(SYSTEM_RESOURCES_BANK_1)

#define FLASH_REGION_SAFE_FIRMWARE_BEGIN FLASH_REGION_START_ADDR(SAFE_FIRMWARE)
#define FLASH_REGION_SAFE_FIRMWARE_END FLASH_REGION_END_ADDR(SAFE_FIRMWARE)

#define FLASH_REGION_DEBUG_DB_BEGIN FLASH_REGION_START_ADDR(DEBUG_DB)
#define FLASH_REGION_DEBUG_DB_END FLASH_REGION_END_ADDR(DEBUG_DB)
#define FLASH_DEBUG_DB_BLOCK_SIZE SUBSECTOR_SIZE_BYTES

#define FLASH_REGION_FILESYSTEM_BEGIN FLASH_REGION_START_ADDR(FILESYSTEM)
#define FLASH_REGION_FILESYSTEM_END FLASH_REGION_END_ADDR(FILESYSTEM)
#define FLASH_FILESYSTEM_BLOCK_SIZE SUBSECTOR_SIZE_BYTES

#define FLASH_REGION_SHARED_PRF_STORAGE_BEGIN FLASH_REGION_START_ADDR(SHARED_PRF_STORAGE)
#define FLASH_REGION_SHARED_PRF_STORAGE_END FLASH_REGION_END_ADDR(SHARED_PRF_STORAGE)

#define FLASH_REGION_MFG_INFO_BEGIN FLASH_REGION_START_ADDR(MFG_INFO)
#define FLASH_REGION_MFG_INFO_END FLASH_REGION_END_ADDR(MFG_INFO)

#define BOARD_NOR_FLASH_SIZE FLASH_REGION_START_ADDR(_COUNT)

// Static asserts to make sure everything worked out
//////////////////////////////////////////////////////////////////////////////

// make sure all the sizes are multiples of the subsector size (4k)
FLASH_REGION_SIZE_CHECK(SUBSECTOR_SIZE_BYTES)

// make sure the total size is what we expect (32mb)
_Static_assert(BOARD_NOR_FLASH_SIZE == 0x2000000, "Flash size should be 32mb");
