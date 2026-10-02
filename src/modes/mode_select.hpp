#pragma once

#include "../device.hpp"

enum class Mode {
    Morse,           // key/buzzer Morse code, text console with commands
    SerialTerminal,  // transparent 9600-baud serial link between two terminals
};

// Blocks until the first key press after power-up and returns the mode it
// selects: a short press picks Morse, a hold of config::kModeSelectHoldMs
// or longer picks serial terminal. LD2 blinks while waiting. The buzzer
// confirms with one beep (Morse) or two beeps (serial terminal).
Mode selectMode(Device& device);

const char* modeName(Mode mode);
