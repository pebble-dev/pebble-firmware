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
#include "drivers/als/als.h"
#include "drivers/gpio.h"
#include "drivers/pmic.h"
#include "system/logging.h"
#include "system/passert.h"
#include "drivers/i2c.h"
#include "kernel/util/delay.h"
#include "stk_alps_lib.h"


//////////////////////////////////////////////////////////////////////
#ifndef ARRAY_COUNT
#define ARRAY_COUNT(X)              (sizeof(X) / sizeof(X[0]))
#endif

//#define USE_HW_AGC
//#define ALS_DATA_12BIT
//#define USE_STK_LIB     1
#define ALS_FIFO_DATA_MAX 512
#define ALS_READ_FIFO_BYTE 64
#define ALS_TARGET_FIFO_DATA 64

#define W1160_7BITI2C_ADDRESS       0x48
// Registers
#define W1160_STATE_REG             0x00
#define W1160_IT_FAST1_REG          0x02
#define W1160_IT_FAST2_REG          0x03
#define W1160_SAMPLE_FAST1_REG      0x05
#define W1160_SAMPLE_FAST2_REG      0x06
#define W1160_SAMPLE_SLOW1_REG      0x07
#define W1160_SAMPLE_SLOW2_REG      0x08
#define W1160_FLAG1_REG             0x10
#define W1160_DATA1_ALS_REG         0x13
#define W1160_DATA2_ALS_REG         0x14
#define W1160_DATA_GC_REG           0x17
#define W1160_POWER_MODE_REG        0x3D
#define W1160_PDT_ID_REG            0x3E
#define W1160_FIFOCTRL1_REG         0x60
#define W1160_FIFOCTRL1_REG_VALUE   0x20//0xA0
#define W1160_FIFO1_WM_LV_REG       0x61
#define W1160_FIFO2_WM_LV_REG       0x62
#define W1160_FIFOCTRL2_REG         0x63
#define W1160_FIFO_FCNT1_REG        0x64
#define W1160_FIFO_FCNT2_REG        0x65
#define W1160_FIFO_OUT_REG          0x66
#define W1160_FIFO_FLAG_REG         0x67
#define W1160_FIFOCTRL3_REG         0x68
#define W1160_FIFOCTRL4_REG         0x69
#define W1160_AGCCTRL1_REG          0x6A
#define W1160_MANUAL_GAIN_CTRL_REG  0x6B
#define W1160_AGCCTRL2_REG          0x6C
#define W1160_THD_SAT_GC_REG        0x6D
#define W1160_IT_SLOW1_REG          0x6E
#define W1160_IT_SLOW2_REG          0x6F
#define W1160_SOFT_RESET_REG        0x80
// Configuration register bits
#define W1160_ALSCTRL_REG           0xA4


static const uint8_t GainTable[] = {20, 18, 16, 15, 14, 13, 12, 11, 10, 8, 6, 4, 3, 2, 1, 0};
// Note adjust also ALS_CONVERSION_DELAY if moving to use 800 ms cycle.
#define W1160_MEAS_TIME_100MS 0x3E7

#define W1160_CONFIG_DATA_GC_GAIN_LV0    (0x00<<4) //0.003906
#define W1160_CONFIG_DATA_GC_GAIN_LV1    (0x01<<4) //0.015625
#define W1160_CONFIG_DATA_GC_GAIN_LV2    (0x02<<4) //0.0625
#define W1160_CONFIG_DATA_GC_GAIN_LV3    (0x03<<4) //0.125
#define W1160_CONFIG_DATA_GC_GAIN_LV4    (0x04<<4) //0.25
#define W1160_CONFIG_DATA_GC_GAIN_LV5    (0x05<<4) //0.5
#define W1160_CONFIG_DATA_GC_GAIN_LV6    (0x06<<4) //1
#define W1160_CONFIG_DATA_GC_GAIN_LV7    (0x07<<4) //2
#define W1160_CONFIG_DATA_GC_GAIN_LV8    (0x08<<4) //4
#define W1160_CONFIG_DATA_GC_GAIN_LV9    (0x09<<4) //16
#define W1160_CONFIG_DATA_GC_GAIN_LV10   (0x0A<<4) //64
#define W1160_CONFIG_DATA_GC_GAIN_LV11   (0x0B<<4) //256
#define W1160_CONFIG_DATA_GC_GAIN_LV12   (0x0C<<4) //512
#define W1160_CONFIG_DATA_GC_GAIN_LV13   (0x0D<<4) //1024
#define W1160_CONFIG_DATA_GC_GAIN_LV14   (0x0E<<4) //2048
#define W1160_CONFIG_DATA_GC_GAIN_LV15   (0x0F<<4) //4096

