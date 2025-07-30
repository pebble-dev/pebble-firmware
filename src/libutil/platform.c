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

#include "util/assert.h"
#include "util/logging.h"
#include "util/rand32.h"

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

// Below are some default implementations for system-specific functions required by libutil.
// These functions assume a working C standard library is linked into the program.
// For programs where this isn't the case (e.g. the Pebble FW),
// alternate implementations need to be provided.
// The functions are defined as WEAK so they may be easily overridden.

// If you are getting link errors due to printf not being defined, you probably
// need to provide your own implementation of the functions below.

WEAK void util_log(const char *filename, int line, const char *string) {
  printf("%s:%d %s\n", filename, line, string);
}

WEAK void util_dbgserial_str(const char *string) {
  printf("%s\n", string);
}

WEAK NORETURN util_assertion_failed(const char *filename, int line) {
  util_log(filename, line, "*** UTIL ASSERT FAILED");
  exit(EXIT_FAILURE);
}

WEAK uint32_t rand32(void) {
  return ((uint32_t)rand() << 1) + (uint32_t)rand();
}
