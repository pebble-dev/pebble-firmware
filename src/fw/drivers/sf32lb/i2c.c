

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "os/mutex.h"
#include "os/tick.h"
#include "drivers/dma.h"
#include "drivers/gpio.h"
#include "drivers/periph_config.h"
#include "system/passert.h"
#include "drivers/sf32lb/i2c_definitions.h"
#include "services/common/analytics/analytics.h"

#ifdef HAL_I2C_MODULE_ENABLED
static uint32_t s_max_transfer_duration_ticks;

static void prv_analytics_track_i2c_error(void);

static bool prv_semaphore_take(bf0_i2c_t *obj) 
{
    return (xSemaphoreTake(obj->sema, 0) == pdPASS);
}
  
static bool prv_semaphore_wait(bf0_i2c_t *obj) 
{
TickType_t timeout_ticks = milliseconds_to_ticks(obj->bf0_i2c_cfg->timeout);
return (xSemaphoreTake(obj->sema, timeout_ticks) == pdPASS);
}
  
static void prv_semaphore_give(bf0_i2c_t *obj) 
{
// If this fails, something is very wrong
(void)xSemaphoreGive(obj->sema);
}
  
static portBASE_TYPE prv_semaphore_give_from_isr(bf0_i2c_t *obj) 
{
portBASE_TYPE should_context_switch = pdFALSE;
(void)xSemaphoreGiveFromISR(obj->sema,  &should_context_switch);
return should_context_switch;
}


static void I2Cx_IRQHandler(bf0_i2c_t *obj)
{
    I2C_HandleTypeDef *handle;
    handle = &(obj->handle);

    if (handle->XferISR != NULL)
    {
        handle->XferISR(handle, 0, 0);
    }

    if ((HAL_I2C_STATE_BUSY_TX != handle->State) && (HAL_I2C_STATE_BUSY_RX != handle->State))
    {
        prv_semaphore_give(obj);
    }

}

#if defined(I2C1)
bf0_i2c_config_t bf0_i2c1_cfg =
{
    .device_name = "i2c1",
    .Instance = I2C1,
    .irq_type = I2C1_IRQn,
    .core     = I2C1_CORE,
    .open_flag= RT_DEVICE_FLAG_RDWR,
};
struct rt_i2c_configuration i2c1_cfg_default = 
{
    0,       // mode;  RT_I2C_ADDR_10BIT / RT_I2C_NO_START / RT_I2C_IGNORE_NACK / RT_I2C_NO_READ_ACK
    0,       // addr;
    5000,    // timeout;
    400000   // max_hz;
}; 
bf0_i2c_t i2c1_obj;
bf0_i2c_t *const bf0_i2c1 = &i2c1_obj;
void I2C1_IRQHandler(void)
{
    I2Cx_IRQHandler(&i2c1_obj); 
}

#endif

#if defined(I2C2)
bf0_i2c_config_t bf0_i2c2_cfg =
{
    .device_name = "i2c2",
    .Instance = I2C2,
    .irq_type = I2C2_IRQn,
    .core     = I2C2_CORE,
    .open_flag= RT_DEVICE_FLAG_RDWR,
};
struct rt_i2c_configuration i2c2_cfg_default = 
{
    0,       // mode;  RT_I2C_ADDR_10BIT / RT_I2C_NO_START / RT_I2C_IGNORE_NACK / RT_I2C_NO_READ_ACK
    0,       // addr;
    5000,    // timeout;
    400000   // max_hz;
}; 

bf0_i2c_t i2c2_obj;
bf0_i2c_t *const bf0_i2c2 = &i2c2_obj;
void I2C2_IRQHandler(void)
{
    I2Cx_IRQHandler(&i2c2_obj); 
}
#endif

#if defined(I2C3)
bf0_i2c_config_t bf0_i2c3_cfg =
{
    .device_name = "i2c3",
    .Instance = I2C3,
    .irq_type = I2C3_IRQn,
    .core     = I2C3_CORE,
    .open_flag= RT_DEVICE_FLAG_RDWR,
};
struct rt_i2c_configuration i2c3_cfg_default = 
{
    0,       // mode;  RT_I2C_ADDR_10BIT / RT_I2C_NO_START / RT_I2C_IGNORE_NACK / RT_I2C_NO_READ_ACK
    0,       // addr;
    5000,    // timeout;
    400000   // max_hz;
}; 

bf0_i2c_t i2c3_obj;
bf0_i2c_t *const bf0_i2c3 = &i2c3_obj;
void I2C3_IRQHandler(void)
{
    I2Cx_IRQHandler(&i2c3_obj);  
}
#endif

