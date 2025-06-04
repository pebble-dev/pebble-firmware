#include "drivers/watchdog.h"

#include "util/bitset.h"
#include "system/logging.h"


#include <mcu.h>

#include <inttypes.h>
//#define SUPPORT_IWDT

static WDT_HandleTypeDef hwdt;
#if defined(SOC_BF0_HCPU) && defined(SUPPORT_IWDT)
static WDT_HandleTypeDef hiwdt;
#define IWDT_RELOAD_DIFFTIME 5   // iwdt to wdt reload difference time

#endif


#ifndef WDT_RELOADER_TIMEOUT
#define WDT_RELOADER_TIMEOUT   3
#endif
#ifndef WDT_REBOOT_TIMEOUT
#ifdef SOC_BF0_LCPU
    #define WDT_REBOOT_TIMEOUT   60
#else
    #define WDT_REBOOT_TIMEOUT   50
#endif
#endif
int sys_freq = 0;

void wdt_store_exception_information(void)
{
    return;
}

static HAL_StatusTypeDef wdt_set_timeout(WDT_HandleTypeDef *wdt, uint32_t reload_timeout)
{
    wdt->Init.Reload = sys_freq * reload_timeout;
#if defined(SOC_BF0_HCPU) && defined(SUPPORT_IWDT)
    if (hwp_iwdt == wdt->Instance)
        wdt->Init.Reload2 = sys_freq * IWDT_RELOAD_DIFFTIME;
    else
#endif        
        wdt->Init.Reload2 = sys_freq * WDT_REBOOT_TIMEOUT;

    //Disable by status
    if (HAL_WDT_Init(wdt) != HAL_OK)//wdt init
    {
        PBL_LOG(LOG_LEVEL_ERROR,"wdg set wdt timeout failed.");
        return HAL_ERROR;
    }
#if !defined(SOC_SF32LB52X)&&!defined(SOC_SF32LB55X)    // 52x should not acces PMU for PMU in HCPU
    HAL_PMU_SetWdt((uint32_t)wdt->Instance);            // Add reboot cause for watchdog
#endif
    __HAL_SYSCFG_Enable_WDT_REBOOT(1);                  // When timeout, reboot whole system instead of subsys.
    return HAL_OK;
}

void wdt_reconfig(void)
{
    hwdt.Instance = hwp_wdt2;
    HAL_WDT_Refresh(&hwdt);
    __HAL_WDT_STOP(&hwdt);
    hwdt.Instance = hwp_wdt1;
    HAL_WDT_Refresh(&hwdt);
    __HAL_WDT_STOP(&hwdt);
    
#if defined(SOC_BF0_HCPU) && defined(SUPPORT_IWDT)
    hiwdt.Instance = hwp_iwdt;
    HAL_WDT_Refresh(&hiwdt);
    wdt_set_timeout(&hiwdt, IWDT_RELOAD_DIFFTIME);
#endif
}

#ifdef SOC_SF32LB55X
void WDT_IRQHandler(void)
{
    static int printed;
    if (printed == 0)
    {
        printed++;
        //sifli_record_crash_status(1);
        //sifli_record_wdt_irq_status(1);
        //sifli_record_crash_save_process(RECORD_CRASH_SAVE_WDT_START);
        wdt_reconfig();
        wdt_store_exception_information();
    }
}
#else
void WDT_IRQHandler(void)
{
    static int printed;
    PBL_LOG(LOG_LEVEL_ERROR,"WDT_IRQHandler.");
    if (printed == 0)
    {
        printed++;

#ifdef SOC_BF0_LCPU
        // Avoid recurisive
        if (__HAL_SYSCFG_Get_Trigger_Assert_Flag() == 0)
        {
            wdt_store_exception_information();
            HAL_LPAON_WakeCore(CORE_ID_HCPU);
            __HAL_SYSCFG_Trigger_Assert();
            hwdt.Instance = hwp_wdt2;
            __HAL_WDT_STOP(&hwdt);
            __HAL_SYSCFG_Enable_WDT_REBOOT(1);              // When timeout, reboot whole system instead of subsys.
        }
#else // HCPU
        //sifli_record_crash_status(1);
        //sifli_record_wdt_irq_status(1);
        //sifli_record_crash_save_process(RECORD_CRASH_SAVE_WDT_START);
        if (__HAL_SYSCFG_Get_Trigger_Assert_Flag() == 0)
        {
            HAL_HPAON_WakeCore(CORE_ID_LCPU);
            __HAL_SYSCFG_Trigger_Assert();
        }
        wdt_reconfig();
        wdt_store_exception_information();
#endif
    }
}
#endif

