#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "os/mutex.h"
#include "os/tick.h"
#include "drivers/dma.h"
#include "drivers/gpio.h"
#include "drivers/periph_config.h"
#include "system/passert.h"
#include "drivers/sf32lb/adc_hal_definitions.h"
#include "board/board_sf32lb.h"
#include "bf0_pin_const.h"

//#define ADC_DEBUG   1

#ifdef hwp_gpadc1
#define ADC1_CONFIG                                                 \
{                                                               \
   .ADC_Handler.Instance                   = hwp_gpadc1,                          \
   .ADC_Handler.Init.atten3        = 0,             \
   .ADC_Handler.Init.adc_se            = 1,            \
   .ADC_Handler.Init.adc_force_on             = 0,           \
   .ADC_Handler.Init.dma_en          = 0,    \
   .ADC_Handler.Init.op_mode          = 0,           \
   .ADC_Handler.Init.en_slot      = 0,                       \
   .ADC_Handler.Init.data_samp_delay        = 2,          \
   .ADC_Handler.Init.conv_width        = 75,          \
   .ADC_Handler.Init.sample_width        = 71,          \
   .device_name             = "adc1",       \
}
#endif /* hwp_gpadc1 */

#ifdef GPADC_DMA_INSTANCE
struct dma_config adc1_dma = 
{ 
    .dma_irq_prio = GPADC_IRQ_PRIO,
    .Instance = GPADC_DMA_INSTANCE,
    .dma_irq = GPADC_DMA_IRQ,
    .Init.request = GPADC_DMA_REQUEST,
    .Init.Direction = DMA_PERIPH_TO_MEMORY,
    .Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD,
    .Init.MemDataAlignment = DMA_MDATAALIGN_WORD,
    .Init.PeriphInc           = DMA_PINC_DISABLE,
    .Init.MemInc              = DMA_MINC_ENABLE,
    .Init.Mode                = DMA_NORMAL,
    .Init.Priority            = DMA_PRIORITY_MEDIUM,

};


#endif  /* GPADC_DMA_INSTANCE */

#ifdef hwp_gpadc2
#define ADC2_CONFIG                                                 \
{                                                               \
   .ADC_Handler.Instance                   = hwp_gpadc2,                          \
   .ADC_Handler.Init.atten3        = 0,             \
   .ADC_Handler.Init.adc_se            = 1,            \
   .ADC_Handler.Init.adc_force_on             = 0,           \
   .ADC_Handler.Init.dma_en          = 0,    \
   .ADC_Handler.Init.op_mode          = 0,           \
   .ADC_Handler.Init.en_slot      = 0,                       \
   .ADC_Handler.Init.data_samp_delay        = 2,          \
   .ADC_Handler.Init.conv_width        = 75,          \
   .ADC_Handler.Init.sample_width        = 71,          \
   .device_name             = "adc2",       \

}
#endif /* hwp_gpadc1 */

#ifdef GPADC2_DMA_INSTANCE
struct dma_config adc1_dma = 
{ 
    .dma_irq_prio = GPADC_IRQ_PRIO,
    .Instance = GPADC2_DMA_INSTANCE,
    .dma_irq = GPADC2_DMA_IRQ,
    .Init.request = GPADC2_DMA_REQUEST,
    .Init.Direction = DMA_PERIPH_TO_MEMORY,
    .Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD,
    .Init.MemDataAlignment = DMA_MDATAALIGN_WORD,
    .Init.PeriphInc           = DMA_PINC_DISABLE,
    .Init.MemInc              = DMA_MINC_ENABLE,
    .Init.Mode                = DMA_NORMAL,
    .Init.Priority            = DMA_PRIORITY_MEDIUM,
};
#endif  /* GPADC2_DMA_INSTANCE */




enum
{
#ifdef hwp_gpadc1
    ADC1_INDEX,
#endif
#ifdef hwp_gpadc2
    ADC2_INDEX,
#endif
    ADC_MAX_INDEX,
};

