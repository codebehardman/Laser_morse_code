#pragma once

#include <Arduino.h>

// Digital input that only changes state after the raw level has been stable
// for the debounce time. Active-low inputs report true when pulled low.
class DebouncedInput {
public:
    DebouncedInput(uint32_t pin, bool activeLow, uint32_t debounceMs, bool usePullup)
        : pin_(pin), activeLow_(activeLow), debounceMs_(debounceMs), usePullup_(usePullup) {}

    void begin();

    // Sample the pin. Returns true if the debounced state changed.
    bool update(uint32_t nowMs);

    bool active() const { return state_; }
    bool readRaw() const;

    // Number of inactive -> active transitions since the last reset.
    uint32_t activations() const { return activations_; }
    void resetActivations() { activations_ = 0; }

private:
    uint32_t pin_;
    bool activeLow_;
    uint32_t debounceMs_;
    bool usePullup_;

    bool state_ = false;
    bool lastRaw_ = false;
    uint32_t lastChangeMs_ = 0;
    uint32_t activations_ = 0;
};
