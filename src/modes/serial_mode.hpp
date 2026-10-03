#pragma once

#include "../device.hpp"

// Serial terminal mode: a text link between the two PCs' terminals.
// Typed characters are collected locally (with backspace) and the line is
// sent over the laser at config::kLinkBaud (8E1) when Enter is pressed, as
// one frame with a CRC (see line_frame.hpp). The other unit prints the line
// only if it arrived intact; otherwise it reports a corrupted message.
// The status LED flashes when data arrives.
//
// The key holds the laser on for aiming; the other unit ignores this.
namespace serial_mode {

void begin(Device& device);
void update(Device& device);

}  // namespace serial_mode