#if defined(I2C4)
bf0_i2c_config_t bf0_i2c4_cfg=
{
    .device_name = "i2c4",
    .Instance = I2C4,
    .irq_type = I2C4_IRQn,
    .core     = I2C4_CORE,
    .open_flag= RT_DEVICE_FLAG_RDWR,
};
struct rt_i2c_configuration i2c4_cfg_default = 
{
    0,       // mode;  RT_I2C_ADDR_10BIT / RT_I2C_NO_START / RT_I2C_IGNORE_NACK / RT_I2C_NO_READ_ACK
    0,       // addr;
    5000,    // timeout;
    400000   // max_hz;
}; 

bf0_i2c_t i2c4_obj;
bf0_i2c_t *const bf0_i2c4 = &i2c4_obj;
void I2C4_IRQHandler(void)
{
    I2Cx_IRQHandler(&i2c4_obj); 
}
#endif

#if defined(I2C5)
bf0_i2c_config_t bf0_i2c5_cfg=
{
    .device_name = "i2c5",
    .Instance = I2C5,
    .irq_type = I2C5_IRQn,
    .core     = I2C5_CORE,
    .open_flag= RT_DEVICE_FLAG_RDWR,
};
struct rt_i2c_configuration i2c5_cfg_default = 
{
    0,       // mode;  RT_I2C_ADDR_10BIT / RT_I2C_NO_START / RT_I2C_IGNORE_NACK / RT_I2C_NO_READ_ACK
    0,       // addr;
    5000,    // timeout;
    400000   // max_hz;
}; 

bf0_i2c_t i2c5_obj;
bf0_i2c_t *const bf0_i2c5 = &i2c5_obj;
void I2C5_IRQHandler(void)
{
    I2Cx_IRQHandler(&i2c5_obj); 
}
#endif

#if defined(I2C6)
bf0_i2c_config_t bf0_i2c6_cfg =
{
    .device_name = "i2c6",
    .Instance = I2C6,
    .irq_type = I2C6_IRQn,
    .core     = I2C6_CORE,
    .open_flag= RT_DEVICE_FLAG_RDWR,
};
struct rt_i2c_configuration i2c6_cfg_default = 
{
    0,       // mode;  RT_I2C_ADDR_10BIT / RT_I2C_NO_START / RT_I2C_IGNORE_NACK / RT_I2C_NO_READ_ACK
    0,       // addr;
    5000,    // timeout;
    400000   // max_hz;
}; 

bf0_i2c_t i2c6_obj;
bf0_i2c_t *const bf0_i2c6 = &i2c6_obj;
void I2C6_IRQHandler(void)
{
    I2Cx_IRQHandler(&i2c6_obj); 
}
#endif
static void I2Cx_DMA_IRQHandler(bf0_i2c_t *obj)
{
    I2C_HandleTypeDef *handle;
    handle = &(obj->handle);
    if (handle->State == HAL_I2C_STATE_BUSY_TX)
    {
        HAL_DMA_IRQHandler(handle->hdmatx);
    }
    else if (handle->State == HAL_I2C_STATE_BUSY_RX)
    {
        HAL_DMA_IRQHandler(handle->hdmarx);
    }
    else
    {
        /*The I2C_handle->State is READY before this DMA IRQ sometimes.*/
        if (handle->hdmatx != NULL)
            if (HAL_DMA_STATE_BUSY == handle->hdmatx->State)
                HAL_DMA_IRQHandler(handle->hdmatx);

        if (handle->hdmarx != NULL)
            if (HAL_DMA_STATE_BUSY == handle->hdmarx->State)
                HAL_DMA_IRQHandler(handle->hdmarx);
    }


}
#if defined(I2C1_DMA_INSTANCE)
struct dma_config i2c1_trx_dma = 
{ 
    .dma_irq_prio = I2C1_DMA_IRQ_PRIO,
    .Instance = I2C1_DMA_INSTANCE,
    .dma_irq = I2C1_DMA_IRQ, 
    .request = I2C1_DMA_REQUEST, 
};

#endif
#if defined(I2C1_DMA_IRQHandler)
void I2C1_DMA_IRQHandler(void)
{
    I2Cx_DMA_IRQHandler(&i2c1_obj);
}
#endif

