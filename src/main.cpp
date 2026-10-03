// Laser Morse code transceiver firmware (ESP32-WROOM-32 DevKit).
//
// Both units run this same firmware; each one transmits and receives.
// After power-up the first key press selects the mode:
//
//   Short press  -> Morse code mode (modes/morse_mode.cpp)
//                   The key turns the laser on, the status LED follows the
//                   received light, and received Morse is decoded to text.
//   Hold >= 2 s  -> Serial terminal mode (modes/serial_mode.cpp)
//                   Whatever is typed in one unit's serial terminal appears
//                   in the other's, sent at 9600 baud over the laser.
//
// The mode stays until reset. The serial console is the board's USB port.

#include <Arduino.h>

#include "board_config.hpp"
#include "console.hpp"
#include "device.hpp"
#include "modes/mode_select.hpp"
#include "modes/morse_mode.hpp"
#include "modes/serial_mode.hpp"
#include "morse_code.hpp"

namespace {

Laser laser(LMC_PIN_LASER);
PhotoSensor sensor(LMC_PIN_SENSOR, config::kSensorDebounceMs, config::kSensorThreshold);
LaserUart link(LMC_PIN_LASER, sensor);
Button button(LMC_PIN_BUTTON, config::kButtonDebounceMs);
StatusLed led(LMC_PIN_STATUS_LED, LMC_STATUS_LED_ACTIVE_LOW);

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
