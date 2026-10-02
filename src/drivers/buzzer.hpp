#pragma once

#include <Arduino.h>

// Active buzzer (drives its own tone, so it is simply switched on/off).
class Buzzer {
public:
    explicit Buzzer(uint32_t pin) : pin_(pin) {}

    void begin();
    void set(bool on);
    bool isOn() const { return on_; }

    // Blocking beep, for diagnostics and start-up feedback only.
    void beep(uint32_t durationMs);

private:
    uint32_t pin_;
    bool on_ = false;
};
