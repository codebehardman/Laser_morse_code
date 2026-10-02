#include "mode_select.hpp"

#include "../console.hpp"
#include "board_config.hpp"

namespace {

constexpr uint32_t kBlinkPeriodMs = 500;
constexpr uint32_t kBeepMs = 150;

void confirmBeeps(Device& device, int count) {
    for (int i = 0; i < count; ++i) {
        if (i > 0) delay(kBeepMs);
        device.buzzer.beep(kBeepMs);
    }
}

}  // namespace

const char* modeName(Mode mode) {
    return mode == Mode::Morse ? "Morse code" : "serial terminal";
}

Mode selectMode(Device& device) {
    console::printf("Select mode with the key: short press = Morse code, hold %lu s = serial terminal\n",
                    static_cast<unsigned long>(config::kModeSelectHoldMs / 1000));

    bool pressing = false;
    uint32_t pressStartMs = 0;
    Mode mode = Mode::Morse;

    while (true) {
        const uint32_t now = millis();
        device.button.update(now);
        digitalWrite(LMC_PIN_STATUS_LED, (now / (kBlinkPeriodMs / 2)) % 2 ? HIGH : LOW);

        if (device.button.pressed()) {
            if (!pressing) {
                pressing = true;
                pressStartMs = now;
            } else if (now - pressStartMs >= config::kModeSelectHoldMs) {
                mode = Mode::SerialTerminal;
                break;
            }
        } else if (pressing) {
            mode = Mode::Morse;  // released before the hold time
            break;
        }
    }

    digitalWrite(LMC_PIN_STATUS_LED, LOW);
    confirmBeeps(device, mode == Mode::Morse ? 1 : 2);

    // Don't let the selecting press leak into the chosen mode.
    while (device.button.pressed()) device.button.update(millis());

    console::printf("Mode: %s\n", modeName(mode));
    return mode;
}
