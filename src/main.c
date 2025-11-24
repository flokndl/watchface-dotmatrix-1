#include <pebble.h>
#include <stdlib.h>
#include "message_keys.auto.h"

static Window *s_main_window;

// Transition system: each digit has a mask layer, wrapper layer, and two bitmaps (old/new)
static Layer *s_mask_layers[4];           // Clipping mask (like #digit in CSS)
static Layer *s_wrapper_layers[4];        // Wrapper that slides (like #wrapper in CSS)
static GBitmap *s_old_bitmaps[4];         // Old digit bitmap
static GBitmap *s_new_bitmaps[4];         // New digit bitmap
static int s_current_digits[4];           // Track current digit for each position
static Animation *s_animations[4];        // Animation for each digit transition
static PropertyAnimation *s_prop_animations[4];  // Property animations (need to be destroyed)
static AppTimer *s_delay_timers[4];       // Delay timers for image_0 and image_2

// Image dimensions
#define IMAGE_WIDTH 56
#define IMAGE_HEIGHT 72
#define TRANSITION_DURATION_MS 500  // Animation duration in milliseconds
#define DELAY_MS 200  // Delay for image_0 and image_2 transitions

// Position constants
#define PADDING_TOP 12
#define PADDING_LEFT 16

// Persistent storage key
#define SETTINGS_KEY 1

// Settings structure
typedef struct {
  bool use_tabular_style_1;
} Settings;

static Settings s_settings = {
  .use_tabular_style_1 = true  // Default: checked
};

// Forward declarations
static void load_settings(void);
static void save_settings(void);
static void inbox_received_handler(DictionaryIterator *iter, void *context);
static void inbox_dropped_handler(AppMessageResult reason, void *context);
static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context);
static void outbox_sent_handler(DictionaryIterator *iter, void *context);
static void update_time(void);

// Helper function to get resource ID for a digit (0-9)
// Resource IDs are: NUM_0=1, NUM_1=2, NUM_1_ALT=3, NUM_2=4, NUM_3=5, etc.
// So digits 2-9 need to account for NUM_1_ALT being inserted
static uint32_t get_num_resource_id(int digit) {
  // Use alternate image for "1" if tabular style is disabled
  if (digit == 1 && !s_settings.use_tabular_style_1) {
    return RESOURCE_ID_NUM_1_ALT;
  }
  
  // Map digits to resource IDs correctly
  // 0 → NUM_0, 1 → NUM_1, 2 → NUM_2, 3 → NUM_3, etc.
  // Since NUM_1_ALT (ID 3) is between NUM_1 and NUM_2, digits 2-9 need +1 offset
  if (digit == 0) {
    return RESOURCE_ID_NUM_0;
  } else if (digit == 1) {
    return RESOURCE_ID_NUM_1;
  } else {
    // For digits 2-9, add 1 to account for NUM_1_ALT
    return RESOURCE_ID_NUM_0 + digit + 1;
  }
}

// Animation stopped callback - clean up old bitmap and swap
static void animation_stopped_handler(Animation *animation, bool finished, void *context) {
  int digit_index = (int)context;
  
  if (finished && digit_index >= 0 && digit_index < 4) {
    // Destroy old bitmap
    if (s_old_bitmaps[digit_index]) {
      gbitmap_destroy(s_old_bitmaps[digit_index]);
      s_old_bitmaps[digit_index] = NULL;
    }
    
    // Swap: new becomes old
    s_old_bitmaps[digit_index] = s_new_bitmaps[digit_index];
    s_new_bitmaps[digit_index] = NULL;
    
    // Reset wrapper position to 0 (showing the "old" which is now the current)
    layer_set_frame(s_wrapper_layers[digit_index], GRect(0, 0, IMAGE_WIDTH * 2, IMAGE_HEIGHT));
    layer_mark_dirty(s_wrapper_layers[digit_index]);
  }
  
  // Clean up animation
  if (s_animations[digit_index]) {
    animation_destroy(s_animations[digit_index]);
    s_animations[digit_index] = NULL;
  }
  if (s_prop_animations[digit_index]) {
    property_animation_destroy(s_prop_animations[digit_index]);
    s_prop_animations[digit_index] = NULL;
  }
}

