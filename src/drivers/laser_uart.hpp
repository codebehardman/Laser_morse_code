#pragma once

#include <Arduino.h>

#include "photo_sensor.hpp"
#include "soft_uart.hpp"

// 8E1 serial link over the laser, bit-banged at baud * optical::kOversample
// ticks per second (9.6 kHz for 1200 baud).
//
// The ticks run in a dedicated task on CPU core 0 rather than in a timer
// interrupt, because the receiver is sampled with the ADC (to apply the
// light threshold) and the ESP32 ADC driver can't be used from an interrupt.
// The Arduino loop keeps running on core 1.
//
// Laser on = space (0), laser off = mark (1, idle). Light at the sensor
// (level below the threshold) = space.
//
// Only one instance can be active.
class LaserUart {
public:
    LaserUart(uint32_t laserPin, const PhotoSensor& sensor) : laserPin_(laserPin), sensor_(sensor) {}

    // Takes over the laser pin and starts the sampling task.
    void begin(uint32_t baud);

    // Change the link speed while running (both units must match).
    void setBaud(uint32_t baud);
    uint32_t baud() const { return baud_; }

    bool write(uint8_t byte) { return tx_.write(byte); }
    size_t txSpace() const { return tx_.space(); }
    bool read(uint8_t& byte) { return rx_.read(byte); }

    // Keep the laser on regardless of data (aiming). The receiving unit sees
    // this as a line break and outputs nothing.
    void setForceOn(bool on) { forceOn_ = on; }

    const optical::SoftUartRx& receiver() const { return rx_; }

    // One oversampling tick; called by the sampling task.
    void tick();

private:
    uint32_t laserPin_;
    const PhotoSensor& sensor_;
    uint32_t baud_ = 0;

    optical::SoftUartTx tx_;
    optical::SoftUartRx rx_;
    volatile bool forceOn_ = false;
};
