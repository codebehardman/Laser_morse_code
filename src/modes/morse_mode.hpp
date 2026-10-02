#pragma once

#include "../device.hpp"

// Morse code mode: the key drives the laser directly, LD2 follows the
// received light, received Morse is decoded to the console, and
// console lines are sent as Morse (lines starting with '/' are commands).
namespace morse_mode {

void begin(Device& device);
void update(Device& device);

}  // namespace morse_mode
