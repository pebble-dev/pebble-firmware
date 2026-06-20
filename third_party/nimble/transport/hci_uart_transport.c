/* SPDX-FileCopyrightText: 2025 Google LLC */
/* SPDX-License-Identifier: Apache-2.0 */

// TODO: transport.h needs os_mbuf.h to be included first
// clang-format off
#include <os/os_mbuf.h>
// clang-format on

#include <board/board.h>
#include <drivers/gpio.h>
#include <drivers/exti.h>
#include <drivers/uart.h>
#include <kernel/pebble_tasks.h>
#include <nimble/transport.h>
#include <nimble/transport/hci_h4.h>
#include <nimble/transport_impl.h>
#include <os/os_mempool.h>
#include <queue.h>
#include <services/common/system_task.h>
#include <system/passert.h>
#include <util/circular_buffer.h>
#include <util/math.h>

// XXX: HACK: we need this to get access to rts_gpio and cts_gpio
#include "drivers/stm32f2/uart_definitions.h"

#define TX_Q_SIZE                                                                      \
  (MYNEWT_VAL(BLE_TRANSPORT_ACL_FROM_LL_COUNT) + MYNEWT_VAL(BLE_TRANSPORT_EVT_COUNT) + \
   MYNEWT_VAL(BLE_TRANSPORT_EVT_DISCARDABLE_COUNT))

extern void ble_chipset_init(void);
extern bool ble_chipset_start(void);
extern bool ble_chipset_is_hcill(void);

static void prv_rtscts_trigger(bool *should_context_switch);

static uint32_t s_uart_baud = 115200; /* boot the system at 115200 */

static bool prv_uart_tx_irq_handler(UARTDevice *dev);
static bool prv_uart_rx_irq_handler(UARTDevice *dev, uint8_t data, const UARTRXErrorFlags *err_flags);

static void prv_uart_init() {
  uart_init(BLUETOOTH_UART);
  uart_set_baud_rate(BLUETOOTH_UART, s_uart_baud);
  uart_set_rx_interrupt_handler(BLUETOOTH_UART, prv_uart_rx_irq_handler);
  uart_set_tx_interrupt_handler(BLUETOOTH_UART, prv_uart_tx_irq_handler);
  uart_set_rx_interrupt_enabled(BLUETOOTH_UART, true);
  
  uart_enable_flow_control(BLUETOOTH_UART);
}

// For chipsets that support eHCILL for power management (CC2564; ref:
// https://www.ti.com/lit/an/swra288b/swra288b.pdf), we intercept this at
// the packet-callback level.  hci_uart_packet_cb implements most of 

#define CMD_HCILL_GO_TO_SLEEP_IND 0x30
#define CMD_HCILL_GO_TO_SLEEP_ACK 0x31
#define CMD_HCILL_WAKE_UP_IND     0x32
#define CMD_HCILL_WAKE_UP_ACK     0x33

#define EHCILL_DBG(x, ...) PBL_LOG_D(LOG_DOMAIN_BT_STACK, LOG_LEVEL_ERROR, "eHCILL: " x, ##__VA_ARGS__)

// this should be fixed with the eHCILL SP update
// #define EHCILL_RETRANSMISSIONS_LEGAL

