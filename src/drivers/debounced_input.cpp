#include "debounced_input.hpp"

void DebouncedInput::begin() {
    pinMode(pin_, usePullup_ ? INPUT_PULLUP : INPUT);
    lastRaw_ = readRaw();
    state_ = lastRaw_;
    lastChangeMs_ = millis();
}

bool DebouncedInput::readRaw() const {
    const bool high = digitalRead(pin_) == HIGH;
    return activeLow_ ? !high : high;
}

bool DebouncedInput::update(uint32_t nowMs) {
    const bool raw = readRaw();
    if (raw != lastRaw_) {
        lastRaw_ = raw;
        lastChangeMs_ = nowMs;
        return false;
    }
    if (raw != state_ && nowMs - lastChangeMs_ >= debounceMs_) {
        state_ = raw;
        if (state_) ++activations_;
        return true;
    }
    return false;
}
