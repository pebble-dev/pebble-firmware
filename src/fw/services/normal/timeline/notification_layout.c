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

#include "notification_jumboji_table.h"
#include "notification_layout.h"
#include "timeline_layout.h"

#include "applib/fonts/fonts.h"
#include "applib/graphics/gtypes.h"
#include "applib/graphics/text.h"
#include "apps/system_apps/timeline/peek_layer.h"
#include "font_resource_keys.auto.h"
#include "kernel/pbl_malloc.h"
#include "kernel/ui/kernel_ui.h"
#include "resource/resource_ids.auto.h"
#include "resource/timeline_resource_ids.auto.h"
#include "services/common/analytics/analytics.h"
#include "services/common/clock.h"
#include "services/common/clock.h"
#include "services/common/i18n/i18n.h"
#include "services/normal/blob_db/pin_db.h"
#include "services/normal/timeline/timeline_resources.h"
#include "shell/system_theme.h"
#include "system/hexdump.h"
#include "system/logging.h"
#include "system/passert.h"
#include "util/math.h"
#include "util/size.h"
#include "util/string.h"
#include "util/trig.h"

#if !TINTIN_FORCE_FIT

// NOTIFICATION
// Title -> Sender/App
// Subtitle -> Subject (Emails)
// Body -> Body
// Footer -> Friendly Timestamp

// REMINDER
// Title -> Friendly Timestamp
// Subtitle -> NA
// Body -> Title
// Footer -> Location

#define LAYOUT_MAX_HEIGHT 2500
#define CARD_MARGIN PBL_IF_ROUND_ELSE(12, 10)
// All paddings relate to padding above the object unless othersize noted
#define CARD_BOTTOM_PADDING 18
// The y-position of a layout frame when its banner is peeking
#define BANNER_PEEK_STATIC_Y (DISP_ROWS - STATUS_BAR_LAYER_HEIGHT)
#define BOTTOM_BANNER_CIRCLE_RADIUS 8

static void prv_card_render(NotificationLayout *layout, GContext *ctx, bool render);
static const LayoutColors *prv_layout_get_colors(const LayoutLayer *layout);

static time_t prv_get_parent_timestamp(TimelineItem *reminder) {
  TimelineItem pin;
  if (S_SUCCESS != pin_db_get(&reminder->header.parent_id, &pin)) {
    return reminder->header.timestamp;
  }
  timeline_item_free_allocated_buffer(&pin);
  return pin.header.timestamp;
}

//////////////////////////////////////////
//  Card Mode
//////////////////////////////////////////

static const NotificationStyle s_notification_styles[NumPreferredContentSizes] = {
  [PreferredContentSizeSmall] = {
    .header_padding = 3,
    .title_padding = 3,
    .subtitle_upper_padding = PBL_IF_RECT_ELSE(1, 4),
    .subtitle_lower_padding = PBL_IF_RECT_ELSE(2, 1),
    .location_offset = PBL_IF_RECT_ELSE(3, 7),
    .location_margin = PBL_IF_RECT_ELSE(5, 9),
    .body_icon_offset = 3,
    .body_icon_margin = -5,
    .body_padding = PBL_IF_RECT_ELSE(1, 1),
#if PBL_ROUND
    .timestamp_upper_padding = 6,
    .timestamp_lower_padding = -3,
#else
    .timestamp_upper_padding = 3,
#endif
  },
  [PreferredContentSizeMedium] = {
#if PBL_ROUND
    .body_padding = 3,
    .subtitle_upper_padding = 3,
#endif
    .header_padding = 3,
    .title_padding = 3,
    .title_line_delta = -1,
    .subtitle_lower_padding = PBL_IF_RECT_ELSE(6, 2),
    .subtitle_line_delta = -1,
    .location_offset = PBL_IF_RECT_ELSE(-2, 6),
    .location_margin = PBL_IF_RECT_ELSE(3, 10),
    .body_icon_offset = 3,
    .body_icon_margin = -5,
    .body_line_delta = -1,
#if PBL_ROUND
    .timestamp_upper_padding = 6,
    .timestamp_lower_padding = -3,
#else
    .timestamp_upper_padding = 3
#endif
  },
  [PreferredContentSizeLarge] = {
    .title_offset_if_body_icon = -2,
    .subtitle_upper_padding = 2,
    .subtitle_lower_padding = PBL_IF_RECT_ELSE(4, 2),
    .subtitle_line_delta = -2,
    .location_offset = 6,
    .location_margin = 10,
    .body_icon_margin = -10,
    .body_padding = 2,
    .body_line_delta = -2,
#if PBL_ROUND
    .timestamp_upper_padding = 6,
#else
    .timestamp_upper_padding = 3,
#endif
  },
  [PreferredContentSizeExtraLarge] = {
    .subtitle_upper_padding = 2,
    .subtitle_lower_padding = 4,
    .subtitle_line_delta = -2,
    .location_offset = 6,
    .location_margin = 10,
    .body_icon_offset = 6,
    .body_icon_margin = -10,
    .body_line_delta = -2,
    .timestamp_upper_padding = 6,
  },
};

static bool prv_is_reminder(const NotificationLayout *layout) {
  return (layout->info.item->header.type == TimelineItemTypeReminder);
}

