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
#include "drivers/battery.h"
#include "drivers/gpio.h"
#include "drivers/pmic.h"
#include "system/logging.h"
#include "system/passert.h"
#include "drivers/i2c.h"
#include "charger_eta4662.h"


/////////////////////////////////////////////////////////////////////////////
// @brief 
static void eta4662_i2c_start(void) {
  i2c_use(I2C_ETA4662);
}
static void eta4662_i2c_stop(void) {
  i2c_release(I2C_ETA4662);
}

static uint8_t eta4662_set_value(uint8_t reg, uint8_t reg_bit,uint8_t reg_shift, uint8_t val) {
  uint8_t tmp;
  i2c_read_register(I2C_ETA4662, reg, &tmp);
  tmp = (tmp &(~(reg_bit<< reg_shift))) |(val << reg_shift);
  i2c_write_register(I2C_ETA4662, reg, tmp);
  return tmp;
}

#if 0
static uint8_t eta4662_get_value(uint8_t reg, uint8_t reg_bit, uint8_t reg_shift) {
  uint8_t data = 0;
  uint8_t ret = 0 ;
  ret = i2c_read_register(I2C_ETA4662, reg, &data);
  ret = (data & reg_bit) >> reg_shift;
  return ret;
}
#endif

void eta4662_vin_dpm(uint16_t val) {
  uint8_t regval;
  regval=(val-3880)/80;
  eta4662_set_value(eta4662_reg00,vin_dpm,vin_dpm_shift,regval);
}
 
void eta4662_iin_lim(uint16_t val) {
  uint8_t regval;
  regval=(val-50)/30; 
  eta4662_set_value(eta4662_reg00,iin_limit,iin_limit_shift,regval);
}

void eta4662_trst_dgl(uint8_t val) {
  uint8_t regval;
  if (val <= 8) {
    regval = 0;
  } else if (val > 8 && val <= 12) {
    regval = 1;
  } else if (val > 12 && val <= 16) {
    regval = 2;
  } else if (val > 16) {
    regval = 3;
  }
  eta4662_set_value(eta4662_reg01,trst_dgl,trst_dgl_shift,regval);
}

void eta4662_trst_dur(uint8_t val) {
  uint8_t regval;
  if (val <= 2) {
    regval = 0;
  } else {
    regval = 1;
  }
  eta4662_set_value(eta4662_reg01,trst_dur,trst_dur_shift,regval);
}

void eta4662_en_hiz(uint8_t val) {
  eta4662_set_value(eta4662_reg01,en_hiz,en_hiz_shift,1);
}

void eta4662_charger_enable(uint8_t val) {
  if (val) {
    eta4662_set_value(eta4662_reg01,ceb,ceb_shift,0);
  } else {
    eta4662_set_value(eta4662_reg01,ceb,ceb_shift,1);
  }
}

void eta4662_bat_uvlo(uint16_t val) {
  uint8_t regval;
  if (val <= 2400) {
    regval = 0;
  } else if (val >= 3030) {
    regval = 7;
  } else {
    regval = (val - 2400) / 90;
  }
  eta4662_set_value(eta4662_reg01,vbat_uvlo,vbat_uvlo_shift,regval);
}
 
void eta4662_reg_reset(void) {
  eta4662_set_value(eta4662_reg02,reg_reset,reg_reset_shift,1);
}

void eta4662_wd_reset(void) {
  eta4662_set_value(eta4662_reg02,wd_reset,wd_reset_shift,1);
}

void eta4662_ichrg(uint16_t val) {
  uint8_t regval;
  if (val <= 8) {
    regval = 0;
  } else if (val >= 512) {
    regval = 0x3f;
  } else {
    regval = (val - 8) / 8;
  }
  eta4662_set_value(eta4662_reg02,ichrg,ichrg_shift,regval);
}

void eta4662_idschg(uint16_t val) {
  uint8_t regval;
  if (val <= 200) {
    regval = 0;
  } else if (val >= 3200) {
    regval = 0xf;
  } else {
    regval = (val - 200) / 200;
  }
  eta4662_set_value(eta4662_reg03,idschg,idschg_shift,regval);
}