#if defined(I2C2_DMA_INSTANCE)
struct dma_config i2c2_trx_dma = 
{ 
    .dma_irq_prio = I2C2_DMA_IRQ_PRIO,
    .Instance = I2C2_DMA_INSTANCE,
    .dma_irq = I2C2_DMA_IRQ, 
    .request = I2C2_DMA_REQUEST, 
};
#endif
#if defined(I2C2_DMA_IRQHandler)
void I2C2_DMA_IRQHandler(void)
{
    I2Cx_DMA_IRQHandler(&i2c2_obj);
}
#endif
#if defined(I2C3_DMA_INSTANCE)
struct dma_config i2c3_trx_dma = 
{ 
    .dma_irq_prio = I2C3_DMA_IRQ_PRIO,
    .Instance = I2C3_DMA_INSTANCE,
    .dma_irq = I2C3_DMA_IRQ, 
    .request = I2C3_DMA_REQUEST, 
};
#endif
#if defined(I2C3_DMA_IRQHandler)
void I2C3_DMA_IRQHandler(void)
{
    I2Cx_DMA_IRQHandler(&i2c3_obj);
}
#endif
#if defined(I2C4_DMA_INSTANCE)
struct dma_config i2c4_trx_dma = 
{ 
    .dma_irq_prio = I2C4_DMA_IRQ_PRIO,
    .Instance = I2C4_DMA_INSTANCE,
    .dma_irq = I2C4_DMA_IRQ, 
    .request = I2C4_DMA_REQUEST, 
};
#endif
#if defined(I2C4_DMA_IRQHandler)
void I2C4_DMA_IRQHandler(void)
{
    I2Cx_DMA_IRQHandler(&i2c4_obj);
}
#endif
#if defined(I2C5_DMA_INSTANCE)
struct dma_config i2c4_trx_dma = 
{ 
    .dma_irq_prio = I2C5_DMA_IRQ_PRIO,
    .Instance = I2C5_DMA_INSTANCE,
    .dma_irq = I2C5_DMA_IRQ, 
    .request = I2C5_DMA_REQUEST, 
};
#endif
#if defined(I2C5_DMA_IRQHandler)
void I2C5_DMA_IRQHandler(void)
{
    I2Cx_DMA_IRQHandler(&i2c5_obj);
}
#endif
#if defined(I2C6_DMA_INSTANCE)
struct dma_config i2c4_trx_dma = 
{ 
    .dma_irq_prio = I2C6_DMA_IRQ_PRIO,
    .Instance = I2C6_DMA_INSTANCE,
    .dma_irq = I2C6_DMA_IRQ, 
    .request = I2C6_DMA_REQUEST, 
};
#endif
#if defined(I2C6_DMA_IRQHandler)
void I2C6_DMA_IRQHandler(void)
{
    I2Cx_DMA_IRQHandler(&i2c6_obj);
}
#endif
static uint32_t master_xfer(bf0_i2c_t *objs, struct rt_i2c_msg msgs[], uint32_t num)
{
    uint32_t ret = (0);

    uint32_t index = 0;
    bf0_i2c_t *bf0_i2c = NULL;
    struct rt_i2c_msg *msg = NULL;
    HAL_StatusTypeDef status;
    uint16_t mem_addr_type;

    PBL_ASSERTN(objs != NULL);
    bf0_i2c = objs;
    PBL_LOG(LOG_LEVEL_ALWAYS, "i2cmaster_xfer start");

    __HAL_I2C_ENABLE(&bf0_i2c->handle);
    for (index = 0; index < num; index++)
    {

        msg = &msgs[index];
        if (msg->flags & RT_I2C_MEM_ACCESS)
        {
            if (8 >= msg->mem_addr_size)
            {
                mem_addr_type = I2C_MEMADD_SIZE_8BIT;
            }
            else
            {
                mem_addr_type = I2C_MEMADD_SIZE_16BIT;
            }
            if (msg->flags & RT_I2C_RD)
            {
                if ((bf0_i2c->i2c_dma_flag) && (bf0_i2c->bf0_i2c_cfg->open_flag & RT_DEVICE_FLAG_DMA_RX))
                {
                    HAL_DMA_Init(&bf0_i2c->dma.dma_rx);
                    mpu_dcache_invalidate(msg->buf, msg->len);
                    status = HAL_I2C_Mem_Read_DMA(&bf0_i2c->handle, msg->addr, msg->mem_addr, mem_addr_type, msg->buf, msg->len);
                }
                else if (bf0_i2c->bf0_i2c_cfg->open_flag & RT_DEVICE_FLAG_INT_RX)
                {
                    status = HAL_I2C_Mem_Read_IT(&bf0_i2c->handle, msg->addr, msg->mem_addr, mem_addr_type, msg->buf, msg->len);
                }
                //else if (msg->flags & RT_I2C_NORMAL_MODE)
                else
                {
                    status = HAL_I2C_Mem_Read(&bf0_i2c->handle, msg->addr, msg->mem_addr, mem_addr_type, msg->buf, msg->len, bf0_i2c->bf0_i2c_cfg->timeout);
                }
            }
            else
            {
                if ((bf0_i2c->i2c_dma_flag) && (bf0_i2c->bf0_i2c_cfg->open_flag & RT_DEVICE_FLAG_DMA_TX))
                {
                    HAL_DMA_Init(&bf0_i2c->dma.dma_tx);
                    status = HAL_I2C_Mem_Write_DMA(&bf0_i2c->handle, msg->addr, msg->mem_addr, mem_addr_type, msg->buf, msg->len);
                }
                else if (bf0_i2c->bf0_i2c_cfg->open_flag & RT_DEVICE_FLAG_INT_TX)
                {
                    status = HAL_I2C_Mem_Write_IT(&bf0_i2c->handle, msg->addr, msg->mem_addr, mem_addr_type, msg->buf, msg->len);
                }
                //else if (msg->flags & RT_I2C_NORMAL_MODE)
                else
                {
                    status = HAL_I2C_Mem_Write(&bf0_i2c->handle, msg->addr, msg->mem_addr, mem_addr_type, msg->buf, msg->len, bf0_i2c->bf0_i2c_cfg->timeout);
                }

            }
        }
        else
        {
            if (msg->flags & RT_I2C_RD)
            {
                if ((bf0_i2c->i2c_dma_flag) && (bf0_i2c->bf0_i2c_cfg->open_flag & RT_DEVICE_FLAG_DMA_RX))
                {
                    HAL_DMA_Init(&bf0_i2c->dma.dma_rx);
                    mpu_dcache_invalidate(msg->buf, msg->len);
                    status = HAL_I2C_Master_Receive_DMA(&bf0_i2c->handle, msg->addr, msg->buf, msg->len);
                }
                else if (bf0_i2c->bf0_i2c_cfg->open_flag & RT_DEVICE_FLAG_INT_RX)
                {
                    status = HAL_I2C_Master_Receive_IT(&bf0_i2c->handle, msg->addr, msg->buf, msg->len);
                }
                //else if (msg->flags & RT_I2C_NORMAL_MODE)
                else
                {
                    status = HAL_I2C_Master_Receive(&bf0_i2c->handle, msg->addr, msg->buf, msg->len, bf0_i2c->bf0_i2c_cfg->timeout);
                }
            }
            else
            {
                if ((bf0_i2c->i2c_dma_flag) && (bf0_i2c->bf0_i2c_cfg->open_flag & RT_DEVICE_FLAG_DMA_TX))
                {
                    HAL_DMA_Init(&bf0_i2c->dma.dma_tx);
                    status = HAL_I2C_Master_Transmit_DMA(&bf0_i2c->handle, msg->addr, msg->buf, msg->len);
                }
                else if (bf0_i2c->bf0_i2c_cfg->open_flag & RT_DEVICE_FLAG_INT_TX)
                {
                    status = HAL_I2C_Master_Transmit_IT(&bf0_i2c->handle, msg->addr, msg->buf, msg->len);
                }
                else
                {
                    status = HAL_I2C_Master_Transmit(&bf0_i2c->handle, msg->addr, msg->buf, msg->len, bf0_i2c->bf0_i2c_cfg->timeout);
                }
            }
        }

        if (HAL_OK != status) goto exit;

        while (1)
        {
            HAL_I2C_StateTypeDef i2c_state = HAL_I2C_GetState(&bf0_i2c->handle);

            if (HAL_I2C_STATE_READY == i2c_state)
            {
                status = HAL_OK;
            }
            else if (HAL_I2C_STATE_TIMEOUT == i2c_state)
            {
                status = HAL_TIMEOUT;
            }
            else if ((HAL_I2C_STATE_BUSY_TX == i2c_state) || (HAL_I2C_STATE_BUSY_RX == i2c_state)) //Interrupt or DMA mode, wait semaphore
            {
                if(prv_semaphore_wait(objs) == pdTRUE)
                {
                    //已获取到二值信号量，二值信号量值-1，可以开始处理输出
                    continue;
                }
                else
                {
                    PBL_LOG(LOG_LEVEL_ALWAYS, "i2c sem timeout!");
                    status = HAL_TIMEOUT;   
                    prv_analytics_track_i2c_error();
                }
 
            }
            else
            {
                status = HAL_ERROR;
            }

            break;
        }

        if (objs->handle.ErrorCode) goto exit;

        if (HAL_OK != status) goto exit;

#if 0
        //Wordaround: I2C Wait TE fail in 'I2C_MasterRequestWrite' after ISR mode read(After 58x)  - Fixed
        bf0_i2c->handle.Instance->CR |= I2C_CR_UR;
        HAL_Delay_us(1);        // Delay at least 9 cycle.
        bf0_i2c->handle.Instance->CR &= ~I2C_CR_UR;
#endif
 
    }
    ret = index;

exit:
    if ((ret != num) || (HAL_OK != status))
    {
        PBL_LOG(LOG_LEVEL_ALWAYS,"bus err:%d, xfer:%d/%d, i2c_stat:%x, i2c_errcode=%x", status, (int)index, (int)num, HAL_I2C_GetState(&bf0_i2c->handle), (int)bf0_i2c->handle.ErrorCode);
        HAL_I2C_Reset(&bf0_i2c->handle);
        PBL_LOG(LOG_LEVEL_ALWAYS,"reset and send 9 clks");
    }
    __HAL_I2C_DISABLE(&bf0_i2c->handle);

    PBL_LOG(LOG_LEVEL_ALWAYS,"master_xfer end");

    return ret;
}
int rt_i2c_transfer(bf0_i2c_t *objs, struct rt_i2c_msg msgs[], uint32_t num)
{
    int  ret = 0;    
    mutex_lock(objs->mutex);
    master_xfer(objs, msgs, num);
    mutex_unlock(objs->mutex);
    return ret;
}

