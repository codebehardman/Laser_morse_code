#pragma once

#include "debounced_input.hpp"

// Phototransistor with an external pull-up, read with the ADC.
//
// The reading (level) runs from 0 (very bright: the phototransistor pulls the
// pin to GND) to 4095 (dark: the pull-up holds it at 3.3 V). A level below
// the threshold counts as "laser detected". Ambient light also pulls the
// level down, so bright rooms need a lower threshold than dark ones (see
// config::kLighting in board_config.hpp, and the /threshold command).
class PhotoSensor : public DebouncedInput {
public:
    static constexpr uint16_t kMaxLevel = 4095;  // 12-bit ADC

    PhotoSensor(uint32_t pin, uint32_t debounceMs, uint16_t threshold)
        : DebouncedInput(pin, /*activeLow=*/false, debounceMs, /*usePullup=*/false),
          threshold_(threshold) {}

    bool lightDetected() const { return active(); }

    // Raw ADC reading, 0 (bright) .. 4095 (dark).
    uint16_t readLevel() const;
    bool isLight(uint16_t level) const { return level < threshold_; }
    bool readRaw() const override { return isLight(readLevel()); }

    uint16_t threshold() const { return threshold_; }
    void setThreshold(uint16_t threshold) { threshold_ = threshold > kMaxLevel ? kMaxLevel : threshold; }

private:
    volatile uint16_t threshold_;  // also read by the laser link task
};
