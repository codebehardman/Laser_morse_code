#pragma once

#include <Arduino.h>

// Laser diode switched by an NPN transistor (active high).
class Laser {
public:
    explicit Laser(uint32_t pin) : pin_(pin) {}

    void begin();
    void set(bool on);
    bool isOn() const { return on_; }

private:
    uint32_t pin_;
    bool on_ = false;
};