int i2c_bus_configure(bf0_i2c_t *objs, struct rt_i2c_configuration *configuration)
{
    int  ret = 0;

    PBL_LOG(LOG_LEVEL_ALWAYS, "i2c_bus_configure start");

    PBL_ASSERTN(objs != NULL);
    PBL_ASSERTN(configuration != NULL);

    if (configuration->mode & RT_I2C_ADDR_10BIT)
    {
        objs->handle.Init.AddressingMode = I2C_ADDRESSINGMODE_10BIT;
        objs->handle.Init.OwnAddress1 = (configuration->addr & 0x7fff);
    }
    else
    {
        objs->handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
        objs->handle.Init.OwnAddress1 = (configuration->addr & 0x7fff) << 1;
    }
    objs->bf0_i2c_cfg->open_flag = RT_DEVICE_FLAG_RDWR | configuration->open_flag ;
    objs->bf0_i2c_cfg->timeout = configuration->timeout;
    objs->handle.Init.ClockSpeed = configuration->max_hz;
    objs->handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    objs->handle.core = objs->bf0_i2c_cfg->core;
    objs->handle.Mode = HAL_I2C_MODE_MASTER;

    memcpy(objs->i2c_configuration,
           configuration, sizeof(struct rt_i2c_configuration));
    #if defined(I2C1)               
    if (objs->handle.Instance == hwp_i2c1)
        HAL_RCC_EnableModule(RCC_MOD_I2C1);
    #endif
    #if defined(I2C2)  
    if (objs->handle.Instance == hwp_i2c2)
        HAL_RCC_EnableModule(RCC_MOD_I2C2);
    #endif
    #if defined(I2C3)  
    if (objs->handle.Instance == hwp_i2c3)
        HAL_RCC_EnableModule(RCC_MOD_I2C3);
    #endif
    #if defined(I2C4)      
    if (objs->handle.Instance == hwp_i2c4)
        HAL_RCC_EnableModule(RCC_MOD_I2C4);
    #endif
    #if defined(I2C5)  
    if (objs->handle.Instance == hwp_i2c5)
        HAL_RCC_EnableModule(RCC_MOD_I2C5);
    #endif    
    #if defined(I2C6)  
    if (objs->handle.Instance == hwp_i2c6)
        HAL_RCC_EnableModule(RCC_MOD_I2C6);
    #endif

    ret = HAL_I2C_Init(&(objs->handle));

    PBL_LOG(LOG_LEVEL_ALWAYS, "i2c_bus_configure end");

    return ret;
}
int rt_i2c_configure(bf0_i2c_t *objs, struct rt_i2c_configuration *cfg)
{
    int ret =0;
    PBL_ASSERTN(objs != NULL);
    /* take lock */
    mutex_lock(objs->mutex);
    i2c_bus_configure(objs, cfg);
    /* release lock */
    mutex_unlock(objs->mutex);
    return ret;
}
static void i2c_get_dma_info(void)
{
#if defined(I2C1_DMA_INSTANCEC) && defined(I2C1)
    i2c1_obj.i2c_dma_flag = 1;
    i2c1_obj.dma_rx = &i2c1_trx_dma;
    i2c1_obj.dma_tx = &i2c1_trx_dma;
#endif
#if defined(I2C2_DMA_INSTANCEC) && defined(I2C2)
    i2c2_obj.i2c_dma_flag = 1;
    i2c2_obj.dma_rx = &i2c2_trx_dma;
    i2c2_obj.dma_tx = &i2c2_trx_dma;
#endif
#if defined(I2C3_DMA_INSTANCEC) && defined(I2C3)
    i2c3_obj.i2c_dma_flag = 1;
    i2c3_obj.dma_rx = &i2c3_trx_dma;
    i2c3_obj.dma_tx = &i2c3_trx_dma;
#endif
#if defined(I2C4_DMA_INSTANCEC) && defined(I2C4)
    i2c4_obj.i2c_dma_flag = 1;
    i2c4_obj.dma_rx = &i2c4_trx_dma;
    i2c4_obj.dma_tx = &i2c4_trx_dma;
#endif
#if defined(I2C5_DMA_INSTANCEC) && defined(I2C5)
    i2c5_obj.i2c_dma_flag = 1;
    i2c5_obj.dma_rx = &i2c5_trx_dma;
    i2c5_obj.dma_tx = &i2c5_trx_dma;
#endif
#if defined(I2C6_DMA_INSTANCEC) && defined(I2C6)
    i2c6_obj.i2c_dma_flag = 1;
    i2c6_obj.dma_rx = &i2c6_trx_dma;
    i2c6_obj.dma_tx = &i2c6_trx_dma;
#endif
}

