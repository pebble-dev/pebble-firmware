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

#include "system/logging.h"
#include "drivers/mag.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>


// We don't actually support the mag at all on snowy_evt, as we tried a more complicated
// arrangement where the mag was a slave of the bmi160 chip. We never fully implemented it, and
// as we abandoned that approach there's no point in ever implementing it. Below are a bunch of
// stubs that do nothing on this board.

void mag_use(void) {
}

void mag_start_sampling(void) {
}

void mag_release(void) {
}

MagReadStatus mag_read_data(MagData *data) {
  return MagReadCommunicationFail;
}

bool mag_change_sample_rate(MagSampleRate rate) {
  return false;
}