// Wrapper layer update proc - draws both old and new bitmaps side by side
static void wrapper_layer_update_proc(Layer *layer, GContext *ctx) {
  int *digit_index_ptr = (int*)layer_get_data(layer);
  if (!digit_index_ptr) {
    return;  // Safety check
  }
  int digit_index = *digit_index_ptr;
  
  if (digit_index < 0 || digit_index >= 4) {
    return;  // Safety check
  }
  
  // Set compositing mode for transparency
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  
  // Draw old bitmap at (0, 0)
  if (s_old_bitmaps[digit_index]) {
    graphics_draw_bitmap_in_rect(ctx, s_old_bitmaps[digit_index], 
                                 GRect(0, 0, IMAGE_WIDTH, IMAGE_HEIGHT));
  }
  
  // Draw new bitmap at (IMAGE_WIDTH, 0) - side by side
  if (s_new_bitmaps[digit_index]) {
    graphics_draw_bitmap_in_rect(ctx, s_new_bitmaps[digit_index], 
                                 GRect(IMAGE_WIDTH, 0, IMAGE_WIDTH, IMAGE_HEIGHT));
  }
}

// Start the animation for a digit (called directly or after delay)
static void start_digit_animation(int digit_index) {
  if (digit_index < 0 || digit_index >= 4 || !s_wrapper_layers[digit_index]) {
    return;
  }
  
  // Cancel any existing animation for this digit
  if (s_animations[digit_index]) {
    animation_unschedule(s_animations[digit_index]);
    animation_destroy(s_animations[digit_index]);
    s_animations[digit_index] = NULL;
  }
  if (s_prop_animations[digit_index]) {
    property_animation_destroy(s_prop_animations[digit_index]);
    s_prop_animations[digit_index] = NULL;
  }
  
  // Create animation to slide wrapper from 0 to -IMAGE_WIDTH
  GRect start_frame = GRect(0, 0, IMAGE_WIDTH * 2, IMAGE_HEIGHT);
  GRect end_frame = GRect(-IMAGE_WIDTH, 0, IMAGE_WIDTH * 2, IMAGE_HEIGHT);
  
  PropertyAnimation *prop_anim = property_animation_create_layer_frame(
    s_wrapper_layers[digit_index],
    &start_frame,
    &end_frame
  );
  
  if (!prop_anim) {
    // Animation creation failed, just swap immediately
    if (s_old_bitmaps[digit_index]) {
      gbitmap_destroy(s_old_bitmaps[digit_index]);
    }
    s_old_bitmaps[digit_index] = s_new_bitmaps[digit_index];
    s_new_bitmaps[digit_index] = NULL;
    layer_mark_dirty(s_wrapper_layers[digit_index]);
    return;
  }
  
  Animation *anim = property_animation_get_animation(prop_anim);
  animation_set_duration(anim, TRANSITION_DURATION_MS);
  animation_set_curve(anim, AnimationCurveEaseOut);
  animation_set_handlers(anim, (AnimationHandlers) {
    .stopped = animation_stopped_handler
  }, (void*)digit_index);
  
  // Store animations
  s_animations[digit_index] = anim;
  s_prop_animations[digit_index] = prop_anim;
  
  // Mark wrapper dirty to show new bitmap
  layer_mark_dirty(s_wrapper_layers[digit_index]);
  
  // Start animation
  animation_schedule(anim);
}

// Timer callback to start delayed animation
static void delayed_animation_callback(void *context) {
  int digit_index = (int)context;
  s_delay_timers[digit_index] = NULL;
  start_digit_animation(digit_index);
}