typedef enum EhcillSleepSmState {
  
  // AWAKE: We can transmit packets freely.
  //
  //   Receive GO_TO_SLEEP_IND ->
  //     Transition to EHCILL_PENDING_XMIT_SLEEP
  EHCILL_AWAKE,
  
  // SEND_ACK_THEN_AWAKE: They have just sent us a WAKE_UP_IND, and we need
  // to ACK it.
  //
  //   TX-empty IRQ fires ->
  //     Send WAKE_UP_ACK
  //     Transition to AWAKE
  EHCILL_SEND_ACK_THEN_AWAKE,
  
  // PENDING_XMIT_SLEEP: We need to complete transmitting whatever packet
  // we're in, and then send a GO_TO_SLEEP_ACK.
  //
  //   Packet is complete and TX-ready IRQ fires ->
  //     Transmit GO_TO_SLEEP_ACK
  //     If we have nothing to transmit:
  //       Shut down flow control
  //       Transition to EHCILL_PENDING_XMIT_SLEEP_FLUSH
  //     If we have something to transmit:
  //       Transition to EHCILL_SEND_WAKE_UP_IND
  EHCILL_PENDING_XMIT_SLEEP,
  
  // PENDING_XMIT_SLEEP_FLUSH: We sent the GO_TO_SLEEP_ACK and are waiting
  // for the UART to flush so we can shut it off.  RTS/CTS are already in
  // low-power mode.
  //
  //   TX-empty IRQ fires ->
  //     Transition to PENDING_SHUTDOWN_UART_BH
  //     Trigger IRQ bottom half
  //   RTS/CTS IRQ fires ->
  //     Turn off RTS/CTS
  //     Reenable flow control
  //     Transition to EHCILL_WAIT_FOR_WAKE_IND
  //   We want to transmit a packet ->
  //     Turn off RTS/CTS
  //     Reenable flow control
  //     Transition to EHCILL_SEND_WAKE_UP_IND
  EHCILL_PENDING_XMIT_SLEEP_FLUSH,
  
  // PENDING_SHUTDOWN_UART_BH: The UART has been flushed, and the bottom
  // half of the IRQ handler needs to actually go and switch the UART off.
  //
  //   IRQ bottom half scheduled ->
  //     Disable UART
  //     Transition to EHCILL_ASLEEP
  //   RTS/CTS IRQ fires ->
  //     Turn off RTS/CTS
  //     Reenable flow control
  //     Transition to EHCILL_WAIT_FOR_WAKE_IND
  //   We want to transmit a packet ->
  //     Turn off RTS/CTS
  //     Reenable flow control
  //     Transition to EHCILL_SEND_WAKE_UP_IND
  EHCILL_PENDING_SHUTDOWN_UART_BH,
  
  // ASLEEP: The UART is off.
  //
  //   RTS/CTS IRQ fires ->
  //     Schedule IRQ bottom half
  //     Transition to EHCILL_WAKE_UP_UART_BH
  //   We want to transmit a packet ->
  //     Turn on the UART
  //     Transition to EHCILL_SEND_WAKE_UP_IND
  EHCILL_ASLEEP,
  
  // SEND_WAKE_UP_IND: We need to transmit a WAKE_UP_IND.  The UART is on.
  //
  //   TX is ready ->
  //     Transmit WAKE_UP_IND
  //     Transition to EHCILL_WAIT_FOR_WAKE_ACK 
  EHCILL_SEND_WAKE_UP_IND,

  // WAKE_UP_UART_BH: The remote end has just woken us, and we need to turn
  // on the UART.
  //
  //   IRQ bottom half scheduled ->
  //     Turn on the UART
  //     Transition to EHCILL_WAIT_FOR_WAKE_IND
  //   We want to transmit a packet ->
  //     Turn on the UART
  //     Transition to EHCILL_SEND_WAKE_UP_IND
  EHCILL_WAKE_UP_UART_BH,

  // WAIT_FOR_WAKE_IND: The remote end has just woken us, and we are
  // waiting for their WAKE_UP_IND.
  //
  //   RX WAKE_UP_IND ->
  //     Transition to EHCILL_SEND_ACK_THEN_AWAKE
  EHCILL_WAIT_FOR_WAKE_IND,

  // WAIT_FOR_WAKE_ACK: We have just sent a WAKE_UP_IND and we are waiting
  // for their WAKE_UP_ACK (or, if wires cross, a WAKE_UP_IND).
  //
  //   RX WAKE_UP_IND or RX WAKE_UP_ACK ->
  //     Transition to EHCILL_AWAKE
  EHCILL_WAIT_FOR_WAKE_ACK,
} EhcillSleepSmState;

