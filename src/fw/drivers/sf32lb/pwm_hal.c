
#include "drivers/gpio.h"
#include "drivers/periph_config.h"
#include "drivers/pwm.h"
#include "drivers/timer.h"
#include "system/passert.h"
#include "board/board_sf32lb.h"
#include "pwm_hal_definitions.h"



  
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))


#define MAX_PERIOD_GPT (0xFFFF)
#define MAX_PERIOD_ATM (0xFFFFFFFF)
#define MIN_PERIOD 3
#define MIN_PULSE 1


#define PWM2_CORE CORE_ID_HCPU
#define PWM3_CORE CORE_ID_HCPU
#define PWM4_CORE CORE_ID_LCPU
#define PWM5_CORE CORE_ID_LCPU
#define PWM6_CORE CORE_ID_LCPU
#define PWMA1_CORE CORE_ID_HCPU
#define PWMA2_CORE CORE_ID_HCPU



#ifdef hwp_gptim1
TimerConfig gptime1_config =
{
    .peripheral = GPTIM1,
};

#define PWM2_CONFIG                             \
{                                           \
   .tim_handle.Instance     = hwp_gptim1,       \
   .tim_handle.core         = PWM2_CORE,    \
   .name                    = "pwm2",       \
   .tim_config              = &gptime1_config\
}
#endif /* GPTIM1 */

#ifdef hwp_gptim2
TimerConfig gptime2_config =
{
    .peripheral = GPTIM2
};

#define PWM3_CONFIG                             \
{                                           \
   .tim_handle.Instance     = hwp_gptim2,         \
   .tim_handle.core         = PWM3_CORE,    \
   .name                    = "pwm3",       \
   .tim_config              = &gptime2_config\
}
#endif /* GPTIM2 */

#ifdef hwp_gptim3
TimerConfig gptime3_config =
{
    .peripheral = GPTIM3,
};

#define PWM4_CONFIG                             \
{                                           \
   .tim_handle.Instance     = hwp_gptim3,         \
   .tim_handle.core         = PWM4_CORE,    \
   .name                    = "pwm4",       \
   .tim_config              = &gptime3_config\
}

#endif /* GPTIM3 */

#ifdef hwp_gptim4
TimerConfig gptime4_config =
{
    .peripheral = GPTIM4
};
#define PWM5_CONFIG                             \
{                                           \
   .tim_handle.Instance     = hwp_gptim4,         \
   .tim_handle.core         = PWM5_CORE,    \
   .name                    = "pwm5",       \
   .tim_config             = &gptime4_config             \
}
#endif /* GPTIM4 */

#ifdef hwp_gptim5
TimerConfig gptime5_config =
{
    .peripheral = GPTIM5
};

#define PWM6_CONFIG                             \
{                                           \
   .tim_handle.Instance     = hwp_gptim5,         \
   .tim_handle.core         = PWM6_CORE,    \
   .name                    = "pwm6",       \
   .tim_config              = &gptime5_config\
}
#endif /* GPTIM5 */

#ifdef hwp_atim1
TimerConfig atim1_config =
{
    .peripheral = (GPT_TypeDef *)hwp_atim1
};
#define PWMA1_CONFIG                             \
{                                           \
   .tim_handle.Instance     = (GPT_TypeDef *)hwp_atim1,         \
   .tim_handle.core         = PWMA1_CORE,    \
   .name                    = "pwma1",       \
   .tim_config             = &atim1_config       \
}
#endif /* BSP_USING_PWM_A1 */

#ifdef hwp_atim2
TimerConfig atim2_config =
{
    .peripheral = (GPT_TypeDef *)hwp_atim2
};
#define PWMA2_CONFIG                             \
{                                           \
   .tim_handle.Instance     = (GPT_TypeDef *)hwp_atim2,         \
   .tim_handle.core         = PWMA2_CORE,    \
   .name                    = "pwma2",       \
   .tim_config              = &atim2_config  \
}
#endif /* ATIM2 */

enum
{
#ifdef hwp_gptim1
    PWM2_INDEX,
#endif
#ifdef hwp_gptim2
    PWM3_INDEX,
#endif
#ifdef hwp_gptim3
    PWM4_INDEX,
#endif
#ifdef hwp_gptim4
    PWM5_INDEX,
#endif
#ifdef hwp_gptim5
    PWM6_INDEX,
#endif

#ifdef hwp_atim1
    PWMA1_INDEX,
#endif
#ifdef hwp_atim2
    PWMA2_INDEX,
#endif
    PMM_MAX_INDEX,

};