// Update a single digit with transition
static void update_digit_with_transition(int digit_index, int new_digit, GRect position) {
  // Safety check
  if (digit_index < 0 || digit_index >= 4) {
    return;
  }
  
  // Check if layers are ready
  if (!s_wrapper_layers[digit_index]) {
    return;
  }
  
  // Check if digit actually changed
  int old_digit = s_current_digits[digit_index];
  if (old_digit == new_digit) {
    return;  // No change, no transition needed
  }
  
  // Load new bitmap
  GBitmap *new_bitmap = gbitmap_create_with_resource(get_num_resource_id(new_digit));
  if (!new_bitmap) {
    return;
  }
  
  // Store new bitmap
  s_new_bitmaps[digit_index] = new_bitmap;
  
  // If this is the first time, also set up the old bitmap
  if (!s_old_bitmaps[digit_index]) {
    s_old_bitmaps[digit_index] = new_bitmap;
    s_new_bitmaps[digit_index] = NULL;
    s_current_digits[digit_index] = new_digit;
    if (s_wrapper_layers[digit_index]) {
      layer_mark_dirty(s_wrapper_layers[digit_index]);
    }
    return;
  }
  
  // Cancel any existing delay timer for this digit
  if (s_delay_timers[digit_index]) {
    app_timer_cancel(s_delay_timers[digit_index]);
    s_delay_timers[digit_index] = NULL;
  }
  
  // Update current digit
  s_current_digits[digit_index] = new_digit;
  
  // For image_0 (index 0) and image_2 (index 2), add 100ms delay
  if (digit_index == 0 || digit_index == 2) {
    s_delay_timers[digit_index] = app_timer_register(DELAY_MS, delayed_animation_callback, (void*)digit_index);
  } else {
    // For other digits, start animation immediately
    start_digit_animation(digit_index);
  }
}