static void prv_reminder_timestamp_update(const LayoutLayer *layout_ref,
                                          const LayoutNodeTextDynamicConfig *config, char *buffer,
                                          bool render) {
  const NotificationLayout *layout = (NotificationLayout *)layout_ref;
  const int max_relative_hrs = 1;
  clock_get_until_time(buffer, config->buffer_size, prv_get_parent_timestamp(layout->info.item),
                       max_relative_hrs);
  const char *buffer_ptr = string_strip_leading_whitespace(buffer);
  memmove(buffer, buffer_ptr, config->buffer_size - (buffer_ptr - buffer));
}

static void prv_notification_timestamp_update(const LayoutLayer *layout_ref,
                                              const LayoutNodeTextDynamicConfig *config,
                                              char *buffer,
                                              bool render) {
  const NotificationLayout *layout = (NotificationLayout *)layout_ref;
  clock_get_since_time(buffer, config->buffer_size, layout->info.item->header.timestamp);
}

#if !PLATFORM_TINTIN
static const EmojiEntry s_emoji_table[] = JUMBOJI_TABLE(EMOJI_ENTRY);

static bool prv_each_emoji_codepoint(int index, Codepoint codepoint, void *context) {
  Codepoint *emoji_codepoint = context;
  if (codepoint_is_end_of_word(codepoint) ||
      codepoint_is_formatting_indicator(codepoint) ||
      codepoint_is_skin_tone_modifier(codepoint) ||
      codepoint_is_special(codepoint) ||
      codepoint_is_zero_width(codepoint) ||
      codepoint_should_skip(codepoint)) {
    // Skip this codepoint
    return true;
  } else if (codepoint_is_emoji(codepoint)) {
    if (*emoji_codepoint) {
      // This has more than one emoji
      goto fail;
    }
    // Found an emoji
    *emoji_codepoint = codepoint;
    return true;
  }
  // This is not an emoji-only string
fail:
  *emoji_codepoint = NULL_CODEPOINT;
  return false;
}

T_STATIC ResourceId prv_get_emoji_icon_by_string(const EmojiEntry *table, const char *str) {
  if (!str) {
    return INVALID_RESOURCE;
  }
  Codepoint emoji_codepoint = NULL_CODEPOINT;
  utf8_each_codepoint(str, prv_each_emoji_codepoint, &emoji_codepoint);
  for (unsigned int i = 0; i < ARRAY_LENGTH(s_emoji_table); i++) {
    const EmojiEntry *emoji = &s_emoji_table[i];
    if (emoji->codepoint == emoji_codepoint) {
      return emoji->resource_id;
    }
  }
  return INVALID_RESOURCE;
}

static ResourceId prv_get_emoji_icon(NotificationLayout *layout) {
  const char *body = attribute_get_string(layout->layout.attributes, AttributeIdBody, NULL);
  return prv_get_emoji_icon_by_string(s_emoji_table, body);
}

static bool prv_should_enlarge_emoji(NotificationLayout *layout) {
  return (!layout->info.show_notification_timestamp &&
          prv_get_emoji_icon(layout) != INVALID_RESOURCE);
}
#endif

