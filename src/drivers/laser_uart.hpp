#pragma once

#include <Arduino.h>

#include "soft_uart.hpp"

// 8E1 serial link over the laser, bit-banged from a hardware timer interrupt
// running at baud * optical::kOversample. PA0/PA1 have no UART peripheral,
// hence the software UART.
//
// Laser on = space (0), laser off = mark (1, idle). The phototransistor pulls
// its pin low when lit, so the receive pin level is the UART level directly.
class LaserUart {
public:
    LaserUart(uint32_t laserPin, uint32_t sensorPin, TIM_TypeDef* timer)
        : laserPin_(laserPin), sensorPin_(sensorPin), timerInstance_(timer) {}

    // Takes over the laser and sensor pins and starts the tick interrupt.
    void begin(uint32_t baud);

    bool write(uint8_t byte) { return tx_.write(byte); }
    size_t txSpace() const { return tx_.space(); }
    bool read(uint8_t& byte) { return rx_.read(byte); }

    // Keep the laser on regardless of data (aiming). The receiving unit sees
    // this as a line break and outputs nothing.
    void setForceOn(bool on) { forceOn_ = on; }

    const optical::SoftUartRx& receiver() const { return rx_; }

private:
    void onTick();

    uint32_t laserPin_;
    uint32_t sensorPin_;
    TIM_TypeDef* timerInstance_;
    HardwareTimer* timer_ = nullptr;
    PinName laserPinName_ = NC;
    PinName sensorPinName_ = NC;

    optical::SoftUartTx tx_;
    optical::SoftUartRx rx_;
    volatile bool forceOn_ = false;
};
