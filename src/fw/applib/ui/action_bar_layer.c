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

#include "action_bar_layer.h"

#include "animation.h"
#include "animation_timing.h"
#include "applib/app_timer.h"
#include "applib/applib_malloc.auto.h"
#include "applib/graphics/graphics.h"
#include "applib/ui/window_private.h"
#include "process_management/process_manager.h"
#include "system/passert.h"
#include "util/trig.h"

const int16_t MAX_ICON_HEIGHT = 18;

const int64_t PRESS_ANIMATION_DURATION_MS = 144;

const int64_t ICON_CHANGE_ANIMATION_DURATION_MS = 144;
const int16_t ICON_CHANGE_OFFSET[NUM_ACTION_BAR_ITEMS] = {-5, 0, 5};

const uint32_t MILLISECONDS_PER_FRAME = 1000 / 30;

static int prv_width(void) {
  const PlatformType platform = process_manager_current_platform();
  return _ACTION_BAR_WIDTH(platform);
}

static int prv_vertical_icon_margin(void) {
  const PlatformType platform = process_manager_current_platform();
  return PBL_PLATFORM_SWITCH(platform,
                             /*aplite*/ 24,
                             /*basalt*/ 24,
                             /*chalk*/ 53,
                             /*diorite*/ 24,
                             /*emery*/ 45);
}

static int prv_press_animation_offset(void) {
  const PlatformType platform = process_manager_current_platform();
  return PBL_PLATFORM_SWITCH(platform,
                             /*aplite*/ 5,
                             /*basalt*/ 5,
                             /*chalk*/ 4,
                             /*diorite*/ 5,
                             /*emery*/ 5);
}

// TODO: Once PBL-16032 is implemented, use that instead.
static int64_t prv_get_precise_time(void) {
  time_t seconds;
  uint16_t milliseconds;
  time_ms(&seconds, &milliseconds);
  return (seconds * 1000) + milliseconds;
}

inline static bool action_bar_is_highlighted(ActionBarLayer *action_bar, uint8_t index) {
  PBL_ASSERTN(index < NUM_ACTION_BAR_ITEMS);
  return (bool) (action_bar->is_highlighted & (1 << index));
}

static void prv_register_redraw_timer(ActionBarLayer *layer);

static void prv_timed_redraw(void *context) {
  ActionBarLayer *action_bar = context;
  layer_mark_dirty(&action_bar->layer);
  action_bar->redraw_timer = NULL;
  int64_t now = prv_get_precise_time();
  for (int i = 0; i < NUM_ACTION_BAR_ITEMS; ++i) {
    if ((action_bar->state_change_times[i] != 0 &&
          (now - action_bar->state_change_times[i]) <= PRESS_ANIMATION_DURATION_MS) ||
        (action_bar->icon_change_times[i] != 0 &&
          (now - action_bar->icon_change_times[i]) <= ICON_CHANGE_ANIMATION_DURATION_MS)) {
      prv_register_redraw_timer(action_bar);
      return;
    }
  }
}

static void prv_register_redraw_timer(ActionBarLayer *action_bar) {
  if (!action_bar->redraw_timer) {
    action_bar->redraw_timer = app_timer_register(MILLISECONDS_PER_FRAME, prv_timed_redraw,
                                                  action_bar);
  }
}

inline static void action_bar_set_highlighted(ActionBarLayer *action_bar, uint8_t index, bool highlighted) {
  PBL_ASSERT(index < NUM_ACTION_BAR_ITEMS, "Index: %"PRIu8, index);

  const uint8_t bit = (1 << index);
  if (action_bar_is_highlighted(action_bar, index) == highlighted) {
    return;
  }
  if (highlighted) {
    action_bar->is_highlighted |= bit;
  } else {
    action_bar->is_highlighted &= ~bit;
    prv_register_redraw_timer(action_bar);
  }
  action_bar->state_change_times[index] = prv_get_precise_time();
  layer_mark_dirty(&action_bar->layer);
}