//! Creates a GTextNode view node representing the inner content of the notification
//! @param layout NotificationLayout of the notification
//! @param use_body_icon Whether to display a body icon. Currently used by Jumboji
//! @return the GTextNode view node of the notification
static NOINLINE GTextNode *prv_create_view(NotificationLayout *layout, bool use_body_icon) {
  const NotificationStyle *style = &s_notification_styles[system_theme_get_content_size()];

  const bool is_reminder = prv_is_reminder(layout);
  const LayoutNodeTextAttributeConfig header_config = {
    .attr_id = AttributeIdAppName,
    .text.style_font = TextStyleFont_Header,
    .text.extent.offset.y = style->header_padding,
    .text.extent.margin.h = style->header_padding,
  };
  const LayoutNodeTextDynamicConfig notification_timestamp_config = {
    .text.extent.node.type = LayoutNodeType_TextDynamic,
    .update = prv_notification_timestamp_update,
    .buffer_size = TIME_STRING_REQUIRED_LENGTH,
    .text.style_font = PBL_IF_RECT_ELSE(TextStyleFont_Footer, TextStyleFont_Caption),
    .text.extent.offset.y = style->timestamp_upper_padding,
    .text.extent.margin.h = style->timestamp_upper_padding + style->timestamp_lower_padding,
  };
  const LayoutNodeTextDynamicConfig reminder_timestamp_config = {
    .text.extent.node.type = LayoutNodeType_TextDynamic,
    .update = prv_reminder_timestamp_update,
    .buffer_size = TIME_STRING_REQUIRED_LENGTH,
    .text.style_font = TextStyleFont_Header,
    .text.extent.offset.y = style->header_padding,
    .text.extent.margin.h = style->header_padding,
  };
  const LayoutNodeTextAttributeConfig title_config = {
    .attr_id = is_reminder ? AttributeIdUnused : AttributeIdTitle,
    .text.style_font = TextStyleFont_Header,
    .text.line_spacing_delta = style->title_line_delta,
    .text.alignment = use_body_icon ? LayoutTextAlignment_Center : LayoutTextAlignment_Auto,
    .text.extent.offset.y =
        style->title_padding + (use_body_icon ? style->title_offset_if_body_icon : 0),
    .text.extent.margin.h = style->title_padding,
  };
  const LayoutNodeTextAttributeConfig subtitle_config = {
    .attr_id = is_reminder ? AttributeIdTitle : AttributeIdSubtitle,
    .text.style_font = TextStyleFont_Title,
    .text.line_spacing_delta = style->subtitle_line_delta,
    .text.alignment = use_body_icon ? LayoutTextAlignment_Center : LayoutTextAlignment_Auto,
    .text.extent.offset.y = style->subtitle_upper_padding,
    .text.extent.margin.h = style->subtitle_upper_padding + style->subtitle_lower_padding,
  };
#if !PLATFORM_TINTIN
  const LayoutNodeIconConfig body_icon_config = {
    .extent.node.type = LayoutNodeType_Icon,
    .res_info = &(AppResourceInfo) {
      .res_app_num = SYSTEM_APP,
      .res_id = prv_get_emoji_icon(layout),
    },
    .align = GAlignCenter,
    .icon_layer = &layout->detail_icon_layer,
    .extent.offset.y = style->body_icon_offset,
    .extent.margin.h = style->body_icon_margin,
  };
#endif
  const LayoutNodeTextAttributeConfig location_config = {
    .attr_id = AttributeIdLocationName,
    .text.style_font = TextStyleFont_Footer,
    .text.extent.offset.y = style->location_offset,
    .text.extent.margin.h = style->location_margin,
  };
  const int reminder_body_line_delta = 0;
  const LayoutNodeTextAttributeConfig body_config = {
    .attr_id = AttributeIdBody,
    .text.style_font = is_reminder ? TextStyleFont_Caption : TextStyleFont_Body,
    .text.line_spacing_delta = is_reminder ? reminder_body_line_delta :
                                             style->body_line_delta,
    .text.extent.offset.y = style->body_padding,
    .text.extent.margin.h = style->body_padding,
  };
  const LayoutNodeHeadingsParagraphsConfig headings_paragraphs_node = {
    .extent.node.type = LayoutNodeType_HeadingsParagraphs,
    .extent.offset.y = 12,
    .extent.margin.h = 5,
    .heading_style_font = TextStyleFont_Header,
    .paragraph_style_font = TextStyleFont_Body,
  };
  const LayoutNodeConfig *reminder_timestamp_node_config = NULL;
  const LayoutNodeConfig *notification_timestamp_node_config = NULL;
  const LayoutNodeConfig *header_node_config = NULL;

  if (is_reminder) {
    reminder_timestamp_node_config = &reminder_timestamp_config.text.extent.node;
  } else {
    notification_timestamp_node_config = &notification_timestamp_config.text.extent.node;
    header_node_config = &header_config.text.extent.node;
  }
#if !PLATFORM_TINTIN
  if (!layout->info.show_notification_timestamp && PBL_IF_RECT_ELSE(use_body_icon, true)) {
    notification_timestamp_node_config = NULL;
  }
#endif
  const LayoutNodeConfig * const vertical_config_nodes[] = {
    reminder_timestamp_node_config,
#if PBL_ROUND
    notification_timestamp_node_config,
#endif
    header_node_config,
    &title_config.text.extent.node,
    &subtitle_config.text.extent.node,
    &location_config.text.extent.node,
#if PLATFORM_TINTIN
    &body_config.text.extent.node,
#else
    use_body_icon ? &body_icon_config.extent.node :
                    &body_config.text.extent.node,
#endif
    &headings_paragraphs_node.extent.node,
#if PBL_RECT
    notification_timestamp_node_config,
#endif
  };
  const LayoutNodeVerticalConfig vertical_config = {
    .container.extent.node.type = LayoutNodeType_Vertical,
    .container.num_nodes = ARRAY_LENGTH(vertical_config_nodes),
    .container.nodes = (LayoutNodeConfig **)&vertical_config_nodes,
  };
  return layout_create_text_node_from_config(&layout->layout,
                                             &vertical_config.container.extent.node);
}

static void prv_destroy_view(NotificationLayout *layout) {
  graphics_text_node_destroy(layout->view_node);
  layout->view_node = NULL;
#if !PLATFORM_TINTIN
  kino_layer_destroy(layout->detail_icon_layer);
  layout->detail_icon_layer = NULL;
#endif
}

static void prv_title_mask_update_proc(Layer *layer, GContext *ctx) {
  // The mask should be invisible, so we don't draw anything.
  // The clipping is handled by layer_set_clips(title_mask, true);
  // which is set when the layer is created.
}


static bool prv_should_animate_banner(NotificationLayout *layout) {
  GRect frame;
  layer_get_frame(&layout->layout.layer, &frame);  

  bool is_at_top = frame.origin.y <= STATUS_BAR_LAYER_HEIGHT;
  
  bool banner_fully_visible = !layer_get_hidden(&layout->layout.layer) && 
                            frame.origin.y >= 0 &&
                            frame.origin.y <= STATUS_BAR_LAYER_HEIGHT;
  
  return is_at_top && banner_fully_visible;
}

// Constants for animation control
#define BANNER_SCROLL_LEFT_DURATION 5000 
#define BANNER_SCROLL_RIGHT_DURATION 600
#define BANNER_PAUSE_DURATION 1200
#define BANNER_MAX_CYCLES 1

