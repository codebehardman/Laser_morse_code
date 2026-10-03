#pragma once

// Non-blocking text -> Morse keying scheduler.
//
// Queue text with enqueue(), then call update(now_ms) every loop iteration.
// update() returns true while the key (laser) should be on. Nothing here
// touches hardware, so it can be unit-tested with a simulated clock.

#include <cstddef>
#include <cstdint>

namespace morse {

class MorseTransmitter {
public:
    // With startProsign, every message (text sent after an idle period)
    // begins with kStartProsign.
    explicit MorseTransmitter(uint32_t unitMs, bool startProsign = true);

    // Queue text for transmission. Characters without a Morse representation
    // are skipped when they are reached. Returns the number of characters
    // queued (less than the input length if the queue is full).
    size_t enqueue(const char* text);
    bool enqueue(char c);

    // Drop everything queued and release the key immediately.
    void clear();

    void setUnitMs(uint32_t unitMs) { unitMs_ = unitMs == 0 ? 1 : unitMs; }
    uint32_t unitMs() const { return unitMs_; }

    // True while something is queued or being sent (including trailing gap).
    bool busy() const { return segmentActive_ || head_ != tail_; }

    // Advance the schedule. Returns true while the key should be down.
    bool update(uint32_t nowMs);

private:
    static constexpr size_t kQueueSize = 256;  // must be a power of two

    bool pop(char& c);
    bool startNextSegment(uint32_t startMs);
    void startSegment(bool keyDown, uint32_t units, uint32_t startMs);

    char queue_[kQueueSize] = {};
    size_t head_ = 0;  // next write
    size_t tail_ = 0;  // next read

    uint32_t unitMs_;
    bool startProsign_;
    const char* pattern_ = nullptr;      // remaining elements of current character
    const char* nextPattern_ = nullptr;  // first letter, waiting behind the prosign
    bool keyDown_ = false;
    bool segmentActive_ = false;
    bool sentSinceIdle_ = false;  // a character was sent since the last idle period
    uint32_t segmentEndMs_ = 0;
};

}  // namespace morse
