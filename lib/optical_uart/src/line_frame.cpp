#include "line_frame.hpp"

namespace optical {
namespace {

constexpr char kHexDigits[] = "0123456789ABCDEF";

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

}  // namespace

uint8_t crc8(const uint8_t* data, size_t length) {
    uint8_t crc = 0;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07) : static_cast<uint8_t>(crc << 1);
        }
    }
    return crc;
}

size_t encodeLine(const char* text, size_t length, uint8_t* out) {
    size_t n = 0;
    out[n++] = kFrameStart;
    for (size_t i = 0; i < length && n - 1 < kMaxLineLength; ++i) {
        if (isFrameable(text[i])) out[n++] = static_cast<uint8_t>(text[i]);
    }
    const uint8_t crc = crc8(out + 1, n - 1);
    out[n++] = static_cast<uint8_t>(kHexDigits[crc >> 4]);
    out[n++] = static_cast<uint8_t>(kHexDigits[crc & 0x0F]);
    out[n++] = kFrameEnd;
    return n;
}

LineDeframer::Result LineDeframer::push(uint8_t byte) {
    if (byte == kFrameStart) {
        // A new frame while one was open means the previous end was lost.
        const bool lostPrevious = inFrame_;
        inFrame_ = true;
        skipping_ = false;
        length_ = 0;
        return lostPrevious ? Result::Corrupted : Result::None;
    }

    if (!inFrame_) {
        // An end without a start means the start was lost; stray text
        // bytes are the rest of that broken frame and are reported at its
        // end, unless the frame was already reported.
        if (byte != kFrameEnd) return Result::None;
        if (skipping_) {
            skipping_ = false;
            return Result::None;
        }
        return Result::Corrupted;
    }

    if (byte == kFrameEnd) {
        inFrame_ = false;
        if (length_ < 2) return Result::Corrupted;
        const int high = hexValue(buffer_[length_ - 2]);
        const int low = hexValue(buffer_[length_ - 1]);
        length_ -= 2;
        buffer_[length_] = '\0';
        if (high < 0 || low < 0) return Result::Corrupted;
        const uint8_t expected = static_cast<uint8_t>((high << 4) | low);
        if (crc8(reinterpret_cast<const uint8_t*>(buffer_), length_) != expected) {
            return Result::Corrupted;
        }
        return Result::Line;
    }

    if (!isFrameable(static_cast<char>(byte)) || length_ >= kMaxLineLength + 2) {
        inFrame_ = false;  // garbage or overlong: drop the frame
        skipping_ = true;
        return Result::Corrupted;
    }
    buffer_[length_++] = static_cast<char>(byte);
    return Result::None;
}

bool LineDeframer::abandon() {
    const bool wasInFrame = inFrame_;
    inFrame_ = false;
    skipping_ = false;
    length_ = 0;
    return wasInFrame;
}

}  // namespace optical
