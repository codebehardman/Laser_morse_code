#pragma once

#include "../device.hpp"

// Serial terminal mode: a transparent link between the two PCs' terminals.
// Every byte typed into this unit's terminal is sent over the laser at
// config::kLinkBaud (8E1), and every byte received is written unchanged to
// this terminal. Bytes that fail the parity or framing check are dropped
// instead of being shown wrong. The status LED flashes when data arrives.
//
// The key holds the laser on for aiming; the other unit ignores this.
namespace serial_mode {

void begin(Device& device);
void update(Device& device);

}  // namespace serial_mode
