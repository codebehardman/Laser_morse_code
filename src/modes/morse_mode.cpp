#include "morse_mode.hpp"

#include "../commands.hpp"
#include "../console.hpp"
#include "board_config.hpp"

namespace morse_mode {
namespace {

// Console output state for received text.
bool rxLineOpen = false;
uint32_t lastRxMs = 0;

void printReceived(Device& device, uint32_t now) {
    char c;
    while (device.decoder.read(c)) {
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

void begin(Device&) {
    console::println();
    commands::printHelp();
    console::println();
}

void update(Device& device) {
    const uint32_t now = millis();
    device.updateInputs();

    char line[128];
    if (console::readLine(line, sizeof(line))) {
        commands::handleLine(device, line);
    }

    // Transmit: the key, queued text and aim mode all drive the laser.
    const bool textKey = device.transmitter.update(now);
    const bool keyDown = device.button.pressed() || textKey;
    device.laser.set(keyDown || device.settings.aim);

    // Receive.
    const bool light = device.sensor.lightDetected();
    device.decoder.update(light, now);
    device.led.set(light);

    printReceived(device, now);
}

}  // namespace morse_mode
