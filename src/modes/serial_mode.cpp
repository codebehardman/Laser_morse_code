#include "serial_mode.hpp"

#include "../commands.hpp"
#include "../console.hpp"
#include "board_config.hpp"
#include "line_frame.hpp"

namespace serial_mode {
namespace {

constexpr uint32_t kActivityLedMs = 30;

// A frame whose bytes stop arriving for this long is reported as corrupted
// (its end marker was lost). A full 120-character frame takes ~140 ms.
constexpr uint32_t kFrameTimeoutMs = 500;

constexpr const char* kCorruptedNotice = "[message corrupted - ask to resend]";

uint32_t ledOffAtMs = 0;
uint32_t lastRxByteMs = 0;

// The line being typed; sent when Enter is pressed.
char input[optical::kMaxLineLength + 1];
size_t inputLength = 0;
bool previousWasCr = false;

optical::LineDeframer deframer;

// Print a message on its own line without losing what the user is typing.
void printAboveInput(const char* prefix, const char* text) {
    if (inputLength > 0) console::print("\r\n");
    console::print(prefix);
    console::print(text);
    console::print("\r\n");
    for (size_t i = 0; i < inputLength; ++i) console::write(input[i]);
}

void sendInputLine(Device& device) {
    uint8_t frame[optical::kMaxFrameLength];
    const size_t frameLength = optical::encodeLine(input, inputLength, frame);
    console::print("\r\n");
    if (device.link.txSpace() < frameLength) {
        console::println("(link busy - line not sent, try again)");
    } else {
        for (size_t i = 0; i < frameLength; ++i) device.link.write(frame[i]);
    }
    inputLength = 0;
}

void handleTyped(Device& device, char c) {
    const bool isCr = c == '\r';
    const bool isLf = c == '\n';
    if (isLf && previousWasCr) {  // second half of CRLF
        previousWasCr = false;
        return;
    }
    previousWasCr = isCr;

    if (isCr || isLf) {
        if (inputLength > 0 && input[0] == '/') {
            // Commands are never sent as text. Only the sensor ones work here.
            input[inputLength] = '\0';
            console::print("\r\n");
            if (!commands::handleSensorLine(device, input)) {
                console::println("Only /level and /threshold work in this mode. For other commands: "
                                 "press EN (reset) and tap the key for Morse mode.");
            }
            inputLength = 0;
            return;
        }
        sendInputLine(device);
    } else if (c == '\b' || c == 0x7F) {
        if (inputLength > 0) {
            --inputLength;
            if (config::kSerialLocalEcho) console::print("\b \b");
        }
    } else if (optical::isFrameable(c) && inputLength < optical::kMaxLineLength) {
        input[inputLength++] = c;
        if (config::kSerialLocalEcho) console::write(c);
    }
}

void handleReceived(uint8_t byte) {
    switch (deframer.push(byte)) {
        case optical::LineDeframer::Result::Line:
            printAboveInput("RX< ", deframer.line());
            break;
        case optical::LineDeframer::Result::Corrupted:
            printAboveInput("", kCorruptedNotice);
            break;
        case optical::LineDeframer::Result::None:
            break;
    }
}

}  // namespace

void begin(Device& device) {
    device.link.begin(config::kLinkBaud);
    console::printf("Serial terminal mode: %lu baud 8E1 over the laser.\r\n",
                    static_cast<unsigned long>(config::kLinkBaud));
    console::println("Type a line and press Enter to send it to the other unit.");
    console::println("Hold the key to aim the laser. Press reset to change mode.");
    console::println();
}

void update(Device& device) {
    const uint32_t now = millis();
    device.button.update(now);
    device.link.setForceOn(device.button.pressed());

    // PC -> line editor -> laser (whole lines, on Enter).
    char c;
    while (console::readByte(c)) handleTyped(device, c);

    // Laser -> deframer -> PC (only complete, verified lines).
    uint8_t byte;
    while (device.link.read(byte)) {
        handleReceived(byte);
        lastRxByteMs = now;
        device.led.set(true);
        ledOffAtMs = now + kActivityLedMs;
    }
    if (deframer.pending() && now - lastRxByteMs >= kFrameTimeoutMs && deframer.abandon()) {
        printAboveInput("", kCorruptedNotice);
    }
    if (device.led.isOn() && static_cast<int32_t>(now - ledOffAtMs) >= 0) device.led.set(false);
}

}  // namespace serial_mode
