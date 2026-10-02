#include "status_led.hpp"

void StatusLed::begin() {
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW);
    on_ = false;
}

void StatusLed::set(bool on) {
    if (on == on_) return;
    digitalWrite(pin_, on ? HIGH : LOW);
    on_ = on;
}

void StatusLed::flash(uint32_t count, uint32_t onMs, uint32_t offMs) {
    for (uint32_t i = 0; i < count; ++i) {
        if (i > 0) delay(offMs);
        set(true);
        delay(onMs);
        set(false);
    }
}