void action_bar_changed_proc(ActionBarLayer *action_bar, GContext* ctx) {
  if (action_bar->layer.window && action_bar->layer.window->on_screen == false) {
    // clear first, fixes issue of returning from other page while highlighted
    for (int i = 0; i < NUM_ACTION_BAR_ITEMS; i++) {
      action_bar_set_highlighted(action_bar, i, false);
    }
  }
}

static GPoint prv_offset_since_time(int64_t time_ms, int64_t duration_ms,
                                     GPoint max_offset) {
  if (time_ms == 0) {
    return GPointZero;
  }
  const int64_t delta_ms = prv_get_precise_time() - time_ms;
  if (delta_ms >= duration_ms) {
    return GPointZero;
  }
  const uint32_t normalized_time = (delta_ms * ANIMATION_NORMALIZED_MAX) / duration_ms;
  const uint32_t normalized_distance = animation_timing_curve(normalized_time,
                                                             AnimationCurveEaseOut);
  const GPoint real_offset = GPoint(
      max_offset.x - ((normalized_distance * max_offset.x) / ANIMATION_NORMALIZED_MAX),
      max_offset.y - ((normalized_distance * max_offset.y) / ANIMATION_NORMALIZED_MAX));
  return real_offset;
}

static GPoint prv_get_button_press_offset(ActionBarLayer *action_bar, uint8_t button_index) {
  const int16_t animation_offset = prv_press_animation_offset();
  const GPoint offset[5] = {
      GPointZero,
      GPoint(-animation_offset, 0),
      GPoint(0, -animation_offset),
      GPoint(animation_offset, 0),
      GPoint(0, animation_offset),
  };
  return offset[action_bar->animation[button_index]];
}

static void prv_draw_background_rect(ActionBarLayer *action_bar, GContext *ctx, GColor bg_color) {
  graphics_fill_rect(ctx, &action_bar->layer.bounds);
}

void prv_draw_background_round(ActionBarLayer *action_bar, GContext *ctx, GColor bg_color) {
  const uint32_t action_bar_circle_diameter = DISP_ROWS * 19 / 9;
  GRect action_bar_circle_frame = (GRect) {
      .size = GSize(action_bar_circle_diameter, action_bar_circle_diameter)
  };
  grect_align(&action_bar_circle_frame, &action_bar->layer.bounds, GAlignLeft, false /* clips */);
  graphics_fill_oval(ctx, action_bar_circle_frame, GOvalScaleModeFitCircle);
}

void action_bar_update_proc(ActionBarLayer *action_bar, GContext* ctx) {
  const GColor bg_color = action_bar->background_color;
  if (!gcolor_is_transparent(bg_color)) {
    graphics_context_set_fill_color(ctx, bg_color);
    PBL_IF_RECT_ELSE(prv_draw_background_rect,
                     prv_draw_background_round)(action_bar, ctx, bg_color);
  }

  for (unsigned int index = 0; index < NUM_ACTION_BAR_ITEMS; ++index) {
    const GBitmap *icon = action_bar->icons[index];
    if (icon) {
      GRect rect = GRect(1, 0, prv_width(), MAX_ICON_HEIGHT);
      const int button_id = index + 1;
      const int vertical_icon_margin = prv_vertical_icon_margin();
      switch (button_id) {
        case BUTTON_ID_UP:
          rect.origin.y = vertical_icon_margin;
          break;
        case BUTTON_ID_SELECT:
          rect.origin.y = (action_bar->layer.bounds.size.h / 2) - (rect.size.h / 2);
          break;
        case BUTTON_ID_DOWN:
          rect.origin.y = action_bar->layer.bounds.size.h - vertical_icon_margin -
                          rect.size.h;
          break;
        default:
          WTF;
      }

      // In order to avoid creating relatively heavy animations, we instead just base our drawing
      // directly on time. The time is set when the animation should start; we convert the delta
      // since then into an offset and apply that to our rendering.
      GPoint offset;
      if (action_bar_is_highlighted(action_bar, index)) {
        offset = prv_get_button_press_offset(action_bar, index);
      } else {
        const int64_t state_change_time = action_bar->state_change_times[index];
        offset = prv_offset_since_time(state_change_time, PRESS_ANIMATION_DURATION_MS,
                                       prv_get_button_press_offset(action_bar, index));
      }

      const int64_t icon_change_time = action_bar->icon_change_times[index];
      offset = gpoint_add(offset, prv_offset_since_time(icon_change_time,
                                                        ICON_CHANGE_ANIMATION_DURATION_MS,
                                                        GPoint(0, ICON_CHANGE_OFFSET[index])));

      GRect icon_rect = icon->bounds;
      const bool clip = true;
      grect_align(&icon_rect, &rect, GAlignCenter, clip);
#if PBL_ROUND
      // Offset needed because the new curvature of the action bar makes the icons look off-center
      const int32_t icon_horizontal_offset = -2;
      icon_rect.origin.x += icon_horizontal_offset;
#endif
      icon_rect.origin.x += offset.x;
      icon_rect.origin.y += offset.y;
      // We use GCompOpAssign on 1-bit images, because they still support the old operations.
      // We use GCompOpSet otherwise to ensure we support transparency.
      if (gbitmap_get_format(icon) == GBitmapFormat1Bit) {
        graphics_context_set_compositing_mode(ctx, GCompOpAssign);
      } else {
        graphics_context_set_compositing_mode(ctx, GCompOpSet);
      }
      graphics_draw_bitmap_in_rect(ctx, (GBitmap*)icon, &icon_rect);
    }
  }
}

