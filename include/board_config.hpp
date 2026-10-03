#pragma once

// Pin assignments and tunables for the transceiver.
//
// Supported boards (pick one with the PlatformIO environment):
//   - ESP32-WROOM-32 DevKit (env:esp32)
//   - STM32F103C8T6 "Blue Pill" (env:bluepill)
//
// Any value can be overridden from platformio.ini with a build flag,
// e.g. -D LMC_PIN_BUTTON=33. All pins are 3.3 V only: tie the
// phototransistor pull-up to 3.3 V, never 5 V.

#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP32)

// Avoids the boot-strapping pins (GPIO 0, 2, 12, 15) for external parts and
// the input-only pins (GPIO 34-39).
#ifndef LMC_PIN_LASER
#define LMC_PIN_LASER 25  // laser via NPN transistor, high = on
#endif
#ifndef LMC_PIN_SENSOR
#define LMC_PIN_SENSOR 26  // phototransistor + pull-up, low = light
#endif
#ifndef LMC_PIN_BUTTON
#define LMC_PIN_BUTTON 27  // key to GND, internal pull-up
#endif
#ifndef LMC_PIN_STATUS_LED
#define LMC_PIN_STATUS_LED 2  // on-board blue LED
#endif
#ifndef LMC_STATUS_LED_ACTIVE_LOW
#define LMC_STATUS_LED_ACTIVE_LOW 0
#endif
// Hardware timer (0-3) for the serial-terminal-mode laser link interrupt.
#ifndef LMC_LINK_TIMER
#define LMC_LINK_TIMER 0
#endif

#elif defined(ARDUINO_ARCH_STM32)

// Avoid PA11/PA12 (USB) and PA13/PA14 (SWD programming).
#ifndef LMC_PIN_LASER
#define LMC_PIN_LASER PA1  // laser via NPN transistor, high = on
#endif
#ifndef LMC_PIN_SENSOR
#define LMC_PIN_SENSOR PA0  // phototransistor + pull-up, low = light
#endif
#ifndef LMC_PIN_BUTTON
#define LMC_PIN_BUTTON PA2  // key to GND, internal pull-up
#endif
#ifndef LMC_PIN_STATUS_LED
#define LMC_PIN_STATUS_LED PC13  // on-board LED, wired to 3.3 V
#endif
#ifndef LMC_STATUS_LED_ACTIVE_LOW
#define LMC_STATUS_LED_ACTIVE_LOW 1
#endif
// Hardware timer for the serial-terminal-mode laser link interrupt (not used
// by the Arduino core).
#ifndef LMC_LINK_TIMER
#define LMC_LINK_TIMER TIM2
#endif

#else
#error "Unsupported board: add its pins to board_config.hpp"
#endif

namespace config {

// Serial console speed (set the same in your terminal). The STM32's native
// USB ignores it.
constexpr uint32_t kConsoleBaud = 115200;

// Default transmit speed for text typed in the serial console.
constexpr uint32_t kDefaultWpm = 12;
constexpr uint32_t kMinWpm = 5;
constexpr uint32_t kMaxWpm = 30;

// Debounce times.
constexpr uint32_t kButtonDebounceMs = 15;
constexpr uint32_t kSensorDebounceMs = 3;

// Print a newline in the console after this much receive silence.
constexpr uint32_t kMessageEndMs = 3000;

// Mode selection: the first key press after power-up picks the mode.
// Shorter than this = Morse mode, held at least this long = serial terminal.
constexpr uint32_t kModeSelectHoldMs = 2000;

// Serial terminal mode: laser link speed (8E1 framing, as in the design doc).
constexpr uint32_t kLinkBaud = 9600;

// Serial terminal mode: show what you type in your own terminal too, since
// most terminals don't echo locally.
constexpr bool kSerialLocalEcho = true;

}  // namespace config