static struct bf0_pwm bf0_pwm_obj[] =
{
#ifdef hwp_gptim1
    PWM2_CONFIG,
#endif

#ifdef hwp_gptim2
    PWM3_CONFIG,
#endif

#ifdef hwp_gptim3
    PWM4_CONFIG,
#endif

#ifdef hwp_gptim4
    PWM5_CONFIG,
#endif

#ifdef hwp_gptim5
    PWM6_CONFIG,
#endif
#ifdef hwp_atim1
    PWMA1_CONFIG,
#endif

#ifdef hwp_atim2
    PWMA2_CONFIG,
#endif

};
struct bf0_pwm *  find_tim_obj(TIM_TypeDef* TIMx)
{
    struct bf0_pwm * pwm_obj = NULL;
    if(!PMM_MAX_INDEX)
        return NULL;
    for(int i = 0; i < PMM_MAX_INDEX;i++)
    {
        if(bf0_pwm_obj[i].tim_handle.Instance == TIMx)
        {
            pwm_obj = &(bf0_pwm_obj[i]);
            break;
        }
    }
    return pwm_obj;
}


// Note: pwm peripheral needs to be enabled before the duty cycle can be set
void pwm_set_duty_cycle(const PwmConfig *pwm, uint32_t duty_cycle)
{
    struct bf0_pwm * p_pwm_obj = container_of(&(pwm->timer), struct bf0_pwm, tim_config);
    GPT_HandleTypeDef *htim = &(p_pwm_obj->tim_handle);
    

    uint32_t period, pulse;
    uint32_t GPT_clock, psc;
    /* Converts the channel number to the channel number of Hal library */
    uint32_t channel = 0x04 * (pwm->state->channel - 1);
    uint32_t max_period = MAX_PERIOD_GPT;

#ifdef HAL_ATIM_MODULE_ENABLED
    if (IS_GPT_ADVANCED_INSTANCE(htim->Instance) != RESET)
        max_period = MAX_PERIOD_ATM;
#endif

#ifdef SF32LB52X
    if (htim->Instance == hwp_gptim2)
        GPT_clock = 24000000;
    else
#endif
    GPT_clock = HAL_RCC_GetPCLKFreq(htim->core, 1);
    PBL_LOG(LOG_LEVEL_INFO, "pwm use PCLK = %d;\n", (int)GPT_clock);

    /* Convert nanosecond to frequency and duty cycle. 1s = 1 * 1000 * 1000 * 1000 ns */
    GPT_clock /= 1000000UL;
    period = (unsigned long long)p_pwm_obj->pwm_config.period * GPT_clock / 1000ULL ;
    psc = period / max_period + 1;
    period = period / psc;
    __HAL_GPT_SET_PRESCALER(htim, psc - 1);

    PBL_LOG(LOG_LEVEL_INFO,"psc %d, Period %d,", (int)psc, (int)period);

    if (period < MIN_PERIOD)
    {
        period = MIN_PERIOD;
    }
    __HAL_GPT_SET_AUTORELOAD(htim, period - 1);
    /* transfer cycle to ns*/
    pulse = duty_cycle * p_pwm_obj->pwm_config.period/p_pwm_obj->pwm_config.resolution;

    pulse = (unsigned long long)pulse* GPT_clock / psc / 1000ULL;
    PBL_LOG(LOG_LEVEL_INFO, "Pulse %d", (int)pulse);
    
    if (pulse < MIN_PULSE)
    {
        pulse = MIN_PULSE;
    }
    else if (pulse >= period)       /*if pulse reach to 100%, need set pulse = period + 1, because pulse = period, the real percentage = 99.9983%  */
    {
        pulse = period + 1;
    }
    __HAL_GPT_SET_COMPARE(htim, channel, pulse - 1);
    //__HAL_GPT_SET_COUNTER(htim, 0);

    /* Update frequency value */
    HAL_GPT_GenerateEvent(htim, GPT_EVENTSOURCE_UPDATE);

    return ;

}




