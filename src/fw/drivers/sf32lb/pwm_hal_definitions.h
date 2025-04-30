#pragma once

#include "kernel/util/stop.h"
#include "freertos_types.h"
#include "os/mutex.h"
#include <stdbool.h>
#include <stdint.h>
#include "board/board.h"
#include "board/board_sf32lb.h"
#include "drivers/dma.h"
#include "bf0_hal.h"
#include "bf0_hal_def.h"
#include "bf0_hal_rcc.h"
#include "register.h"
#include "bf0_hal_pinmux.h"


#include "bf0_hal_tim.h"

struct pwm_configuration
{
    uint16_t channel; /* 0-n */
    uint8_t  is_comp; /* Is complementary*/
    uint8_t  reserved;
    uint32_t period;  /* unit:ns 1ns~4.29s:1Ghz~0.23hz */
    uint32_t pulse;   /* unit:ns (pulse<=period) */
    uint32_t break_dead;
    uint32_t dead_time; /*uint:ns if pclk=120MH, dead_time:0~136000ns*/
    uint32_t resolution;//
};

struct bf0_pwm
{
    GPT_HandleTypeDef       tim_handle;    /*!<General Purpose Timer(GPT) device object handle used in PWM*/
    TimerConfig*              tim_config;
    struct pwm_configuration    pwm_config;
    char                    *name;                         /*!<Device name*/
    
};

extern void tim_init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct);
extern void tim_preload(TIM_TypeDef* TIMx, uint16_t TIM_OCPreload);