// Animation state enum
#define BANNER_ANIM_STATE_IDLE 0
#define BANNER_ANIM_STATE_SCROLLING_LEFT 1
#define BANNER_ANIM_STATE_PAUSED_END 2
#define BANNER_ANIM_STATE_SCROLLING_RIGHT 3
#define BANNER_ANIM_STATE_PAUSED_START 4
#define BANNER_CYCLES_COMPLETED_FLAG 0x80  // Flag to indicate we've completed all cycles

static void prv_start_next_banner_animation(void *context);

static Animation* s_banner_animation = NULL;

static void prv_banner_animation_stopped(Animation *animation, bool finished, void *context) {
  NotificationLayout *layout = (NotificationLayout *)context;
  
  if (!layout || layout->destroyed) {
    // Layout was destroyed, just cleanup
    if (s_banner_animation) {
      animation_destroy(s_banner_animation);
      s_banner_animation = NULL;
    }
    return;
  }
  
  // Clear animation reference
  if (animation == s_banner_animation) {
    animation_destroy(s_banner_animation);
    s_banner_animation = NULL;
  }
  
  // Don't continue if we're no longer at the top
  if (!prv_should_animate_banner(layout)) {
    if (layout->banner_title_layer) {
      Layer *text_layer = text_layer_get_layer(layout->banner_title_layer);
      GRect frame;
      layer_get_frame(text_layer, &frame);
      frame.origin.x = 0;
      layer_set_frame(text_layer, &frame);
    }
    layout->banner_scroll_state = BANNER_ANIM_STATE_IDLE;
    return;
  }
  
  switch (layout->banner_scroll_state) {
    case BANNER_ANIM_STATE_SCROLLING_LEFT:
      // Just finished scrolling left, pause at end
      layout->banner_scroll_state = BANNER_ANIM_STATE_PAUSED_END;
      layout->banner_scroll_timer = app_timer_register(
        BANNER_PAUSE_DURATION, prv_start_next_banner_animation, layout);
      break;
      
    case BANNER_ANIM_STATE_SCROLLING_RIGHT:
      // Finished scrolling right, increment cycle
      layout->banner_scroll_cycle++;
      
      if (layout->banner_scroll_cycle >= BANNER_MAX_CYCLES) {
        // Done with all cycles - set completed flag
        layout->banner_scroll_cycle = BANNER_CYCLES_COMPLETED_FLAG;  // Set flag in high bit
        layout->banner_scroll_state = BANNER_ANIM_STATE_IDLE;
      } else {
        // Pause at start before next cycle
        layout->banner_scroll_state = BANNER_ANIM_STATE_PAUSED_START;
        layout->banner_scroll_timer = app_timer_register(
          BANNER_PAUSE_DURATION, prv_start_next_banner_animation, layout);
      }
      break;
      
    default:
      // Unexpected state, reset
      layout->banner_scroll_state = BANNER_ANIM_STATE_IDLE;
      break;
  }
}

static void prv_start_next_banner_animation(void *context) {
  NotificationLayout *layout = context;
  
  if (!layout || layout->destroyed || !layout->banner_title_layer) {
    return;
  }
  
  layout->banner_scroll_timer = NULL;
  
  // Don't animate if not at top
  if (!prv_should_animate_banner(layout)) {
    layout->banner_scroll_state = BANNER_ANIM_STATE_IDLE;
    return;
  }
  
  Layer *text_layer = text_layer_get_layer(layout->banner_title_layer);
  GRect start_frame, end_frame;
  layer_get_frame(text_layer, &start_frame);
  
  // Default to left scrolling if state is unexpected
  if (layout->banner_scroll_state != BANNER_ANIM_STATE_PAUSED_END) {
    end_frame = start_frame;
    end_frame.origin.x = -layout->banner_title_scroll_distance;
    layout->banner_scroll_state = BANNER_ANIM_STATE_SCROLLING_LEFT;
  } else {
    // Scrolling right (back to start)
    end_frame = start_frame;
    end_frame.origin.x = 0;
    layout->banner_scroll_state = BANNER_ANIM_STATE_SCROLLING_RIGHT;
  }
  
  // Cancel existing animation if any
  if (s_banner_animation) {
    animation_unschedule(s_banner_animation);
    animation_destroy(s_banner_animation);
    s_banner_animation = NULL;
  }
  
  // Create new animation
  PropertyAnimation* prop_anim = property_animation_create_layer_frame(
    text_layer, &start_frame, &end_frame);
    
  if (!prop_anim) {
    return;
  }
  
  // Set duration based on direction
  uint32_t duration = (layout->banner_scroll_state == BANNER_ANIM_STATE_SCROLLING_LEFT) ?
                      BANNER_SCROLL_LEFT_DURATION : BANNER_SCROLL_RIGHT_DURATION;
  
  s_banner_animation = (Animation*)prop_anim;
  animation_set_curve(s_banner_animation, AnimationCurveEaseInOut);
  animation_set_duration(s_banner_animation, duration);
  animation_set_handlers(s_banner_animation, (AnimationHandlers) {
    .stopped = prv_banner_animation_stopped
  }, layout);
  
  animation_schedule(s_banner_animation);
}


