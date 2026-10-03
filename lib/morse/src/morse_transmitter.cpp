#include "morse_transmitter.hpp"

#include "morse_code.hpp"

namespace morse {

MorseTransmitter::MorseTransmitter(uint32_t unitMs, bool startProsign) : startProsign_(startProsign) {
    setUnitMs(unitMs);
}

size_t MorseTransmitter::enqueue(const char* text) {
    size_t count = 0;
    while (text != nullptr && text[count] != '\0') {
        if (!enqueue(text[count])) break;
        ++count;
    }
    return count;
}

bool MorseTransmitter::enqueue(char c) {
    const size_t next = (head_ + 1) & (kQueueSize - 1);
    if (next == tail_) return false;  // full
    queue_[head_] = c;
    head_ = next;
    return true;
}

void MorseTransmitter::clear() {
    head_ = tail_ = 0;
    pattern_ = nullptr;
    nextPattern_ = nullptr;
    keyDown_ = false;
    segmentActive_ = false;
    sentSinceIdle_ = false;
}

bool MorseTransmitter::pop(char& c) {
    if (head_ == tail_) return false;
    c = queue_[tail_];
    tail_ = (tail_ + 1) & (kQueueSize - 1);
    return true;
}

bool MorseTransmitter::update(uint32_t nowMs) {
    // Segments are chained back-to-back from their scheduled end time (not
    // from "now") so that a late update() call does not stretch the timing.
    while (!segmentActive_ || static_cast<int32_t>(nowMs - segmentEndMs_) >= 0) {
        const uint32_t start = segmentActive_ ? segmentEndMs_ : nowMs;
        if (!startNextSegment(start)) {
            segmentActive_ = false;
            keyDown_ = false;
            return false;
        }
    }
    return keyDown_;
}

void MorseTransmitter::startSegment(bool keyDown, uint32_t units, uint32_t startMs) {
    keyDown_ = keyDown;
    segmentEndMs_ = startMs + units * unitMs_;
    segmentActive_ = true;
}

bool MorseTransmitter::startNextSegment(uint32_t startMs) {
    // Every dot/dash is followed by a one-unit gap.
    if (keyDown_) {
        startSegment(false, kElementGapUnits, startMs);
        return true;
    }

    // More elements left in the current character.
    if (pattern_ != nullptr && *pattern_ != '\0') {
        const uint32_t units = (*pattern_ == '-') ? kDashUnits : kDotUnits;
        ++pattern_;
        startSegment(true, units, startMs);
        return true;
    }

    // Current character finished: the 1-unit element gap has already elapsed,
    // so only the remainder of the character/word gap is added here.
    if (nextPattern_ != nullptr) {  // prosign done: on to the first letter
        pattern_ = nextPattern_;
        nextPattern_ = nullptr;
        startSegment(false, kCharGapUnits - kElementGapUnits, startMs);
        return true;
    }

    uint32_t extraGapUnits = 0;
    char c;
    while (pop(c)) {
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            if (sentSinceIdle_) extraGapUnits = kWordGapUnits - kElementGapUnits;
            continue;
        }
        const char* pattern = encode(c);
        if (pattern == nullptr) continue;  // unsupported character

        if (sentSinceIdle_ && extraGapUnits == 0) {
            extraGapUnits = kCharGapUnits - kElementGapUnits;
        }
        if (!sentSinceIdle_ && startProsign_) {
            nextPattern_ = pattern;
            pattern = kStartProsign;
        }
        sentSinceIdle_ = true;
        pattern_ = pattern;

        if (extraGapUnits > 0) {
            startSegment(false, extraGapUnits, startMs);
        } else {
            const uint32_t units = (*pattern_ == '-') ? kDashUnits : kDotUnits;
            ++pattern_;
            startSegment(true, units, startMs);
        }
        return true;
    }

    // Queue drained: hold a word gap so the receiver flushes the last word
    // before anything queued later starts.
    pattern_ = nullptr;
    if (sentSinceIdle_) {
        sentSinceIdle_ = false;
        startSegment(false, kWordGapUnits - kElementGapUnits, startMs);
        return true;
    }
    return false;
}

}  // namespace morse