void watchdog_init(void) {
    
    /*##-1- Initialize WDT peripheral #######################################*/

    /*HCPU use hwp_wdt1, LCPU use hwp_wdt2, both of them could use hwp_iwdt*/
#ifdef SOC_BF0_HCPU
    hwdt.Instance = hwp_wdt1;
#else
    hwdt.Instance = hwp_wdt2;
#endif

#ifdef SOC_SF32LB55X
    if (HAL_PMU_LXT_ENABLED())
        sys_freq = LXT_FREQ;
    else
#elif defined(SOC_SF32LB52X)
    if (HAL_PMU_LXT_ENABLED())
        sys_freq = RC32K_FREQ;
    else
#endif
    sys_freq = RC10K_FREQ;                                        // For 58x and 56x, WDT is always using RC10K
    
    hwdt.Init.Reload = (uint32_t)WDT_RELOADER_TIMEOUT * sys_freq;       // first Reload timeout will generate WDT interrupt is enabled (only apply to hwp_wdt1/hwp_wdt2).
    hwdt.Init.Reload2 = hwdt.Init.Reload * WDT_REBOOT_TIMEOUT;     // Second Reload2 timeout after frist one will reset HCPU/LCPU if use hwp_wdt1/hwp_wdt2

    __HAL_WDT_STOP(&hwdt);                             // Stop previous watch dog
    // No interrupt generated
    __HAL_WDT_INT(&hwdt, 1);                           // Disable WDT interrupt for Reload timeout


    if (HAL_WDT_Init(&hwdt) != HAL_OK)//wdt init
    {
        PBL_LOG(LOG_LEVEL_ERROR,"wdg set wdt timeout failed.");
        return ;
    }
#if !defined(SOC_SF32LB52X)&&!defined(SOC_SF32LB55X)    // 52x should not acces PMU for PMU in HCPU
    HAL_PMU_SetWdt((uint32_t)hwdt.Instance);            // Add reboot cause for watchdog
#endif
    __HAL_SYSCFG_Enable_WDT_REBOOT(1);                  // When timeout, reboot whole system instead of subsys.


#if defined(SOC_BF0_HCPU) && defined(SUPPORT_IWDT)
    hiwdt.Instance = hwp_iwdt;
    hiwdt.Init.Reload = (uint32_t)WDT_RELOADER_TIMEOUT * sys_freq + IWDT_RELOAD_DIFFTIME;
    hiwdt.Init.Reload2 = sys_freq * IWDT_RELOAD_DIFFTIME;
    __HAL_WDT_INT(&hiwdt, 1);
    if (HAL_WDT_Init(&hiwdt) != HAL_OK)//wdt init
    {
        PBL_LOG(LOG_LEVEL_ERROR,"wdg set wdt timeout failed.");
        return ;
    }
#if !defined(SOC_SF32LB52X)&&!defined(SOC_SF32LB55X)    // 52x should not acces PMU for PMU in HCPU
    HAL_PMU_SetWdt((uint32_t)hiwdt.Instance);            // Add reboot cause for watchdog
#endif
#endif
    
    PBL_LOG(LOG_LEVEL_INFO,"WDT init ok");


}

void watchdog_start(void) {
    __HAL_WDT_START(&hwdt);                            // Start Watch dog
    
#if defined(SOC_BF0_HCPU) && defined(SUPPORT_IWDT)
    __HAL_WDT_START(&hiwdt);
#endif

}

void watchdog_feed(void) {
    HAL_WDT_Refresh(&hwdt);                        // Pet watchdog
}

bool watchdog_check_reset_flag(void) {
  return 0;
}

McuRebootReason watchdog_clear_reset_flag(void) {
  McuRebootReason mcu_reboot_reason = {
    .brown_out_reset = 0,
    .pin_reset = 0,
    .power_on_reset = 1,
    .software_reset = 0,
    .independent_watchdog_reset = 0,
    .window_watchdog_reset = 0,
    .low_power_manager_reset = 0,
  };

  return mcu_reboot_reason;
}