void action_bar_layer_init(ActionBarLayer *action_bar) {
  *action_bar = (ActionBarLayer){};
  layer_set_clips(&action_bar->layer, true);
  action_bar->layer.update_proc = (LayerUpdateProc) action_bar_update_proc;
  action_bar->layer.property_changed_proc =
      (PropertyChangedProc) (void *) action_bar_changed_proc;
  action_bar->background_color = GColorBlack;
  for (unsigned int i = 0; i < NUM_ACTION_BAR_ITEMS; ++i) {
    action_bar->animation[i] = ActionBarLayerIconPressAnimationMoveLeft;
  }
}

ActionBarLayer* action_bar_layer_create(void) {
  ActionBarLayer* layer = applib_type_malloc(ActionBarLayer);
  if (layer) {
    action_bar_layer_init(layer);
  }
  return layer;
}

void action_bar_layer_deinit(ActionBarLayer *action_bar_layer) {
  if (action_bar_layer->redraw_timer) {
    app_timer_cancel(action_bar_layer->redraw_timer);
  }
  layer_deinit(&action_bar_layer->layer);
}

void action_bar_layer_destroy(ActionBarLayer *action_bar_layer) {
  if (action_bar_layer == NULL) {
    return;
  }
  action_bar_layer_deinit(action_bar_layer);
  applib_free(action_bar_layer);
}

Layer* action_bar_layer_get_layer(ActionBarLayer *action_bar_layer) {
  return &action_bar_layer->layer;
}

inline static void* action_bar_get_context(ActionBarLayer *action_bar) {
  return action_bar->context ? action_bar->context : action_bar;
}

void action_bar_layer_set_context(ActionBarLayer *action_bar, void *context) {
  action_bar->context = context;
}

static void action_bar_raw_up_down_handler(ClickRecognizerRef recognizer, ActionBarLayer *action_bar, bool is_highlighted) {
  const ButtonId button_id = click_recognizer_get_button_id(recognizer);
  const uint8_t index = button_id - 1;

  // is_highlighted will cause the icon in the action bar to render normal or inverted:
  action_bar_set_highlighted(action_bar, index, is_highlighted);
}

static void action_bar_raw_up_handler(ClickRecognizerRef recognizer, void *context) {
  ActionBarLayer *action_bar = (ActionBarLayer *)context;
  action_bar_raw_up_down_handler(recognizer, action_bar, false);
}

static void action_bar_raw_down_handler(ClickRecognizerRef recognizer, void *context) {
  ActionBarLayer *action_bar = (ActionBarLayer *)context;
  action_bar_raw_up_down_handler(recognizer, action_bar, true);
}

