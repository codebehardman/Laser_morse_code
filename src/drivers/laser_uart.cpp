#include "laser_uart.hpp"

#include "board_config.hpp"

// The tick has ~13 us between interrupts, so pin access and the timer setup
// use each platform's fast native APIs instead of digitalRead/digitalWrite.

namespace {
LaserUart* activeLink = nullptr;
}  // namespace

#if defined(ARDUINO_ARCH_ESP32)

#include <driver/gpio.h>

namespace {

constexpr uint32_t kTimerClockHz = 40000000;  // 80 MHz APB / 2

bool readSensor(uint32_t pin) { return gpio_get_level(static_cast<gpio_num_t>(pin)) != 0; }
void writeLaser(uint32_t pin, bool on) { gpio_set_level(static_cast<gpio_num_t>(pin), on ? 1 : 0); }

void IRAM_ATTR onTimer() {
    if (activeLink != nullptr) activeLink->tick();
}

void startTimer(uint32_t tickHz) {
    const uint32_t alarmTicks = (kTimerClockHz + tickHz / 2) / tickHz;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    hw_timer_t* timer = timerBegin(kTimerClockHz);
    timerAttachInterrupt(timer, &onTimer);
    timerAlarm(timer, alarmTicks, /*autoreload=*/true, /*reload_count=*/0);
#else
    hw_timer_t* timer = timerBegin(LMC_LINK_TIMER, 80000000 / kTimerClockHz, /*countUp=*/true);
    timerAttachInterrupt(timer, &onTimer, /*edge=*/true);
    timerAlarmWrite(timer, alarmTicks, /*autoreload=*/true);
    timerAlarmEnable(timer);
#endif
}

}  // namespace

#elif defined(ARDUINO_ARCH_STM32)

namespace {

PinName laserPinName = NC;
PinName sensorPinName = NC;

bool readSensor(uint32_t) { return digitalReadFast(sensorPinName) != 0; }
void writeLaser(uint32_t, bool on) { digitalWriteFast(laserPinName, on ? HIGH : LOW); }

void startTimer(uint32_t tickHz) {
    HardwareTimer* timer = new HardwareTimer(LMC_LINK_TIMER);
    timer->setOverflow(tickHz, HERTZ_FORMAT);
    timer->attachInterrupt([]() {
        if (activeLink != nullptr) activeLink->tick();
    });
    timer->resume();
}

}  // namespace

#else
#error "LaserUart: unsupported platform"
#endif

void LaserUart::begin(uint32_t baud) {
#if defined(ARDUINO_ARCH_STM32)
    laserPinName = digitalPinToPinName(laserPin_);
    sensorPinName = digitalPinToPinName(sensorPin_);
#endif
    pinMode(laserPin_, OUTPUT);
    writeLaser(laserPin_, false);
    pinMode(sensorPin_, INPUT);

    activeLink = this;
    startTimer(baud * optical::kOversample);
}

void LaserUart::tick() {
    rx_.tick(readSensor(sensorPin_));
    const bool mark = tx_.tick();
    writeLaser(laserPin_, !mark || forceOn_);
}
