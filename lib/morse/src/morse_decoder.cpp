#include "morse_decoder.hpp"

#include <cstring>

namespace morse {

MorseDecoder::MorseDecoder(uint32_t initialUnitMs) {
    setUnitMs(initialUnitMs);
    initialUnitMs_ = unitMs_;
}

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
            if (elapsed >= kNewMessageMs || elapsed >= kNewMessageUnits * unitMs_) {
                // New message: its speed may differ, start from hand-keying speed.
                historyCount_ = 0;
                unitMs_ = initialUnitMs_;
            }
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
    updateSpeed(durationMs);
    if (patternLength_ < kMaxPatternLength + 1) marks_[patternLength_++] = durationMs;
}

void MorseDecoder::updateSpeed(uint32_t durationMs) {
    history_[historyNext_] = durationMs;
    historyNext_ = (historyNext_ + 1) % kHistorySize;
    if (historyCount_ < kHistorySize) ++historyCount_;

    uint32_t shortest = UINT32_MAX;
    uint32_t longest = 0;
    for (size_t i = 0; i < historyCount_; ++i) {
        const size_t index = (historyNext_ + kHistorySize - 1 - i) % kHistorySize;
        if (history_[index] < shortest) shortest = history_[index];
        if (history_[index] > longest) longest = history_[index];
    }
    // Only when both dots and dashes are present: cutoff (2 units) halfway.
    if (longest >= 2 * shortest) setUnitMs((shortest + longest) / 4);
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
    // Dots are ~1 unit, dashes ~3 units: split at 2 units, using the speed
    // estimate that includes all of this letter's marks.
    char c = '\0';
    if (patternLength_ <= kMaxPatternLength) {
        char pattern[kMaxPatternLength + 1];
        for (size_t i = 0; i < patternLength_; ++i) {
            pattern[i] = marks_[i] >= 2 * unitMs_ ? '-' : '.';
        }
        pattern[patternLength_] = '\0';
        if (std::strcmp(pattern, kStartProsign) == 0) {  // start of a typed message
            patternLength_ = 0;
            return;
        }
        c = decode(pattern);
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
