#pragma once

#include "display.h"

#include "drivers/button_id.h"
#include "debug/power_tracking.h"

#define NRF52840_COMPATIBLE
#include <mcu.h>

#include <stdint.h>
#include <stdbool.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable" 
#include <hal/nrf_gpio.h>
#include <nrfx_spim.h>
#include <nrfx_gpiote.h>
#include <nrfx_timer.h>
#include <nrfx_pwm.h>
#pragma GCC diagnostic pop

#define GPIO_Port_NULL (NULL)
#define GPIO_Pin_NULL ((uint16_t)-1)
//! Guaranteed invalid IRQ priority
#define IRQ_PRIORITY_INVALID (1 << __NVIC_PRIO_BITS)

// This is generated in order to faciliate the check within the IRQ_MAP macro below
enum {
#define IRQ_DEF(num, irq) IS_VALID_IRQ__##irq,
#if defined(MICRO_FAMILY_NRF52840)
#  include "irq_nrf52840.def"
#else
#  error need IRQ table for new micro family
#endif
#undef IRQ_DEF
};

//! Creates a trampoline to the interrupt handler defined within the driver
#define IRQ_MAP(irq, handler, device) \
  void irq##_IRQHandler(void) { \
    handler(device); \
  } \
  _Static_assert(IS_VALID_IRQ__##irq || true, "(See comment below)")

#define IRQ_MAP_NRFX(irq, handler) \
  void irq##_IRQHandler(void) { \
    handler(); \
  } \
  _Static_assert(IS_VALID_IRQ__##irq || true, "(See comment below)")

/*
 * The above static assert checks that the requested IRQ is valid by checking that the enum
 * value (generated above) is declared. The static assert itself will not trip, but you will get
 * a compilation error from that line if the IRQ does not exist within irq_stm32*.def.
 */

// There are a lot of DMA streams and they are very straight-forward to define. Let's use some
// macro magic to make it a bit less tedious and error-prone.
#define CREATE_DMA_STREAM(cnum, snum) \
  static DMAStreamState s_dma##cnum##_stream##snum##_state; \
  static DMAStream DMA##cnum##_STREAM##snum##_DEVICE = { \
    .state = &s_dma##cnum##_stream##snum##_state, \
    .controller = &DMA##cnum##_DEVICE, \
    .periph = DMA##cnum##_Stream##snum, \
    .irq_channel = DMA##cnum##_Stream##snum##_IRQn, \
  }; \
  IRQ_MAP(DMA##cnum##_Stream##snum, dma_stream_irq_handler, &DMA##cnum##_STREAM##snum##_DEVICE)

typedef struct {
  nrfx_gpiote_t peripheral;
  uint8_t channel;
  uint32_t gpio_pin; ///< The result of NRF_GPIO_PIN_MAP(port, pin).
} GpioteConfig;

typedef GpioteConfig ExtiConfig; /* compatibility */

typedef enum {
  AccelThresholdLow, ///< A sensitive state used for stationary mode
  AccelThresholdHigh, ///< The accelerometer's default sensitivity
  AccelThreshold_Num,
} AccelThreshold;

typedef struct {
  const char* const name; ///< Name for debugging purposes.
  GpioteConfig gpiote;
  nrf_gpio_pin_pull_t pull;
} ButtonConfig;

typedef struct {
  const uint32_t gpio_pin; ///< The result of NRF_GPIO_PIN_MAP(port, pin).
} ButtonComConfig;

#define NRF5_GPIO_RESOURCE_EXISTS ((void *)1)
typedef struct {
  void *gpio; ///< For compatibility, GPIO_RESOURCE_EXISTS if this is in use, NULL if not.
  const uint32_t gpio_pin; ///< The result of NRF_GPIO_PIN_MAP(port, pin).
} InputConfig;

#if 0
typedef struct {
  ADC_TypeDef *const adc; ///< One of ADCX. For example ADC1.
  const uint8_t adc_channel; ///< One of ADC_Channel_*
  uint32_t clock_ctrl;  ///< Peripheral clock control flag
  GPIO_TypeDef* const gpio; ///< One of GPIOX. For example, GPIOA.
  const uint16_t gpio_pin; ///< One of GPIO_Pin_*
} ADCInputConfig;
#endif

typedef struct {
  NRF_TIMER_Type *peripheral;
} TimerConfig;

typedef struct {
  const TimerConfig timer;
  const uint8_t irq_channel;
} TimerIrqConfig;

typedef struct {
  void *gpio; ///< For compatibility, GPIO_RESOURCE_EXISTS if this is in use, NULL if not.
  const uint32_t gpio_pin; ///< The result of NRF_GPIO_PIN_MAP(port, pin).
  bool active_high; ///< Pin is active high or active low
} OutputConfig;

//! Alternate function pin configuration
//! Used to configure a pin for use by a peripheral
typedef struct {
  void *gpio; ///< For compatibility, GPIO_RESOURCE_EXISTS if this is in use, NULL if not.
  const uint32_t gpio_pin; ///< The result of NRF_GPIO_PIN_MAP(port, pin).
} AfConfig;

typedef struct {
  uint16_t value;
  uint16_t resolution;
  int enabled;
  nrf_pwm_sequence_t seq;
} PwmState;

typedef struct {
  OutputConfig output;
  nrfx_pwm_t peripheral;
  PwmState *state;
} PwmConfig;

typedef struct {
  int axes_offsets[3];
  bool axes_inverts[3];
  uint32_t shake_thresholds[AccelThreshold_Num];
  uint32_t double_tap_threshold;
} AccelConfig;

typedef struct {
  int axes_offsets[3];
  bool axes_inverts[3];
} MagConfig;

typedef struct {
  AfConfig i2s_ck;
  AfConfig i2s_sd;
  NRF_SPIM_Type *spi;
  uint32_t spi_clock_ctrl;
  uint16_t gain;

  //! Pin we use to control power to the microphone. Only used on certain boards.
  OutputConfig mic_gpio_power;
} MicConfig;

typedef enum {
  OptionNotPresent = 0, // FIXME
  OptionActiveLowOpenDrain,
  OptionActiveHigh
} PowerCtl5VOptions;

typedef enum {
  ActuatorOptions_Ctl = 1 << 0, ///< GPIO is used to enable / disable vibe
  ActuatorOptions_Pwm = 1 << 1, ///< PWM control
  ActuatorOptions_IssiI2C = 1 << 2, ///< I2C Device, currently used for V1_5 -> OG steel backlight
  ActuatorOptions_HBridge = 1 << 3, //< PWM actuates an H-Bridge, requires ActuatorOptions_PWM
} ActuatorOptions;

typedef struct {
  // Audio Configuration
  /////////////////////////////////////////////////////////////////////////////
  const bool has_mic;
  const MicConfig mic_config;

  // Ambient Light Configuration
  /////////////////////////////////////////////////////////////////////////////
  const uint32_t ambient_light_dark_threshold;
  const uint32_t ambient_k_delta_threshold;
  const OutputConfig photo_en;
  const bool als_always_on;

  // Debug Serial Configuration
  /////////////////////////////////////////////////////////////////////////////
  const GpioteConfig dbgserial_int;
  const InputConfig dbgserial_int_gpio;

  // MFi Configuration
  /////////////////////////////////////////////////////////////////////////////
  const OutputConfig mfi_reset_pin;

  // Display Configuration
  /////////////////////////////////////////////////////////////////////////////
  const OutputConfig lcd_com; //!< This needs to be pulsed regularly to keep the sharp display fresh.

  //! Controls power to the sharp display
  const PowerCtl5VOptions power_5v0_options;
  const OutputConfig power_ctl_5v0;

  const uint8_t backlight_on_percent; // percent of max possible brightness
  const uint8_t backlight_max_duty_cycle_percent; // Calibrated such that the preceived brightness
                    // of "backlight_on_percent = 100" (and all other values, to a reasonable
                    // tolerance) is identical across all platforms. >100% isn't possible, so
                    // future backlights must be at least as bright as Tintin's.

  // FPC Pinstrap Configuration
  /////////////////////////////////////////////////////////////////////////////
  const InputConfig fpc_pinstrap_1;
  const InputConfig fpc_pinstrap_2;

  // GPIO Configuration
  /////////////////////////////////////////////////////////////////////////////
  const uint16_t num_avail_gpios;
} BoardConfig;

// Button Configuration
/////////////////////////////////////////////////////////////////////////////
typedef struct {
  const ButtonConfig buttons[NUM_BUTTONS];
  const ButtonComConfig button_com;
  const bool active_high;
  nrfx_timer_t timer;
} BoardConfigButton;

typedef struct {
  const uint32_t numerator;
  const uint32_t denominator;
} VMonScale;

// Power Configuration
/////////////////////////////////////////////////////////////////////////////
typedef struct {
  const GpioteConfig pmic_int;
  const InputConfig pmic_int_gpio;

  //! Voltage rail control lines
  const OutputConfig rail_4V5_ctrl;
  const OutputConfig rail_6V6_ctrl;
  const nrf_gpio_pin_drive_t rail_6V6_ctrl_otype;

  //! Scaling factor for battery vmon
  const VMonScale battery_vmon_scale;
  //! Tells us if the USB cable plugged in.
  const InputConfig vusb_stat;
  const GpioteConfig vusb_gpiote;
  //! Tells us whether the charger thinks we're charging or not.
  const InputConfig chg_stat;
  //! Tell the charger to use 2x current to charge faster (MFG only).
  const OutputConfig chg_fast;
  //! Enable the charger. We may want to disable this in MFG, normally it's always on.
  const OutputConfig chg_en;

  //! Interrupt that fires when the USB cable is plugged in
  const bool has_vusb_interrupt;

  const bool wake_on_usb_power;

  const int charging_cutoff_voltage;
  const int charging_status_led_voltage_compensation;

  //! Percentage for watch only mode
  const uint8_t low_power_threshold;

  //! Approximate hours of battery life
  const uint8_t battery_capacity_hours;
} BoardConfigPower;

typedef struct {
  const AccelConfig accel_config;
  const InputConfig accel_int_gpios[2];
  const GpioteConfig accel_ints[2];
} BoardConfigAccel;

typedef struct {
  const MagConfig mag_config;
  const InputConfig mag_int_gpio;
  const GpioteConfig mag_int;
} BoardConfigMag;

typedef struct {
  const ActuatorOptions options;
  const OutputConfig ctl;
  const PwmConfig pwm;
  const uint16_t vsys_scale; //< Voltage to scale duty cycle to in mV. 0 if no scaling should occur.
                             //< For example, Silk VBat may droop to 3.3V, so we scale down vibe
                             //< duty cycle so that 100% duty cycle will always be 3.3V RMS.
} BoardConfigActuator;

typedef struct {
  const OutputConfig power_en; //< Enable power supply to the accessory connector.
  const InputConfig int_gpio;
  const GpioteConfig gpiote;
} BoardConfigAccessory;

typedef struct {
  const bool output_enabled;
  const AfConfig af_cfg;
  const InputConfig an_cfg;
} BoardConfigMCO1;

typedef enum {
  SpiPeriphClockNrf5
} SpiPeriphClock;

typedef struct {
  nrfx_spim_t spi;

  const OutputConfig mosi;
  const OutputConfig clk;
  const OutputConfig cs;

  const OutputConfig on_ctrl;
  const nrf_gpio_pin_drive_t on_ctrl_otype;
} BoardConfigSharpDisplay;

typedef const struct DMARequest DMARequest;
typedef const struct UARTDevice UARTDevice;
typedef const struct SPIBus SPIBus;
typedef const struct SPISlavePort SPISlavePort;
typedef const struct I2CBus I2CBus;
typedef const struct I2CSlavePort I2CSlavePort;
typedef const struct HRMDevice HRMDevice;
typedef const struct VoltageMonitorDevice VoltageMonitorDevice;
typedef const struct TemperatureSensor TemperatureSensor;
typedef const struct MicDevice MicDevice;
typedef const struct QSPIPort QSPIPort;
typedef const struct QSPIFlash QSPIFlash;
typedef const struct ICE40LPDevice ICE40LPDevice;
typedef const struct TouchSensor TouchSensor;

void board_early_init(void);
void board_init(void);

#include "board_definitions.h"
