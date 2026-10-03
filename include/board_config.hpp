#pragma once

// Pin assignments and tunables for the ESP32-WROOM-32 DevKit transceiver.
//
// Any pin can be overridden from platformio.ini with a build flag,
// e.g. -D LMC_PIN_BUTTON=33. All pins are 3.3 V only: tie the
// phototransistor pull-up to 3.3 V, never 5 V.

#include <Arduino.h>

// Avoids the boot-strapping pins (GPIO 0, 2, 12, 15) for external parts.
#ifndef LMC_PIN_LASER
#define LMC_PIN_LASER 25  // laser via NPN transistor, high = on
#endif
#ifndef LMC_PIN_SENSOR
#define LMC_PIN_SENSOR 26  // phototransistor + pull-up, read with the ADC
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

namespace config {

// ---------------------------------------------------------------------------
// Light-sensor threshold
//
// The phototransistor is read with the ADC: 0 = very bright ... 4095 = dark.
// A reading BELOW the threshold counts as "laser detected". Ambient light
// pulls the reading down too, so in a bright room the threshold must be
// lower (closer to what only the laser can reach).
//
// Use /level to see the live reading, and /threshold to change it without
// reflashing (/threshold auto measures the room for you). Runtime changes
// are lost on reset; put the value you settle on below.

enum class Lighting { Dark, Bright };

// >>> Change this line to match the room: Lighting::Dark or Lighting::Bright
constexpr Lighting kLighting = Lighting::Dark;

constexpr uint16_t kThresholdDarkRoom = 2000;   // dark room: ambient reads ~4000
constexpr uint16_t kThresholdBrightRoom = 700;  // bright room: ambient reads lower

constexpr uint16_t kSensorThreshold =
    kLighting == Lighting::Dark ? kThresholdDarkRoom : kThresholdBrightRoom;
// ---------------------------------------------------------------------------

// Serial console speed (set the same in your terminal).
constexpr uint32_t kConsoleBaud = 115200;

// Default transmit speed for text typed in the serial console.
constexpr uint32_t kDefaultWpm = 20;
constexpr uint32_t kMinWpm = 5;
constexpr uint32_t kMaxWpm = 40;

// Debounce times.
constexpr uint32_t kButtonDebounceMs = 15;
constexpr uint32_t kSensorDebounceMs = 3;

// Print a newline in the console after this much receive silence.
constexpr uint32_t kMessageEndMs = 3000;

// Mode selection: the first key press after power-up picks the mode.
// Shorter than this = Morse mode, held at least this long = serial terminal.
constexpr uint32_t kModeSelectHoldMs = 1000;

// Serial terminal mode: laser link speed (8E1 framing). The design doc asks
// for 9600, but with a 100k phototransistor pull-up the pin rises too slowly
// for 104 us bits, so this uses 1200 baud (833 us bits). Raise it together
// with a smaller pull-up (faster but less sensitive) on both units.
constexpr uint32_t kLinkBaud = 1200;

// Serial terminal mode: show what you type in your own terminal too, since
// most terminals don't echo locally.
constexpr bool kSerialLocalEcho = true;

}  // namespace config
