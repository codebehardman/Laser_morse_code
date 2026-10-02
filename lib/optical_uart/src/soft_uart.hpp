#pragma once

// Software UART framing for the laser link: 8 data bits, even parity,
// 1 stop bit (8E1), LSB first.
//
// Both halves are driven by a fixed-rate tick at kOversample ticks per bit
// (a timer interrupt on the STM32, a loop in the unit tests), so this code
// has no hardware dependencies.
//
// Line levels follow UART convention: true = mark/idle (laser off),
// false = space (laser on). The ring-buffer indices are volatile because the
// tick runs in an interrupt while the main loop reads/writes the buffers
// (single producer, single consumer).

#include <cstddef>
#include <cstdint>

namespace optical {

constexpr uint32_t kOversample = 8;    // ticks per bit
constexpr uint32_t kBitsPerFrame = 11;  // start + 8 data + parity + stop

// 1 if `byte` has an odd number of set bits, so that data + parity is even.
constexpr uint8_t evenParityBit(uint8_t byte) {
    byte ^= byte >> 4;
    byte ^= byte >> 2;
    byte ^= byte >> 1;
    return byte & 1u;
}

class SoftUartTx {
public:
    // Queue a byte. Returns false if the queue is full.
    bool write(uint8_t byte);
    size_t space() const;
    bool busy() const { return bitsLeft_ != 0 || head_ != tail_; }

    // Advance one tick. Returns the line level to output for this tick.
    bool tick();

private:
    static constexpr size_t kQueueSize = 512;  // power of two

    uint8_t queue_[kQueueSize] = {};
    volatile uint16_t head_ = 0;
    volatile uint16_t tail_ = 0;

    uint16_t frame_ = 0;     // remaining bits, LSB is the current bit
    uint8_t bitsLeft_ = 0;   // bits left in the current frame
    uint8_t subTick_ = 0;    // tick within the current bit
};

class SoftUartRx {
public:
    // Feed one line sample. Call exactly once per tick.
    void tick(bool level);

    // Pop a received byte. Returns false if none is available.
    bool read(uint8_t& byte);

    uint32_t bytesReceived() const { return bytesReceived_; }
    uint32_t parityErrors() const { return parityErrors_; }
    uint32_t framingErrors() const { return framingErrors_; }
    uint32_t overruns() const { return overruns_; }

private:
    enum class State : uint8_t { WaitIdle, Idle, Receiving };
    static constexpr size_t kBufferSize = 512;  // power of two

    void finishFrame();
    void push(uint8_t byte);

    State state_ = State::WaitIdle;
    uint16_t tickCount_ = 0;   // ticks since the start bit was first seen
    uint8_t votes_ = 0;        // high samples in the current bit's vote window
    uint16_t frame_ = 0;       // decided bits, LSB first
    uint8_t bitIndex_ = 0;     // bit being sampled (0 = start bit)

    uint8_t buffer_[kBufferSize] = {};
    volatile uint16_t head_ = 0;
    volatile uint16_t tail_ = 0;

    volatile uint32_t bytesReceived_ = 0;
    volatile uint32_t parityErrors_ = 0;
    volatile uint32_t framingErrors_ = 0;
    volatile uint32_t overruns_ = 0;
};

}  // namespace optical
