#include "drivers/exti.h"

#include "board/board.h"
#include "drivers/periph_config.h"
#include "kernel/events.h"
#include "mcu/interrupts.h"
#include "system/passert.h"


#include <mcu.h>

#include <stdbool.h>

#if 0

static void prv_exti_handler(nrfx_gpiote_pin_t pin, nrfx_gpiote_trigger_t trigger, void *p_context) {
  ExtiHandlerCallback cb = (ExtiHandlerCallback) p_context;
  
  bool should_context_switch = false;
  cb(&should_context_switch);
  
  portEND_SWITCHING_ISR(should_context_switch);
}

#endif

void exti_configure_pin(ExtiConfig cfg, ExtiTrigger trigger, ExtiHandlerCallback cb) {
  
}

void exti_configure_other(ExtiLineOther exti_line, ExtiTrigger trigger) {

}

void exti_enable_other(ExtiLineOther exti_line) {
  
}

void exti_disable_other(ExtiLineOther exti_line) {

}

#if 0
void exti_set_pending(ExtiConfig cfg) {

}

void exti_clear_pending_other(ExtiLineOther exti_line) {

}




void exti_enable(ExtiConfig cfg) {

}

void exti_disable(ExtiConfig cfg) {

}

#endif 