static PebbleMutex *s_ehcill_mutex;
static EhcillSleepSmState s_ehcill_sm = EHCILL_AWAKE;

static TaskHandle_t s_rx_task_handle;
static CircularBuffer s_rx_buffer;
static uint8_t s_rx_storage[256];
static SemaphoreHandle_t s_rx_data_ready;
static SemaphoreHandle_t s_cmd_done;

static QueueHandle_t s_tx_queue;
static struct hci_h4_sm hci_uart_h4sm;
static bool chipset_start_done = false;

static struct uart_tx *uart_tx_dispose_head = NULL;

static void prv_ehcill_state(EhcillSleepSmState st) {
  switch(st) {
#define ST(x) case x: /*EHCILL_DBG("SM transition " #x);*/ break;
  ST(EHCILL_AWAKE)
  ST(EHCILL_SEND_ACK_THEN_AWAKE)
  ST(EHCILL_PENDING_XMIT_SLEEP)
  ST(EHCILL_PENDING_XMIT_SLEEP_FLUSH)
  ST(EHCILL_PENDING_SHUTDOWN_UART_BH)
  ST(EHCILL_ASLEEP)
  ST(EHCILL_SEND_WAKE_UP_IND)
  ST(EHCILL_WAIT_FOR_WAKE_ACK)
  ST(EHCILL_WAKE_UP_UART_BH)
  ST(EHCILL_WAIT_FOR_WAKE_IND)
#undef ST
  default: WTF;
  }
  s_ehcill_sm = st;
}

static void prv_uart_prepare_for_sleep() {
  /* eHCILL requires that we activate the RTS/CTS pins before we transmit
   * the 'we are going to sleep' message.  Do this, but don't switch off the
   * UART, because it still needs to finish sending.
   */
  const InputConfig input_config = {
    .gpio = BLUETOOTH_UART->cts_gpio.gpio,
    .gpio_pin = BLUETOOTH_UART->cts_gpio.gpio_pin,
  };
  gpio_input_init(&input_config);
  const OutputConfig output_config = {
    .gpio = BLUETOOTH_UART->rts_gpio.gpio,
    .gpio_pin = BLUETOOTH_UART->rts_gpio.gpio_pin,
    .active_high = true,
  };
  gpio_output_init(&output_config, GPIO_OType_PP, GPIO_Speed_25MHz);
  gpio_output_set(&output_config, true);

  gpio_input_init(&BOARD_CONFIG_BT_COMMON.wakeup.int_gpio);
  exti_enable(BOARD_CONFIG_BT_COMMON.wakeup.int_exti);
}

static void prv_uart_go_to_sleep() {
  /* must not be called in ISR context! */
  uart_set_rx_interrupt_enabled(BLUETOOTH_UART, false);
  uart_set_tx_interrupt_enabled(BLUETOOTH_UART, false);

  uart_deinit(BLUETOOTH_UART);
}

static void prv_rtscts_trigger(bool *should_context_switch) {
  exti_disable(BOARD_CONFIG_BT_COMMON.wakeup.int_exti);

  if (s_ehcill_sm == EHCILL_PENDING_XMIT_SLEEP_FLUSH || s_ehcill_sm == EHCILL_PENDING_SHUTDOWN_UART_BH) {
    EHCILL_DBG("CTS: UART was not shut down yet, just turning on flow control again");
    uart_enable_flow_control(BLUETOOTH_UART); // this is ISR-safe, no need to delegate to bottom half for this
    prv_ehcill_state(EHCILL_WAIT_FOR_WAKE_IND);
  } else if (s_ehcill_sm == EHCILL_ASLEEP) {
    EHCILL_DBG("CTS: enqueuing UART restart");
    prv_ehcill_state(EHCILL_WAKE_UP_UART_BH);
    xSemaphoreGiveFromISR(s_rx_data_ready, (BaseType_t *)should_context_switch);
  } else {
    EHCILL_DBG("CTS: spurious IRQ?");
  }
}

