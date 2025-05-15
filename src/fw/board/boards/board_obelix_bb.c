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
#include "rtconfig.h"
#include "board/board.h"
#include "drivers/sf32lb/uart_definitions.h"
#include "drivers/i2c.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "drivers/sf32lb/i2c_hal_definitions.h"
#include "drivers/sf32lb/pwm_hal_definitions.h"
#include "board/board_sf32lb.h"



#define USING_UART1
#ifdef USING_UART1
#define UART_INST USART1
#define UART_TX PAD_PA19
#define UART_RX PAD_PA18
#define UART_DMAREQ DMA_REQUEST_5
#else
#define UART_INST USART3
#define UART_TX PAD_PA20
#define UART_RX PAD_PA27
#define UART_DMAREQ DMA_REQUEST_27
#endif

static UARTDeviceState s_dbg_uart_state;
static UART_HandleTypeDef s_dbg_uart_handle = {.Instance = UART_INST,
                                               .Init = {
                                                   .WordLength = UART_WORDLENGTH_8B,
                                                   .StopBits = UART_STOPBITS_1,
                                                   .Parity = UART_PARITY_NONE,
                                                   .HwFlowCtl = UART_HWCONTROL_NONE,
                                                   .OverSampling = UART_OVERSAMPLING_16,
                                               }};
static DMA_HandleTypeDef s_dbg_uart_rx_dma_handle = {
    .Instance = DMA1_Channel1,
    .Init.Request = UART_DMAREQ,
};
static UARTDevice DBG_UART_DEVICE = {.state = &s_dbg_uart_state,
                                     .tx_gpio = UART_TX,
                                     .rx_gpio = UART_RX,  // TODO: Setting GPIOs to actual values
                                     .periph = &s_dbg_uart_handle,
                                     .rx_dma = &s_dbg_uart_rx_dma_handle,
                                     .irq_priority = 5};
UARTDevice *const DBG_UART = &DBG_UART_DEVICE;
UARTDevice *const ACCESSORY_UART;  //TODO

#ifdef USING_UART1
IRQ_MAP(USART1, uart_irq_handler, DBG_UART);
#else
IRQ_MAP(USART3, uart_irq_handler, DBG_UART);
#endif

void DMAC1_CH1_IRQHandler(void) { HAL_DMA_IRQHandler(&s_dbg_uart_rx_dma_handle); }


/*tp*/
I2CBusState I2C1_BUS_STATE;
I2CBus I2C1_BUS = {
    .hal = &i2c_hal_obj[0],
    .state = &I2C1_BUS_STATE,
    .scl_gpio = 
    {
        .gpio_pin = 29,
    },
    .sda_gpio = 
    {
        .gpio_pin = 28,
    },
    .name = "i2c1",
}; 

/*ioe*/
I2CBusState I2C2_BUS_STATE;
I2CBus I2C2_BUS = {
    .hal = &i2c_hal_obj[1],
    .state = &I2C2_BUS_STATE,
    .scl_gpio = 
    {
        .gpio_pin = 21,
    },
    .sda_gpio = 
    {
        .gpio_pin = 20,
    },
    .name = "i2c2",
}; 

/*hr*/
I2CBusState I2C3_BUS_STATE;
I2CBus I2C3_BUS = {
    .hal = &i2c_hal_obj[2],
    .state = &I2C2_BUS_STATE,
    .scl_gpio = 
    {
        .gpio_pin = 33,
    },
    .sda_gpio = 
    {
        .gpio_pin = 32,
    },
    .name = "i2c3",
}; 

/*common*/
I2CBusState I2C_COMM_BUS_STATE;
I2CBus I2C_COMM_BUS = {
    .hal = &i2c_hal_obj[3],
    .state = &I2C_COMM_BUS_STATE,
    .scl_gpio = 
    {
        .gpio_pin = 31,
    },
    .sda_gpio = 
    {
        .gpio_pin = 30,
    },
    .name = "i2c4",
};

/*to pass the compiling for iic test routing */
struct I2CSlavePort i2c1_device = {
    .bus = &I2C1_BUS,
    .address = 0x38,
};

static const struct I2CSlavePort I2C_SLAVE_TP_CST816 = {
    .bus = &I2C1_BUS,
    .address = 0x15,
};