static struct sifli_adc bf0_adc_obj[] =
{
#ifdef hwp_gpadc1
    ADC1_CONFIG,
#endif
#ifdef hwp_gpadc2
    ADC2_CONFIG,
#endif
};

    //MAX support voltage, for A0, voltage = 1.1v, for RPO, voltage = 3.3v
#ifdef SF32LB55X
    #define ADC_MAX_VOLTAGE_MV     (1100)
#else
    #define ADC_MAX_VOLTAGE_MV     (3300)
#endif
    
#define ADC_SW_AVRA_CNT         (22)
    
    // Standard voltage from ATE , it should not changed !!!
#define ADC_BIG_RANGE_VOL1           (1000)
#define ADC_BIG_RANGE_VOL2           (2500)
#define ADC_SML_RANGE_VOL1           (300)
#define ADC_SML_RANGE_VOL2           (800)
    
    
#define ADC_RATIO_ACCURATE          (1000)

#ifdef SF32LB55X
    // default value, they should be over write by calibrate
    // it should be register value offset vs 0 v value.
    static float adc_vol_offset = 199.0;
    // mv per bit, if accuracy not enough, change to 0.1 mv or 0.01 mv later
    static float adc_vol_ratio = 3930.0; // 4296; //6 * ADC_RATIO_ACCURATE; //600; //6;
    static int adc_range = 0;   /* flag for ATE calibration voltage range,
    *  0 for big range (1.0v/2.5v)
    *  1 for small range () */
#elif defined(SF32LB56X)
    // it should be register value offset vs 0 v value.
    static float adc_vol_offset = 822.0;
    // 0.001 mv per bit
    static float adc_vol_ratio = 1068.0; //
    static int adc_range = 1;
#else
    // it should be register value offset vs 0 v value.
    static float adc_vol_offset = 822.0;
    // 0.001 mv per bit,
    static float adc_vol_ratio = 1068.0; //
    static int adc_range = 1;
#endif

static float adc_vbat_factor = 2.01;

static uint32_t adc_thd_reg;
static SemaphoreHandle_t adc_semaphore;





#ifdef BSP_GPADC_SUPPORT_MULTI_CH_SAMPLING
#define GPADC_GPTIME_PEROID 100 //100ns
struct bf0_hwtimer
{
    rt_hwtimer_t time_device;           /*!< HW timer os device */
    GPT_HandleTypeDef    tim_handle;    /*!< HW timer low level handle */
    IRQn_Type tim_irqn;                 /*!< interrupt number for timer*/
    uint8_t core;                       /*!< Clock source from which core*/
    char *name;                         /*!< HW timer device name*/
};
struct bf0_hwtimer_info
{
    int32_t maxfreq;    /* the maximum count frequency timer support */
    int32_t minfreq;    /* the minimum count frequency timer support */
    uint32_t maxcnt;    /* counter maximum value */
    uint8_t  cntmode;   /* count mode (inc/dec) */
};
typedef struct hwtimer_device
{
    const struct bf0_hwtimer_info *info;
    int32_t freq;                /* counting frequency set by the user */
    int32_t overflow;            /* timer overflows */
    float period_sec;
    int32_t cycles;              /* how many times will generate a timeout event after overflow */
    int32_t reload;              /* reload cycles(using in period mode) */
} hwtimer_t;
typedef struct hwtimerval
{
    int32_t sec;      /* second */
    int32_t usec;     /* microsecond */
} hwtimerval_t;

static const struct bf0_hwtimer_info hwtimer_info = {                                           \
    .maxfreq = 1000000,                     \
    .minfreq = 2000,                        \
    .maxcnt  = 0xFFFF,                      \
    .cntmode = HWTIMER_CNTMODE_UP,          \
};
GPT_HandleTypeDef tim;
struct bf0_hwtimer hw_gptimer;
extern void pin_debug_toggle();

void GPTIM1_IRQHandler(void)
{
    HAL_GPT_IRQHandler(&tim);
}