struct uart_tx {
  uint8_t type;
  uint8_t sent_type;
  uint16_t len;
  uint16_t idx;

  struct os_mbuf *om;
  uint8_t *buf;
  bool buf_needs_free;

  struct uart_tx *dispose_next;
};


static void prv_lock(void) { portENTER_CRITICAL(); }

static void prv_unlock(void) { portEXIT_CRITICAL(); }


static uint16_t hci_uart_packet_cb(const uint8_t *buf, uint16_t len) {
  assert(len > 0);
  mutex_lock(s_ehcill_mutex);
  prv_lock(); // make sure that eHCILL state machine transitions are atomic with respect to ISR also
  switch (s_ehcill_sm) {
  case EHCILL_AWAKE:
  case EHCILL_SEND_ACK_THEN_AWAKE:
    if (buf[0] == CMD_HCILL_GO_TO_SLEEP_IND) {
      prv_ehcill_state(EHCILL_PENDING_XMIT_SLEEP);
      uart_set_tx_interrupt_enabled(BLUETOOTH_UART, true);
      prv_unlock();
      mutex_unlock(s_ehcill_mutex);
      EHCILL_DBG("recv CMD_HCILL_GO_TO_SLEEP_IND");
      return 1 /* we consumed the byte */;
    }
    if (buf[0] == CMD_HCILL_WAKE_UP_IND) {
#ifdef EHCILL_RETRANSMISSIONS_LEGAL
      /* This can also happen if waking the UART up sends a glitch, which
       * causes the next byte to be garbled.  We can't just send another
       * ACK, though, because if we were slow on the draw to wake up, maybe
       * we will be stomping on a packet?  I guess we eat it for now and
       * just cry
       */
      EHCILL_DBG("recv CMD_HCILL_WAKE_UP_IND out of sequence, maybe we were slow on the draw to wake up?");
      return 1;
#else
      PBL_CROAK("CMD_HCILL_WAKE_UP_IND while already awake!");
      WTF;
#endif
    }
   if (buf[0] == CMD_HCILL_WAKE_UP_ACK) {
      PBL_CROAK("CMD_HCILL_WAKE_UP_ACK while already awake!");
      WTF;
    }
    prv_unlock();
    mutex_unlock(s_ehcill_mutex);
    return 0; /* otherwise, this has to be a H4 packet; let the parser do its job */
  
  case EHCILL_PENDING_XMIT_SLEEP:
  case EHCILL_PENDING_XMIT_SLEEP_FLUSH:
  case EHCILL_PENDING_SHUTDOWN_UART_BH:
    if (buf[0] == CMD_HCILL_GO_TO_SLEEP_IND) {
      /* Ok, we were just slow answering. */
      prv_unlock();
      mutex_unlock(s_ehcill_mutex);

      EHCILL_DBG("baseband is grumpy that we were slow going to sleep");
      return 1;
    }
    PBL_CROAK("packet from baseband while pending entering sleep!");
    return 0;
  
  case EHCILL_ASLEEP:
  case EHCILL_WAKE_UP_UART_BH:
  case EHCILL_SEND_WAKE_UP_IND:
  case EHCILL_WAIT_FOR_WAKE_IND:
    if (buf[0] == CMD_HCILL_WAKE_UP_IND) {
      prv_ehcill_state(EHCILL_SEND_ACK_THEN_AWAKE);
      uart_set_tx_interrupt_enabled(BLUETOOTH_UART, true); // if there's work to do, we can continue again
      prv_unlock();
      EHCILL_DBG("recv CMD_HCILL_WAKE_UP_IND");
      mutex_unlock(s_ehcill_mutex);
      return 1 /* we consumed it */;
    }
#ifdef EHCILL_RETRANSMISSIONS_LEGAL
    if (buf[0] == CMD_HCILL_GO_TO_SLEEP_IND) {
      prv_unlock();
      EHCILL_DBG("recv CMD_HCILL_GO_TO_SLEEP_IND while already asleep.  wow, we must have been slow");
      mutex_unlock(s_ehcill_mutex);
      return 1 /* we consumed it */;
    }
#endif
    PBL_CROAK("illegal HCI command received while nominally asleep");
    WTF;
    break;
  
  case EHCILL_WAIT_FOR_WAKE_ACK:
    if (buf[0] == CMD_HCILL_WAKE_UP_IND || buf[0] == CMD_HCILL_WAKE_UP_ACK) {
      prv_ehcill_state(EHCILL_AWAKE);
      uart_set_tx_interrupt_enabled(BLUETOOTH_UART, true); // if there's work to do, we can continue again
      prv_unlock();
      if (buf[0] == CMD_HCILL_WAKE_UP_IND) {
        EHCILL_DBG("recv CMD_HCILL_WAKE_UP_IND");
      } else {
        EHCILL_DBG("recv CMD_HCILL_WAKE_UP_ACK");
      }
      mutex_unlock(s_ehcill_mutex);
      return 1 /* we consumed it */;
    }
    PBL_CROAK("illegal HCI command received while waiting for wake ack");
    WTF;
    break;
  
  default:
    WTF;
  }
}

