#include "laser_uart.hpp"

void LaserUart::begin(uint32_t baud) {
    laserPinName_ = digitalPinToPinName(laserPin_);
    sensorPinName_ = digitalPinToPinName(sensorPin_);
    pinMode(laserPin_, OUTPUT);
    digitalWriteFast(laserPinName_, LOW);
    pinMode(sensorPin_, INPUT);

    timer_ = new HardwareTimer(timerInstance_);
    timer_->setOverflow(baud * optical::kOversample, HERTZ_FORMAT);
    timer_->attachInterrupt([this]() { onTick(); });
    timer_->resume();
}

// Runs in interrupt context at baud * kOversample (76.8 kHz for 9600 baud).
void LaserUart::onTick() {
    rx_.tick(digitalReadFast(sensorPinName_) != 0);
    const bool mark = tx_.tick();
    digitalWriteFast(laserPinName_, (!mark || forceOn_) ? HIGH : LOW);
}