static void prv_card_init(NotificationLayout *layout, AttributeList *attributes,
                          const Uuid *app_id) {
  // init the icon
  const TimelineResourceId fallback_icon_id =
      notification_layout_get_fallback_icon_id(layout->info.item->header.type);
  const uint32_t timeline_res_id = attribute_get_uint32(attributes, AttributeIdIconTiny,
                                                        fallback_icon_id);
  const TimelineResourceInfo timeline_res = {
    .res_id = timeline_res_id,
    .app_id = app_id,
    .fallback_id = fallback_icon_id
  };
  timeline_resources_get_id(&timeline_res, TimelineResourceSizeTiny, &layout->icon_res_info);

  const GRect *frame = &layout->layout.layer.frame;
  const GSize icon_size = NOTIFICATION_TINY_RESOURCE_SIZE;
  int16_t origin_x = frame->origin.x + (frame->size.w / 2) - (icon_size.w / 2);
  const int16_t origin_y = frame->origin.y + CARD_ICON_UPPER_PADDING + PBL_IF_RECT_ELSE(3, 0);
  
  // Check if we have a banner title
  const char *banner_title_str = attribute_get_string(attributes, AttributeIdBannerTitle, "");
  if (banner_title_str != NULL && banner_title_str[0] != '\0') {
  LayoutColors *colors = &layout->colors;
  #if PBL_ROUND
  // For round displays, place the banner title below the icon
    const int16_t text_origin_y = origin_y + icon_size.h - 5;
    const int16_t side_margin = 20; // Increased margin (was 10)
    const GRect title_mask_frame = GRect(frame->origin.x + side_margin, text_origin_y, 
                                        frame->size.w - (side_margin * 2), 30);
  #else
    origin_x = frame->origin.x + 5;
    const int16_t text_origin_x = frame->origin.x + icon_size.w + 5;
    const GRect title_mask_frame = GRect(text_origin_x, 1, frame->size.w - origin_x - icon_size.w, CARD_ICON_UPPER_PADDING +  
                                                                                                     NOTIFICATION_TINY_RESOURCE_HEIGHT +
                                                                                                     6);
  #endif
    // First create a temporary text layer to measure text size
    TextLayer *temp_title = text_layer_create(GRect(0, 0, 2000, title_mask_frame.size.h));
    text_layer_set_text(temp_title, banner_title_str);
    text_layer_set_font(temp_title, fonts_get_system_font(PBL_IF_RECT_ELSE(FONT_KEY_GOTHIC_24_BOLD, FONT_KEY_GOTHIC_18_BOLD)));
    
    // Measure text size
    GSize text_size = text_layer_get_content_size(graphics_context_get_current_context(), temp_title);
    text_layer_destroy(temp_title);
    
    // Add padding to the text width
    int16_t banner_width = text_size.w + 10; // Add 10px padding
    
    // Create the optimized banner title frame based on text width
    const GRect banner_title_frame = GRect(0, 0, banner_width, title_mask_frame.size.h);
    
    TextLayer *banner_title = text_layer_create(banner_title_frame);
    text_layer_set_text(banner_title, banner_title_str);
    text_layer_set_font(banner_title, fonts_get_system_font(PBL_IF_RECT_ELSE(FONT_KEY_GOTHIC_24_BOLD, FONT_KEY_GOTHIC_18_BOLD)));
    text_layer_set_text_alignment(banner_title, PBL_IF_RECT_ELSE(GTextAlignmentLeft, GTextAlignmentCenter)); // Center align for round
    text_layer_set_background_color(banner_title, GColorClear);
    text_layer_set_text_color(banner_title, colors->primary_color);
    text_layer_set_overflow_mode(banner_title, GTextOverflowModeFill);

    Layer *title_mask = layer_create(title_mask_frame);
    layer_set_update_proc(title_mask, prv_title_mask_update_proc);
    layer_set_clips(title_mask, true);
    
  #if PBL_ROUND
    // Position text layer in the center of mask horizontally
    GRect centered_frame = GRect((title_mask_frame.size.w - banner_width) / 2, 0,
                         banner_width, title_mask_frame.size.h);
    layer_set_frame(text_layer_get_layer(banner_title), &centered_frame);
  #endif                  
    layer_add_child(title_mask, text_layer_get_layer(banner_title));
    layer_add_child(&layout->layout.layer, title_mask);
    
    layout->banner_title_layer = banner_title;
    
    int16_t scroll_distance = text_size.w > title_mask_frame.size.w ? 
                              text_size.w - title_mask_frame.size.w + 15 : 0;
    
    layout->banner_title_scroll_distance = scroll_distance;
  
    #if PBL_ROUND
    if (scroll_distance > 0) {
      // For scrolling text, start at position 0
      GRect initial_frame = GRect(0, 0, banner_width, title_mask_frame.size.h);
      layer_set_frame(text_layer_get_layer(banner_title), &initial_frame);
    } else {
      // For non-scrolling text, ensure it's properly centered
      GRect centered_text_frame = GRect((title_mask_frame.size.w - banner_width) / 2, 0,
                                       banner_width, title_mask_frame.size.h);
      layer_set_frame(text_layer_get_layer(banner_title), &centered_text_frame);
    }
    #endif
  }

  kino_layer_init(&layout->icon_layer, &GRect(origin_x, origin_y, icon_size.w, icon_size.h));

  #if PBL_BW
  kino_layer_set_reel_with_resource_system(&layout->icon_layer, layout->icon_res_info.res_app_num,
                                           layout->icon_res_info.res_id, true);
  #else
  kino_layer_set_reel_with_resource_system(&layout->icon_layer, layout->icon_res_info.res_app_num,
                                           layout->icon_res_info.res_id, false);
  #endif
  layer_add_child(&layout->layout.layer, kino_layer_get_layer(&layout->icon_layer));
}

