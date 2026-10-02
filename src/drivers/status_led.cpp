#include "status_led.hpp"

void StatusLed::begin() {
    pinMode(pin_, OUTPUT);
    write(false);
    on_ = false;
}

void StatusLed::set(bool on) {
    if (on == on_) return;
    write(on);
    on_ = on;
}

void StatusLed::write(bool on) { digitalWrite(pin_, (on != activeLow_) ? HIGH : LOW); }

void StatusLed::flash(uint32_t count, uint32_t onMs, uint32_t offMs) {
    for (uint32_t i = 0; i < count; ++i) {
        if (i > 0) delay(offMs);
        set(true);
        delay(onMs);
        set(false);
    }
}