int rt_hw_i2c_init(bf0_i2c_t *objs, bf0_i2c_config_t *cfg, struct rt_i2c_configuration *cfg_default)
{
    int ret = 0;

    objs->bf0_i2c_cfg = cfg;
    objs->i2c_configuration = cfg_default;
    objs->handle.Instance = cfg->Instance;

    if (objs->i2c_dma_flag)
    {
        __HAL_LINKDMA(&(objs->handle), hdmarx, objs->dma.dma_rx);
        __HAL_LINKDMA(&(objs->handle), hdmatx, objs->dma.dma_tx);
        HAL_I2C_DMA_Init(&(objs->handle), cfg->dma_rx, cfg->dma_tx);
    }
    i2c_bus_configure(objs, cfg_default);
    objs->sema = xSemaphoreCreateBinary();
    objs->mutex = mutex_create();
    return ret;
}
void i2c_init()
{
    i2c_get_dma_info();
    #if defined(I2C1)
    rt_hw_i2c_init(&i2c1_obj, &bf0_i2c1_cfg, &i2c1_cfg_default);
    #endif
    #if defined(I2C2)
    rt_hw_i2c_init(&i2c2_obj, &bf0_i2c2_cfg, &i2c2_cfg_default);
    #endif
    #if defined(I2C3)
    rt_hw_i2c_init(&i2c3_obj, &bf0_i2c3_cfg, &i2c3_cfg_default);
    #endif
    #if defined(I2C4)
    rt_hw_i2c_init(&i2c4_obj, &bf0_i2c4_cfg, &i2c4_cfg_default);
    #endif
    #if defined(I2C5)
    rt_hw_i2c_init(&i2c5_obj, &bf0_i2c5_cfg, &i2c5_cfg_default);
    #endif
    #if defined(I2C6)
    rt_hw_i2c_init(&i2c6_obj, &bf0_i2c6_cfg, &i2c6_cfg_default);
    #endif
}

/*------------------------ANALYTICS----------------------------------*/

static void prv_analytics_track_i2c_error(void) {
    analytics_inc(ANALYTICS_DEVICE_METRIC_I2C_ERROR_COUNT, AnalyticsClient_System);
  }
  
  void analytics_external_collect_i2c_stats(void) {
    analytics_set(ANALYTICS_DEVICE_METRIC_I2C_MAX_TRANSFER_DURATION_TICKS,
                  s_max_transfer_duration_ticks, AnalyticsClient_System);
    s_max_transfer_duration_ticks = 0;
  }  #endif