void eta4662_iterm(uint8_t val) {
  uint8_t regval;
  if (val <= 1) {
    regval = 0;
  } else if (val >= 31) {
    regval = 0xf;
  } else {
    regval = (val - 1) / 2;
  }
  eta4662_set_value(eta4662_reg03,iterm,iterm_shift,regval);
}

void eta4662_vbat_reg(uint16_t val) {
  uint8_t regval;
  if (val <= 3600) {
    regval = 0;
  } else if (val >= 4545) {
    regval = 0x3f;
  } else {
    regval = (val - 3600) / 15;
  }
  eta4662_set_value(eta4662_reg04,vbat_reg1,vbat_reg1_shift,regval);
}

void eta4662_vbat_pre_2v8(uint8_t val) {
  if (val) {
    eta4662_set_value(eta4662_reg04,vbat_pre,vbat_pre_shift,0);
  } else {
    eta4662_set_value(eta4662_reg04,vbat_pre,vbat_pre_shift,1);
  }
}

void eta4662_vrech_100mv(uint8_t val){
  if (val) {
    eta4662_set_value(eta4662_reg04,vrech,vrech_shift,0);
  } else {
    eta4662_set_value(eta4662_reg04,vrech,vrech_shift,1);
  }
}

void eta4662_en_wd_dischg(uint8_t val) {
  if (val) {
    eta4662_set_value(eta4662_reg05,en_wd_dischg,en_wd_dischg_shift,1);
  } else {
    eta4662_set_value(eta4662_reg05,en_wd_dischg,en_wd_dischg_shift,0);
  }
}

void eta4662_wd_time(uint8_t val) {
  uint8_t regval;
  if (val == 0) {
    regval = 0;
  } else if (val < 40) {
    regval = 1;
  } else if (val > 40 && val <= 80) {
    regval = 2;
  } else {
    regval = 3;
  }
  eta4662_set_value(eta4662_reg05, watchdog, watchdog_shift,regval);
}

void eta4662_en_term(uint8_t val) {
  if (val) {
    eta4662_set_value(eta4662_reg05,en_term,en_term_shift,1);
  } else {
    eta4662_set_value(eta4662_reg05,en_term,en_term_shift,1);
  }
}

void eta4662_en_timer(uint8_t val) {
  if (val) {
    eta4662_set_value(eta4662_reg05,en_timer,en_timer_shift,1);
  } else {
    eta4662_set_value(eta4662_reg05,en_timer,en_timer_shift,0);
  }
}

void eta4662_chg_tmr_h(uint8_t val) {
  uint8_t regval;
  if (val <= 3) {
    regval = 0;
  } else if (val > 5 && val <= 8) {
    regval = 2;
  } else {
    regval = 1;
  }
  eta4662_set_value(eta4662_reg05,chg_tmr,chg_tmr_shift,regval);
}

void eta4662_en_term_tmr(uint8_t val) {
  if (val) {
    eta4662_set_value(eta4662_reg05,term_tmr,term_tmr_shift,1);
  } else {
    eta4662_set_value(eta4662_reg05,term_tmr,term_tmr_shift,0);
  }
}

void eta4662_en_ntc(uint8_t val) {
  if (val) {
    eta4662_set_value(eta4662_reg06,en_ntc,en_ntc_shift,1);
  } else {
    eta4662_set_value(eta4662_reg06,en_ntc,en_ntc_shift,0);
  }
}

void eta4662_tmr2x_en(uint8_t val) {
  if (val) {
    eta4662_set_value(eta4662_reg06,tmr2x_en,tmr2x_en_shift,1);
  } else {
    eta4662_set_value(eta4662_reg06,tmr2x_en,tmr2x_en_shift,0);
  }
}

void eta4662_bfet_dis(uint8_t val) {
  if (val) {
    eta4662_set_value(eta4662_reg06,bfet_dis,bfet_dis_shift,0);
  } else {
    eta4662_set_value(eta4662_reg06,bfet_dis,bfet_dis_shift,1);
  }
}

