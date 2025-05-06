#pragma once

#include "os/mutex.h"
#include "kernel/util/stop.h"
#include "freertos_types.h"
#include <stdbool.h>
#include <stdint.h>
#include "os/mutex.h"
#include "board/board.h"
#include "drivers/dma.h"
#include "bf0_hal.h"
#include "register.h"
#include "../display.h"

/**
  * @brief  JDI Registers
  */
#define REG_LCD_ID             0x04
#define REG_SLEEP_IN           0x10
#define REG_SLEEP_OUT          0x11
#define REG_PARTIAL_DISPLAY    0x12
#define REG_DISPLAY_INVERSION  0x21
#define REG_DISPLAY_OFF        0x28
#define REG_DISPLAY_ON         0x29
#define REG_WRITE_RAM          0x2C
#define REG_READ_RAM           0x2E
#define REG_CASET              0x2A
#define REG_RASET              0x2B

#define REG_TEARING_EFFECT     0x35

#define REG_IDLE_MODE_OFF      0x38
#define REG_IDLE_MODE_ON       0x39
#define REG_COLOR_MODE         0x3A
#define REG_WBRIGHT            0x51

#define REG_VDV_VRH_EN         0xC2
#define REG_VDV_SET            0xC4

/**
  * @brief JDI chip IDs
  */
#define THE_LCD_ID                  0x1d1

/**
  * @brief  JDI Size
  */
#define  THE_LCD_PIXEL_WIDTH    ((uint16_t)240)
#define  THE_LCD_PIXEL_HEIGHT   ((uint16_t)240)

typedef enum {
  DISPLAY_STATE_IDLE,
  DISPLAY_STATE_WRITING
} DisplayState;

typedef struct {
  DisplayState state;
  NextRowCallback get_next_row;
  UpdateCompleteCallback complete;
} DisplayContext;