#define W1160_CONFIG_SAMPLE_SLOW_100MS   0x3E7
#define W1160_CONFIG_CMANUAL_GAIN_CTRL_MODE_EN  0x01
#define W1160_CONFIG_CMANUAL_GAIN_CTRL_MODE_DIS 0x00




//////////////////////////////////////////////////////////////////////
// @brief read one byte
static bool w1160_reg_read(uint8_t reg, uint8_t *regvalue) {
  i2c_use(I2C_W1160);
  bool rv = i2c_read_register(I2C_W1160, reg, regvalue);
  i2c_release(I2C_W1160);
  return rv;
}
// @brief write one byte
static bool w1160_reg_write(uint8_t reg, uint8_t val) {
  i2c_use(I2C_W1160);
  bool rv = i2c_write_register(I2C_W1160, reg, val);
  i2c_release(I2C_W1160);
  return rv;
}
#if 0
// @brief write data
static bool w1160_data_write(uint8_t reg, uint8_t *p_data, uint8_t data_len) {
  i2c_use(I2C_W1160);
  bool rv = i2c_read_register_block(I2C_W1160, reg, data_len, p_data);
  i2c_release(I2C_W1160);
  return rv;
}
#endif
// @brief read data
static bool w1160_data_read(uint8_t reg, uint8_t *p_data, uint8_t data_len) {
  i2c_use(I2C_W1160);
  bool rv = i2c_read_register_block(I2C_W1160, reg, data_len, p_data);
  i2c_release(I2C_W1160);
  return rv;
}

// @brief w1160 dump register
static void w1160_registers_dump(void) {
  uint8_t reg_table[27] = {
    0x00, 0x02, 0x03, 0x05, 0x06, 0x07, 0x08, 0x10, 
    0x13, 0x3D, 0x3E, 0x60, 0x61, 0x62, 0x63, 0x67, 
    0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x17, 0x6E, 
    0x6F, 0xA4};
  
  uint32_t i;
  uint16_t word_data;
  uint8_t buf[2];
  uint8_t reg_val;
  for (i=0; i<ARRAY_COUNT(reg_table); ++i) {
    if (reg_table[i] == 0x13) {
      w1160_data_read(reg_table[i], buf, 2);
      word_data = (buf[0]<<8) | buf[1]; 
      PBL_LOG(LOG_LEVEL_DEBUG, "reg[0x%x]=%d", reg_table[i], word_data);
    } else {
      w1160_reg_read(reg_table[i], &reg_val);
      PBL_LOG(LOG_LEVEL_DEBUG, "reg[0x%x]=0x%x", reg_table[i], reg_val);
    }
  }
}