void eta4662_pg_int_ctl(uint8_t val) {
  if (val) {
    eta4662_set_value(eta4662_reg06,pg_int_ctl,pg_int_ctl_shift,1);
  } else {
    eta4662_set_value(eta4662_reg06,pg_int_ctl,pg_int_ctl_shift,0);
  }
}

void eta4662_eoc_int_ctl(uint8_t val){
  if (val) {
    eta4662_set_value(eta4662_reg06,eoc_int_ctl,eoc_int_ctl_shift,1);
  } else {
    eta4662_set_value(eta4662_reg06,eoc_int_ctl,eoc_int_ctl_shift,0);
  }
}

void eta4662_chg_stat_int_ctl(uint8_t val) {
  if (val) {
    eta4662_set_value(eta4662_reg06,chg_stat_int_ctl,chg_stat_int_ctl_shift,1);
  } else {
    eta4662_set_value(eta4662_reg06,chg_stat_int_ctl,chg_stat_int_ctl_shift,0);
  }
}

void eta4662_ntc_int_ctl(uint8_t val) {
  if (val) {
    eta4662_set_value(eta4662_reg06,ntc_int_ctl,ntc_int_ctl_shift,1);
  } else {
    eta4662_set_value(eta4662_reg06,ntc_int_ctl,ntc_int_ctl_shift,0);
  }
}

void eta4662_battovp_int_ctl(uint8_t val) {
  if (val) {
    eta4662_set_value(eta4662_reg06,battovp_int_ctl,battovp_int_ctl_shift,1);
  } else {
    eta4662_set_value(eta4662_reg06,battovp_int_ctl,battovp_int_ctl_shift,0);
  }
}

void eta4662_en_pcb_otp(uint8_t val) {
  if (val)
    eta4662_set_value(eta4662_reg07,enb_pcb_otp,enb_pch_otp_shift,0);
  else
    eta4662_set_value(eta4662_reg07,enb_pcb_otp,enb_pch_otp_shift,1);
}

void eta4662_dis_vindpm(uint8_t val) {
  if (val) {
    eta4662_set_value(eta4662_reg07,dis_vindpm,dis_vindpm_shift,0);
  } else {
    eta4662_set_value(eta4662_reg07,dis_vindpm,dis_vindpm_shift,1);
  }
}

void eta4662_tj_reg(uint8_t val) {
  uint8_t regval;
  if (val <= 60) {
    regval = 0;
  } else if (val > 60 && val <= 80) {
    regval = 1;
  } else if (val > 80 && val <= 100) {
    regval = 2;
  } else {
    regval = 3;
  }
  eta4662_set_value(eta4662_reg07,tj_reg,tj_reg_shift,regval);
}

void eta4662_vsys_reg(uint16_t val){
  uint8_t regval;
  if (val <= 4200) {
    regval = 0;
  } else if (val >= 4950) {
    regval = 0xf;
  } else {
    regval = (val - 4200) / 50;
  }
  eta4662_set_value(eta4662_reg07,vsys_reg,vsys_reg_shift,regval);
}

void eta4662_en_ship_dgl(uint8_t val){
  uint8_t regval;
  if (val <= 1) {
    regval = 0;
  } else if (val > 1 && val <= 2) {
    regval = 1;
  } else if (val > 2 && val <= 4) {
    regval = 2;
  } else if (val > 4 && val <= 8) {
    regval = 3;
  } else {
    regval = 1;
  }
  eta4662_set_value(eta4662_reg09,en_ship_dgl,en_ship_dgl_shift,regval);
}

void eta4662_cold_reset(uint8_t val){
  if (val) {
    eta4662_set_value(eta4662_reg0a,cold_reset,cold_reset_shift,1);
  } else {
    eta4662_set_value(eta4662_reg0a,cold_reset,cold_reset_shift,0);
  }
}

void eta4662_switch_mode(uint8_t val){
  if (val) {
    eta4662_set_value(eta4662_reg0a,switch_mode,switch_mode_shift,1);
  } else {
    eta4662_set_value(eta4662_reg0a,switch_mode,switch_mode_shift,0);
  }
}

