#include "laser.hpp"

void Laser::begin() {
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW);
    on_ = false;
}

void Laser::set(bool on) {
    if (on == on_) return;
    digitalWrite(pin_, on ? HIGH : LOW);
    on_ = on;
}
