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

#include <inttypes.h>
#include <stdbool.h>
#include "fake_bootbits.h"

static uint32_t s_bootbits = 0;

void boot_bit_clear(BootBitValue bit) {
  s_bootbits &= ~bit;
}

bool boot_bit_test(BootBitValue bit) {
  return (s_bootbits & bit);
}

void fake_boot_bit_set(BootBitValue bit) {
  s_bootbits |= bit;
}
