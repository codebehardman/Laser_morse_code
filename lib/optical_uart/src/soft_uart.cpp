#include "soft_uart.hpp"

namespace optical {

// ---------------------------------------------------------------- transmit

bool SoftUartTx::write(uint8_t byte) {
    const uint16_t next = (head_ + 1) & (kQueueSize - 1);
    if (next == tail_) return false;
    queue_[head_] = byte;
    head_ = next;
    return true;
}

size_t SoftUartTx::space() const {
    return (tail_ - head_ - 1) & (kQueueSize - 1);
}

bool SoftUartTx::tick() {
    if (bitsLeft_ == 0) {
        if (head_ == tail_) return true;  // idle: mark
        const uint8_t byte = queue_[tail_];
        tail_ = (tail_ + 1) & (kQueueSize - 1);
        // start(0) | data | parity | stop(1), sent LSB first
        frame_ = static_cast<uint16_t>((byte << 1) | (evenParityBit(byte) << 9) | (1u << 10));
        bitsLeft_ = kBitsPerFrame;
        subTick_ = 0;
    }

    const bool level = (frame_ & 1u) != 0;
    if (++subTick_ == kOversample) {
        subTick_ = 0;
        frame_ >>= 1;
        --bitsLeft_;
    }
    return level;
}

// ----------------------------------------------------------------- receive

void SoftUartRx::tick(bool level) {
    switch (state_) {
        case State::WaitIdle:
            // After a break or framing error, require one full bit time of
            // idle line before trusting the next start bit.
            tickCount_ = level ? tickCount_ + 1 : 0;
            if (tickCount_ >= kOversample) state_ = State::Idle;
            return;

        case State::Idle:
            if (!level) {  // falling edge: possible start bit
                state_ = State::Receiving;
                tickCount_ = 0;
                bitIndex_ = 0;
                votes_ = 0;
                frame_ = 0;
            }
            return;

        case State::Receiving:
            break;
    }

    // Majority vote over the three samples around the middle of the bit.
    // The start edge is seen on average half a tick late (on the first low
    // sample), so the window is centred one tick before the nominal middle.
    ++tickCount_;
    const uint16_t center = bitIndex_ * kOversample + kOversample / 2 - 1;
    if (tickCount_ + 1 < center) return;
    if (level) ++votes_;
    if (tickCount_ < center + 1) return;

    const bool bit = votes_ >= 2;
    votes_ = 0;

    if (bitIndex_ == 0 && bit) {  // start bit didn't hold: it was a glitch
        state_ = State::Idle;
        return;
    }
    frame_ |= static_cast<uint16_t>(bit) << bitIndex_;
    if (++bitIndex_ == kBitsPerFrame) finishFrame();
}

void SoftUartRx::finishFrame() {
    const bool stopBit = (frame_ >> 10) & 1u;
    if (!stopBit) {
        ++framingErrors_;  // also what a laser held on (aiming) looks like
        state_ = State::WaitIdle;
        tickCount_ = 0;
        return;
    }

    const uint8_t byte = static_cast<uint8_t>(frame_ >> 1);
    const uint8_t parity = (frame_ >> 9) & 1u;
    if (parity != evenParityBit(byte)) {
        ++parityErrors_;
    } else {
        ++bytesReceived_;
        push(byte);
    }
    // The rest of the stop bit is idle line; look for the next start bit.
    state_ = State::Idle;
}

void SoftUartRx::push(uint8_t byte) {
    const uint16_t next = (head_ + 1) & (kBufferSize - 1);
    if (next == tail_) {
        ++overruns_;
        return;
    }
    buffer_[head_] = byte;
    head_ = next;
}

bool SoftUartRx::read(uint8_t& byte) {
    if (head_ == tail_) return false;
    byte = buffer_[tail_];
    tail_ = (tail_ + 1) & (kBufferSize - 1);
    return true;
}

}  // namespace optical
