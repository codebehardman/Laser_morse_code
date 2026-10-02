#include "mode_select.hpp"

#include "../console.hpp"
#include "board_config.hpp"

namespace {

// While waiting: a short blip every half second.
constexpr uint32_t kWaitBlinkPeriodMs = 500;
constexpr uint32_t kWaitBlinkOnMs = 50;

// Confirmation: long flashes, clearly different from the waiting blips.
constexpr uint32_t kConfirmOnMs = 400;
constexpr uint32_t kConfirmOffMs = 300;

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
        device.led.set(now % kWaitBlinkPeriodMs < kWaitBlinkOnMs);

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

    device.led.set(false);
    delay(kConfirmOffMs);
    device.led.flash(mode == Mode::Morse ? 1 : 2, kConfirmOnMs, kConfirmOffMs);

    // Don't let the selecting press leak into the chosen mode.
    while (device.button.pressed()) device.button.update(millis());

    console::printf("Mode: %s\n", modeName(mode));
    return mode;
}
