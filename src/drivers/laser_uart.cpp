#include "laser_uart.hpp"

#include <driver/gpio.h>
#include <esp_timer.h>

namespace {

constexpr BaseType_t kLinkCore = 0;  // the Arduino loop runs on core 1
constexpr UBaseType_t kLinkTaskPriority = 5;

struct TaskArgs {
    LaserUart* link;
    int64_t periodNs;
};
TaskArgs taskArgs;

// Runs forever on core 0, calling tick() at a fixed rate. It busy-waits
// between ticks (a tick is only ~104 us), so core 0's idle watchdog is
// disabled in begin().
void linkTask(void* param) {
    const TaskArgs& args = *static_cast<TaskArgs*>(param);
    int64_t nextNs = esp_timer_get_time() * 1000;
    for (;;) {
        args.link->tick();
        nextNs += args.periodNs;
        int64_t nowNs = esp_timer_get_time() * 1000;
        if (nowNs - nextNs > 10 * args.periodNs) nextNs = nowNs;  // fell far behind: resync
        while (nowNs < nextNs) nowNs = esp_timer_get_time() * 1000;
    }
}

}  // namespace

void LaserUart::begin(uint32_t baud) {
    pinMode(laserPin_, OUTPUT);
    gpio_set_level(static_cast<gpio_num_t>(laserPin_), 0);

    taskArgs = {this, 1000000000LL / (static_cast<int64_t>(baud) * optical::kOversample)};
    disableCore0WDT();
    xTaskCreatePinnedToCore(linkTask, "laser_link", 4096, &taskArgs, kLinkTaskPriority, nullptr,
                            kLinkCore);
}

void LaserUart::tick() {
    rx_.tick(!sensor_.readRaw());  // dark = mark (1)
    const bool mark = tx_.tick();
    gpio_set_level(static_cast<gpio_num_t>(laserPin_), (!mark || forceOn_) ? 1 : 0);
}