static void action_bar_click_config_provider(void *config_context) {
  ActionBarLayer *action_bar = config_context;
  void *context = action_bar_get_context(action_bar);
  // For UP, SELECT and DOWN, setup the raw handler and assign the user specified context:
  for (ButtonId button_id = BUTTON_ID_UP; button_id < NUM_BUTTONS; ++button_id) {
    window_raw_click_subscribe(button_id, action_bar_raw_down_handler, action_bar_raw_up_handler, action_bar);
    window_set_click_context(button_id, context);
  }
  // If back button is overridden, set context of BACK click recognizer as well:
  if (action_bar->window && action_bar->window->overrides_back_button) {
    window_set_click_context(BUTTON_ID_BACK, context);
  }
  if (action_bar->click_config_provider) {
    action_bar->click_config_provider(context);
  }
}

inline static void action_bar_update_click_config_provider(ActionBarLayer *action_bar) {
  if (action_bar->window) {
    window_set_click_config_provider_with_context(action_bar->window,
        action_bar_click_config_provider, action_bar);
  }
}

void action_bar_layer_set_click_config_provider(ActionBarLayer *action_bar, ClickConfigProvider click_config_provider) {
  action_bar->click_config_provider = click_config_provider;
  action_bar_update_click_config_provider(action_bar);
}

void action_bar_layer_set_icon_animated(ActionBarLayer *action_bar, ButtonId button_id,
                                        const GBitmap *icon, bool animated) {
  if (button_id < BUTTON_ID_UP || button_id >= NUM_BUTTONS) {
    return;
  }
  const uint8_t index = button_id - 1;
  if (action_bar->icons[index] == icon) {
    return;
  }
  action_bar->icons[index] = icon;
  if (animated) {
    action_bar->icon_change_times[index] = prv_get_precise_time();
    prv_register_redraw_timer(action_bar);
  } else {
    action_bar->icon_change_times[index] = 0;
  }
  layer_mark_dirty(&action_bar->layer);
}

void action_bar_layer_set_icon(ActionBarLayer *action_bar, ButtonId button_id,
                               const GBitmap *icon) {
  action_bar_layer_set_icon_animated(action_bar, button_id, icon, false);
}

void action_bar_layer_clear_icon(ActionBarLayer *action_bar, ButtonId button_id) {
  action_bar_layer_set_icon(action_bar, button_id, NULL);
}

void action_bar_layer_set_icon_press_animation(ActionBarLayer *action_bar, ButtonId button_id,
                                               ActionBarLayerIconPressAnimation animation) {
  if (button_id < BUTTON_ID_UP || button_id >= NUM_BUTTONS) {
    return;
  }
  action_bar->animation[button_id - 1] = animation;
}

void action_bar_layer_add_to_window(ActionBarLayer *action_bar, struct Window *window) {
  const GRect *window_bounds = &window->layer.bounds;
  const int16_t width = prv_width();
  GRect rect = GRect(0, 0, width, window_bounds->size.h);
  layer_set_bounds(&action_bar->layer, &rect);
  rect.origin.x = window_bounds->size.w - width;
  layer_set_frame(&action_bar->layer, &rect);
  layer_add_child(&window->layer, &action_bar->layer);

  action_bar->window = window;
  action_bar_update_click_config_provider(action_bar);
}

void action_bar_layer_remove_from_window(ActionBarLayer *action_bar) {
  if (action_bar == NULL || action_bar->window == NULL) {
    return;
  }
  layer_remove_from_parent(&action_bar->layer);
  window_set_click_config_provider_with_context(action_bar->window, NULL, NULL);
  action_bar->window = NULL;
}

void action_bar_layer_set_background_color(ActionBarLayer *action_bar, GColor background_color) {
  if (gcolor_equal(background_color, action_bar->background_color)) {
    return;
  }
  action_bar->background_color = background_color;
  layer_mark_dirty(&(action_bar->layer));
}
