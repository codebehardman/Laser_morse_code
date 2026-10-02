// Host-side tests for lib/optical_uart: run with `pio test -e native`.

#include <unity.h>

#include <string>
#include <vector>

#include "soft_uart.hpp"

using optical::kOversample;
using optical::SoftUartRx;
using optical::SoftUartTx;

void setUp() {}
void tearDown() {}

namespace {

// Line waveform at one sample per tick, padded with idle on both sides.
std::vector<bool> transmit(const std::string& data) {
    SoftUartTx tx;
    for (char c : data) tx.write(static_cast<uint8_t>(c));
    std::vector<bool> line(5 * kOversample, true);
    while (tx.busy()) line.push_back(tx.tick());
    line.insert(line.end(), 5 * kOversample, true);
    return line;
}

// Sample `line` with a receiver whose tick is `phase` (0..1 tick) late and
// whose clock runs `drift` faster (e.g. 0.02 = 2%).
std::string receive(const std::vector<bool>& line, SoftUartRx& rx, double phase = 0.0,
                    double drift = 0.0) {
    for (double t = phase; t < line.size(); t += 1.0 / (1.0 + drift)) {
        rx.tick(line[static_cast<size_t>(t)]);
    }
    std::string out;
    uint8_t byte;
    while (rx.read(byte)) out += static_cast<char>(byte);
    return out;
}

std::string allBytes() {
    std::string s;
    for (int i = 0; i < 256; ++i) s += static_cast<char>(i);
    return s;
}

}  // namespace

void test_parity_bit() {
    TEST_ASSERT_EQUAL_UINT8(0, optical::evenParityBit(0x00));
    TEST_ASSERT_EQUAL_UINT8(1, optical::evenParityBit(0x01));
    TEST_ASSERT_EQUAL_UINT8(0, optical::evenParityBit(0x03));
    TEST_ASSERT_EQUAL_UINT8(0, optical::evenParityBit('A'));  // 0b01000001
    TEST_ASSERT_EQUAL_UINT8(1, optical::evenParityBit('C'));  // 0b01000011
    TEST_ASSERT_EQUAL_UINT8(0, optical::evenParityBit(0xFF));
}

void test_frame_shape_for_A() {
    // 'A' = 0x41 = 0b01000001, parity 0: start, 1,0,0,0,0,0,1,0, parity 0, stop
    SoftUartTx tx;
    tx.write('A');
    std::string bits;
    for (uint32_t i = 0; i < 11; ++i) {
        const bool level = tx.tick();
        for (uint32_t s = 1; s < kOversample; ++s) TEST_ASSERT_EQUAL(level, tx.tick());
        bits += level ? '1' : '0';
    }
    TEST_ASSERT_EQUAL_STRING("01000001001", bits.c_str());
    TEST_ASSERT_FALSE(tx.busy());
    TEST_ASSERT_TRUE(tx.tick());  // idle line is mark
}

void test_loopback_all_byte_values() {
    SoftUartRx rx;
    const std::string data = allBytes();
    TEST_ASSERT_TRUE(receive(transmit(data), rx) == data);
    TEST_ASSERT_EQUAL_UINT32(256, rx.bytesReceived());
    TEST_ASSERT_EQUAL_UINT32(0, rx.parityErrors());
    TEST_ASSERT_EQUAL_UINT32(0, rx.framingErrors());
}

void test_loopback_any_phase() {
    const std::string msg = "Hello, NRQZ!\r\n";
    const std::vector<bool> line = transmit(msg);
    for (int i = 0; i < 10; ++i) {
        SoftUartRx rx;
        TEST_ASSERT_EQUAL_STRING(msg.c_str(), receive(line, rx, i / 10.0).c_str());
    }
}

void test_loopback_with_clock_drift() {
    const std::string msg = "The quick brown fox jumps over the lazy dog 0123456789";
    const std::vector<bool> line = transmit(msg);
    for (double drift : {-0.03, -0.01, 0.01, 0.03}) {
        SoftUartRx rx;
        TEST_ASSERT_EQUAL_STRING(msg.c_str(), receive(line, rx, 0.5, drift).c_str());
    }
}

void test_single_bit_error_is_detected_and_dropped() {
    std::vector<bool> line = transmit("ABC");
    // Flip data bit 3 of the second frame ('B').
    const size_t frame2 = 5 * kOversample + 11 * kOversample;
    for (uint32_t s = 0; s < kOversample; ++s) {
        const size_t i = frame2 + 4 * kOversample + s;  // bit index 4 = data bit 3
        line[i] = !line[i];
    }
    SoftUartRx rx;
    TEST_ASSERT_EQUAL_STRING("AC", receive(line, rx).c_str());
    TEST_ASSERT_EQUAL_UINT32(1, rx.parityErrors());
}

void test_short_glitch_is_ignored() {
    std::vector<bool> line(40 * kOversample, true);
    line[100] = false;  // one-tick flash
    const std::vector<bool> msg = transmit("ok");
    line.insert(line.end(), msg.begin(), msg.end());
    SoftUartRx rx;
    TEST_ASSERT_EQUAL_STRING("ok", receive(line, rx).c_str());
}

void test_held_laser_produces_no_output() {
    // Laser held on (aiming) for 50 bit times, then a normal message.
    std::vector<bool> line(10 * kOversample, true);
    line.insert(line.end(), 50 * kOversample, false);
    const std::vector<bool> msg = transmit("hi");
    line.insert(line.end(), msg.begin(), msg.end());
    SoftUartRx rx;
    TEST_ASSERT_EQUAL_STRING("hi", receive(line, rx).c_str());
    TEST_ASSERT_EQUAL_UINT32(1, rx.framingErrors());
}

void test_tx_queue_reports_space() {
    SoftUartTx tx;
    const size_t initial = tx.space();
    TEST_ASSERT_TRUE(initial > 0);
    tx.write('x');
    TEST_ASSERT_EQUAL(initial - 1, tx.space());
    for (size_t i = 0; i < initial - 1; ++i) TEST_ASSERT_TRUE(tx.write('x'));
    TEST_ASSERT_FALSE(tx.write('x'));
    TEST_ASSERT_EQUAL(0, tx.space());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_parity_bit);
    RUN_TEST(test_frame_shape_for_A);
    RUN_TEST(test_loopback_all_byte_values);
    RUN_TEST(test_loopback_any_phase);
    RUN_TEST(test_loopback_with_clock_drift);
    RUN_TEST(test_single_bit_error_is_detected_and_dropped);
    RUN_TEST(test_short_glitch_is_ignored);
    RUN_TEST(test_held_laser_produces_no_output);
    RUN_TEST(test_tx_queue_reports_space);
    return UNITY_END();
}
