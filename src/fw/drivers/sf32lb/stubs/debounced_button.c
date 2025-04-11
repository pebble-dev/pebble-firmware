#include "drivers/debounced_button.h"

#include "board/board.h"
#include "drivers/button.h"
#include "drivers/exti.h"
#include "drivers/gpio.h"
#include "drivers/periph_config.h"
#include "drivers/timer.h"
#include "kernel/events.h"
#include "kernel/util/stop.h"
#include "system/bootbits.h"
#include "system/reset.h"
#include "util/bitset.h"
#include "kernel/util/sleep.h"

// #define NRF5_COMPATIBLE
#include <mcu.h>


void debounced_button_init(void) {

}

// Serial commands
///////////////////////////////////////////////////////////
void command_put_raw_button_event(const char* button_index, const char* is_button_down_event) {
  PebbleEvent e;
  int is_down = atoi(is_button_down_event);
  int button = atoi(button_index);

  if ((button < 0 || button > NUM_BUTTONS) || (is_down != 1 && is_down != 0)) {
    return;
  }
  e.type = (is_down) ? PEBBLE_BUTTON_DOWN_EVENT : PEBBLE_BUTTON_UP_EVENT;
  e.button.button_id = button;
  event_put(&e);
}
