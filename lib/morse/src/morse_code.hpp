#pragma once

// International (ITU-R M.1677-1) Morse code alphabet.
// Hardware independent: compiled for the STM32 and for native unit tests.

#include <cstdint>

namespace morse {

// Standard timing ratios, expressed in "units" (the length of one dot).
constexpr uint32_t kDotUnits = 1;
constexpr uint32_t kDashUnits = 3;
constexpr uint32_t kElementGapUnits = 1;  // between dots/dashes of one character
constexpr uint32_t kCharGapUnits = 3;     // between characters
constexpr uint32_t kWordGapUnits = 7;     // between words

// Longest pattern in the table (e.g. "$" = "...-..-").
constexpr uint32_t kMaxPatternLength = 7;

// Character used to report a received pattern that is not in the table.
constexpr char kUnknownChar = '*';

// PARIS standard: one word = 50 units, so unit_ms = 1200 / wpm.
constexpr uint32_t wpmToUnitMs(uint32_t wpm) { return wpm == 0 ? 1200 : 1200 / wpm; }

// Returns the dot/dash pattern (e.g. ".-") for a character, case-insensitive,
// or nullptr if the character has no Morse representation.
const char* encode(char c);

// Returns the character for a dot/dash pattern, or '\0' if unknown.
char decode(const char* pattern);

// Number of units needed to key `text` with standard gaps, from the first
// mark to the end of the last one. Unsupported characters are skipped.
uint32_t messageUnits(const char* text);

}  // namespace morse