static int hci_uart_frame_cb(uint8_t pkt_type, void *data) {
  xSemaphoreGive(s_cmd_done);

  // HACK: passing responses to commands Nimble didn't generate causes issues
  if (!chipset_start_done) {
    // let the caller free it by returning nonzero
    return 1;
  }

  switch (pkt_type) {
    case HCI_H4_ACL:
      return ble_transport_to_hs_acl(data);
    case HCI_H4_EVT:
      return ble_transport_to_hs_evt(data);
    case HCI_H4_ISO:
      return ble_transport_to_hs_iso(data);
    default:
      WTF;
  }

  return -1;
}

static void prv_dispose_uart_tx(struct uart_tx *tx) {
  struct uart_tx *next;
  while (tx) {
    if (tx->type == HCI_H4_CMD && tx->buf_needs_free) {
      ble_transport_free(tx->buf);
    }
    if (tx->type == HCI_H4_ISO || tx->type == HCI_H4_ACL) {
      os_mbuf_free_chain(tx->om);
    }
    next = tx->dispose_next;
    kernel_free(tx);
    tx = next;
  }
}

static int hci_uart_tx_char(BaseType_t *should_context_switch) {
  struct uart_tx *tx = NULL;
  uint8_t ch;
  bool should_dispose = false;

  if (s_ehcill_sm == EHCILL_SEND_ACK_THEN_AWAKE) {
    EHCILL_DBG("waiting to send ack, xmit CMD_HCILL_WAKE_UP_ACK");
    prv_ehcill_state(EHCILL_AWAKE);
    return CMD_HCILL_WAKE_UP_ACK;
  }
  
  if (s_ehcill_sm == EHCILL_PENDING_XMIT_SLEEP_FLUSH) {
    EHCILL_DBG("last tx is done");
    prv_ehcill_state(EHCILL_PENDING_SHUTDOWN_UART_BH);
    xSemaphoreGiveFromISR(s_rx_data_ready, should_context_switch);
    return -1;
  }

  if ((xQueuePeekFromISR(s_tx_queue, &tx) == pdFALSE) &&
      (s_ehcill_sm != EHCILL_PENDING_XMIT_SLEEP)) {
    return -1;
  }

  if (s_ehcill_sm == EHCILL_ASLEEP || s_ehcill_sm == EHCILL_WAKE_UP_UART_BH || s_ehcill_sm == EHCILL_PENDING_SHUTDOWN_UART_BH) {
    PBL_CROAK("tx ready IRQ while eHCILL is asleep");
    return -1;
  }

  if (s_ehcill_sm == EHCILL_SEND_WAKE_UP_IND) {
    PBL_ASSERT(!tx || !tx->sent_type, "we found ourselves asleep in the middle of a packet");
    /* We have work to do; we must initiate a wakeup first, though. */
    EHCILL_DBG("was asleep, xmit CMD_HCILL_WAKE_UP_IND");
    prv_ehcill_state(EHCILL_WAIT_FOR_WAKE_ACK);
    return CMD_HCILL_WAKE_UP_IND;
  }
  
  if (s_ehcill_sm == EHCILL_WAIT_FOR_WAKE_ACK || s_ehcill_sm == EHCILL_WAIT_FOR_WAKE_IND) {
    PBL_ASSERT(!tx || !tx->sent_type, "we found ourselves exiting sleep in the middle of a packet");
    /* We are asleep and cannot transmit until we get the wake ack. */
    EHCILL_DBG("xmit requested, but waiting for wake ACK");
    return -1;
  }
  
  if ((!tx || !tx->sent_type) && s_ehcill_sm == EHCILL_PENDING_XMIT_SLEEP) {
    /* The baseband has told us to go to sleep; we need to ACK, but only at
     * a packet boundary.  Since there is either no packet or we have not
     * started sending it yet, we can send the ACK and go to sleep.
     */
    ch = CMD_HCILL_GO_TO_SLEEP_ACK;
    if (tx) {
      // we have work to do again, so wake right back up 
      EHCILL_DBG("xmit CMD_HCILL_GO_TO_SLEEP_ACK, but there's still work to do");
      prv_ehcill_state(EHCILL_SEND_WAKE_UP_IND);
    } else {
      EHCILL_DBG("xmit CMD_HCILL_GO_TO_SLEEP_ACK");
      prv_uart_prepare_for_sleep();
      prv_ehcill_state(EHCILL_PENDING_XMIT_SLEEP_FLUSH);
    }
  } else if (!tx->sent_type) {
    tx->sent_type = 1;
    ch = tx->type;
  } else {
    switch (tx->type) {
      case HCI_H4_CMD:
        ch = tx->buf[tx->idx];
        tx->idx++;
        if (tx->idx == tx->len) {
          should_dispose = true;
        }
        break;
      case HCI_H4_ACL:
      case HCI_H4_ISO:
        os_mbuf_copydata(tx->om, 0, 1, &ch);
        os_mbuf_adj(tx->om, 1);
        tx->len--;
        if (tx->len == 0) {
          should_dispose = true;
        }
        break;
    
      default:
        WTF;
    }
  }
  
  if (should_dispose) {
    tx->dispose_next = uart_tx_dispose_head;
    uart_tx_dispose_head = tx;
    xSemaphoreGiveFromISR(s_rx_data_ready, should_context_switch);
    xQueueReceiveFromISR(s_tx_queue, &tx, should_context_switch);
  }
  
  return ch;
}

