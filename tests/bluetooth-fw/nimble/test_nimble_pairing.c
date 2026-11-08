/* SPDX-FileCopyrightText: 2026 Geert van Dijk */
/* SPDX-License-Identifier: Apache-2.0 */

// Pairing user interaction for the NimBLE driver: numeric comparison (LE Secure Connections) and
// passkey entry with the watch as the display (what Android falls back to for legacy pairing).

#include "clar.h"

#include "stubs_logging.h"

#include "nimble_pairing.h"

#include <bluetooth/pairing_confirm.h>
#include <host/ble_hs.h>

#include <stdint.h>
#include <string.h>

#define CONN_HANDLE 7

static const PairingUserConfirmationCtx *prv_ctx(void) {
  return (const PairingUserConfirmationCtx *)(uintptr_t)CONN_HANDLE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Fakes

static uint32_t s_rng_value;

bool rng_rand(uint32_t *rand_out) {
  *rand_out = s_rng_value;
  return true;
}

static struct {
  int count;
  uint16_t conn_handle;
  struct ble_sm_io io;
} s_injected;

int ble_sm_inject_io(uint16_t conn_handle, struct ble_sm_io *pkey) {
  s_injected.count++;
  s_injected.conn_handle = conn_handle;
  s_injected.io = *pkey;
  return 0;
}

static struct {
  int count;
  uint16_t conn_handle;
  uint8_t reason;
} s_terminated;

int ble_gap_terminate(uint16_t conn_handle, uint8_t hci_reason) {
  s_terminated.count++;
  s_terminated.conn_handle = conn_handle;
  s_terminated.reason = hci_reason;
  return 0;
}

static uint32_t prv_request(uint8_t action, uint32_t numcmp) {
  const struct ble_gap_passkey_params params = {.action = action, .numcmp = numcmp};
  return nimble_pairing_handle_passkey_action(CONN_HANDLE, &params);
}

void test_nimble_pairing__initialize(void) {
  s_rng_value = 0;
  memset(&s_injected, 0, sizeof(s_injected));
  memset(&s_terminated, 0, sizeof(s_terminated));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Passkey entry, watch displays

void test_nimble_pairing__display_passkey_is_given_to_stack_and_shown(void) {
  s_rng_value = 4123456789;

  const uint32_t shown = prv_request(BLE_SM_IOACT_DISP, 0);

  // The code on the watch is the one the phone will be checked against, and is a valid passkey.
  cl_assert_equal_i(shown, 456789);
  cl_assert_equal_i(s_injected.count, 1);
  cl_assert_equal_i(s_injected.conn_handle, CONN_HANDLE);
  cl_assert_equal_i(s_injected.io.action, BLE_SM_IOACT_DISP);
  cl_assert_equal_i(s_injected.io.passkey, shown);
}

void test_nimble_pairing__display_confirm_on_watch_does_not_disturb_stack(void) {
  prv_request(BLE_SM_IOACT_DISP, 0);

  bt_driver_pairing_confirm(prv_ctx(), true);

  // A numeric-comparison answer here would be rejected by NimBLE; nothing more must be sent.
  cl_assert_equal_i(s_injected.count, 1);
  cl_assert_equal_i(s_terminated.count, 0);
}

void test_nimble_pairing__display_reject_on_watch_drops_connection(void) {
  prv_request(BLE_SM_IOACT_DISP, 0);

  bt_driver_pairing_confirm(prv_ctx(), false);

  cl_assert_equal_i(s_injected.count, 1);
  cl_assert_equal_i(s_terminated.count, 1);
  cl_assert_equal_i(s_terminated.conn_handle, CONN_HANDLE);
  cl_assert_equal_i(s_terminated.reason, BLE_ERR_REM_USER_CONN_TERM);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Numeric comparison

void test_nimble_pairing__numcmp_shows_number_and_waits_for_user(void) {
  const uint32_t shown = prv_request(BLE_SM_IOACT_NUMCMP, 123456);

  cl_assert_equal_i(shown, 123456);
  cl_assert_equal_i(s_injected.count, 0);
}

void test_nimble_pairing__numcmp_answer_goes_to_stack(void) {
  prv_request(BLE_SM_IOACT_NUMCMP, 123456);
  bt_driver_pairing_confirm(prv_ctx(), true);

  cl_assert_equal_i(s_injected.count, 1);
  cl_assert_equal_i(s_injected.conn_handle, CONN_HANDLE);
  cl_assert_equal_i(s_injected.io.action, BLE_SM_IOACT_NUMCMP);
  cl_assert_equal_i(s_injected.io.numcmp_accept, 1);

  prv_request(BLE_SM_IOACT_NUMCMP, 654321);
  bt_driver_pairing_confirm(prv_ctx(), false);

  cl_assert_equal_i(s_injected.count, 2);
  cl_assert_equal_i(s_injected.io.action, BLE_SM_IOACT_NUMCMP);
  cl_assert_equal_i(s_injected.io.numcmp_accept, 0);
  cl_assert_equal_i(s_terminated.count, 0);
}

void test_nimble_pairing__numcmp_after_passkey_pairing_uses_numcmp(void) {
  prv_request(BLE_SM_IOACT_DISP, 0);
  bt_driver_pairing_confirm(prv_ctx(), true);

  prv_request(BLE_SM_IOACT_NUMCMP, 111111);
  bt_driver_pairing_confirm(prv_ctx(), true);

  cl_assert_equal_i(s_injected.count, 2);
  cl_assert_equal_i(s_injected.io.action, BLE_SM_IOACT_NUMCMP);
  cl_assert_equal_i(s_injected.io.numcmp_accept, 1);
}