void pwm_enable(const PwmConfig *pwm, bool enable)
{
    struct bf0_pwm * p_pwm_obj = container_of(&(pwm->timer), struct bf0_pwm, tim_config);
    GPT_HandleTypeDef *htim = &(p_pwm_obj->tim_handle);
    
    /* Converts the channel number to the channel number of Hal library */
    uint32_t channel = 0x04 * (pwm->state->channel - 1);

    if (!enable)
    {
        HAL_GPT_PWM_Stop(htim, channel);
#ifdef HAL_ATIM_MODULE_ENABLED
        if (pwm->state->is_comp)
            HAL_TIMEx_PWMN_Stop(htim, channel);
#endif
    }
    else
    {
        GPT_OC_InitTypeDef oc_config = {0};
        oc_config.OCMode = GPT_OCMODE_PWM1;
        oc_config.Pulse = __HAL_GPT_GET_COMPARE(htim, channel);
        oc_config.OCPolarity = GPT_OCPOLARITY_HIGH;
        oc_config.OCFastMode = GPT_OCFAST_DISABLE;
        if (HAL_GPT_PWM_ConfigChannel(htim, &oc_config, channel) != HAL_OK)
        {
            PBL_LOG(LOG_LEVEL_ERROR, "%x channel %d config failed", (unsigned int )htim, (int)pwm->state->channel);

            return ;
        }

        HAL_GPT_PWM_Start(htim, channel);
#ifdef HAL_ATIM_MODULE_ENABLED
        if (pwm->state->is_comp)
            HAL_TIMEx_PWMN_Start(htim, channel);
#endif
    }

    return ;
}


static int bf0_hw_pwm_init(struct bf0_pwm *device)
{
    int ret = 0;
    GPT_HandleTypeDef *tim = NULL;
    GPT_ClockConfigTypeDef clock_config = {0};

    PBL_ASSERTN(device != NULL);

    tim = (GPT_HandleTypeDef *)&device->tim_handle;

    /* configure the timer to pwm mode */
    tim->Init.Prescaler = 0;
    tim->Init.CounterMode = GPT_COUNTERMODE_UP;
    tim->Init.Period = 0;

    if (HAL_GPT_Base_Init(tim) != HAL_OK)
    {
        PBL_LOG(LOG_LEVEL_ERROR, "%s time base init failed", device->name);
        
        goto __exit;
    }

    clock_config.ClockSource = GPT_CLOCKSOURCE_INTERNAL;
    if (HAL_GPT_ConfigClockSource(tim, &clock_config) != HAL_OK)
    {
        PBL_LOG(LOG_LEVEL_ERROR,"%s clock init failed", device->name);
        goto __exit;
    }

    if (HAL_GPT_PWM_Init(tim) != HAL_OK)
    {
        PBL_LOG(LOG_LEVEL_ERROR,"%s pwm init failed", device->name);
        goto __exit;
    }
    ret = 1;
      /* pwm pin configuration */
      //HAL_GPT_MspPostInit(tim);
  
  /* enable update request source */
  __HAL_GPT_URS_ENABLE(tim);
  
  __exit:
      return ret;
}

void tim_init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct)
{
  struct bf0_pwm * p_pwm_obj = find_tim_obj(TIMx);
  if(p_pwm_obj)
  {
      bf0_hw_pwm_init(p_pwm_obj);
  }
  
  return ;
}

