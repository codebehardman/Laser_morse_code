#pragma once

// Pin assignments and tunables for the STM32F103C8T6 transceiver.
//
// Defaults match the schematic in the design document. Any value can be
// overridden from platformio.ini with a build flag, e.g. -D LMC_PIN_BUTTON=PA4.
// Avoid PA11/PA12 (USB) and PA13/PA14 (SWD programming).

#include <Arduino.h>

// Laser diode, switched by the NPN transistor (high = laser on).
#ifndef LMC_PIN_LASER
#define LMC_PIN_LASER PA1
#endif

// Phototransistor with pull-up resistor (low = light detected).
// PA0 is NOT 5 V tolerant on the F103: tie the pull-up to 3.3 V, not 5 V.
#ifndef LMC_PIN_SENSOR
#define LMC_PIN_SENSOR PA0
#endif

// Momentary key button to GND, uses the internal pull-up (low = pressed).
#ifndef LMC_PIN_BUTTON
#define LMC_PIN_BUTTON PA2
#endif

// On-board LED (PC13 on Blue Pill boards): the only local indicator. Mirrors
// the receiver (handy when aiming) and confirms the selected mode. On most
// F103C8T6 boards it is wired to 3.3 V, so it lights when the pin is LOW.
#ifndef LMC_PIN_STATUS_LED
#define LMC_PIN_STATUS_LED PC13
#endif
#ifndef LMC_STATUS_LED_ACTIVE_LOW
#define LMC_STATUS_LED_ACTIVE_LOW 1
#endif

// Hardware timer for the serial-terminal-mode laser link interrupt (not used
// by the Arduino core).
#ifndef LMC_LINK_TIMER
#define LMC_LINK_TIMER TIM2
#endif

namespace config {

// Ignored by USB CDC (it always runs at USB speed); kept for terminals
// that insist on a baud rate.
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
