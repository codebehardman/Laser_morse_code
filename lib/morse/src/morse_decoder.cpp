#include "morse_decoder.hpp"

namespace morse {

MorseDecoder::MorseDecoder(uint32_t initialUnitMs) { setUnitMs(initialUnitMs); }

void MorseDecoder::setUnitMs(uint32_t unitMs) {
    if (unitMs < kMinUnitMs) unitMs = kMinUnitMs;
    if (unitMs > kMaxUnitMs) unitMs = kMaxUnitMs;
    unitMs_ = unitMs;
}

void MorseDecoder::reset() {
    patternLength_ = 0;
    wordPending_ = false;
    outHead_ = outTail_ = 0;
}

void MorseDecoder::update(bool lightOn, uint32_t nowMs) {
    if (!started_) {
        started_ = true;
        lightOn_ = lightOn;
        lastEdgeMs_ = nowMs;
        return;
    }

    const uint32_t elapsed = nowMs - lastEdgeMs_;
    if (lightOn != lightOn_) {
        if (lightOn) {
            handleGap(elapsed);  // gap ended
        } else {
            onMarkEnded(elapsed);
        }
        lightOn_ = lightOn;
        lastEdgeMs_ = nowMs;
    } else if (!lightOn) {
        // Still dark: flush characters/words once the gap is long enough,
        // without waiting for the next mark.
        handleGap(elapsed);
    }
}

void MorseDecoder::onMarkEnded(uint32_t durationMs) {
    if (durationMs >= kAimHoldMs) {
        patternLength_ = 0;  // aiming, not a symbol
        return;
    }
    ++marksSeen_;

    // Dots are ~1 unit, dashes ~3 units: split at 2 units.
    const bool isDash = durationMs >= 2 * unitMs_;
    if (patternLength_ < kMaxPatternLength + 1) {
        pattern_[patternLength_++] = isDash ? '-' : '.';
    }

    // Track the sender's speed with a slow moving average.
    const uint32_t measuredUnit = isDash ? durationMs / kDashUnits : durationMs;
    setUnitMs((3 * unitMs_ + measuredUnit) / 4);
}

void MorseDecoder::handleGap(uint32_t gapMs) {
    // Thresholds sit halfway between the nominal gap lengths:
    // element(1) | 2 | char(3) | 5 | word(7)
    if (patternLength_ > 0 && gapMs >= 2 * unitMs_) {
        flushCharacter();
    }
    if (wordPending_ && gapMs >= 5 * unitMs_) {
        push(' ');
        wordPending_ = false;
    }
}

void MorseDecoder::flushCharacter() {
    char c = '\0';
    if (patternLength_ <= kMaxPatternLength) {
        pattern_[patternLength_] = '\0';
        c = decode(pattern_);
    }
    if (c == '\0') {
        c = kUnknownChar;
        ++unknownPatterns_;
    }
    push(c);
    patternLength_ = 0;
    wordPending_ = true;
}

void MorseDecoder::push(char c) {
    const size_t next = (outHead_ + 1) & (kOutputSize - 1);
    if (next == outTail_) return;  // full: drop
    output_[outHead_] = c;
    outHead_ = next;
}

bool MorseDecoder::read(char& c) {
    if (outHead_ == outTail_) return false;
    c = output_[outTail_];
    outTail_ = (outTail_ + 1) & (kOutputSize - 1);
    return true;
}

}  // namespace morse