static void NOINLINE prv_init_view(NotificationLayout *layout) {
#if PLATFORM_TINTIN
  layout->view_node = prv_create_view(layout, false /* use_body_icon */);
#else
  const bool use_body_icon = prv_should_enlarge_emoji(layout);
  layout->view_node = prv_create_view(layout, use_body_icon);

  if (use_body_icon) {
    // Only calculate size if using a body icon, calculating size is stack expensive
    prv_card_render(layout, graphics_context_get_current_context(), false /* render */);

    if ((layout->view_size.h > (LAYOUT_HEIGHT + LAYOUT_ARROW_HEIGHT))) {
      // The large emoji won't fit in a single screen, so don't use the large emoji
      prv_destroy_view(layout);
      layout->view_node = prv_create_view(layout, false /* use_body_icon */);
    } else {
      analytics_inc(ANALYTICS_DEVICE_METRIC_NOTIFICATION_JUMBOJI_COUNT, AnalyticsClient_System);
    }
  }
#endif
}

#if PBL_ROUND
static void prv_hide_or_show_banner_icon(KinoLayer *icon_layer,
                                         const GRect *notification_layout_frame) {
  const int32_t frame_too_high_for_icon_threshold = -2;
  const int32_t top_banner_not_visible_threshold = 18;
  const bool icon_hidden =
      (notification_layout_frame->origin.y < frame_too_high_for_icon_threshold) ||
      (notification_layout_frame->origin.y > top_banner_not_visible_threshold);
  layer_set_hidden(&icon_layer->layer, icon_hidden);
}
#endif

#if PBL_ROUND
static CONST_FUNC int32_t prv_interpolate_linear(int32_t out_min, int32_t out_max, int32_t in_min,
                                                 int32_t in_max, int32_t progress) {
  return out_min + (out_max - out_min) * (progress - in_min) / (in_max - in_min);
}

static void prv_draw_banner_round(NotificationLayout *notification_layout, GContext *ctx,
                                  const GRect *const notification_layout_frame,
                                  LayoutColors colors) {
  // We use DISP_ROWS and DISP_COLS instead of the layer's frame or bounds because the
  // notification layout's frame is not the same size as the display
  const int32_t half_screen_width = DISP_COLS / 2;
  graphics_context_set_fill_color(ctx, colors.bg_color);
  const int32_t saved_clip_box_size_h = ctx->draw_state.clip_box.size.h;
  const int32_t saved_clip_box_origin_y = ctx->draw_state.clip_box.origin.y;
  ctx->draw_state.clip_box.origin.y =
      MAX(ctx->draw_state.clip_box.origin.y - STATUS_BAR_LAYER_HEIGHT, 0);
  ctx->draw_state.clip_box.size.h = DISP_ROWS;
  grect_clip(&ctx->draw_state.clip_box, &DISP_FRAME);

  const int32_t banner_movement_raw_offset =
      CLIP(BANNER_PEEK_STATIC_Y - notification_layout_frame->origin.y,
           0, BANNER_PEEK_STATIC_Y);
  int32_t banner_radius = prv_interpolate_linear(BOTTOM_BANNER_CIRCLE_RADIUS,
                                                 BANNER_CIRCLE_RADIUS,
                                                 0, BANNER_PEEK_STATIC_Y,
                                                 banner_movement_raw_offset);
                                               
  // Adjust radius based on banner text width if available
  if (notification_layout->banner_title_layer) {
    banner_radius = (int32_t)(banner_radius * 1.5f);
  }
  
  const int32_t banner_diameter = banner_radius * 2;
  int32_t banner_center_y = prv_interpolate_linear(0, LAYOUT_TOP_BANNER_ORIGIN_Y,
                                                         0, BANNER_PEEK_STATIC_Y,
                                                         banner_movement_raw_offset);
  
  // Only use flat-bottomed pill shape on the big banner
  if (notification_layout->banner_title_layer && notification_layout_frame->origin.y <= STATUS_BAR_LAYER_HEIGHT) {
    banner_center_y = banner_center_y - 56;
    // Draw flat-bottomed pill shape when there's text and we're on the banner
    
    // Top half circle
    const GRect top_half_frame = GRect(half_screen_width - banner_radius,
                                     banner_center_y - banner_radius,
                                     banner_diameter, banner_radius);
    
    // Rectangle for bottom half
    const GRect bottom_half_frame = GRect(half_screen_width - banner_radius,
                                        banner_center_y,
                                        banner_diameter, banner_radius);
    
    // Draw the top half circle (actually an oval since we're only using half the height)
    graphics_fill_radial(ctx, top_half_frame, GOvalScaleModeFitCircle, 
                         banner_radius, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(180));
    
    // Draw the rectangle for bottom half
    graphics_fill_rect(ctx, &bottom_half_frame);
  } else {
    // Draw full circle for other pages or when there's no text
    const GRect banner_frame = GRect(half_screen_width - banner_radius,
                                   banner_center_y - banner_radius,
                                   banner_diameter, banner_diameter);
    graphics_fill_oval(ctx, banner_frame, GOvalScaleModeFitCircle);
  }
  
  ctx->draw_state.clip_box.origin.y = saved_clip_box_origin_y;
  ctx->draw_state.clip_box.size.h = saved_clip_box_size_h;
}
#endif

