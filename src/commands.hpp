#pragma once

#include "device.hpp"

namespace commands {

// Handle one line typed in the serial console. Lines starting with '/' are
// commands (see printHelp); anything else is queued for Morse transmission.
void handleLine(Device& device, const char* line);

// Handle /level or /threshold (used by the serial terminal mode, where the
// other commands don't apply). Returns false for any other line.
bool handleSensorLine(Device& device, const char* line);

void printHelp();
void applyWpm(Device& device, uint32_t wpm);

}  // namespace commands
