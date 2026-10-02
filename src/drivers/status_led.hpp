#pragma once

#include <Arduino.h>

// On-board LED: the unit's only local indicator. It shows received light,
// mode selection and receive activity.
class StatusLed {
public:
    StatusLed(uint32_t pin, bool activeLow) : pin_(pin), activeLow_(activeLow) {}

    void begin();
    void set(bool on);
    bool isOn() const { return on_; }

    // Blocking flashes, for confirmations and diagnostics only.
    void flash(uint32_t count, uint32_t onMs, uint32_t offMs);

private:
    void write(bool on);

    uint32_t pin_;
    bool activeLow_;
    bool on_ = false;
};