static const struct I2CSlavePort I2C_SLAVE_IOE_AW9527 = {
    .bus = &I2C2_BUS,
    .address = 0x5B,
};

static const struct I2CSlavePort I2C_SLAVE_HR_GH3026 = {
    .bus = &I2C3_BUS,
    .address = 0x5B,
};

static const struct I2CSlavePort I2C_SLAVE_CHG_ETA4662 = {
    .bus = &I2C_COMM_BUS,
    .address = 0x07,
};

static const struct I2CSlavePort I2C_SLAVE_MOT_AW86225 = {
    .bus = &I2C_COMM_BUS,
    .address = 0x58,
};

static const struct I2CSlavePort I2C_SLAVE_IMU_LSM6DSOW = {
    .bus = &I2C_COMM_BUS,
    .address = 0x6A,
};

static const struct I2CSlavePort I2C_SLAVE_MAG_MMC5603 = {
    .bus = &I2C_COMM_BUS,
    .address = 0x5B,
};

static const struct I2CSlavePort I2C_SLAVE_ALS_W1160 = {
    .bus = &I2C_COMM_BUS,
    .address = 0x5B,
};

I2CSlavePort* const I2C_CST816 = &I2C_SLAVE_TP_CST816;
I2CSlavePort* const I2C_AW9527 = &I2C_SLAVE_IOE_AW9527;
I2CSlavePort* const I2C_GH3026 = &I2C_SLAVE_HR_GH3026;
I2CSlavePort* const I2C_ETA4662 = &I2C_SLAVE_CHG_ETA4662;
I2CSlavePort* const I2C_AW86225 = &I2C_SLAVE_MOT_AW86225;
I2CSlavePort* const I2C_SM6DSOW = &I2C_SLAVE_IMU_LSM6DSOW;
I2CSlavePort* const I2C_MMC5603 = &I2C_SLAVE_MAG_MMC5603;
I2CSlavePort* const I2C_W1160 = &I2C_SLAVE_ALS_W1160;
            
PwmState pwm2_1_state =
{
    .channel = 1,
};


PwmConfig pwm2_ch1 =
{
    .timer =
        {
            .peripheral = hwp_gptim1,
            .init = tim_init,
            .preload = tim_preload,
        },
     .state = &pwm2_1_state,
     .output = 
      {
            .gpio_pin = 26,
      },

};

ADCInputConfig adc1_ch7 ={
    .adc = hwp_gpadc1,
    .adc_channel = 7,
    .gpio_pin = 255,        //adc1_ch7 don't need config pin
    };
ADCInputConfig adc1_ch0 ={
    .adc = hwp_gpadc1,
    .adc_channel = 0,
    .gpio_pin = 28,        //adc1_ch0 PA28
    };


#define LCD_RESET_PIN           (0)         // GPIO_A00
static void BSP_GPIO_Set(int pin, int val, int is_porta)
{
    GPIO_TypeDef *gpio = (is_porta) ? hwp_gpio1 : hwp_gpio2;
    GPIO_InitTypeDef GPIO_InitStruct;

    // set sensor pin to output mode
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(gpio, &GPIO_InitStruct);

    // set sensor pin to high == power on sensor board
    HAL_GPIO_WritePin(gpio, pin, (GPIO_PinState)val);
}
void BSP_LCD_Reset(uint8_t high1_low0)
{
    BSP_GPIO_Set(LCD_RESET_PIN, high1_low0, 1);
}

void BSP_LCD_PowerDown(void)
{
    // TODO: LCD power down
    BSP_GPIO_Set(LCD_RESET_PIN, 0, 1);
}

void BSP_LCD_PowerUp(void)
{
    // TODO: LCD power up
    HAL_Delay_us(500);      // lcd power on finish ,need 500us
}



uint32_t BSP_GetOtpBase(void)
{
    return MPI2_MEM_BASE;
}


#define HXT_DELAY_EXP_VAL 1000
static void LRC_init(void)
{
    HAL_PMU_RC10Kconfig();

    HAL_RC_CAL_update_reference_cycle_on_48M(LXT_LP_CYCLE);
    uint32_t ref_cnt = HAL_RC_CAL_get_reference_cycle_on_48M();
    uint32_t cycle_t = (uint32_t)ref_cnt / (48 * LXT_LP_CYCLE);

    HAL_PMU_SET_HXT3_RDY_DELAY((HXT_DELAY_EXP_VAL / cycle_t + 1));
}

