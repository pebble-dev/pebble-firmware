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

#pragma once

#include "default/compositor_dot_transitions.h"
#include "default/compositor_launcher_app_transitions.h"
#include "default/compositor_slide_transitions.h"
#include "default/compositor_shutter_transitions.h"
#include "legacy/compositor_app_slide_transitions.h"
#if PLATFORM_SILK || PLATFORM_ASTERIX
# include "legacy/compositor_modal_slide_transitions.h"
#else
# include "default/compositor_modal_transitions.h"
# include "default/compositor_port_hole_transitions.h"
# include "default/compositor_round_flip_transitions.h"
#endif
#if CAPABILITY_HAS_TIMELINE_PEEK
#include "default/compositor_peek_transitions.h"
#endif

#include "applib/graphics/gdraw_command_sequence.h"

//! @return Whether an app-to-app compositor animation should be skipped (e.g. if a modal is
//!         being displayed)
bool compositor_transition_app_to_app_should_be_skipped(void);

//! Return a new normalized distance that represents the provided distance as a new normalized
//! distance between the new start and end. `normalized` must be between start_distance and
//! end_distance if you want a valid result.
AnimationProgress animation_timing_scaled(AnimationProgress time_normalized,
                                          AnimationProgress interval_start,
                                          AnimationProgress interval_end);

//! Draw the next frame of the provided PDC sequence using the given options
//! @param ctx The graphics context to use to draw the frame
//! @param sequence The PDC sequence whose frame you want to draw
//! @param distance_normalized The normalized distance for the current moment in the animation
//! @param chroma_key_color The color to replace with the app's frame buffer
//! @param stroke_color The color to use when drawing the stroke of the ring in the frame
//! @param overdraw_color The color to use when "overdrawing" areas of the frame with no app content
//!        (e.g. flip/flop animations need to draw the right color beyond the edges of the app face)
//! @param inner If true, draw the app frame buffer inside the ring, otherwise outside
//! @param framebuffer_offset Visual offset of the app frame buffer
void compositor_transition_pdcs_animation_update(
    GContext *ctx, GDrawCommandSequence *sequence, uint32_t distance_normalized,
    GColor chroma_key_color, GColor stroke_color, GColor overdraw_color, bool inner,
    const GPoint *framebuffer_offset);

//! Draw implementation that can be used to fill lines with the contents of the app framebuffer
extern const GDrawRawImplementation g_compositor_transitions_app_fb_draw_implementation;