static uint32_t time_calc(hwtimer_t *timer, hwtimerval_t *tv)
{
    float overflow;
    float timeout;
    uint32_t counter;
    int i, index = 0;
    float tv_sec;
    float devi_min = 1;
    float devi;

    /* changed to second */
    overflow = timer->info->maxcnt / (float)timer->freq;
    tv_sec = tv->sec + tv->usec / (float)1000000;

    if (tv_sec < (1 / (float)timer->freq))
    {
        /* little timeout */
        i = 0;
        timeout = 1 / (float)timer->freq;
    }
    else
    {

        for (i = 1; i > 0; i ++)
        {
            timeout = tv_sec / i;

            if (timeout <= overflow)
            {
// Do not need to be such accurate, want to sleep longer each time.
#if 0
                counter = timeout * timer->freq;
                devi = tv_sec - (counter / (float)timer->freq) * i;
                /* Minimum calculation error */
                if (devi > devi_min)
                {
                    i = index;
                    timeout = tv_sec / i;
                    break;
                }
                else if (devi == 0)
                {
                    break;
                }
                else if (devi < devi_min)
                {
                    devi_min = devi;
                    index = i;
                }
#else
                break;
#endif
            }
        }
    }

    timer->cycles = i;
    timer->reload = i;
    timer->period_sec = timeout;
    counter = timeout * timer->freq;
    return counter;
}

static int sifli_adc_use_gptime_init()
{

    /*Init gptime*/
#if GPTIM1
    {
        hw_gptimer.tim_handle.Instance     = GPTIM1;
        hw_gptimer.tim_irqn                = GPTIM1_IRQn;
        hw_gptimer.name                    = "gptim1";
        hw_gptimer.core                    = GPTIM1_CORE;
    }
#endif


    hw_gptimer.time_device.info = &hwtimer_info;


    if ((1000000 <= hw_gptimer.time_device.info->maxfreq) && (1000000 >= hw_gptimer.time_device.info->minfreq))
    {
        hw_gptimer.time_device.freq = 1000000;
    }
    else
    {
        hw_gptimer.time_device.freq = hw_gptimer.time_device.info->minfreq;
    }
    hw_gptimer.time_device.mode = HWTIMER_MODE_PERIOD;
    hw_gptimer.time_device.cycles = 0;
    hw_gptimer.time_device.overflow = 0;


    uint32_t  prescaler_value = HAL_RCC_GetPCLKFreq(
                                    hw_gptimer.core,
                                    1);

    prescaler_value = prescaler_value / 1000 - 1;
    tim.Init.Period            = 10000 - 1;
    tim.Init.Prescaler         = prescaler_value;
    tim.Instance = hw_gptimer.tim_handle.Instance;
    tim.core                   = hw_gptimer.core;
    tim.Init.CounterMode   = GPT_COUNTERMODE_UP;
    tim.Init.RepetitionCounter = 0;

    if (HAL_GPT_Base_Init(&tim) != HAL_OK)
    {
        PBL_LOG(LOG_LEVEL_ERROR,"%s init failed", hw_gptimer.name);
        return -1;
    }
    else
    {
        /* set the TIMx priority */
        HAL_NVIC_SetPriority(hw_gptimer.tim_irqn, 3, 0);

        /* enable the TIMx global Interrupt */
        HAL_NVIC_EnableIRQ(hw_gptimer.tim_irqn);

        /* clear update flag */
        __HAL_GPT_CLEAR_FLAG(&tim, GPT_FLAG_UPDATE);
        /* enable update request source */
        __HAL_GPT_URS_ENABLE(&tim);
        tim.Instance->CR2 &= ~GPT_CR2_MMS;
        tim.Instance->CR2 = (0x2 << GPT_CR2_MMS_Pos);

        PBL_LOG(LOG_LEVEL_INFO,"%s init success;", hw_gptimer.name);
    }

    /*update  prescaler*/
    uint32_t val = 0;
    val = HAL_RCC_GetPCLKFreq(tim.core, 1);
    val /= 1000000;
    __HAL_GPT_SET_PRESCALER(&tim,  val - 1);
    tim.Instance->EGR |= GPT_EVENTSOURCE_UPDATE; /* Update frequency value */
    hw_gptimer.time_device.freq = 1000000;


    hwtimerval_t t_value;
    t_value.sec = 0;
    t_value.usec = GPADC_GPTIME_PEROID;
    uint32_t t = time_calc(&(hw_gptimer.time_device), &t_value);
    __HAL_GPT_SET_AUTORELOAD(&tim, t);

    /* set timer to Repetitive mode */
    tim.Instance->CR1 &= ~GPT_OPMODE_SINGLE;
    HAL_GPT_Base_Start_IT(&tim);

    return 0;
}

