

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "os/mutex.h"
#include "os/tick.h"
#include "drivers/dma.h"
#include "drivers/gpio.h"
#include "drivers/periph_config.h"
#include "system/passert.h"
#include "drivers/sf32lb/i2c_hal_definitions.h"
#include "drivers/i2c_definitions.h"
#include "board/board_sf32lb.h"
#include "bf0_pin_const.h"
//#ifdef HAL_I2C_MODULE_ENABLED

static void hal_semaphore_give(I2CBusState *bus_state) 
{
    // If this fails, something is very wrong
    xSemaphoreGive(bus_state->event_semaphore);
}

static void I2Cx_IRQHandler(struct I2CBusHal *hal)
{
    I2C_HandleTypeDef *handle;
    I2CBus *bus = hal->bus;
    handle = &(hal->handle);

    if (handle->XferISR != NULL)
    {
        handle->XferISR(handle, 0, 0);
    }

    if ((HAL_I2C_STATE_BUSY_TX != handle->State) && (HAL_I2C_STATE_BUSY_RX != handle->State))
    {
        hal_semaphore_give(bus->state); 
        HAL_I2C_StateTypeDef i2c_state = HAL_I2C_GetState(handle);
        if(i2c_state == HAL_I2C_STATE_READY)
            bus->state->transfer_event = I2CTransferEvent_TransferComplete;
        else if(i2c_state == HAL_I2C_STATE_TIMEOUT)
            bus->state->transfer_event = I2CTransferEvent_Timeout;
        else
            bus->state->transfer_event = I2CTransferEvent_Error;
        __HAL_I2C_DISABLE(handle);
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
struct I2CBusHal i2c1_hal_obj;


void I2C1_IRQHandler(void)
{
    I2Cx_IRQHandler(&i2c1_hal_obj); 
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

struct I2CBusHal i2c2_hal_obj;

void I2C2_IRQHandler(void)
{
    I2Cx_IRQHandler(&i2c2_hal_obj); 
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

struct I2CBusHal i2c3_hal_obj;

void I2C3_IRQHandler(void)
{
    I2Cx_IRQHandler(&i2c3_hal_obj);  
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

struct I2CBusHal i2c4_hal_obj;

void I2C4_IRQHandler(void)
{
    I2Cx_IRQHandler(&i2c4_hal_obj); 
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

struct I2CBusHal i2c5_hal_obj;

void I2C5_IRQHandler(void)
{
    I2Cx_IRQHandler(&i2c5_hal_obj); 
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

struct I2CBusHal i2c6_hal_obj;

void I2C6_IRQHandler(void)
{
    I2Cx_IRQHandler(&i2c6_hal_obj); 
}
#endif
static void I2Cx_DMA_IRQHandler(struct I2CBusHal *hal )
{
    I2C_HandleTypeDef *handle;
    handle = &(hal->handle);
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
struct rt_i2c_msg msgs[2];
uint32_t msgs_num;
static HAL_StatusTypeDef master_xfer(struct I2CBusHal * i2c_hal, struct rt_i2c_msg msgs[], uint32_t num)
{
    uint32_t index = 0;
    struct I2CBusHal *bf0_i2c = NULL;
    struct rt_i2c_msg *msg = NULL;
    HAL_StatusTypeDef status;
    uint16_t mem_addr_type;

    PBL_ASSERTN(i2c_hal != NULL);
    bf0_i2c = i2c_hal;
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
#if 1
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
                //if(prv_semaphore_wait(objs) == pdTRUE)
                status = HAL_BUSY;                
            }
            else
            {
                status = HAL_ERROR;
            }

            break;
        }

        if (bf0_i2c->handle.ErrorCode) goto exit;

        if (HAL_OK != status) goto exit;

#if 1
        //Wordaround: I2C Wait TE fail in 'I2C_MasterRequestWrite' after ISR mode read(After 58x)  - Fixed
        bf0_i2c->handle.Instance->CR |= I2C_CR_UR;
        HAL_Delay_us(1);        // Delay at least 9 cycle.
        bf0_i2c->handle.Instance->CR &= ~I2C_CR_UR;
#endif
#endif
 
    }


exit:
#if  0
    if ((ret != num) || (HAL_OK != status))
    {
        PBL_LOG(LOG_LEVEL_ALWAYS,"bus err:%d, xfer:%d/%d, i2c_stat:%x, i2c_errcode=%x", status, (int)index, (int)num, HAL_I2C_GetState(&bf0_i2c->handle), (int)bf0_i2c->handle.ErrorCode);
        HAL_I2C_Reset(&bf0_i2c->handle);
        PBL_LOG(LOG_LEVEL_ALWAYS,"reset and send 9 clks");
    }
#endif     

    if(status != HAL_BUSY)
        __HAL_I2C_DISABLE(&bf0_i2c->handle);

    PBL_LOG(LOG_LEVEL_ALWAYS,"master_xfer");

    return status;
}

void i2c_hal_init_transfer(I2CBus *bus)
{
    if(I2CTransferType_SendRegisterAddress == bus->state->transfer.type)
    {
        if(bus->state->transfer.direction == I2CTransferDirection_Write)
        {
            msgs[0].addr   = bus->state->transfer.device_address;
            msgs[0].mem_addr = bus->state->transfer.register_address;
            msgs[0].mem_addr_size = I2C_MEMADD_SIZE_8BIT;
            msgs[0].flags  = RT_I2C_WR | RT_I2C_MEM_ACCESS;
            msgs[0].len    = bus->state->transfer.size;
            msgs[0].buf    = bus->state->transfer.data;
            msgs_num = 1;
         }
        else 
        {
            msgs[0].addr   = bus->state->transfer.device_address;
            msgs[0].mem_addr = bus->state->transfer.register_address;
            msgs[0].mem_addr_size = I2C_MEMADD_SIZE_8BIT;
            msgs[0].flags  = RT_I2C_RD | RT_I2C_MEM_ACCESS;
            msgs[0].len    = bus->state->transfer.size;
            msgs[0].buf    = bus->state->transfer.data;
            msgs_num = 1;
        }


    }
    else
    {
        if(bus->state->transfer.direction == I2CTransferDirection_Write)
        {
            msgs[0].addr   = bus->state->transfer.device_address;
            msgs[0].flags  = RT_I2C_WR;
            msgs[0].len    = bus->state->transfer.size;
            msgs[0].buf    = bus->state->transfer.data;
            msgs_num = 1;
         }
        else 
        {
            msgs[0].addr   = bus->state->transfer.device_address;
            msgs[0].flags  = RT_I2C_RD ;
            msgs[0].len    = bus->state->transfer.size;
            msgs[0].buf    = bus->state->transfer.data;
            msgs_num = 1;
        } 

    }  
}

void i2c_hal_abort_transfer(I2CBus *bus)
{
    struct I2CBusHal * hal = (struct I2CBusHal * )bus->hal;
    HAL_I2C_Reset(&(hal->handle));
    //I2C_HandleTypeDef
    PBL_LOG(LOG_LEVEL_ALWAYS,"reset and send 9 clks");
    __HAL_I2C_DISABLE(&(hal->handle));
}
void i2c_hal_start_transfer(I2CBus *bus)
{
    struct I2CBusHal * hal = (struct I2CBusHal * )bus->hal;
    HAL_StatusTypeDef status = master_xfer(hal, &msgs[0], msgs_num);
    if(status ==HAL_BUSY)
        return;
    if(status == HAL_OK)
        bus->state->transfer_event = I2CTransferEvent_TransferComplete;
    else if(status == HAL_TIMEOUT)
        bus->state->transfer_event = I2CTransferEvent_Timeout;
    else
        bus->state->transfer_event = I2CTransferEvent_Error;
    return;
}

int i2c_bus_configure(struct I2CBusHal * i2c_hal, struct rt_i2c_configuration *configuration)
{
    int  ret = 0;

    PBL_LOG(LOG_LEVEL_ALWAYS, "i2c_bus_configure start");

    PBL_ASSERTN(i2c_hal != NULL);
    PBL_ASSERTN(configuration != NULL);

    if (configuration->mode & RT_I2C_ADDR_10BIT)
    {
        i2c_hal->handle.Init.AddressingMode = I2C_ADDRESSINGMODE_10BIT;
        i2c_hal->handle.Init.OwnAddress1 = (configuration->addr & 0x7fff);
    }
    else
    {
        i2c_hal->handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
        i2c_hal->handle.Init.OwnAddress1 = (configuration->addr & 0x7fff) << 1;
    }
    i2c_hal->bf0_i2c_cfg->open_flag = RT_DEVICE_FLAG_RDWR | configuration->open_flag ;
    i2c_hal->bf0_i2c_cfg->timeout = configuration->timeout;
    i2c_hal->handle.Init.ClockSpeed = configuration->max_hz;
    i2c_hal->handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    i2c_hal->handle.core = i2c_hal->bf0_i2c_cfg->core;
    i2c_hal->handle.Mode = HAL_I2C_MODE_MASTER;


    #if defined(I2C1)               
    if (i2c_hal->handle.Instance == hwp_i2c1)
        HAL_RCC_EnableModule(RCC_MOD_I2C1);
    #endif
    #if defined(I2C2)  
    if (i2c_hal->handle.Instance == hwp_i2c2)
        HAL_RCC_EnableModule(RCC_MOD_I2C2);
    #endif
    #if defined(I2C3)  
    if (i2c_hal->handle.Instance == hwp_i2c3)
        HAL_RCC_EnableModule(RCC_MOD_I2C3);
    #endif
    #if defined(I2C4)      
    if (i2c_hal->handle.Instance == hwp_i2c4)
        HAL_RCC_EnableModule(RCC_MOD_I2C4);
    #endif
    #if defined(I2C5)  
    if (i2c_hal->handle.Instance == hwp_i2c5)
        HAL_RCC_EnableModule(RCC_MOD_I2C5);
    #endif    
    #if defined(I2C6)  
    if (i2c_hal->handle.Instance == hwp_i2c6)
        HAL_RCC_EnableModule(RCC_MOD_I2C6);
    #endif

    ret = HAL_I2C_Init(&(i2c_hal->handle));

    PBL_LOG(LOG_LEVEL_ALWAYS, "i2c_bus_configure end");

    return ret;
}




void i2c_hal_enable(I2CBus *bus)
{
    if(strcmp(bus->name, "i2c1"))
    {
        #if defined(I2C1)
        HAL_RCC_EnableModule(RCC_MOD_I2C1);
        #endif
    }
    else if(strcmp(bus->name, "i2c2"))
    {
        #if defined(I2C2)
        HAL_RCC_EnableModule(RCC_MOD_I2C2);
        #endif

    }
    else if(strcmp(bus->name, "i2c3"))
    {
        #if defined(I2C3)
        HAL_RCC_EnableModule(RCC_MOD_I2C3);
        #endif

    }
    else if(strcmp(bus->name, "i2c4"))
    {
        #if defined(I2C4)
        HAL_RCC_EnableModule(RCC_MOD_I2C4);
        #endif

    }
    else if(strcmp(bus->name, "i2c5"))
    {
        #if defined(I2C5)
        HAL_RCC_EnableModule(RCC_MOD_I2C5);
        #endif

    }
    else if(strcmp(bus->name, "i2c6"))
    {
        #if defined(I2C6)
        HAL_RCC_EnableModule(RCC_MOD_I2C6);
        #endif
    }


}

void i2c_hal_disable(I2CBus *bus)
{
    if(strcmp(bus->name, "i2c1"))
    {
    #if defined(I2C1)
        HAL_RCC_DisableModule(RCC_MOD_I2C1);
    #endif
    }
    else if(strcmp(bus->name, "i2c2"))
    {
    #if defined(I2C2)
        HAL_RCC_DisableModule(RCC_MOD_I2C2);
    #endif

    }
    else if(strcmp(bus->name, "i2c3"))
    {
    #if defined(I2C3)
        HAL_RCC_DisableModule(RCC_MOD_I2C3);
    #endif

    }
    else if(strcmp(bus->name, "i2c4"))
    {
    #if defined(I2C4)
        HAL_RCC_DisableModule(RCC_MOD_I2C4);
    #endif

    }
    else if(strcmp(bus->name, "i2c5"))
    {
    #if defined(I2C5)
        HAL_RCC_DisableModule(RCC_MOD_I2C5);
    #endif

    }
    else if(strcmp(bus->name, "i2c6"))
    {
    #if defined(I2C6)
        HAL_RCC_DisableModule(RCC_MOD_I2C6);
    #endif
    }


}

bool i2c_hal_is_busy(I2CBus *bus)
{
    bool ret = true;
    struct I2CBusHal * hal = (struct I2CBusHal * )bus->hal;
    if(HAL_I2C_GetState(&(hal->handle)) != HAL_I2C_STATE_READY)
        ret = false;
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

int rt_hw_i2c_init(struct I2CBusHal * i2c_hal, bf0_i2c_config_t *cfg, struct rt_i2c_configuration *cfg_default)
{
    int ret = 0;

    i2c_hal->bf0_i2c_cfg = cfg;
    i2c_hal->i2c_configuration = cfg_default;
    i2c_hal->handle.Instance = cfg->Instance;

    if (i2c_hal->i2c_dma_flag)
    {
        __HAL_LINKDMA(&(i2c_hal->handle), hdmarx, i2c_hal->dma.dma_rx);
        __HAL_LINKDMA(&(i2c_hal->handle), hdmatx, i2c_hal->dma.dma_tx);
        HAL_I2C_DMA_Init(&(i2c_hal->handle), cfg->dma_rx, cfg->dma_tx);
    }
    i2c_bus_configure(i2c_hal, cfg_default);
    return ret;
}


void i2c_hal_init(I2CBus *bus)
{
    PBL_ASSERTN(bus != NULL);
    struct I2CBusHal * hal = (struct I2CBusHal * )bus->hal;
    i2c_get_dma_info();
    
    if(strcmp(bus->name, "i2c1"))
    {
        #if defined(I2C1)
        rt_hw_i2c_init(hal, &bf0_i2c1_cfg, &i2c1_cfg_default);
        #endif
    }
    else if(strcmp(bus->name, "i2c2"))
    {
        #if defined(I2C2)
        rt_hw_i2c_init(hal, &bf0_i2c2_cfg, &i2c2_cfg_default);
        #endif
    }
    else if(strcmp(bus->name, "i2c3"))
    {
        #if defined(I2C3)    
        rt_hw_i2c_init(hal, &bf0_i2c3_cfg, &i2c3_cfg_default);
        #endif
    }
    else if(strcmp(bus->name, "i2c4"))
    {
        #if defined(I2C4)
        rt_hw_i2c_init(hal, &bf0_i2c4_cfg, &i2c4_cfg_default);
        #endif
    }
    else if(strcmp(bus->name, "i2c5"))
    {
        #if defined(I2C5)
        rt_hw_i2c_init(hal, &bf0_i2c5_cfg, &i2c5_cfg_default);
        #endif
    }
    else if(strcmp(bus->name, "i2c6"))
    {
        #if defined(I2C6)
        rt_hw_i2c_init(hal, &bf0_i2c6_cfg, &i2c6_cfg_default);
        #endif
    }
    hal->bus = bus;

}






void i2c_hal_pins_set_gpio(I2CBus *bus)
{
    int pad_sda, pad_scl;
    int hcpu_sda, hcpu_scl;
    pin_function func_sda, func_scl;
    if(strcmp(bus->name, "i2c1"))
    {
        func_sda = I2C1_SDA;
        func_scl = I2C1_SCL;
    }
    else if(strcmp(bus->name, "i2c2"))
    {
        func_sda = I2C2_SDA;
        func_scl = I2C2_SCL;
    }
    else if(strcmp(bus->name, "i2c3"))
    {
        func_sda = I2C3_SDA;
        func_scl = I2C3_SCL;
    }
    else if(strcmp(bus->name, "i2c4"))
    {
        func_sda = I2C4_SDA;
        func_scl = I2C4_SCL;
    }
    else if(strcmp(bus->name, "i2c5"))
    {
        func_sda = I2C5_SDA;
        func_scl = I2C5_SCL;
    }
    else if(strcmp(bus->name, "i2c6"))
    {
        func_sda = I2C6_SDA;
        func_scl = I2C6_SCL;
    }
    else
    {
        func_sda = I2C1_SDA;
        func_scl = I2C1_SCL;
    }

    pad_sda = bus->sda_gpio.gpio_pin;
    pad_scl = bus->scl_gpio.gpio_pin;
    hcpu_sda = (pad_sda > 96) ? 0 : 1;
    hcpu_scl = (pad_scl > 96) ? 0 : 1;
    HAL_PIN_Set(pad_sda, func_sda, PIN_NOPULL, hcpu_sda);
    HAL_PIN_Set(pad_scl, func_scl, PIN_NOPULL, hcpu_scl);
} 
void i2c_hal_pins_set_i2c(I2CBus *bus)
{

}

//#endif