void eta4662_dis_vdd(uint8_t val){
  if (val) {
    eta4662_set_value(eta4662_reg0a,dis_vdd,dis_vdd_shift,1);
  } else {
    eta4662_set_value(eta4662_reg0a,dis_vdd,dis_vdd_shift,0);
  }
}

void eta4662_dis_vinovp(uint8_t val){
  if (val) {
    eta4662_set_value(eta4662_reg0a,dis_vinovp,dis_vinovp_shift,0);
  } else {
    eta4662_set_value(eta4662_reg0a,dis_vinovp,dis_vinovp_shift,1);
  }
}

void eta4662_cc_fine(uint8_t val) {
  if (val) {
    eta4662_set_value(eta4662_reg0a,cc_fine,cc_fine_shift,0);
  } else {
    eta4662_set_value(eta4662_reg0a,cc_fine,cc_fine_shift,1);
  }
}

uint8_t get_eta4662_code(uint8_t* val) {
  return i2c_read_register(I2C_ETA4662, eta4662_reg0b, val);
}


/////////////////////////////////////////////////////////////////////////////
static void charger_int_callback(bool *args) {
  PBL_LOG(LOG_LEVEL_ALWAYS, "charger Interrupt!");
}

void chager_init(void) {
  uint8_t id;
  eta4662_i2c_start();
  uint32_t ret = get_eta4662_code(&id);
  if (!ret) {
    eta4662_i2c_stop();
    PBL_LOG(LOG_LEVEL_ALWAYS, "eta4662 read device id error. ID=0x%02x", id);
    return;
  }
  PBL_LOG(LOG_LEVEL_ALWAYS, "Found eta4662 with ID register ID:0x%02x", id);
  
  //2. initialize the configuration
  //进芯片最低电压
  eta4662_vin_dpm(4600);
  //进芯片最大电流
  eta4662_iin_lim(1000);
  //vbat保护电压
  eta4662_bat_uvlo(2940);
  //进电池电流
  eta4662_ichrg(200);
  //电池放电电流最大值mA
  eta4662_idschg(1600);
  //充电截至电流
  eta4662_iterm(10);
  //使能截至电流功能
  eta4662_en_term(1);
  //充电截至电压
  eta4662_vbat_reg(4350);
  //复充电压 下降值：1：100mV 0：200mV
  eta4662_vrech_100mv(0);
  //预充电压阈值，0：3V；1：2.8V
  eta4662_vbat_pre_2v8(0);
  //看门狗复位时间; 0为关闭; 是能需要喂狗 eta4662_wd_reset
  eta4662_wd_time(0);
  //安全充电计时器；超时停止充电
  eta4662_chg_tmr_h(5);
  //安全计时器使能 1：enalbe; 0:disalbe
  eta4662_en_timer(0);
  //NTC开发；0开闭；1打开
  eta4662_en_ntc(0);
  eta4662_dis_vdd(1);
  //过温保护；0：关 1：开
  eta4662_en_pcb_otp(0);
  //充电状态下vsys；需比电池电压高
  eta4662_vsys_reg(4500);
  //使能充电状态变化中断
  eta4662_chg_stat_int_ctl(0);
  //设置硬复位按键时长
  eta4662_trst_dgl(16);
  //设置赢复位断电时长
  eta4662_trst_dur(2);
  //进入船运模式delay时间
  eta4662_en_ship_dgl(5);
  //使能充电
  eta4662_charger_enable(1);
  //进入开关模式
  eta4662_switch_mode(1);
  // i2c disable
  eta4662_i2c_stop();
  
#if 0 // TODO: config interrupt pin. PA26
  gpio_input_init(&BOARD_CONFIG_POWER.charger_int_gpio);
  exti_configure_pin(BOARD_CONFIG_POWER.charger_exti, ExtiTrigger_RisingFalling, charger_int_callback);
  exti_enable(BOARD_CONFIG_POWER.charger_exti);
#endif
  PBL_LOG(LOG_LEVEL_WARNING, "eta4662 init finish.");
}