#if defined(hwp_gpadc1)

void GPADC_IRQHandler(void)
{
    bf0_adc_obj[0].ADC_Handler.Instance->GPADC_IRQ |= GPADC_GPADC_IRQ_GPADC_ICR;
    
}
#endif
#endif

static void adc_get_dma_info(void)
{
#if defined(GPADC_DMA_INSTANCE) && defined(hwp_gpadc1)
    bf0_adc_obj[ADC1_INDEX].adc_dma_flag = 1;
    bf0_adc_obj[ADC1_INDEX].DMA = &adc1_dma;

#endif
#if defined(GPADC2_DMA_INSTANCE) && defined(hwp_gpadc2)
    bf0_adc_obj[ADC2_INDEX].adc_dma_flag = 1;
    bf0_adc_obj[ADC2_INDEX].DMA = &adc2_dma;
#endif
}

int sifli_adc_calibration(uint32_t value1, uint32_t value2,
                          uint32_t vol1, uint32_t vol2, float *offset, float *ratio)
{
    float gap1, gap2;
    uint32_t reg_max;

    if (offset == NULL || ratio == NULL)
        return 0;

    if (value1 == 0 || value2 == 0)
        return 0;

    gap1 = value1 > value2 ? value1 - value2 : value2 - value1; // register value gap
    gap2 = vol1 > vol2 ? vol1 - vol2 : vol2 - vol1; // voltage gap -- mv

    if (gap1 != 0)
    {
        *ratio = gap2 * ADC_RATIO_ACCURATE / gap1; // gap2 * 10 for 0.1mv, gap2 * 100 for 0.01mv
        adc_vol_ratio = *ratio;
    }
    else //
        return 0;

    *offset = value1 - vol1 * ADC_RATIO_ACCURATE / adc_vol_ratio;
    adc_vol_offset = *offset;

    // get register value for max voltage
    adc_thd_reg = ADC_MAX_VOLTAGE_MV * ADC_RATIO_ACCURATE / adc_vol_ratio + adc_vol_offset;
    reg_max = GPADC_ADC_RDATA0_SLOT0_RDATA >> GPADC_ADC_RDATA0_SLOT0_RDATA_Pos;
    if (adc_thd_reg >= (reg_max - 3))
        adc_thd_reg = reg_max - 3;

    return adc_vol_offset;
}

static void sifli_adc_vbat_fact_calib(uint32_t voltage, uint32_t reg)
{
    float vol_from_reg;

    // get voltage calculate by register data
    vol_from_reg = (reg - adc_vol_offset) * adc_vol_ratio / ADC_RATIO_ACCURATE;
    adc_vbat_factor = (float)voltage / vol_from_reg;
}

struct sifli_adc *  find_adc_obj(ADCInputConfig* adc_config)
{
    struct sifli_adc * adc_obj = NULL;
    if(!ADC_MAX_INDEX)
        return NULL;
    for(int i = 0; i < ADC_MAX_INDEX;i++)
    {
        if(bf0_adc_obj[i].ADC_Handler.Instance == adc_config->adc)
        {
            adc_obj = &(bf0_adc_obj[i]);
            break;
        }
        
    }
    return adc_obj;
}

