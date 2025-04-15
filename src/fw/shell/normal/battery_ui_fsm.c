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

#include "battery_ui.h"

#include <stdint.h>

#include "applib/ui/vibes.h"
#include "apps/system_app_ids.h"
#include "apps/system_apps/battery_critical_app.h"
#include "kernel/low_power.h"
#include "kernel/ui/modals/modal_manager.h"
#include "kernel/util/standby.h"
#include "process_management/app_manager.h"
#include "services/common/battery/battery_curve.h"
#include "services/common/status_led.h"
#include "services/common/vibe_pattern.h"
#include "services/normal/notifications/do_not_disturb.h"
#include "services/normal/vibes/vibe_intensity.h"
#include "shell/normal/watchface.h"
#include "util/ratio.h"
#include "util/size.h"

// The Battery UI state machine keeps track of when to notify the user of a
// change in battery charge state, and when to automatically dismiss the status
// modal window.

#define MAX_TRANSITIONS 6

typedef void (*EntryFunc)(void *);
typedef void (*ExitFunc)(void);

typedef enum BatteryUIStateID {
  BatteryInvalid,
  BatteryGood,
  BatteryWarning,
  BatteryLowPower,
  BatteryCritical,
  BatteryCharging,
  BatteryFullyCharged, // plugged but not charging (aka 100%)
  BatteryShutdownCharging
} BatteryUIStateID;

typedef struct BatteryUIState {
  EntryFunc enter;
  ExitFunc exit;
  BatteryUIStateID next_state[MAX_TRANSITIONS];
} BatteryUIState;

static void prv_display_warning(void *data);
static void prv_dismiss_warning(void);
static void prv_enter_low_power(void *ignored);
static void prv_exit_low_power(void);
static void prv_enter_critical(void *ignored);
static void prv_exit_critical(void);
static void prv_display_plugged(void *data);
static void prv_dismiss_plugged(void);
static void prv_display_fully_charged(void *data);
static void prv_dismiss_fully_charged(void);
// TODO PBL-39883: Replace w/ QUIRK_RESET_ON_SHUTDOWN_WHILE_CHARGING once arbitrary prefixes land
#if PLATFORM_TINTIN || PLATFORM_SILK || PLATFORM_ASTERIX
static void prv_shutdown(void *ignored);
#else
static void prv_enter_shutdown_charging(void *ignored);
#endif

static const BatteryUIState ui_states[] = {
  [BatteryGood] = { .next_state = {
    BatteryWarning, BatteryLowPower, BatteryCritical, BatteryCharging, BatteryFullyCharged
  }},
  [BatteryWarning] = { .enter = prv_display_warning, .exit = prv_dismiss_warning, .next_state = {
    BatteryGood, BatteryWarning, BatteryLowPower, BatteryCharging
  }},
  [BatteryLowPower] = { .enter = prv_enter_low_power, .exit = prv_exit_low_power, .next_state = {
    BatteryWarning, BatteryCritical, BatteryCharging
  }},
  [BatteryCritical] = { .enter = prv_enter_critical, .exit = prv_exit_critical, .next_state = {
    BatteryLowPower, BatteryCharging
  }},
  [BatteryCharging] = { .enter = prv_display_plugged, .exit = prv_dismiss_plugged, .next_state = {
    BatteryGood, BatteryWarning, BatteryLowPower,
    BatteryCritical, BatteryFullyCharged, BatteryShutdownCharging
  }},
  [BatteryFullyCharged] = { .enter = prv_display_fully_charged, .exit = prv_dismiss_fully_charged,
    .next_state = {
      BatteryGood, BatteryWarning, BatteryLowPower, BatteryCritical, BatteryShutdownCharging
  }},
// TODO PBL-39883: Replace w/ QUIRK_RESET_ON_SHUTDOWN_WHILE_CHARGING once arbitrary prefixes land
#if PLATFORM_TINTIN || PLATFORM_SILK || PLATFORM_ASTERIX
  [BatteryShutdownCharging] = { .enter = prv_shutdown }
#else
  [BatteryShutdownCharging] = { .enter = prv_enter_shutdown_charging }
#endif
};

static BatteryUIStateID s_state = BatteryGood;
static BatteryUIWarningLevel s_warning_points_index = -1;

#if PLATFORM_SPALDING
/* first warning for S4 is at 12 hours remaining, second at 6 hours remaining */
static const uint8_t s_warning_points[] = { 12, 6 };
#else
/* first warning is at 18 hours remaining, second at 12 hours remaining */
static const uint8_t s_warning_points[] = { 18, 12 };
#endif

// State functions

static void prv_display_warning(void *data) {
  const uint8_t percent = ratio32_to_percent(((PreciseBatteryChargeState *)data)->charge_percent);
  bool new_warning = false;
  const BatteryUIWarningLevel num_points = ARRAY_LENGTH(s_warning_points) - 1;

  while (s_warning_points_index < num_points && (percent <=
         battery_curve_get_percent_remaining(s_warning_points[s_warning_points_index + 1]))) {
    s_warning_points_index++;
    new_warning = true;
  }

  if (new_warning) {
    if (!do_not_disturb_is_active()) {
      vibes_short_pulse();
    }
    battery_ui_display_warning(percent, s_warning_points_index);
  }
}

