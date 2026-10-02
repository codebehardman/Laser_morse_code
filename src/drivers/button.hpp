#pragma once

#include "debounced_input.hpp"

// Morse key: momentary push button to GND with the internal pull-up.
class Button : public DebouncedInput {
public:
    Button(uint32_t pin, uint32_t debounceMs)
        : DebouncedInput(pin, /*activeLow=*/true, debounceMs, /*usePullup=*/true) {}

    bool pressed() const { return active(); }
};