int adc_init(void)
{
    int ret = 0;
    adc_get_dma_info();
    for (uint16_t i = 0; i < sizeof(bf0_adc_obj) / sizeof(bf0_adc_obj[0]); i++)
    {
        #ifndef SF32LB55X
        #ifdef BSP_AVDD_V18_ENABLE
        bf0_adc_obj[i].ADC_Handler.Init.avdd_v18_en = 1;
        #else
        bf0_adc_obj[i].ADC_Handler.Init.avdd_v18_en = 0;
        #endif
        #endif
        
        if (HAL_ADC_Init(&bf0_adc_obj[i].ADC_Handler) != HAL_OK)
        {
            PBL_LOG(LOG_LEVEL_ERROR,"%s init failed", bf0_adc_obj[i].device_name);
            ret = -1;
            goto exit;
        }
        else 
        {
            PBL_LOG(LOG_LEVEL_INFO,"ADC hal init ok!");
            #ifdef SF32LB52X
            uint32_t adc_freq = 240000; // use 240k for 52x to meet ATE setting
            HAL_ADC_SetFreq(&bf0_adc_obj[i].ADC_Handler, adc_freq);
            #endif


            #ifdef BSP_GPADC_SUPPORT_MULTI_CH_SAMPLING
            {
                ADC_ChannelConfTypeDef ADC_ChanConf;

                /*configure all channels*/
                #ifdef SF32LB52X
                uint8_t ch_num = 7;
                #else
                uint8_t ch_num = 8;
                #endif
                memset(&ADC_ChanConf, 0, sizeof(ADC_ChanConf));
                HAL_ADC_Set_MultiMode(&bf0_adc_obj[i].ADC_Handler, 1);
                for (uint8_t ch = 0; ch < ch_num; ch++)
                {
                    ADC_ChanConf.Channel = ch;
                    ADC_ChanConf.pchnl_sel = ch;
                    ADC_ChanConf.slot_en = 1;
                    ADC_ChanConf.nchnl_sel = 0;
                    ADC_ChanConf.acc_num = 0;   /* remove hardware do multi point average*/
                    HAL_ADC_ConfigChannel(&bf0_adc_obj[i].ADC_Handler, &ADC_ChanConf);
                }

                HAL_ADC_Prepare(&bf0_adc_obj[i].ADC_Handler);

                /*set timer trigger src to gptimer 1*/
                HAL_ADC_SetTimer(&bf0_adc_obj[i].ADC_Handler, HAL_ADC_SRC_GPTIM1);
                sifli_adc_use_gptime_init();        /*add later */

                HAL_NVIC_EnableIRQ(GPADC_IRQn);
                ADC_ENABLE_TIMER_TRIGER(&bf0_adc_obj[i].ADC_Handler);
                
                PBL_LOG(LOG_LEVEL_INFO,"ADC_SUPPORT_MULTI_CH_SAMPLING init ok!");
            }
            #endif
        }
        adc_semaphore = xSemaphoreCreateBinary();
        adc_thd_reg = GPADC_ADC_RDATA0_SLOT0_RDATA >> GPADC_ADC_RDATA0_SLOT0_RDATA_Pos;
    
        FACTORY_CFG_ADC_T cfg;
        int len = sizeof(FACTORY_CFG_ADC_T);
        memset((uint8_t *)&cfg, 0, len);
        if (BSP_CONFIG_get(FACTORY_CFG_ID_ADC, (uint8_t *)&cfg, len))  // TODO: configure read ADC parameters method after ATE confirm
        {
            float off, rat;
            uint32_t vol1, vol2;
            if (cfg.vol10 == 0 || cfg.vol25 == 0) // not valid paramter
            {
                //LOG_I("Get GPADC configure invalid : %d, %d\n", cfg.vol10, cfg.vol25);
            }
            else
            {
#ifndef SF32LB55X
                cfg.vol10 &= 0x7fff;
                cfg.vol25 &= 0x7fff;
                vol1 = cfg.low_mv;
                vol2 = cfg.high_mv;
                adc_range = 1;
#else
                if ((cfg.vol10 & (1 << 15)) && (cfg.vol25 & (1 << 15))) // small range, use X1 mode
                {
                    cfg.vol10 &= 0x7fff;
                    cfg.vol25 &= 0x7fff;
                    vol1 = ADC_SML_RANGE_VOL1;
                    vol2 = ADC_SML_RANGE_VOL2;
                    adc_range = 1;
                }
                else // big range , use X3 mode for A0
                {
                    vol1 = ADC_BIG_RANGE_VOL1;
                    vol2 = ADC_BIG_RANGE_VOL2;
                    adc_range = 0;
                }
#endif
                sifli_adc_calibration(cfg.vol10, cfg.vol25, vol1, vol2, &off, &rat);
#ifdef SF32LB52X
                sifli_adc_vbat_fact_calib(cfg.vbat_mv, cfg.vbat_reg);
    
                if (SF32LB52X_LETTER_SERIES())
                {
#if defined(hwp_gpadc1)
                    if (cfg.ldovref_flag)
                    {
                        __HAL_ADC_SET_LDO_REF_SEL(&bf0_adc_obj[0].ADC_Handler, cfg.ldovref_sel);
                    }
#endif
                }
#endif
                PBL_LOG(LOG_LEVEL_INFO,"\nGPADC :vol10: %d mv, %d; vol25: %d mv reg %d; offset %f, ratio %f, max reg %d;",
                      (int)vol1, (int)cfg.vol10, (int)vol2, (int)cfg.vol25,  off, rat, (int)adc_thd_reg);
#if defined(hwp_gpadc1)

                PBL_LOG(LOG_LEVEL_INFO,"\n vbat_mv: %d mv, %d; ldoref_flag = %d, ldoref_sel = %d;",
                      (int)cfg.vbat_mv, (int)cfg.vbat_reg, (int)cfg.ldovref_flag, (int)cfg.ldovref_sel);
#endif
    
            }
        }
        else
        {
            PBL_LOG(LOG_LEVEL_ERROR,"Get ADC configure fail");
            ret = -1;
        }
    }

exit:
    if(ret < 0)
        PBL_LOG(LOG_LEVEL_ALWAYS,"ADC init fail!");
    else
        PBL_LOG(LOG_LEVEL_ALWAYS,"ADC init ok!");
    return ret;

}
void adc_pins_set_gpio(ADCInputConfig* adc_config)
{
    int pad_adc;
    int hcpu_adc;
    uint16_t pin = adc_config->gpio_pin;
    char name[5] = {'a', 'd', 'c', '0', 0};
    if(pin >= 255)
    {
        PBL_LOG(LOG_LEVEL_INFO, "don't need config pin;");
        return;
    }
    if(adc_config->adc == hwp_gpadc1)
        name[3] = '1';
    else
        name[3] = '2';
    pin_function func_adc = GPIO_A0;
    hcpu_adc = (pin > 96) ? 0 : 1;
    if(hcpu_adc)
    {
        func_adc = pin + GPIO_A0;
        pad_adc = pin + PAD_PA00;
    }
    else
    {
        func_adc = pin - 96 + GPIO_B0;
        pad_adc = pin - 96 + PAD_PB00;
    }
    HAL_PIN_Set(pad_adc, func_adc, PIN_NOPULL, hcpu_adc);
    HAL_PIN_Set_Analog(pad_adc, 1);
    PBL_LOG(LOG_LEVEL_INFO, "ADC set pin[%d],as [%s] channel_%d ;", (int)pin, name, (int) adc_config->adc_channel);
    return;
} 


