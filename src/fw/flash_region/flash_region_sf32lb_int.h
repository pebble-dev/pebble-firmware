#pragma once

#define SECTOR_SIZE_BYTES 0x10000
#define SECTOR_ADDR_MASK (~(SECTOR_SIZE_BYTES - 1))

#define SUBSECTOR_SIZE_BYTES 0x1000
#define SUBSECTOR_ADDR_MASK (~(SUBSECTOR_SIZE_BYTES - 1))
#define FLASH_FILESYSTEM_BLOCK_SIZE SUBSECTOR_SIZE_BYTES

// Filesystem layout
///////////////////////////////////////
// TODO