static NOINLINE void prv_card_render_internal(NotificationLayout *layout, GContext *ctx,
                                              bool render) {
#if PBL_ROUND
  const int orig_clip_height = ctx->draw_state.clip_box.size.h;
  const GRect *notification_layout_frame = &layout->layout.layer.frame;
#endif

  // get layout colors and fill in the banner at the top
  if (render) {
    const LayoutColors *colors = prv_layout_get_colors((LayoutLayer *)layout);
    graphics_context_set_fill_color(ctx, colors->bg_color);

#if PBL_ROUND
    prv_hide_or_show_banner_icon(&layout->icon_layer, notification_layout_frame);
    prv_draw_banner_round(layout, ctx, notification_layout_frame, *colors);
    // work around the clip box and smaller layout_height for circular text paging
    ctx->draw_state.clip_box.size.h = MIN(ctx->draw_state.clip_box.size.h, LAYOUT_HEIGHT);
#else
    static const GRect banner_box = { .size = { DISP_COLS, LAYOUT_BANNER_HEIGHT_RECT } };
    graphics_fill_rect(ctx, &banner_box);
#endif
  }

#if PBL_ROUND
  const bool text_visible =
      (render && WITHIN(notification_layout_frame->origin.y,
                        TEXT_VISIBLE_LOWER_THRESHOLD(notification_layout_frame->size.h),
                        TEXT_VISIBLE_UPPER_THRESHOLD));
#else
  const bool text_visible = render;
#endif
  uint16_t text_shift = 0;
  if (layout->banner_title_layer) {
    text_shift = PBL_IF_ROUND_ELSE(10, 0);
  }
  const GRect box = {
    .origin = { CARD_MARGIN, LAYOUT_TOP_BANNER_HEIGHT + text_shift },
    .size = { DISP_COLS - 2 * CARD_MARGIN, LAYOUT_MAX_HEIGHT },
  };
  static const GRect page_frame_on_screen = {
    .origin = { 0, STATUS_BAR_LAYER_HEIGHT },
    .size = { DISP_COLS, DISP_ROWS - STATUS_BAR_LAYER_HEIGHT - LAYOUT_ARROW_HEIGHT }
  };
  static const GTextNodeDrawConfig config = {
    .page_frame = &page_frame_on_screen,
    .origin_on_screen = &page_frame_on_screen.origin,
    .content_inset = 8, // text flow inset
    .text_flow = PBL_IF_ROUND_ELSE(true, false),
    .paging = PBL_IF_ROUND_ELSE(true, false),
  };
  graphics_context_set_text_color(ctx, GColorBlack);
  (text_visible ? graphics_text_node_draw :
                  graphics_text_node_get_size)(layout->view_node, ctx, &box, &config,
                                               &layout->view_size);

#if PBL_ROUND
  if (render) {
    // restore original clip box
    ctx->draw_state.clip_box.size.h = orig_clip_height;
  }
#endif

  layout->view_size.h += LAYOUT_TOP_BANNER_HEIGHT;

#if PBL_ROUND
  // Notification text is paged by LAYOUT_HEIGHT, so make full page height
  layout->view_size.h = ROUND_TO_MOD_CEIL(layout->view_size.h, LAYOUT_HEIGHT);
  // Notifications are swapped using frame height, so last page includes additional arrow height
  layout->view_size.h += LAYOUT_ARROW_HEIGHT;
#else
  layout->view_size.h += CARD_BOTTOM_PADDING;
#endif
}

static void prv_card_render(NotificationLayout *layout, GContext *ctx, bool render) {
  if (!layout->view_node) {
    prv_init_view(layout);
  }
  prv_card_render_internal(layout, ctx, render);
}

//////////////////////////////////////////
// LayoutLayer API
//////////////////////////////////////////

static void prv_layout_update_proc(Layer *layer, GContext *ctx) {
  NotificationLayout *layout = (NotificationLayout *)layer;
  
  // Check if banner should be animated
  if (layout->banner_title_layer && layout->banner_title_scroll_distance > 0) {
    bool should_animate = prv_should_animate_banner(layout);
    
    if (should_animate && layout->banner_scroll_state == BANNER_ANIM_STATE_IDLE && 
      layout->banner_scroll_cycle != BANNER_CYCLES_COMPLETED_FLAG) {
      // Only start a new animation if not already completed all cycles
      layout->banner_scroll_cycle = 0;
      layout->banner_scroll_state = BANNER_ANIM_STATE_PAUSED_START;
      layout->banner_scroll_timer = app_timer_register(2000, prv_start_next_banner_animation, layout);
    } else if (!should_animate) {
      // Reset the completed flag when notification goes off-screen
      layout->banner_scroll_cycle = 0;
      
      // Cancel any pending animation
      if (layout->banner_scroll_timer) {
        app_timer_cancel(layout->banner_scroll_timer);
        layout->banner_scroll_timer = NULL;
      }

      // Reset text position
      if (layout->banner_title_layer) {
        Layer *text_layer = text_layer_get_layer(layout->banner_title_layer);
        GRect frame;
        layer_get_frame(text_layer, &frame);
        frame.origin.x = 0;
        layer_set_frame(text_layer, &frame);
      }
    }
  }
  
  // Render the notification
  switch (layout->layout.mode) {
    case LayoutLayerModeCard:
      prv_card_render(layout, ctx, true);
      break;
    default:
      break;
  }
}