// Update the displayed time
static void update_time(void) {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  int hours = tick_time->tm_hour;
  int minutes = tick_time->tm_min;
  
  // Convert to 12-hour format if needed
  if (!clock_is_24h_style()) {
    hours = hours % 12;
    if (hours == 0) hours = 12;
  }
  
  // Extract digits
  int hour_tens = hours / 10;
  int hour_ones = hours % 10;
  int minute_tens = minutes / 10;
  int minute_ones = minutes % 10;
  
  // Calculate positions
  int x_positions[4] = {PADDING_LEFT, PADDING_LEFT + IMAGE_WIDTH, 
                        PADDING_LEFT, PADDING_LEFT + IMAGE_WIDTH};
  int y_positions[4] = {PADDING_TOP, PADDING_TOP,
                        PADDING_TOP + IMAGE_HEIGHT, PADDING_TOP + IMAGE_HEIGHT};
  
  // Update each digit with transition
  update_digit_with_transition(0, hour_tens, GRect(x_positions[0], y_positions[0], IMAGE_WIDTH, IMAGE_HEIGHT));
  update_digit_with_transition(1, hour_ones, GRect(x_positions[1], y_positions[1], IMAGE_WIDTH, IMAGE_HEIGHT));
  update_digit_with_transition(2, minute_tens, GRect(x_positions[2], y_positions[2], IMAGE_WIDTH, IMAGE_HEIGHT));
  update_digit_with_transition(3, minute_ones, GRect(x_positions[3], y_positions[3], IMAGE_WIDTH, IMAGE_HEIGHT));
  
  // Hide image_0 (hour tens) when value is 0 (single digit hour)
  if (s_mask_layers[0]) {
    layer_set_hidden(s_mask_layers[0], hour_tens == 0);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  
  window_set_background_color(window, GColorBlack);
  
  int x_positions[4] = {PADDING_LEFT, PADDING_LEFT + IMAGE_WIDTH, 
                        PADDING_LEFT, PADDING_LEFT + IMAGE_WIDTH};
  int y_positions[4] = {PADDING_TOP, PADDING_TOP,
                        PADDING_TOP + IMAGE_HEIGHT, PADDING_TOP + IMAGE_HEIGHT};
  
  // Initialize current digits
  for (int i = 0; i < 4; i++) {
    s_current_digits[i] = -1;  // Invalid value to force first update
    s_old_bitmaps[i] = NULL;
    s_new_bitmaps[i] = NULL;
    s_animations[i] = NULL;
    s_prop_animations[i] = NULL;
    s_delay_timers[i] = NULL;
  }
  
  // Create mask layers (clipping containers) and wrapper layers for each digit
  for (int i = 0; i < 4; i++) {
    // Create mask layer (acts as overflow: hidden container)
    s_mask_layers[i] = layer_create(GRect(x_positions[i], y_positions[i], IMAGE_WIDTH, IMAGE_HEIGHT));
    layer_set_clips(s_mask_layers[i], true);  // Enable clipping
    
    // Create wrapper layer (slides inside mask)
    s_wrapper_layers[i] = layer_create_with_data(GRect(0, 0, IMAGE_WIDTH * 2, IMAGE_HEIGHT), sizeof(int));
    int *wrapper_data = (int*)layer_get_data(s_wrapper_layers[i]);
    *wrapper_data = i;
    layer_set_update_proc(s_wrapper_layers[i], wrapper_layer_update_proc);
    
    // Add wrapper to mask, mask to window
    layer_add_child(s_mask_layers[i], s_wrapper_layers[i]);
    layer_add_child(window_layer, s_mask_layers[i]);
  }
  
  // Load initial time
  update_time();
}

static void main_window_unload(Window *window) {
  // Cancel any running timers
  for (int i = 0; i < 4; i++) {
    if (s_delay_timers[i]) {
      app_timer_cancel(s_delay_timers[i]);
      s_delay_timers[i] = NULL;
    }
  }
  
  // Cancel any running animations
  for (int i = 0; i < 4; i++) {
    if (s_animations[i]) {
      animation_unschedule(s_animations[i]);
      animation_destroy(s_animations[i]);
      s_animations[i] = NULL;
    }
    if (s_prop_animations[i]) {
      property_animation_destroy(s_prop_animations[i]);
      s_prop_animations[i] = NULL;
    }
  }
  
  // Destroy all layers and bitmaps
  for (int i = 0; i < 4; i++) {
    if (s_mask_layers[i]) {
      layer_destroy(s_mask_layers[i]);
      s_mask_layers[i] = NULL;
    }
    if (s_wrapper_layers[i]) {
      layer_destroy(s_wrapper_layers[i]);
      s_wrapper_layers[i] = NULL;
    }
    if (s_old_bitmaps[i]) {
      gbitmap_destroy(s_old_bitmaps[i]);
      s_old_bitmaps[i] = NULL;
    }
    if (s_new_bitmaps[i]) {
      gbitmap_destroy(s_new_bitmaps[i]);
      s_new_bitmaps[i] = NULL;
    }
  }
}

static void init(void) {
  // Load settings from persistent storage
  load_settings();
  
  // Register AppMessage handlers
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_register_outbox_sent(outbox_sent_handler);
  
  // Open AppMessage with appropriate buffer sizes
  app_message_open(128, 128);
  
  s_main_window = window_create();
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  window_stack_push(s_main_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

// Load settings from persistent storage
static void load_settings(void) {
  if (persist_exists(SETTINGS_KEY)) {
    persist_read_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
  }
}

// Save settings to persistent storage
static void save_settings(void) {
  persist_write_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
}

// AppMessage inbox received handler
static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *tabular_style_t = dict_find(iter, MESSAGE_KEY_UseTabularStyle1);
  if (tabular_style_t) {
    bool old_setting = s_settings.use_tabular_style_1;
    s_settings.use_tabular_style_1 = tabular_style_t->value->int32 == 1;
    save_settings();
    
    // If setting changed and we have "1" digits displayed, force them to reload
    if (old_setting != s_settings.use_tabular_style_1) {
      // Force reload of "1" digits by invalidating their current state
      for (int i = 0; i < 4; i++) {
        if (s_current_digits[i] == 1) {
          s_current_digits[i] = -1;  // Force reload
        }
      }
    }
    
    // Update display with new setting
    update_time();
  }
}

// AppMessage inbox dropped handler
static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped! Reason: %d", reason);
}

// AppMessage outbox failed handler
static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed! Reason: %d", reason);
}

// AppMessage outbox sent handler
static void outbox_sent_handler(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
