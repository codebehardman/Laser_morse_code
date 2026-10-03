#include "serial_mode.hpp"

#include <stdlib.h>
#include <string.h>

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
bool rxDebug = false;  // print every received byte (/rxdebug on)

void printRxStats(Device& device) {
    const optical::SoftUartRx& rx = device.link.receiver();
    console::printf("[rx: %lu bytes ok, %lu parity errors, %lu framing errors]\r\n",
                    static_cast<unsigned long>(rx.bytesReceived()),
                    static_cast<unsigned long>(rx.parityErrors()),
                    static_cast<unsigned long>(rx.framingErrors()));
}

// Commands handled by this mode itself. Returns false if `line` isn't one.
bool handleModeCommand(Device& device, const char* line) {
    if (strncmp(line, "/baud", 5) == 0) {
        const long baud = strtol(line + 5, nullptr, 10);
        if (baud >= 100 && baud <= 9600) {
            device.link.setBaud(static_cast<uint32_t>(baud));
            console::printf("Link speed set to %ld baud. Set the SAME on the other unit.\r\n", baud);
        } else {
            console::printf("Link speed is %lu baud. Usage: /baud <100-9600> (both units must match)\r\n",
                            static_cast<unsigned long>(device.link.baud()));
        }
        return true;
    }
    if (strncmp(line, "/rxdebug", 8) == 0) {
        rxDebug = strstr(line, "off") == nullptr;
        console::printf("Receive debug %s.\r\n", rxDebug ? "on: every received byte is shown in hex" : "off");
        printRxStats(device);
        return true;
    }
    return false;
}

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
            if (!handleModeCommand(device, input) && !commands::handleSensorLine(device, input)) {
                console::println("In this mode only /baud, /rxdebug, /level and /threshold work. For "
                                 "other commands: press EN (reset) and tap the key for Morse mode.");
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

void handleReceived(Device& device, uint8_t byte) {
    if (rxDebug) {
        const char shown = (byte >= 0x20 && byte < 0x7F) ? static_cast<char>(byte) : '.';
        console::printf("<%02X %c>", byte, shown);
    }
    const optical::LineDeframer::Result result = deframer.push(byte);
    if (rxDebug && result != optical::LineDeframer::Result::None) console::print("\r\n");
    switch (result) {
        case optical::LineDeframer::Result::Line:
            printAboveInput("RX< ", deframer.line());
            break;
        case optical::LineDeframer::Result::Corrupted:
            printAboveInput("", kCorruptedNotice);
            break;
        case optical::LineDeframer::Result::None:
            break;
    }
    if (rxDebug && result != optical::LineDeframer::Result::None) printRxStats(device);
}

}  // namespace

void begin(Device& device) {
    device.link.begin(config::kLinkBaud);
    console::printf("Serial terminal mode: %lu baud 8E1 over the laser.\r\n",
                    static_cast<unsigned long>(config::kLinkBaud));
    console::println("Type a line and press Enter to send it to the other unit.");
    console::println("Hold the key to aim the laser. Press reset to change mode.");
    console::println("Commands here: /baud <n>, /rxdebug on|off, /level, /threshold.");
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
        handleReceived(device, byte);
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
