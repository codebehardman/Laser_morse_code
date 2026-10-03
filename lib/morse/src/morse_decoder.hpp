#pragma once

// Light-pulse timing -> text decoder.
//
// Call update(lightOn, now_ms) every loop iteration with the (debounced)
// receiver state. Decoded characters are read back with read(); a ' ' is
// produced at each word gap.
//
// The sender's speed varies (hand keying vs. typed text), so the dot length
// ("unit") is estimated from the received marks instead of being fixed:
// a dash is ~3x a dot, so whenever recent marks contain both, the dot/dash
// cutoff is set halfway between the shortest and the longest. Each letter is
// classified only once it is complete, so a speed change is picked up
// within the letter. After a long pause the history is cleared and the speed
// goes back to the initial (hand-keying) value, since the next message may
// come at a different speed. Typed messages start with kStartProsign, which
// sets the speed before their first letter and is not output.

#include <cstddef>
#include <cstdint>

#include "morse_code.hpp"

namespace morse {

class MorseDecoder {
public:
    // Marks at least this long are treated as the operator holding the key
    // to aim the laser, and are not decoded.
    static constexpr uint32_t kAimHoldMs = 1500;
    static constexpr uint32_t kMinUnitMs = 20;   // ~60 WPM
    static constexpr uint32_t kMaxUnitMs = 400;  // ~3 WPM

    explicit MorseDecoder(uint32_t initialUnitMs);

    void update(bool lightOn, uint32_t nowMs);

    // Pop the next decoded character. Returns false if none is available.
    bool read(char& c);

    // Discard the partial character and any undelivered output.
    void reset();

    uint32_t unitMs() const { return unitMs_; }
    void setUnitMs(uint32_t unitMs);

    // Statistics, useful for link diagnostics.
    uint32_t marksSeen() const { return marksSeen_; }
    uint32_t unknownPatterns() const { return unknownPatterns_; }

private:
    static constexpr size_t kOutputSize = 64;  // must be a power of two

    // Recent marks used to estimate the speed.
    static constexpr size_t kHistorySize = 8;
    // A pause this long (in units, or kNewMessageMs) starts a new message.
    static constexpr uint32_t kNewMessageUnits = 14;
    static constexpr uint32_t kNewMessageMs = 1500;

    void onMarkEnded(uint32_t durationMs);
    void updateSpeed(uint32_t durationMs);
    void handleGap(uint32_t gapMs);
    void flushCharacter();
    void push(char c);

    uint32_t unitMs_;
    uint32_t initialUnitMs_;
    bool lightOn_ = false;
    bool started_ = false;
    uint32_t lastEdgeMs_ = 0;

    uint32_t history_[kHistorySize] = {};
    size_t historyCount_ = 0;
    size_t historyNext_ = 0;

    // Mark durations of the letter being received (+1 to detect overlong).
    uint32_t marks_[kMaxPatternLength + 1] = {};
    size_t patternLength_ = 0;
    bool wordPending_ = false;  // characters emitted since the last word gap

    char output_[kOutputSize] = {};
    size_t outHead_ = 0;
    size_t outTail_ = 0;

    uint32_t marksSeen_ = 0;
    uint32_t unknownPatterns_ = 0;
};

}  // namespace morse