// @brief read fifo data. return read data numbers.
static uint32_t w1160_fifo_data_read(uint32_t* p_data) {
  if (p_data == NULL) {
    return 0;
  }

  uint8_t fifo_power = 0;
  w1160_reg_read(W1160_FIFOCTRL4_REG, &fifo_power);
  if (fifo_power != 0x30){
    w1160_reg_write(W1160_FIFOCTRL4_REG, 0x30);     // Enable FIFO Power and Clock
    w1160_reg_write(W1160_FIFOCTRL3_REG, 0x01);     // clear fifo
    w1160_reg_write(W1160_FIFOCTRL2_REG, 0x00);
    PBL_LOG(LOG_LEVEL_WARNING, "fifo power and clear to restart");
    return 0;
  }

#if !defined(USE_HW_AGC) && !defined(ALS_DATA_12BIT)
  uint8_t gainlevel;
  w1160_reg_read(W1160_DATA_GC_REG, &gainlevel);
#endif

  uint8_t fifo_state;
  w1160_reg_read(0x63, &fifo_state);

  uint16_t fifo_count = 0;
  uint8_t fifo_cnt_value[2] = { 0 };
  w1160_data_read(W1160_FIFO_FCNT1_REG, fifo_cnt_value, 2);
  fifo_count = (fifo_cnt_value[0]<<8) | fifo_cnt_value[1]; // Fix endianess
  fifo_count = fifo_count & 0x1FF;
  PBL_LOG(LOG_LEVEL_DEBUG, "fifo_state=0x%x, fifo_count=%d", fifo_state, fifo_count);
  if (fifo_count >= ALS_TARGET_FIFO_DATA) {
    uint16_t buff_idx = 0;
    uint16_t rlen = ALS_READ_FIFO_BYTE;
    uint8_t buff[2*ALS_TARGET_FIFO_DATA];
    
    memset(buff, 0, sizeof(buff));
    w1160_reg_write(W1160_FIFOCTRL2_REG, 0x10);
    while (buff_idx < sizeof(buff)) {
      if ((sizeof(buff)-buff_idx) < rlen) {
          rlen = sizeof(buff) - buff_idx;
      }
      PBL_LOG(LOG_LEVEL_DEBUG, "buff_idx=%d, rlen=%d", buff_idx, rlen);
      w1160_data_read(W1160_FIFO_OUT_REG, &buff[buff_idx], rlen);
      buff_idx += rlen;
    }

#if !defined(USE_HW_AGC) && !defined(ALS_DATA_12BIT)
    for (uint32_t i=0; i<ALS_TARGET_FIFO_DATA; i++) {
      p_data[i] = (uint32_t)(((((uint16_t)buff[2*i])<<8) | buff[2*i+1])&0xFFFF) << (GainTable[gainlevel>>4]);
    }
#else
    for (uint32_t i=0; i<ALS_TARGET_FIFO_DATA; i++) {
      p_data[i] = (uint32_t)(((((uint16_t)buff[2*i])<<8) | buff[2*i+1]) & 0xFFF) << (GainTable[buff[2*i]>>4]);
    }
#endif
    
    w1160_reg_write(W1160_FIFOCTRL3_REG, 0x01); // clear fifo
    w1160_reg_write(W1160_FIFOCTRL2_REG, 0x00);
    return ALS_TARGET_FIFO_DATA;
  } 
  
  if ((fifo_state & 0x10) != 0) {
    w1160_reg_write(W1160_FIFOCTRL3_REG, 0x01); // clear fifo
    w1160_reg_write(W1160_FIFOCTRL2_REG, 0x00);
  }
  return 0;
}

// @brief read chip id
static uint8_t w1160_chip_id_read(void) {
  uint8_t  regval = 0;
  w1160_reg_read(W1160_PDT_ID_REG, &regval);
  return regval;
}

// @brief w1160 init
static void w1160_init(void) {
  // read chip id
  uint8_t id = w1160_chip_id_read();
  if (id != 0xE5 && id != 0xE0) {
    PBL_LOG(LOG_LEVEL_ALWAYS, "w1160 read chip id failed... chip_id=0x%02x", id);
    return;
  }
  PBL_LOG(LOG_LEVEL_ALWAYS, "Found w1160 with ID register ID:0x%02x", id);

  // Write any data to this register will reset the chip.
  w1160_reg_write(W1160_SOFT_RESET_REG, 0x99); 
  delay_us(50 * 1000);
  
  // related defines in w1160.hpp and w1160_registers.h.
  // NOTE:Although it is feasible to write 16bit data directly to the register.
  // However, the supplier recommends not to do this,
  // because the W1160 datasheet can only write 8bit data
  w1160_reg_write(W1160_IT_FAST1_REG, 0x00);
  w1160_reg_write(W1160_IT_FAST2_REG, 0x20);        // IT 3.168ms
  w1160_reg_write(W1160_SAMPLE_FAST1_REG, 0x00);
  w1160_reg_write(W1160_SAMPLE_FAST2_REG, 0x26);    // 0x26
  w1160_reg_write(W1160_DATA_GC_REG, 0xC0);         // Leven 15 = DGain4096x
  w1160_reg_write(W1160_POWER_MODE_REG, 0x01);      // Fast Mode
  w1160_reg_write(W1160_FIFOCTRL1_REG, W1160_FIFOCTRL1_REG_VALUE); // FIFO Mode
  w1160_reg_write(W1160_FIFO1_WM_LV_REG, 0x00);     // Data Length 256
  w1160_reg_write(W1160_FIFOCTRL4_REG, 0x30);       // Enable FIFO Power and Clock

#ifdef USE_HW_AGC
  w1160_reg_write(W1160_AGCCTRL1_REG, 0x80);        // Enable AGC
  w1160_reg_write(W1160_MANUAL_GAIN_CTRL_REG, 0x00); // Auto Gain Level
  w1160_reg_write(W1160_AGCCTRL2_REG, 0x0C);        //Enable AGC 12bit
  w1160_reg_write(W1160_THD_SAT_GC_REG, 0x0A);
  w1160_reg_write(W1160_ALSCTRL_REG, 0x07);         // ALS Data Format 12bit
#else
  w1160_reg_write(W1160_AGCCTRL1_REG, 0x00);          // Disable AGC
  w1160_reg_write(W1160_MANUAL_GAIN_CTRL_REG, 0x01);  // Fixed Gain Level
  w1160_reg_write(W1160_AGCCTRL2_REG, 0x04);
  w1160_reg_write(W1160_THD_SAT_GC_REG, 0x0A);
  w1160_reg_write(W1160_ALSCTRL_REG, 0x03);         // ALS Data Format 16bit
#endif
  w1160_reg_write(W1160_STATE_REG, 0x02);         // Enable the ALS function.

#if defined(USE_STK_LIB) && (USE_STK_LIB == 1)
  // Calibration Parameter Configure
  // DO NOT MODIFIED !!
  AlgoParam CaliData;
  CaliData.DisplayNode[0].Brightness = 255;
  CaliData.DisplayNode[0].Alpha.ParameterG = 1.02005;
  CaliData.DisplayNode[0].Beta.ParameterG = 1.0f;
  CaliData.DisplayNode[0].Bias.ParameterG = -0.78993;
  CaliData.DisplayNode[0].HighTHD.ChannelG = 2680;
  CaliData.DisplayNode[0].LowTHD.ChannelG = 0;
  CaliData.DebouncePct = 0.3f;
  CaliData.DebounceCount = 2;
  STK_initAlgo(&CaliData);
  delay_us(10 * 1000);
#endif
}

