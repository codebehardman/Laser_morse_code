#pragma once

#include "device.hpp"

// Blocking hardware tests. Each one reports its result on the console and
// returns true on pass (tests that need a human to watch return true).
namespace diagnostics {

// Blink the laser so it can be checked by eye (or with a mirror pointed back
// at this unit's own phototransistor, which this test will detect).
bool testLaser(Device& device, uint32_t blinks = 5);

// Report every light/dark change at the phototransistor for `durationMs`.
// The status LED follows the receiver.
bool testSensor(Device& device, uint32_t durationMs = 10000);

// Print the live light-sensor reading every 250 ms for `durationMs`.
void showLevel(Device& device, uint32_t durationMs = 5000);

// Measure the room (other laser off), then the other unit's laser, and set
// the threshold halfway between. Returns false if the laser wasn't seen.
bool calibrateThreshold(Device& device);

// Report button presses for `durationMs`.
bool testButton(Device& device, uint32_t durationMs = 10000);

// Link test, run on two units at once:
//   receiving unit: countPulses()  (start this first)
//   sending unit:   sendPulses()
// The receiver reports how many of the pulses arrived and their widths.
void sendPulses(Device& device, uint32_t count = 100, uint32_t widthMs = 20);
bool countPulses(Device& device, uint32_t expected = 100, uint32_t timeoutMs = 20000);

}  // namespace diagnostics
