#pragma once

#include "debounced_input.hpp"

// Phototransistor receiver with an external pull-up: the transistor pulls
// the pin low when the laser hits it.
class PhotoSensor : public DebouncedInput {
public:
    PhotoSensor(uint32_t pin, uint32_t debounceMs)
        : DebouncedInput(pin, /*activeLow=*/true, debounceMs, /*usePullup=*/false) {}

    bool lightDetected() const { return active(); }
};