static void ble_hci_tx_byte(BaseType_t *should_context_switch) {
  int c = hci_uart_tx_char(should_context_switch);
  if (c == -1) {
    uart_set_tx_interrupt_enabled(BLUETOOTH_UART, false);
  } else {
    uart_write_byte(BLUETOOTH_UART, c);
  }
}

static bool prv_uart_tx_irq_handler(UARTDevice *dev) {
  BaseType_t should_context_switch = false;
  ble_hci_tx_byte(&should_context_switch);
  return should_context_switch;
}

static bool prv_uart_rx_irq_handler(UARTDevice *dev, uint8_t data,
                                    const UARTRXErrorFlags *err_flags) {
  BaseType_t should_context_switch = false;

  if (err_flags->framing_error || err_flags->overrun_error) {
    PBL_LOG_D(LOG_DOMAIN_BT_STACK, LOG_LEVEL_ERROR, "Bluetooth UART overrun:%d framing:%d",
              err_flags->overrun_error, err_flags->framing_error);
  }

  prv_lock();
  PBL_ASSERTN(circular_buffer_get_write_space_remaining(&s_rx_buffer) > 0);
  circular_buffer_write(&s_rx_buffer, &data, 1);
  xSemaphoreGiveFromISR(s_rx_data_ready, &should_context_switch);
  prv_unlock();

  return should_context_switch;
}