static void prv_layout_init(NotificationLayout *layout, const LayoutLayerConfig *config);

LayoutLayer *notification_layout_create(const LayoutLayerConfig *config) {
  NotificationLayout *layout = task_zalloc_check(sizeof(NotificationLayout));
  if (!layout) {
    return NULL;
  }
  prv_layout_init(layout, config);
  return (LayoutLayer *)layout;
}

bool notification_layout_verify(bool existing_attributes[]) {
  return existing_attributes[AttributeIdTitle];
}

static void prv_layout_init_colors(NotificationLayout *notification_layout) {
  LayoutColors *colors = &notification_layout->colors;
  *colors = (LayoutColors) {
    .primary_color = GColorWhite,
    .secondary_color = GColorBlack,
    .bg_color = GColorBlack,
  };

#if PBL_COLOR
  const bool is_notification =
      (notification_layout->info.item->header.type == TimelineItemTypeNotification);
  const GColor default_bg_color = is_notification ? DEFAULT_NOTIFICATION_COLOR :
                                                    DEFAULT_REMINDER_COLOR;

  LayoutLayer *layout = &notification_layout->layout;
  colors->bg_color = (GColor) attribute_get_uint8(layout->attributes, AttributeIdBgColor,
                                                  default_bg_color.argb);
  colors->primary_color = (GColor) attribute_get_uint8(layout->attributes, AttributeIdPrimaryColor,
                                                       GColorBlack.argb);
#endif
}

static const LayoutColors *prv_layout_get_colors(const LayoutLayer *layout_ref) {
  return &((NotificationLayout *)layout_ref)->colors;
}

static void *prv_layout_get_context(LayoutLayer *layout) {
  NotificationLayout *notification_layout = (NotificationLayout *)layout;
  return (void *)notification_layout->info.item;
}

static GSize prv_layout_get_content_size(GContext *ctx, LayoutLayer *layout_ref) {
  NotificationLayout *layout = (NotificationLayout *)layout_ref;
  if (layout->view_size.h == 0) {
    prv_card_render(layout, graphics_context_get_current_context(), false);
  }
  return layout->view_size;
}

static void prv_layout_destroy(LayoutLayer *layout) {
  NotificationLayout *notification_layout = (NotificationLayout *)layout;
  
  // Mark as destroyed
  notification_layout->destroyed = true;
  
  // Cancel any pending timer
  if (notification_layout->banner_scroll_timer) {
    app_timer_cancel(notification_layout->banner_scroll_timer);
    notification_layout->banner_scroll_timer = NULL;
  }
  
  // Cancel any active animation
  if (s_banner_animation) {
    animation_unschedule(s_banner_animation);
    animation_destroy(s_banner_animation);
    s_banner_animation = NULL;
  }
  
  // Clean up text layers
  if (notification_layout->banner_title_layer) {
    text_layer_destroy(notification_layout->banner_title_layer);
    notification_layout->banner_title_layer = NULL;
  }
  
  // Now destroy other components
  prv_destroy_view(notification_layout);
  kino_layer_deinit(&notification_layout->icon_layer);
  task_free(notification_layout);
}

static void prv_layout_init(NotificationLayout *layout, const LayoutLayerConfig *config) {
  NotificationLayoutInfo *layout_info = config->context;
  static const LayoutLayerImpl s_layout_layer_impl = {
    .size_getter = prv_layout_get_content_size,
    .destructor = prv_layout_destroy,
#if PBL_COLOR
    .color_getter = prv_layout_get_colors,
#endif
    .context_getter = prv_layout_get_context,
  };
  // init the layout struct
  layout->layout = (LayoutLayer) {
    .mode = config->mode,
    .attributes = config->attributes,
    .impl = &s_layout_layer_impl,
  };
  layout->info = *layout_info;

  // init the layer in the layout
  layer_init(&layout->layout.layer, config->frame);
  layer_set_update_proc(&layout->layout.layer, prv_layout_update_proc);
#if PBL_ROUND
  layer_set_clips(&layout->layout.layer, false);
#endif

  prv_layout_init_colors(layout);

  switch (layout->layout.mode) {
    case LayoutLayerModeCard:
      prv_card_init(layout, config->attributes, config->app_id);
      break;
    default:
      break;
  }

  layer_mark_dirty(&(layout->layout.layer));
}
#else
LayoutLayer *notification_layout_create(const LayoutLayerConfig *config) { return NULL; }

bool notification_layout_verify(bool existing_attributes[]) { return false; }
#endif

TimelineResourceId notification_layout_get_fallback_icon_id(TimelineItemType item_type) {
  return (item_type == TimelineItemTypeNotification) ? NOTIF_FALLBACK_ICON :
                                                       REMINDER_FALLBACK_ICON;
}
