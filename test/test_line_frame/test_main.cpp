// Host-side tests for lib/optical_uart line framing: `pio test -e native`.

#include <unity.h>

#include <cstring>
#include <string>
#include <vector>

#include "line_frame.hpp"

using optical::LineDeframer;
using Result = LineDeframer::Result;

void setUp() {}
void tearDown() {}

namespace {

std::vector<uint8_t> frame(const std::string& text) {
    uint8_t out[optical::kMaxFrameLength];
    const size_t n = optical::encodeLine(text.data(), text.size(), out);
    return std::vector<uint8_t>(out, out + n);
}

// Feed bytes; collect verified lines and count corrupted messages.
struct Received {
    std::vector<std::string> lines;
    int corrupted = 0;
};

Received feed(LineDeframer& d, const std::vector<uint8_t>& bytes) {
    Received r;
    for (uint8_t b : bytes) {
        switch (d.push(b)) {
            case Result::Line: r.lines.emplace_back(d.line(), d.length()); break;
            case Result::Corrupted: ++r.corrupted; break;
            case Result::None: break;
        }
    }
    return r;
}

}  // namespace

void test_crc8_known_value() {
    const char* check = "123456789";  // CRC-8/SMBUS check value is 0xF4
    TEST_ASSERT_EQUAL_HEX8(0xF4, optical::crc8(reinterpret_cast<const uint8_t*>(check), 9));
}

void test_frame_layout() {
    const std::vector<uint8_t> f = frame("hi");
    TEST_ASSERT_EQUAL(6, f.size());
    TEST_ASSERT_EQUAL_HEX8(0x02, f.front());
    TEST_ASSERT_EQUAL_HEX8(0x03, f.back());
    TEST_ASSERT_EQUAL('h', f[1]);
    TEST_ASSERT_EQUAL('i', f[2]);
}

void test_round_trip_lines() {
    LineDeframer d;
    std::vector<uint8_t> bytes;
    for (const char* s : {"hello world", "", "Testing 1, 2, 3!", "tab\there"}) {
        const std::vector<uint8_t> f = frame(s);
        bytes.insert(bytes.end(), f.begin(), f.end());
    }
    const Received r = feed(d, bytes);
    TEST_ASSERT_EQUAL(0, r.corrupted);
    TEST_ASSERT_EQUAL(4, r.lines.size());
    TEST_ASSERT_EQUAL_STRING("hello world", r.lines[0].c_str());
    TEST_ASSERT_EQUAL_STRING("", r.lines[1].c_str());
    TEST_ASSERT_EQUAL_STRING("Testing 1, 2, 3!", r.lines[2].c_str());
    TEST_ASSERT_EQUAL_STRING("tab\there", r.lines[3].c_str());
}

void test_changed_character_is_corrupted() {
    LineDeframer d;
    std::vector<uint8_t> f = frame("hello");
    f[3] = 'X';  // still printable, so only the CRC can catch it
    const Received r = feed(d, f);
    TEST_ASSERT_EQUAL(0, r.lines.size());
    TEST_ASSERT_EQUAL(1, r.corrupted);
}

void test_lost_byte_is_corrupted() {
    LineDeframer d;
    std::vector<uint8_t> f = frame("hello");
    f.erase(f.begin() + 2);  // a byte dropped by the parity check
    const Received r = feed(d, f);
    TEST_ASSERT_EQUAL(0, r.lines.size());
    TEST_ASSERT_EQUAL(1, r.corrupted);
}

void test_lost_start_reports_and_next_line_survives() {
    LineDeframer d;
    std::vector<uint8_t> bytes = frame("first");
    bytes.erase(bytes.begin());  // STX lost
    const std::vector<uint8_t> second = frame("second");
    bytes.insert(bytes.end(), second.begin(), second.end());
    const Received r = feed(d, bytes);
    TEST_ASSERT_EQUAL(1, r.corrupted);
    TEST_ASSERT_EQUAL(1, r.lines.size());
    TEST_ASSERT_EQUAL_STRING("second", r.lines[0].c_str());
}

void test_lost_end_reports_and_next_line_survives() {
    LineDeframer d;
    std::vector<uint8_t> bytes = frame("first");
    bytes.pop_back();  // ETX lost
    const std::vector<uint8_t> second = frame("second");
    bytes.insert(bytes.end(), second.begin(), second.end());
    const Received r = feed(d, bytes);
    TEST_ASSERT_EQUAL(1, r.corrupted);
    TEST_ASSERT_EQUAL(1, r.lines.size());
    TEST_ASSERT_EQUAL_STRING("second", r.lines[0].c_str());
}

void test_garbage_byte_is_corrupted() {
    LineDeframer d;
    std::vector<uint8_t> f = frame("hello");
    f[2] = 0xB7;
    const Received r = feed(d, f);
    TEST_ASSERT_EQUAL(0, r.lines.size());
    TEST_ASSERT_EQUAL(1, r.corrupted);
}

void test_abandon_partial_frame() {
    LineDeframer d;
    std::vector<uint8_t> f = frame("hello");
    f.pop_back();
    feed(d, f);
    TEST_ASSERT_TRUE(d.pending());
    TEST_ASSERT_TRUE(d.abandon());
    TEST_ASSERT_FALSE(d.abandon());
}

void test_frame_missing_both_ends_is_not_silent() {
    LineDeframer d;
    std::vector<uint8_t> f = frame("hello");
    f.erase(f.begin());  // STX lost
    f.pop_back();        // ETX lost
    const Received r = feed(d, f);
    TEST_ASSERT_EQUAL(0, r.lines.size());
    TEST_ASSERT_TRUE(d.pending());
    TEST_ASSERT_TRUE(d.abandon());  // reported after the timeout
}

void test_long_line_is_truncated_to_limit() {
    const std::string longText(200, 'a');
    LineDeframer d;
    const Received r = feed(d, frame(longText));
    TEST_ASSERT_EQUAL(1, r.lines.size());
    TEST_ASSERT_EQUAL(optical::kMaxLineLength, r.lines[0].size());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_crc8_known_value);
    RUN_TEST(test_frame_layout);
    RUN_TEST(test_round_trip_lines);
    RUN_TEST(test_changed_character_is_corrupted);
    RUN_TEST(test_lost_byte_is_corrupted);
    RUN_TEST(test_lost_start_reports_and_next_line_survives);
    RUN_TEST(test_lost_end_reports_and_next_line_survives);
    RUN_TEST(test_garbage_byte_is_corrupted);
    RUN_TEST(test_abandon_partial_frame);
    RUN_TEST(test_frame_missing_both_ends_is_not_silent);
    RUN_TEST(test_long_line_is_truncated_to_limit);
    return UNITY_END();
}
