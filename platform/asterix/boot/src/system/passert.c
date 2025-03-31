#include "passert.h"

#include "system/die.h"

#include "drivers/dbgserial.h"
#include "util/misc.h"

#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static __attribute__((noreturn)) void handle_passert_failed_vargs(const char* filename, int line_number,
    uintptr_t lr, const char* expr, const char* fmt, va_list fmt_args) {
  (void)lr;
  (void)fmt_args;

  dbgserial_print("ASSERT: ");
  dbgserial_print(expr);
  dbgserial_print("  ");
  dbgserial_print(filename);
  dbgserial_print(":");
  dbgserial_print_hex(line_number);
  if (fmt) {
    dbgserial_print(" ");
    dbgserial_print(fmt);
  }
  dbgserial_putstr("");

  reset_due_to_software_failure();
}

static __attribute__((noreturn)) void handle_passert_failed(const char* filename, int line_number,
    uintptr_t lr, const char *expr, const char* fmt, ...) {
  va_list fmt_args;
  va_start(fmt_args, fmt);

  handle_passert_failed_vargs(filename, line_number, lr, expr, fmt, fmt_args);

  va_end(fmt_args);
}

void passert_failed(const char* filename, int line_number, const char* message, ...) {
  va_list fmt_args;
  va_start(fmt_args, message);

  handle_passert_failed_vargs(filename, line_number,
    (uintptr_t)__builtin_return_address(0), "ASSERT", message, fmt_args);

  va_end(fmt_args);
}

void passert_failed_no_message(const char* filename, int line_number) {
  handle_passert_failed(filename, line_number,
    (uintptr_t)__builtin_return_address(0), "ASSERTN", NULL);
}

void wtf(void) {
  uintptr_t saved_lr = (uintptr_t) __builtin_return_address(0);
  dbgserial_print("*** WTF ");
  dbgserial_print_hex(saved_lr);
  dbgserial_putstr("");
  reset_due_to_software_failure();
}

//! Assert function called by the STM peripheral library's
//! 'assert_param' method. See stm32f2xx_conf.h for more information.
void assert_failed(uint8_t* file, uint32_t line) {
  register uintptr_t lr __asm("lr");
  uintptr_t saved_lr = lr;

  handle_passert_failed((const char*) file, line, saved_lr, "STM32", "STM32 peripheral library tripped an assert");
}

extern void command_dump_malloc_kernel(void);

void croak_oom(const char *filename, int line_number, const char *fmt, ...) {
  register uintptr_t lr __asm("lr");
  uintptr_t saved_lr = lr;

#ifdef MALLOC_INSTRUMENTATION
  command_dump_malloc_kernel();
#endif

  va_list fmt_args;
  va_start(fmt_args, fmt);

  handle_passert_failed_vargs(filename, line_number, saved_lr, "CROAK OOM", fmt, fmt_args);

  va_end(fmt_args);
}
