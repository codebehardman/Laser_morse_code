#pragma once

#include "device.hpp"

// Blocking hardware tests. Each one reports its result on the console and
// returns true on pass (tests that need a human to watch/listen return true).
namespace diagnostics {

// Blink the laser so it can be checked by eye (or with a mirror pointed back
// at this unit's own phototransistor, which this test will detect).
bool testLaser(Device& device, uint32_t blinks = 5);

// Report every light/dark change at the phototransistor for `durationMs`.
// The buzzer and status LED follow the receiver.
bool testSensor(Device& device, uint32_t durationMs = 10000);

// Report button presses for `durationMs`.
bool testButton(Device& device, uint32_t durationMs = 10000);

bool testBuzzer(Device& device);

// Link test, run on two units at once:
//   receiving unit: countPulses()  (start this first)
//   sending unit:   sendPulses()
// The receiver reports how many of the pulses arrived and their widths.
void sendPulses(Device& device, uint32_t count = 100, uint32_t widthMs = 20);
bool countPulses(Device& device, uint32_t expected = 100, uint32_t timeoutMs = 20000);

// Self-test for the standalone build (no console): beeps, blinks the laser,
// then mirrors the phototransistor on the buzzer and LED for 10 seconds.
void standaloneSelfTest(Device& device);

}  // namespace diagnostics