static void prv_dismiss_warning(void) {
  battery_ui_dismiss_modal();
  s_warning_points_index = -1;
}

static void prv_enter_low_power(void *ignored) {
#ifndef RECOVERY_FW
  watchface_start_low_power();
  modal_manager_pop_all_below_priority(ModalPriorityAlarm);
  modal_manager_set_min_priority(ModalPriorityAlarm);
  // Override the vibe intensity to Medium in low-power mode
  vibes_set_default_vibe_strength(get_strength_for_intensity(VibeIntensityMedium));
#else
  app_manager_launch_new_app(&(AppLaunchConfig) {
    .md = prf_low_power_app_get_info(),
  });
#endif
}

static void prv_exit_low_power(void) {
#ifndef RECOVERY_FW
  modal_manager_set_min_priority(ModalPriorityMin);
  watchface_launch_default(NULL);
  vibe_intensity_set(vibe_intensity_get());
#else
  app_manager_close_current_app(true);
#endif
}

static void prv_enter_critical(void *ignored) {
  if (!do_not_disturb_is_active()) {
    vibes_short_pulse();
  }
  // in case there is a warning on screen
  modal_manager_pop_all();
  modal_manager_set_min_priority(ModalPriorityMax);
  app_manager_put_launch_app_event(&(AppLaunchEventConfig) {
    .id = APP_ID_BATTERY_CRITICAL,
  });
}

static void prv_exit_critical(void) {
  app_manager_close_current_app(true);
  modal_manager_set_min_priority(ModalPriorityMin);
}

static void prv_display_plugged(void *data) {
  if (!do_not_disturb_is_active()) {
    vibes_short_pulse();
  }
  battery_ui_display_plugged();

  status_led_set(StatusLedState_Charging);
}

static void prv_dismiss_plugged(void) {
  battery_ui_dismiss_modal();

  status_led_set(StatusLedState_Off);
}

static void prv_display_fully_charged(void *data) {
  battery_ui_display_fully_charged();
  status_led_set(StatusLedState_FullyCharged);
}

static void prv_dismiss_fully_charged(void) {
  battery_ui_dismiss_modal();

  status_led_set(StatusLedState_Off);
}

// TODO PBL-39883: Replace w/ QUIRK_RESET_ON_SHUTDOWN_WHILE_CHARGING once arbitrary prefixes land
#if PLATFORM_TINTIN || PLATFORM_SILK || PLATFORM_ASTERIX
static void prv_shutdown(void *ignored) {
  battery_ui_handle_shut_down();
}
#else
static void prv_enter_shutdown_charging(void *ignored) {
  app_manager_put_launch_app_event(&(AppLaunchEventConfig) {
    .id = APP_ID_SHUTDOWN_CHARGING,
  });
}
#endif

// Internals

static void prv_transition(BatteryUIStateID next_state, void *data) {
  if (s_state != next_state) {
    // All self-transitions are internal.
    // A state's entry function is a its only valid action.
    // The exit function is only called on actual state changes.
    if (ui_states[s_state].exit) {
      ui_states[s_state].exit();
    }
    s_state = next_state;
  }
  if (ui_states[s_state].enter) {
    ui_states[s_state].enter(data);
  }
}

static bool prv_is_valid_transition(BatteryUIStateID next_state) {
  const uint8_t count = ARRAY_LENGTH(ui_states[s_state].next_state);
  for (int i = 0; i < count; i++) {
    if (ui_states[s_state].next_state[i] == next_state) {
      return true;
    }
  }

  return false;
}

static BatteryUIStateID prv_get_state(PreciseBatteryChargeState *state) {
  // TODO: Refactor?
  if (state->is_plugged) {
    // Don't use the PreciseBatteryChargeState definition of is_charging, as it maps to the
    // result of @see battery_charge_controller_thinks_we_are_charging instead of the actual
    // user-facing definition of charging.
    const uint32_t is_charging = battery_get_charge_state().is_charging;
    return is_charging ? BatteryCharging : BatteryFullyCharged;
  } else if (battery_monitor_critical_lockout()) {
    return BatteryCritical;
  } else if (low_power_is_active()) {
    return BatteryLowPower;
  } else if (ratio32_to_percent(state->charge_percent) <=
             battery_curve_get_percent_remaining(s_warning_points[0])) {
    return BatteryWarning;
  } else {
    return BatteryGood;
  }
}

void battery_ui_handle_state_change_event(PreciseBatteryChargeState charge_state) {
  BatteryUIStateID next_state = prv_get_state(&charge_state);
  if (prv_is_valid_transition(next_state)) {
    prv_transition(next_state, &charge_state);
  }
}

void battery_ui_handle_shut_down(void) {
  if (s_state != BatteryCharging) {
    enter_standby(RebootReasonCode_ShutdownMenuItem);
  } else {
    prv_transition(BatteryShutdownCharging, NULL);
  }
}

void battery_ui_reset_fsm_for_tests(void) {
  s_state = BatteryGood;
  s_warning_points_index = -1;
}
