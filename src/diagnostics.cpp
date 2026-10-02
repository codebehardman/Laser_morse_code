#include "diagnostics.hpp"

#include "board_config.hpp"
#include "console.hpp"

namespace diagnostics {
namespace {

// Wait while keeping the inputs sampled.
void waitMs(Device& device, uint32_t durationMs) {
    const uint32_t start = millis();
    while (millis() - start < durationMs) device.updateInputs();
}

// Mirror the receiver on the status LED.
void followSensor(Device& device) { device.led.set(device.sensor.lightDetected()); }

}  // namespace

bool testLaser(Device& device, uint32_t blinks) {
    console::printf("[laser] Blinking %lu times (250 ms on / 250 ms off). Watch the laser.\n",
                    static_cast<unsigned long>(blinks));
    device.sensor.resetActivations();
    for (uint32_t i = 0; i < blinks; ++i) {
        device.laser.set(true);
        waitMs(device, 250);
        device.laser.set(false);
        waitMs(device, 250);
    }
    const uint32_t seen = device.sensor.activations();
    console::printf("[laser] Done. Own phototransistor saw %lu/%lu pulses ",
                    static_cast<unsigned long>(seen), static_cast<unsigned long>(blinks));
    console::println("(0 is normal unless a mirror reflects the beam back).");
    return true;
}

bool testSensor(Device& device, uint32_t durationMs) {
    console::printf("[sensor] Watching phototransistor for %lu s. Shine a laser on it.\n",
                    static_cast<unsigned long>(durationMs / 1000));
    console::printf("[sensor] Current state: %s\n",
                    device.sensor.lightDetected() ? "LIGHT" : "dark");

    device.sensor.resetActivations();
    const uint32_t start = millis();
    uint32_t lastEdge = start;
    while (millis() - start < durationMs) {
        const uint32_t now = millis();
        if (device.sensor.update(now)) {
            console::printf("[sensor] %6lu ms: %s (previous state lasted %lu ms)\n",
                            static_cast<unsigned long>(now - start),
                            device.sensor.lightDetected() ? "LIGHT" : "dark ",
                            static_cast<unsigned long>(now - lastEdge));
            lastEdge = now;
        }
        followSensor(device);
    }
    device.led.set(false);

    const uint32_t pulses = device.sensor.activations();
    console::printf("[sensor] Done: %lu light pulses detected.\n",
                    static_cast<unsigned long>(pulses));
    if (pulses == 0) {
        console::println("[sensor] FAIL? Nothing seen - check aim, wiring and pull-up.");
        return false;
    }
    return true;
}

bool testButton(Device& device, uint32_t durationMs) {
    console::printf("[button] Press the key a few times within %lu s.\n",
                    static_cast<unsigned long>(durationMs / 1000));
    device.button.resetActivations();
    const uint32_t start = millis();
    while (millis() - start < durationMs) {
        if (device.button.update(millis())) {
            console::println(device.button.pressed() ? "[button] pressed" : "[button] released");
        }
    }
    const uint32_t presses = device.button.activations();
    console::printf("[button] Done: %lu presses.\n", static_cast<unsigned long>(presses));
    return presses > 0;
}

void sendPulses(Device& device, uint32_t count, uint32_t widthMs) {
    console::printf("[link] Sending %lu pulses of %lu ms...\n", static_cast<unsigned long>(count),
                    static_cast<unsigned long>(widthMs));
    device.laser.set(false);
    delay(300);
    for (uint32_t i = 0; i < count; ++i) {
        device.laser.set(true);
        delay(widthMs);
        device.laser.set(false);
        delay(widthMs);
    }
    console::println("[link] Sent.");
}

bool countPulses(Device& device, uint32_t expected, uint32_t timeoutMs) {
    constexpr uint32_t kQuietEndMs = 500;  // stop after this long without pulses

    console::printf("[link] Waiting up to %lu s for pulses - now run '/test tx' on the other unit.\n",
                    static_cast<unsigned long>(timeoutMs / 1000));

    uint32_t count = 0;
    uint32_t minWidth = UINT32_MAX;
    uint32_t maxWidth = 0;
    uint32_t riseMs = 0;
    uint32_t lastActivityMs = 0;
    const uint32_t start = millis();

    while (true) {
        const uint32_t now = millis();
        if (device.sensor.update(now)) {
            lastActivityMs = now;
            if (device.sensor.lightDetected()) {
                riseMs = now;
            } else if (riseMs != 0) {
                const uint32_t width = now - riseMs;
                if (width < minWidth) minWidth = width;
                if (width > maxWidth) maxWidth = width;
                ++count;
            }
        }
        if (count == 0 && now - start >= timeoutMs) break;
        if (count > 0 && now - lastActivityMs >= kQuietEndMs) break;
    }

    if (count == 0) {
        console::println("[link] FAIL: no pulses received.");
        return false;
    }

    const uint32_t missing = count >= expected ? 0 : expected - count;
    const uint32_t errorPct = (100 * (count > expected ? count - expected : missing)) / expected;
    console::printf("[link] Received %lu/%lu pulses (error %lu%%), width min %lu ms / max %lu ms.\n",
                    static_cast<unsigned long>(count), static_cast<unsigned long>(expected),
                    static_cast<unsigned long>(errorPct), static_cast<unsigned long>(minWidth),
                    static_cast<unsigned long>(maxWidth));
    const bool pass = errorPct <= 10;  // design requirement: <= 10% error rate
    console::println(pass ? "[link] PASS" : "[link] FAIL: error rate above 10%");
    return pass;
}

void standaloneSelfTest(Device& device) {
    // 1. Three quick flashes: self-test starting.
    device.led.flash(3, 100, 100);
    delay(500);

    // 2. Five laser blinks, with the LED blinking in step.
    for (int i = 0; i < 5; ++i) {
        device.laser.set(true);
        device.led.set(true);
        delay(250);
        device.laser.set(false);
        device.led.set(false);
        delay(250);
    }
    delay(500);

    // 3. Ten seconds of receiver monitoring: the LED follows the light.
    const uint32_t start = millis();
    while (millis() - start < 10000) {
        device.updateInputs();
        followSensor(device);
    }
    device.led.set(false);
    delay(500);

    // 4. One long flash: self-test finished.
    device.led.flash(1, 1000, 0);
}

}  // namespace diagnostics
