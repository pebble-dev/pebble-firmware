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
#include "drivers/exti.h"
#include "drivers/gpio.h"
#include "board/board.h"
#include "drivers/otp.h"
#include "drivers/periph_config.h"
#include "system/logging.h"
#include <mcu.h>

#include <stdlib.h>
#include <string.h>
#include "charger_eta4662.h"

void battery_init(void) {
  chager_init();
}

int battery_get_millivolts(void) {
  //ADCVoltageMonitorReading info = battery_read_voltage_monitor();

  // Apologies for the madness numbers.
  // The previous implementation had some approximations in it. The battery voltage is scaled
  // down by a pair of resistors (750k at the top, 470k at the bottom), resulting in a required
  // scaling of (75 + 47) / 47 or roughly 2.56x, but the previous implementation also required
  // fudging the numbers a bit in order to approximate for leakage current (a 73/64 multiple
  // was arbitrarily increased to 295/256). In order to match this previous arbitrary scaling
  // I've chosen new numbers that provide 2.62x scaling, which is the previous 2.56x with the
  // same amount of fudging applied.

  return 3600;//battery_convert_reading_to_millivolts(info, 3599, 1373);
}

bool battery_charge_controller_thinks_we_are_charging_impl(void) {
  return false;//pmic_is_charging();
}

bool battery_is_usb_connected_impl(void) {
  return false;//pmic_is_usb_connected();
}

void battery_set_charge_enable(bool charging_enabled) {
  //pmic_set_charger_state(charging_enabled);
}

// TODO
// This is my understanding from Figure 9 of the datasheet:
// Charger off -> Pre charge -> Fast Charge (constant current) ->
// Fast Charge (constant voltage) -> Maintain Charge -> Maintain Charge Done
//
// The Pre Charge and Charge Termination currents are programmed via I2C
// The Fast Charge current is determined by Rset
//
// There doesn't seem to be a way to change the current in constant current mode
void battery_set_fast_charge(bool fast_charge_enabled) {

}


