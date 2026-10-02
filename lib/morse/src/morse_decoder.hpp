#pragma once

// Light-pulse timing -> text decoder.
//
// Call update(lightOn, now_ms) every loop iteration with the (debounced)
// receiver state. Decoded characters are read back with read(); a ' ' is
// produced at each word gap.
//
// Human keying speed varies, so the dot length ("unit") is estimated
// continuously from received marks instead of being fixed.

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

    void onMarkEnded(uint32_t durationMs);
    void handleGap(uint32_t gapMs);
    void flushCharacter();
    void push(char c);

    uint32_t unitMs_;
    bool lightOn_ = false;
    bool started_ = false;
    uint32_t lastEdgeMs_ = 0;

    char pattern_[kMaxPatternLength + 2] = {};  // +1 overflow marker, +1 terminator
    size_t patternLength_ = 0;
    bool wordPending_ = false;  // characters emitted since the last word gap

    char output_[kOutputSize] = {};
    size_t outHead_ = 0;
    size_t outTail_ = 0;

    uint32_t marksSeen_ = 0;
    uint32_t unknownPatterns_ = 0;
};

}  // namespace morse