// @brief w1160 deinit
static void w1160_deinit(void) {
  w1160_reg_write(W1160_STATE_REG, 0x00);       // Disable the ALS function.
  w1160_reg_write(W1160_FIFOCTRL4_REG, 0x00);   // Disable FIFO Power and Clock
#if defined(USE_STK_LIB) && (USE_STK_LIB == 1)
  STK_deInitAlgo();
#endif
}

// @brief read ambient light 
static float w1160_ambient_light_lux_get(void) {
#if defined(USE_STK_LIB) && (USE_STK_LIB == 1)
  static float last_lux_data = 0;
  static ChannelDataSet fifo_data;
  static ChannelData ambient_data;
  float luxData = 0.0;
  float scale = 294.4f; // calibration factor  40*3.68
  uint32_t als_fifo_data[ALS_TARGET_FIFO_DATA];
  uint32_t fifo_data_len = w1160_fifo_data_read(als_fifo_data);
  if (fifo_data_len) {
    ChannelData maxData, minData, midData;
    // Convert Sensor FIFO Data to ChannelDataSet Structure
    fifo_data.ChannelG = &als_fifo_data[0];
    fifo_data.Size = ALS_TARGET_FIFO_DATA;
    STK_calcAmbientInfobyFIFO(&fifo_data, &ambient_data, 255, true);

    luxData = ambient_data.ChannelG * (float)scale / (float)1000;
    last_lux_data = luxData;
    PBL_LOG(LOG_LEVEL_DEBUG, "Ambient Raw: %ld, Lux: %f, Scale: %f\n", ambient_data.ChannelG, luxData, scale);
    STK_outputDebugInfo(&fifo_data, &maxData, &minData, &midData);
    PBL_LOG(LOG_LEVEL_DEBUG, "Max G: %ld; Min G: %ld\n", maxData.ChannelG, minData.ChannelG);
  } else {
    luxData = last_lux_data;
    PBL_LOG(LOG_LEVEL_DEBUG, "Ambient Last Lux: %f", last_lux_data);
  }
  return luxData;
#else
  return 0;
#endif
}



//////////////////////////////////////////////////////////////////////
// @brief initialization
void als_init(void) {
  PBL_LOG(LOG_LEVEL_ALWAYS, "---------als_init");
  w1160_init();
}

// @brief deinit
void als_deinit(void) {
  w1160_deinit();
}

// @brief print register value
void als_register_dump(void) {
  w1160_registers_dump();
}

// @brief read ambient light  
float als_ambient_light_lux_get(void) {
  return w1160_ambient_light_lux_get();
}




