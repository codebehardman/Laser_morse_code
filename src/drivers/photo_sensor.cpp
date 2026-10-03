#include "photo_sensor.hpp"

uint16_t PhotoSensor::readLevel() const { return static_cast<uint16_t>(analogRead(pin())); }