int adc_enabled(ADCInputConfig* adc_config, bool enabled)
{
    int ret = 0;
    PBL_ASSERTN(adc_config != NULL);
    struct sifli_adc * adc_obj = NULL;
    adc_obj = find_adc_obj(adc_config);
    if(!adc_obj)
    {
        ret = -1;
        PBL_LOG(LOG_LEVEL_ERROR,"adc enable fail!");
        goto exit;
    }
    HAL_StatusTypeDef status = HAL_OK;
#ifndef BSP_GPADC_SUPPORT_MULTI_CH_SAMPLING
    if (enabled)
    {
        adc_pins_set_gpio(adc_config);
        status = HAL_ADC_EnableSlot(&adc_obj->ADC_Handler, adc_config->adc_channel, 1);
    }
    else
    {
        status = HAL_ADC_EnableSlot(&adc_obj->ADC_Handler, adc_config->adc_channel, 0);
    }
    if(status != HAL_OK)
    {
        ret = -1;
        PBL_LOG(LOG_LEVEL_ERROR,"adc set channel fail!");
        goto exit;
    }
#endif
exit:
    return ret;
}
static uint32_t sifli_adc_get_channel(uint32_t channel)
{
    uint32_t sifli_channel = 0;

    switch (channel)
    {
    case  0:
        sifli_channel = 0;
        break;
    case  1:
        sifli_channel = 1;
        break;
    case  2:
        sifli_channel = 2;
        break;
    case  3:
        sifli_channel = 3;
        break;
    case  4:
        sifli_channel = 4;
        break;
    case  5:
        sifli_channel = 5;
        break;
    case  6:
        sifli_channel = 6;
        break;
    case  7:
        sifli_channel = 7;
        break;
    }

    return sifli_channel;
}
float sifli_adc_get_float_mv(float value)
{
    float offset, ratio;
    // get offset
    offset = adc_vol_offset;
    // get ratio, adc_vol_ratio calculate by calibration voltage
    if (adc_range == 0) // calibration with big range, app use small rage, need div 3
        ratio = adc_vol_ratio / 3;
    else // calibration and app all use small rage
        ratio = adc_vol_ratio;

    return (value - offset) * ratio / ADC_RATIO_ACCURATE;
}
int get_adc_value(ADCInputConfig* adc_config, uint32_t *value)
{
    HAL_StatusTypeDef res = 0;
    ADC_ChannelConfTypeDef ADC_ChanConf;
    PBL_ASSERTN(adc_config != NULL);
    PBL_ASSERTN(value != NULL);
    struct sifli_adc * adc_obj = find_adc_obj(adc_config);
    ADC_HandleTypeDef *sifli_adc_handler = &adc_obj->ADC_Handler;
    uint32_t channel = adc_config->adc_channel;
    xSemaphoreTake(adc_semaphore, milliseconds_to_ticks(2000));
#ifdef BSP_GPADC_SUPPORT_MULTI_CH_SAMPLING
    uint32_t adc_origin = HAL_ADC_GetValue(sifli_adc_handler, channel);
    float fave = (float) adc_origin;

#else
    memset(&ADC_ChanConf, 0, sizeof(ADC_ChanConf));

    if (channel <= 7)
    {
        /* set ADC channel */
        ADC_ChanConf.Channel =  sifli_adc_get_channel(channel);
    }
    else
    {
        PBL_LOG(LOG_LEVEL_ERROR, "ADC channel must be between 0 and 7.");
        xSemaphoreGive(adc_semaphore);
        return -1;
    }
    ADC_ChanConf.pchnl_sel = channel;
    ADC_ChanConf.slot_en = 1;
    ADC_ChanConf.nchnl_sel = 0;
    //ADC_ChanConf.acc_en = 0;
    ADC_ChanConf.acc_num = 0;   // remove hardware do multi point average
    HAL_ADC_ConfigChannel(sifli_adc_handler, &ADC_ChanConf);

    /* start ADC */
    HAL_ADC_Start(sifli_adc_handler);

    // do filter and average here
    int i, j;
    uint32_t data[ADC_SW_AVRA_CNT];
    uint32_t total, ave;
    float fave;
    total = 0;
    for (i = 0; i < ADC_SW_AVRA_CNT; i++)
    {
        if (i != 0)
        {
            #ifndef  SF32LB55X
            // unmute before read adc
            ADC_SET_UNMUTE(sifli_adc_handler);
            HAL_Delay_us(200);
            #else
            // FRC EN before each start
            ADC_FRC_EN(sifli_adc_handler);
            HAL_Delay_us(50);
            #endif

            // triger sw start,
            __HAL_ADC_START_CONV(sifli_adc_handler);
        }

        /* Wait for the ADC to convert */
        res = HAL_ADC_PollForConversion(sifli_adc_handler, 100);
        if (res != HAL_OK)
        {
            HAL_ADC_Stop(sifli_adc_handler);
            PBL_LOG(LOG_LEVEL_ERROR, "Polling ADC fail %d", (int)res);
            xSemaphoreGive(adc_semaphore);
            return -1;
        }

        /* get ADC value */
        data[i] = (uint32_t)HAL_ADC_GetValue(sifli_adc_handler, channel);
        #ifdef ADC_DEBUG
        PBL_LOG(LOG_LEVEL_INFO,"ch[%d]count[%d] adc raw = %d;", (int)channel, (int)i,  (int)data[i]);
        #endif
        if (data[i] >= adc_thd_reg)
        {
            HAL_ADC_Stop(sifli_adc_handler);
            PBL_LOG(LOG_LEVEL_ERROR, "ADC input voltage too large, register value %d", (int)data[i]);
            xSemaphoreGive(adc_semaphore);
            return -1;
        }
        #ifdef ADC_DEBUG
        PBL_LOG(LOG_LEVEL_INFO, "ADC reg value %d", (int)data[i]);
        #endif
        total += data[i];
        // Add a delay between each read to make voltage stable
        #ifndef  SF32LB55X
        ADC_SET_MUTE(sifli_adc_handler);
            #ifdef SF32LB52X
            HAL_Delay_us(1000);
            #else
            HAL_Delay_us(10000);
            #endif
        #else   /* SF32LB55X */
        ADC_CLR_FRC_EN(sifli_adc_handler);
        HAL_Delay_us(5000);
        #endif
    }
    HAL_ADC_Stop(sifli_adc_handler);

    // sort
    for (i = 0; i < ADC_SW_AVRA_CNT - 1; i++)
        for (j = 0; j < ADC_SW_AVRA_CNT - 1 - i; j++)
            if (data[j] > data[j + 1])
            {
                ave = data[j];
                data[j] = data[j + 1];
                data[j + 1] = ave;
            }
    // drop max/min , mid filter
    total -= data[0];
    total -= data[ADC_SW_AVRA_CNT - 1];

    fave = (float)total / (ADC_SW_AVRA_CNT - 2);
    #ifdef ADC_DEBUG
    PBL_LOG(LOG_LEVEL_INFO, "ch[%d]average val =%f;", (int)channel, fave);
    #endif

#endif


        #ifndef SF32LB52X   // TODO: remove macro check after 52x ADC calibration work
    float fval = sifli_adc_get_float_mv(fave) * 10; // mv to 0.1mv based
    *value = (uint32_t)fval;
    #else
    float fval = sifli_adc_get_float_mv(fave) * 10; // mv to 0.1mv based
    *value = (uint32_t)fval;
    if (channel == 7)   // for 52x, channel fix used for vbat with 1/2 update(need calibrate)
        *value = (uint32_t)(fval * adc_vbat_factor);
    #endif
    #ifdef ADC_DEBUG
    PBL_LOG(LOG_LEVEL_INFO, "ADC vol %d , reg %f, max 0x%x, min 0x%x", (int)*value, fave, (unsigned int)data[ADC_SW_AVRA_CNT - 1], (unsigned int)data[0]);
    #endif
    #ifdef BSP_GPADC_SUPPORT_MULTI_CH_SAMPLING
    PBL_LOG(LOG_LEVEL_INFO, "ch[%d]origin:%d, voltage:%d;", (int)channel, (int)adc_origin, (int)*value);
    #else
    PBL_LOG(LOG_LEVEL_INFO, "ch[%d]voltage=%d;", (int)channel, (int)*value);
    #endif
    xSemaphoreGive(adc_semaphore);
    return 0;
}


void example_adc(ADCInputConfig* adc_config)
{
    uint32_t adc_value;
    adc_enabled(adc_config, true);
    get_adc_value(adc_config, &adc_value);
    adc_enabled(adc_config, false);
}

