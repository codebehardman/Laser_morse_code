// Laser Morse code transceiver - NUCLEO-F401RE firmware.
//
// Both units run this same firmware; each one transmits and receives.
//
//   Transmit: pressing the key turns the laser on (straight Morse key).
//             Text typed in the serial console is sent as Morse code.
//   Receive:  the buzzer and LD2 follow the light at the phototransistor,
//             and the pulses are decoded to text on the serial console.
//
// Holding the key during reset runs the hardware self-test.

#include <Arduino.h>

#include "board_config.hpp"
#include "commands.hpp"
#include "console.hpp"
#include "device.hpp"
#include "diagnostics.hpp"
#include "morse_code.hpp"

namespace {

Laser laser(LMC_PIN_LASER);
PhotoSensor sensor(LMC_PIN_SENSOR, config::kSensorDebounceMs);
Button button(LMC_PIN_BUTTON, config::kButtonDebounceMs);
Buzzer buzzer(LMC_PIN_BUZZER);

morse::MorseTransmitter transmitter(morse::wpmToUnitMs(config::kDefaultWpm));
morse::MorseDecoder decoder(morse::wpmToUnitMs(config::kDefaultWpm));

Settings settings{config::kDefaultWpm, /*sidetone=*/false, /*aim=*/false};
Device device{laser, sensor, button, buzzer, transmitter, decoder, settings};

// Console output state for received text.
bool rxLineOpen = false;
uint32_t lastRxMs = 0;

void printReceived(uint32_t now) {
    char c;
    while (decoder.read(c)) {
        if (!rxLineOpen) {
            if (c == ' ') continue;  // don't start a line with a word gap
            console::print("RX< ");
            rxLineOpen = true;
        }
        console::print(c);
        lastRxMs = now;
    }
    if (rxLineOpen && now - lastRxMs >= config::kMessageEndMs) {
        console::println();
        rxLineOpen = false;
    }
}

}  // namespace

void setup() {
    laser.begin();
    buzzer.begin();
    sensor.begin();
    button.begin();
    pinMode(LMC_PIN_STATUS_LED, OUTPUT);
    console::begin();

    // Give the button's pull-up a moment, then check for the self-test request.
    delay(20);
    if (button.readRaw()) {
        if (console::enabled()) {
            diagnostics::testBuzzer(device);
            diagnostics::testLaser(device);
            diagnostics::testSensor(device);
        } else {
            diagnostics::standaloneSelfTest(device);
        }
    } else {
        buzzer.beep(50);  // power-on chirp
    }

    console::println();
    commands::printHelp();
    console::println();
}

void loop() {
    const uint32_t now = millis();
    device.updateInputs();

    char line[128];
    if (console::readLine(line, sizeof(line))) {
        commands::handleLine(device, line);
    }

    // Transmit: the key, queued text and aim mode all drive the laser.
    const bool textKey = transmitter.update(now);
    const bool keyDown = button.pressed() || textKey;
    laser.set(keyDown || settings.aim);

    // Receive.
    const bool light = sensor.lightDetected();
    decoder.update(light, now);
    buzzer.set(light || (settings.sidetone && keyDown));
    digitalWrite(LMC_PIN_STATUS_LED, light ? HIGH : LOW);

    printReceived(now);
}
