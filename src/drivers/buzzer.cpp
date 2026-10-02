#include "buzzer.hpp"

void Buzzer::begin() {
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW);
    on_ = false;
}

void Buzzer::set(bool on) {
    if (on == on_) return;
    digitalWrite(pin_, on ? HIGH : LOW);
    on_ = on;
}

void Buzzer::beep(uint32_t durationMs) {
    set(true);
    delay(durationMs);
    set(false);
}
