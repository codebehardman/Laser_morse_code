// Laser Morse code transceiver - NUCLEO-F401RE firmware.
//
// Both units run this same firmware; each one transmits and receives.
// After power-up the first key press selects the mode:
//
//   Short press  -> Morse code mode (modes/morse_mode.cpp)
//                   The key turns the laser on, LD2 follows the received
//                   light, and received Morse is decoded to text.
//   Hold >= 2 s  -> Serial terminal mode (modes/serial_mode.cpp)
//                   Whatever is typed in one unit's serial terminal appears
//                   in the other's, sent at 9600 baud over the laser.
//
// The mode stays until reset. The standalone build has no USB serial, so a
// long press runs the hardware self-test instead and then enters Morse mode.

#include <Arduino.h>

#include "board_config.hpp"
#include "console.hpp"
#include "device.hpp"
#include "diagnostics.hpp"
#include "modes/mode_select.hpp"
#include "modes/morse_mode.hpp"
#include "modes/serial_mode.hpp"
#include "morse_code.hpp"

namespace {

Laser laser(LMC_PIN_LASER);
LaserUart link(LMC_PIN_LASER, LMC_PIN_SENSOR, LMC_LINK_TIMER);
PhotoSensor sensor(LMC_PIN_SENSOR, config::kSensorDebounceMs);
Button button(LMC_PIN_BUTTON, config::kButtonDebounceMs);
StatusLed led(LMC_PIN_STATUS_LED);

morse::MorseTransmitter transmitter(morse::wpmToUnitMs(config::kDefaultWpm));
morse::MorseDecoder decoder(morse::wpmToUnitMs(config::kDefaultWpm));

Settings settings{config::kDefaultWpm, /*aim=*/false};
Device device{laser, link, sensor, button, led, transmitter, decoder, settings};

Mode mode = Mode::Morse;

}  // namespace

void setup() {
    laser.begin();
    led.begin();
    sensor.begin();
    button.begin();
    console::begin();

    console::println();
    console::println("Laser Morse transceiver");
    mode = selectMode(device);

    if (mode == Mode::SerialTerminal && !console::enabled()) {
        diagnostics::standaloneSelfTest(device);
        mode = Mode::Morse;
    }

    if (mode == Mode::SerialTerminal) {
        serial_mode::begin(device);
    } else {
        morse_mode::begin(device);
    }
}

void loop() {
    if (mode == Mode::SerialTerminal) {
        serial_mode::update(device);
    } else {
        morse_mode::update(device);
    }
}