void HAL_PreInit(void)
{
    // __asm("B .");
    /* not switch back to XT48 if other clock source has been selected already */
    if (RCC_SYSCLK_HRC48 == HAL_RCC_HCPU_GetClockSrc(RCC_CLK_MOD_SYS))
    {
        // To avoid somebody cancel request.
        HAL_HPAON_EnableXT48();
        HAL_RCC_HCPU_ClockSelect(RCC_CLK_MOD_SYS, RCC_SYSCLK_HXT48);
    }

    HAL_RCC_HCPU_ClockSelect(RCC_CLK_MOD_HP_PERI, RCC_CLK_PERI_HXT48);

    if (PM_STANDBY_BOOT != SystemPowerOnModeGet())
    {
        // Halt LCPU first to avoid LCPU in running state
        HAL_HPAON_WakeCore(CORE_ID_LCPU);
        HAL_RCC_Reset_and_Halt_LCPU(1);
        // get system configure from EFUSE
        BSP_System_Config();
        HAL_HPAON_StartGTimer();
        HAL_PMU_EnableRC32K(1);
        HAL_PMU_LpCLockSelect(PMU_LPCLK_RC32);

        HAL_PMU_EnableDLL(1);

#ifndef LXT_DISABLE 
        HAL_PMU_EnableXTAL32();
        if (HAL_PMU_LXTReady() != HAL_OK)
            HAL_ASSERT(0);
        // RTC/GTIME/LPTIME Using same low power clock source
        HAL_RTC_ENABLE_LXT();
#endif
#if 0
        {
            uint8_t is_enable_lxt = 1;
            uint32_t wdt_staus = 0xFF;
            uint32_t wdt_time = 0;
            uint16_t wdt_clk = 32768;
            uint8_t is_lcpu_rccal = 1;
            HAL_LCPU_CONFIG_set(HAL_LCPU_CONFIG_XTAL_ENABLED, &is_enable_lxt, 1);
            HAL_LCPU_CONFIG_set(HAL_LCPU_CONFIG_WDT_STATUS, &wdt_staus, 4);
            HAL_LCPU_CONFIG_set(HAL_LCPU_CONFIG_WDT_TIME, &wdt_time, 4);
            HAL_LCPU_CONFIG_set(HAL_LCPU_CONFIG_WDT_CLK_FEQ, &wdt_clk, 2);
            HAL_LCPU_CONFIG_set(HAL_LCPU_CONFIG_BT_RC_CAL_IN_L, &is_lcpu_rccal, 1);
            HAL_PMU_SetWdt((uint32_t)hwp_wdt2);   // Add reboot cause for watchdog2
        }
#endif

        HAL_RCC_LCPU_ClockSelect(RCC_CLK_MOD_LP_PERI, RCC_CLK_PERI_HXT48);

        HAL_HPAON_CANCEL_LP_ACTIVE_REQUEST();
        if (HAL_LXT_DISABLED())
            LRC_init();
        
    }

    HAL_RCC_HCPU_ConfigHCLK(240);

    // Reset sysclk used by HAL_Delay_us
    HAL_Delay_us(0);
    //HAL_sw_breakpoint();

}

void board_early_init(void) {

    HAL_StatusTypeDef status;

#if defined(HAL_V2D_GPU_MODULE_ENABLED)
    HAL_RCC_ResetModule(RCC_MOD_GPU);
#endif

    HAL_PreInit();

#ifdef SOC_BF0_HCPU
    if (PM_STANDBY_BOOT != SystemPowerOnModeGet())
    {
        // Except Standby mode, all other boot mode need to re-calibrate RC48
        status = HAL_RCC_CalibrateRC48();
        HAL_ASSERT(HAL_OK == status);
    }
#endif /* SOC_BF0_HCPU */

    HAL_RCC_Init();

#ifdef SOC_BF0_HCPU
    if (PM_STANDBY_BOOT != SystemPowerOnModeGet())
    {
        HAL_PMU_Init();
    }
#endif /* SOC_BF0_HCPU */

#ifdef SOC_BF0_HCPU
        /* init AES_ACC as normal mode */
        __HAL_SYSCFG_CLEAR_SECURITY();
        HAL_EFUSE_Init();
#endif

}

void board_init(void) {

}