void tim_preload(TIM_TypeDef* TIMx, uint16_t TIM_OCPreload)
{
    struct bf0_pwm * p_pwm_obj = find_tim_obj(TIMx);

    if(p_pwm_obj)
    {
        p_pwm_obj->pwm_config.period = TIM_OCPreload;
    }
    return ;

}
void pwm_hal_pins_set_gpio(const PwmConfig *pwm)
{
    int pad_pwm;
    int hcpu_pwm;
    pin_function func_pwm = GPTIM1_CH1;
    #ifdef hwp_gptim1
    if(pwm->timer.peripheral == hwp_gptim1)
    {
        if(pwm->state->channel ==1)
            func_pwm = GPTIM1_CH1;
        else if(pwm->state->channel == 2)
            func_pwm = GPTIM1_CH2;
        else if(pwm->state->channel == 3)
            func_pwm = GPTIM1_CH3;
        else if(pwm->state->channel == 4)
            func_pwm = GPTIM1_CH4;
    }
    #endif
    #ifdef hwp_gptim2
    if(pwm->timer.peripheral == hwp_gptim2)
    {
        if(pwm->state->channel ==1)
            func_pwm = GPTIM2_CH1;
        else if(pwm->state->channel == 2)
            func_pwm = GPTIM2_CH2;
        else if(pwm->state->channel == 3)
            func_pwm = GPTIM2_CH3;
        else if(pwm->state->channel == 4)
            func_pwm = GPTIM2_CH4;
    }
    #endif
    #ifdef hwp_gptim3
    if(pwm->timer.peripheral == hwp_gptim3)
    {
        if(pwm->state->channel ==1)
            func_pwm = GPTIM3_CH1;
        else if(pwm->state->channel == 2)
            func_pwm = GPTIM3_CH2;
        else if(pwm->state->channel == 3)
            func_pwm = GPTIM3_CH3;
        else if(pwm->state->channel == 4)
            func_pwm = GPTIM3_CH4;
    }
    #endif
    #ifdef hwp_gptim4
    if(pwm->timer->peripheral == hwp_gptim4)
    {
        if(pwm->state->channel ==1)
            func_pwm = GPTIM4_CH1;
        else if(pwm->state->channel == 2)
            func_pwm = GPTIM4_CH2;
        else if(pwm->state->channel == 3)
            func_pwm = GPTIM4_CH3;
        else if(pwm->state->channel == 4)
            func_pwm = GPTIM4_CH4;
    }
    #endif
    #ifdef hwp_gptim5
    if(pwm->timer.peripheral == hwp_gptim5)
    {
        if(pwm->state->channel ==1)
            func_pwm = GPTIM5_CH1;
        else if(pwm->state->channel == 2)
            func_pwm = GPTIM5_CH2;
        else if(pwm->state->channel == 3)
            func_pwm = GPTIM5_CH3;
        else if(pwm->state->channel == 4)
            func_pwm = GPTIM5_CH4;
    }
    #endif
    #ifdef hwp_atim1
    if(pwm->timer.peripheral == (GPT_TypeDef *)hwp_atim1)
    {
        if(pwm->state->channel ==1)
            func_pwm = ATIM1_CH1;
        else if(pwm->state->channel == 2)
            func_pwm = ATIM1_CH2;
        else if(pwm->state->channel == 3)
            func_pwm = ATIM1_CH3;
        else if(pwm->state->channel == 4)
            func_pwm = ATIM1_CH4;
    }
    #endif
    #ifdef hwp_atim2
    if(pwm->timer.peripheral == (GPT_TypeDef *)hwp_atim2)
    {
        if(pwm->state->channel ==1)
            func_pwm = ATIM2_CH1;
        else if(pwm->state->channel == 2)
            func_pwm = ATIM2_CH2;
        else if(pwm->state->channel == 3)
            func_pwm = ATIM2_CH3;
        else if(pwm->state->channel == 4)
            func_pwm = ATIM2_CH4;
    }
    #endif

    pad_pwm = pwm->output.gpio_pin;
    hcpu_pwm = (pad_pwm > 96) ? 0 : 1;
    HAL_PIN_Set(pad_pwm, func_pwm, PIN_NOPULL, hcpu_pwm);
} 

void pwm_init(const PwmConfig *pwm, uint32_t resolution, uint32_t frequency)
{
    struct bf0_pwm * p_pwm_obj = container_of(&(pwm->timer), struct bf0_pwm, tim_config);
    PBL_ASSERTN(pwm != NULL);
    if(resolution == 0)
        return;
    uint32_t freq = frequency / resolution;      /*get real freq*/
    uint32_t peroid = 1000000000UL/(freq);                               /*get real peoiod ns*/
    p_pwm_obj->pwm_config.resolution = resolution;
    p_pwm_obj->pwm_config.period = peroid;
    p_pwm_obj->pwm_config.channel = pwm->state->channel;
    p_pwm_obj->pwm_config.is_comp = pwm->state->is_comp;
    pwm_hal_pins_set_gpio(pwm);
    if (bf0_hw_pwm_init(p_pwm_obj) != 1)
    {
        PBL_LOG(LOG_LEVEL_ERROR, "%s init failed", p_pwm_obj->name);
        goto __exit;
    }
    
__exit:
    return;
}
