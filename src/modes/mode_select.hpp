#pragma once

#include "../device.hpp"

enum class Mode {
    Morse,           // key/LED Morse code, text console with commands
    SerialTerminal,  // transparent 9600-baud serial link between two terminals
};

// Blocks until the first key press after power-up and returns the mode it
// selects: a short press picks Morse, a hold of config::kModeSelectHoldMs
// or longer picks serial terminal. The status LED blips while waiting, then confirms
// with one long flash (Morse) or two long flashes (serial terminal).
Mode selectMode(Device& device);

const char* modeName(Mode mode);
