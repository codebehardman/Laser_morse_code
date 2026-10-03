#pragma once

// Line framing for the serial terminal mode: each line typed is sent as one
// message, and the receiver only shows it if it arrived complete and intact.
//
//   STX (0x02) | text (printable ASCII) | CRC-8 as 2 hex digits | ETX (0x03)
//
// The CRC is sent as hex text so that the frame bytes never collide with
// STX/ETX. Per-byte parity (soft_uart) drops single-bit errors; the CRC
// catches what parity misses (2-bit errors, lost bytes, lost frame edges).

#include <cstddef>
#include <cstdint>

namespace optical {

constexpr uint8_t kFrameStart = 0x02;  // STX
constexpr uint8_t kFrameEnd = 0x03;    // ETX
constexpr size_t kMaxLineLength = 120;
constexpr size_t kMaxFrameLength = kMaxLineLength + 4;

// CRC-8, polynomial 0x07 (CRC-8/SMBUS).
uint8_t crc8(const uint8_t* data, size_t length);

// Characters that can be sent: printable ASCII and tab.
constexpr bool isFrameable(char c) { return (c >= 0x20 && c <= 0x7E) || c == '\t'; }

// Builds the frame for `text` (only frameable characters, at most
// kMaxLineLength) into `out`, which must hold kMaxFrameLength bytes.
// Returns the frame length.
size_t encodeLine(const char* text, size_t length, uint8_t* out);

// Reassembles frames from received bytes.
class LineDeframer {
public:
    enum class Result {
        None,       // nothing complete yet
        Line,       // line() holds a verified line
        Corrupted,  // a damaged or incomplete message was discarded
    };

    Result push(uint8_t byte);

    // Give up on a frame that stopped arriving (call after a timeout).
    // Returns true if a partial frame or stray bytes were discarded.
    bool abandon();

    // True while a frame (or stray bytes of one whose start was lost) is
    // still incomplete.
    bool pending() const { return inFrame_ || strayBytes_; }
    const char* line() const { return buffer_; }
    size_t length() const { return length_; }

private:
    char buffer_[kMaxLineLength + 3] = {};  // text + 2 CRC digits + NUL
    size_t length_ = 0;
    bool inFrame_ = false;
    bool skipping_ = false;    // rest of an already-reported broken frame
    bool strayBytes_ = false;  // bytes received outside any frame
};

}  // namespace optical
