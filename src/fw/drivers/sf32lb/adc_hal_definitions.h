#pragma once

#include "os/mutex.h"
#include "kernel/util/stop.h"
#include "freertos_types.h"
#include <stdbool.h>
#include <stdint.h>
#include "board/board.h"
#include "board/board_sf32lb.h"
#include "drivers/dma.h"
#include "register.h"
#include "bf0_hal_pinmux.h"
#include "adc.h"

struct sifli_adc
{
    ADC_HandleTypeDef ADC_Handler;
    SemaphoreHandle_t adc_semaphore;
    DMA_HandleTypeDef * DMA;
    const char *device_name;
    uint8_t adc_dma_flag;
    uint32_t adc_thd_reg;
};


