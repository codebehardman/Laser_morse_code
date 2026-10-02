#pragma once

#include <Arduino.h>

// On-board LED (LD2): the unit's only local indicator. It shows received
// light, mode selection and self-test progress.
class StatusLed {
public:
    explicit StatusLed(uint32_t pin) : pin_(pin) {}

    void begin();
    void set(bool on);
    bool isOn() const { return on_; }

    // Blocking flashes, for confirmations and diagnostics only.
    void flash(uint32_t count, uint32_t onMs, uint32_t offMs);

private:
    uint32_t pin_;
    bool on_ = false;
};
