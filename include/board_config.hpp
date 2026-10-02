#pragma once

// Pin assignments and tunables for the NUCLEO-F401RE transceiver.
//
// Defaults match the schematic in the design document. Any value can be
// overridden from platformio.ini with a build flag, e.g. -D LMC_PIN_BUTTON=PA4.

#include <Arduino.h>

// Laser diode, switched by the NPN transistor (high = laser on).
#ifndef LMC_PIN_LASER
#define LMC_PIN_LASER PA1
#endif

// Phototransistor with 47k pull-up (low = light detected).
#ifndef LMC_PIN_SENSOR
#define LMC_PIN_SENSOR PA0
#endif

// Momentary key button to GND, uses the internal pull-up (low = pressed).
#ifndef LMC_PIN_BUTTON
#define LMC_PIN_BUTTON PA2
#endif

// On-board green LED (LD2): the only local indicator. Mirrors the receiver
// (handy when aiming) and confirms the selected mode.
#ifndef LMC_PIN_STATUS_LED
#define LMC_PIN_STATUS_LED LED_BUILTIN
#endif

// USB serial console through the ST-LINK virtual COM port.
// The ST-LINK VCP uses PA2 (TX) and PA3 (RX), so it cannot be enabled while
// the button is still wired to PA2.
#ifndef LMC_USB_SERIAL
#define LMC_USB_SERIAL 0
#endif

#if LMC_USB_SERIAL
static_assert(LMC_PIN_BUTTON != PA2 && LMC_PIN_BUTTON != PA3,
              "PA2/PA3 are the ST-LINK USB serial lines: move the button "
              "(see README) or build the 'standalone' environment");
#endif

// Hardware timer for the serial-terminal-mode laser link interrupt (not used
// by the Arduino core).
#ifndef LMC_LINK_TIMER
#define LMC_LINK_TIMER TIM2
#endif

namespace config {

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