static uint8_t read_buf[64];
static void prv_rx_task_main(void *unused) {
  int consumed_bytes;
  uint16_t bytes_remaining;
  struct uart_tx *dispose_head;

  while (true) {
    xSemaphoreTake(s_rx_data_ready, portMAX_DELAY);

    mutex_lock(s_ehcill_mutex);
    prv_lock();
    if (s_ehcill_sm == EHCILL_PENDING_SHUTDOWN_UART_BH) {
      prv_ehcill_state(EHCILL_ASLEEP);
  
      prv_unlock();
  
      EHCILL_DBG("bottom half shutting down UART, goodbye!");
      prv_uart_go_to_sleep();
    } else if (s_ehcill_sm == EHCILL_WAKE_UP_UART_BH) {
      prv_ehcill_state(EHCILL_WAIT_FOR_WAKE_IND);
      prv_unlock();
      
      EHCILL_DBG("bottom half waking up UART");      
      prv_uart_init();
    } else {
      prv_unlock();
    }
    mutex_unlock(s_ehcill_mutex);

    /* Dispose of any UART transmissions that the ISR has taken care of for
     * us.  We can't do that in the ISR, because the ISR might need to take
     * locks to hand things back to NimBLE!  Atomically grab the list of
     * things to dispose, and then dispose of them.
     */
    prv_lock();
    dispose_head = uart_tx_dispose_head;
    uart_tx_dispose_head = NULL;
    prv_unlock();

    prv_dispose_uart_tx(dispose_head);

    while (true) {
      prv_lock();

      bytes_remaining = circular_buffer_get_read_space_remaining(&s_rx_buffer);
      if (bytes_remaining == 0) {
        prv_unlock();
        break;
      }

      bytes_remaining = MIN(sizeof(read_buf), bytes_remaining);
      circular_buffer_copy(&s_rx_buffer, read_buf, bytes_remaining);
      prv_unlock();

      consumed_bytes = hci_h4_sm_rx(&hci_uart_h4sm, read_buf, bytes_remaining);
      if (consumed_bytes <= 0) {
        PBL_LOG_D(LOG_DOMAIN_BT_STACK, LOG_LEVEL_ERROR, "hci_h4_sm_rx rc=%d", consumed_bytes);
        break;
      }

      prv_lock();
      circular_buffer_consume(&s_rx_buffer, consumed_bytes);
      prv_unlock();
    }
  }
}

void ble_transport_ll_init(void) {
  s_ehcill_mutex = mutex_create();

  hci_h4_sm_init(&hci_uart_h4sm, &hci_h4_allocs_from_ll, hci_uart_frame_cb);
  if (ble_chipset_is_hcill()) {
    hci_h4_sm_set_packet_cb(&hci_uart_h4sm, hci_uart_packet_cb);
  }

  s_tx_queue = xQueueCreate(TX_Q_SIZE, sizeof(struct uart_tx *));
  PBL_ASSERTN(s_tx_queue);

  s_rx_data_ready = xSemaphoreCreateBinary();
  s_cmd_done = xSemaphoreCreateBinary();
  circular_buffer_init(&s_rx_buffer, s_rx_storage, sizeof(s_rx_storage));

  ble_chipset_init();

  exti_configure_pin(BOARD_CONFIG_BT_COMMON.wakeup.int_exti, ExtiTrigger_Rising, prv_rtscts_trigger);
  
  s_uart_baud = 115200;
  prv_uart_init();

  TaskParameters_t task_params = {
      .pvTaskCode = prv_rx_task_main,
      .pcName = "NimbleRX",
      .usStackDepth = 4000 / sizeof(StackType_t), // TODO: can probably be reduced
      .uxPriority = (tskIDLE_PRIORITY + 3) | portPRIVILEGE_BIT,
      .puxStackBuffer = NULL,
  };

  pebble_task_create(PebbleTask_BTHCI, &task_params, &s_rx_task_handle);
  PBL_ASSERTN(s_rx_task_handle);

  if (ble_chipset_start()) {
    chipset_start_done = true;
  }
}

