// Host-side tests for lib/morse: run with `pio test -e native`.

#include <unity.h>

#include <cstdlib>
#include <cstring>
#include <string>

#include "morse_code.hpp"
#include "morse_decoder.hpp"
#include "morse_transmitter.hpp"

using morse::MorseDecoder;
using morse::MorseTransmitter;

void setUp() {}
void tearDown() {}

namespace {

// Run a transmitter straight into a decoder with a simulated 1 ms clock.
std::string loopback(const char* text, uint32_t txUnitMs, uint32_t rxInitialUnitMs) {
    MorseTransmitter tx(txUnitMs);
    MorseDecoder rx(rxInitialUnitMs);
    tx.enqueue(text);

    std::string out;
    uint32_t now = 1000;
    bool started = false;
    // Keep running until the transmitter is idle and the decoder has flushed.
    for (uint32_t idleMs = 0; idleMs < 20 * txUnitMs; ++now) {
        const bool key = tx.update(now);
        rx.update(key, now);
        started = started || key;
        idleMs = (started && !tx.busy()) ? idleMs + 1 : 0;
        char c;
        while (rx.read(c)) out += c;
    }
    return out;
}

std::string trimRight(std::string s) {
    while (!s.empty() && s.back() == ' ') s.pop_back();
    return s;
}

// Feed the decoder a hand-keyed sequence. Elements: '.', '-', ' ' (char
// gap), '/' (word gap). Each duration is jittered by up to +/- jitterPct.
std::string keyByHand(const char* sequence, uint32_t unitMs, int jitterPct, uint32_t seed) {
    std::srand(seed);
    auto jitter = [&](uint32_t ms) {
        const int pct = jitterPct == 0 ? 0 : (std::rand() % (2 * jitterPct + 1)) - jitterPct;
        return static_cast<uint32_t>(static_cast<int>(ms) * (100 + pct) / 100);
    };

    MorseDecoder rx(100);  // deliberately not the sender's speed
    std::string out;
    uint32_t now = 0;
    auto hold = [&](bool light, uint32_t ms) {
        for (uint32_t end = now + ms; now < end; ++now) {
            rx.update(light, now);
            char c;
            while (rx.read(c)) out += c;
        }
    };

    hold(false, 10 * unitMs);
    for (const char* p = sequence; *p != '\0'; ++p) {
        switch (*p) {
            case '.': hold(true, jitter(unitMs)); hold(false, jitter(unitMs)); break;
            case '-': hold(true, jitter(3 * unitMs)); hold(false, jitter(unitMs)); break;
            case ' ': hold(false, jitter(2 * unitMs)); break;  // +1 already = 3
            case '/': hold(false, jitter(6 * unitMs)); break;  // +1 already = 7
        }
    }
    hold(false, 20 * unitMs);
    return out;
}

}  // namespace

void test_encode_letters_and_digits() {
    TEST_ASSERT_EQUAL_STRING(".-", morse::encode('A'));
    TEST_ASSERT_EQUAL_STRING(".-", morse::encode('a'));
    TEST_ASSERT_EQUAL_STRING("...", morse::encode('S'));
    TEST_ASSERT_EQUAL_STRING("---", morse::encode('O'));
    TEST_ASSERT_EQUAL_STRING("-----", morse::encode('0'));
    TEST_ASSERT_EQUAL_STRING("..--..", morse::encode('?'));
    TEST_ASSERT_NULL(morse::encode('#'));
    TEST_ASSERT_NULL(morse::encode(' '));
}

void test_decode_round_trip_whole_table() {
    const char* symbols = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?'!/()&:;=+-_\"$@";
    for (const char* s = symbols; *s != '\0'; ++s) {
        const char* pattern = morse::encode(*s);
        TEST_ASSERT_NOT_NULL(pattern);
        TEST_ASSERT_TRUE(strlen(pattern) <= morse::kMaxPatternLength);
        TEST_ASSERT_EQUAL_CHAR(*s, morse::decode(pattern));
    }
    TEST_ASSERT_EQUAL_CHAR('\0', morse::decode("........"));
    TEST_ASSERT_EQUAL_CHAR('\0', morse::decode(""));
}

void test_transmitter_timing_for_letter_A() {
    // A = .-  -> on 1u, off 1u, on 3u, then trailing gap.
    MorseTransmitter tx(10);
    tx.enqueue("A");
    std::string trace;
    for (uint32_t t = 0; t < 60; ++t) trace += tx.update(t) ? '#' : '_';
    TEST_ASSERT_EQUAL_STRING(
        "##########__________##############################"
        "__________",
        trace.c_str());
}

void test_transmitter_skips_unsupported_characters() {
    TEST_ASSERT_EQUAL_STRING("SOS", trimRight(loopback("S#O~S", 50, 50)).c_str());
}

void test_loopback_sos() {
    TEST_ASSERT_EQUAL_STRING("SOS", trimRight(loopback("SOS", 100, 100)).c_str());
}

void test_loopback_sentence_with_word_gaps() {
    TEST_ASSERT_EQUAL_STRING("HELLO WORLD 123",
                             trimRight(loopback("hello world 123", 60, 60)).c_str());
}

void test_decoder_adapts_to_faster_sender() {
    // Sender at 20 WPM (60 ms) while the decoder starts at 12 WPM (100 ms).
    // A leading "PARIS" lets the speed estimate settle.
    const std::string out = trimRight(loopback("PARIS PARIS TEST", 60, 100));
    TEST_ASSERT_EQUAL_STRING("TEST", out.substr(out.size() - 4).c_str());
}

void test_decoder_handles_human_jitter() {
    // "CQ DE" keyed at ~10 WPM with +/-20% timing error.
    const std::string out = keyByHand("-.-. --.-/-.. .", 120, 20, 42);
    TEST_ASSERT_EQUAL_STRING("CQ DE", trimRight(out).c_str());
}

void test_decoder_ignores_aim_hold() {
    MorseDecoder rx(100);
    std::string out;
    uint32_t now = 0;
    auto hold = [&](bool light, uint32_t ms) {
        for (uint32_t end = now + ms; now < end; ++now) {
            rx.update(light, now);
            char c;
            while (rx.read(c)) out += c;
        }
    };
    hold(false, 500);
    hold(true, 3000);  // operator holds the key to aim
    hold(false, 1000);
    hold(true, 100);   // E
    hold(false, 1000);
    TEST_ASSERT_EQUAL_STRING("E", trimRight(out).c_str());
}

void test_decoder_reports_unknown_pattern() {
    // Eight dots is not a valid character.
    const std::string out = keyByHand("........", 100, 0, 1);
    TEST_ASSERT_EQUAL_STRING("*", trimRight(out).c_str());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_encode_letters_and_digits);
    RUN_TEST(test_decode_round_trip_whole_table);
    RUN_TEST(test_transmitter_timing_for_letter_A);
    RUN_TEST(test_transmitter_skips_unsupported_characters);
    RUN_TEST(test_loopback_sos);
    RUN_TEST(test_loopback_sentence_with_word_gaps);
    RUN_TEST(test_decoder_adapts_to_faster_sender);
    RUN_TEST(test_decoder_handles_human_jitter);
    RUN_TEST(test_decoder_ignores_aim_hold);
    RUN_TEST(test_decoder_reports_unknown_pattern);
    return UNITY_END();
}