void ble_update_baudrate(uint32_t baud) {
  s_uart_baud = baud;
  uart_set_baud_rate(BLUETOOTH_UART, baud);
}

static void ble_transport_tx_item(struct uart_tx *tx_item) {
  xQueueSendToBack(s_tx_queue, &tx_item, portMAX_DELAY);
  mutex_lock(s_ehcill_mutex);
  if (s_ehcill_sm == EHCILL_PENDING_XMIT_SLEEP_FLUSH || s_ehcill_sm == EHCILL_PENDING_SHUTDOWN_UART_BH) {
    EHCILL_DBG("enqueuing item to tx while asleep, but UART hasn't been shut down yet; just turning on flow control");
    uart_enable_flow_control(BLUETOOTH_UART);
    prv_ehcill_state(EHCILL_SEND_WAKE_UP_IND);
  } else if (s_ehcill_sm == EHCILL_ASLEEP || s_ehcill_sm == EHCILL_WAKE_UP_UART_BH) {
    EHCILL_DBG("enqueuing item to tx while asleep, triggering wakeup");
    prv_uart_init();
    prv_ehcill_state(EHCILL_SEND_WAKE_UP_IND);
  }
  mutex_unlock(s_ehcill_mutex);
  uart_set_tx_interrupt_enabled(BLUETOOTH_UART, true);
}

void ble_queue_cmd(void *buf, bool needs_free, bool wait) {
  struct uart_tx *tx_item = kernel_malloc(sizeof(struct uart_tx));
  PBL_ASSERTN(tx_item);
  tx_item->type = HCI_H4_CMD;
  tx_item->sent_type = 0;
  tx_item->len = 3 + ((uint8_t *)buf)[2];
  tx_item->buf = buf;
  tx_item->idx = 0;
  tx_item->om = NULL;
  tx_item->buf_needs_free = needs_free;

  ble_transport_tx_item(tx_item);

  if (wait) xSemaphoreTake(s_cmd_done, portMAX_DELAY);
}

/* APIs to be implemented by HS/LL side of transports */
int ble_transport_to_ll_cmd_impl(void *buf) {
  ble_queue_cmd(buf, true, false);
  return 0;
}

int ble_transport_to_ll_acl_impl(struct os_mbuf *om) {
  struct uart_tx *tx_item = kernel_malloc(sizeof(struct uart_tx));
  PBL_ASSERTN(tx_item);
  tx_item->type = HCI_H4_ACL;
  tx_item->sent_type = 0;
  tx_item->len = OS_MBUF_PKTLEN(om);
  tx_item->buf = NULL;
  tx_item->idx = 0;
  tx_item->om = om;

  ble_transport_tx_item(tx_item);

  return 0;
}

int ble_transport_to_ll_iso_impl(struct os_mbuf *om) {
  struct uart_tx *tx_item = kernel_malloc(sizeof(struct uart_tx));
  PBL_ASSERTN(tx_item);
  tx_item->type = HCI_H4_ISO;
  tx_item->sent_type = 0;
  tx_item->len = OS_MBUF_PKTLEN(om);
  tx_item->buf = NULL;
  tx_item->idx = 0;
  tx_item->om = om;

  ble_transport_tx_item(tx_item);

  return 0;
}
