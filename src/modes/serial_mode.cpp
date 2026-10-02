#include "serial_mode.hpp"

#include "../console.hpp"
#include "board_config.hpp"

namespace serial_mode {
namespace {

constexpr uint32_t kActivityLedMs = 30;
constexpr uint32_t kErrorChirpMs = 30;

uint32_t ledOffAtMs = 0;
uint32_t buzzerOffAtMs = 0;
uint32_t lastParityErrors = 0;

// Terminals disagree on line endings (CR, LF or CRLF). The bytes on the link
// are left untouched; only what is printed locally is normalised to CRLF so
// every line shows up on its own line.
class TerminalWriter {
public:
    void write(char c) {
        if (c == '\n' && previous_ == '\r') {
            // Second half of a CRLF already printed.
        } else if (c == '\r' || c == '\n') {
            console::write('\r');
            console::write('\n');
        } else {
            console::write(c);
        }
        previous_ = c;
    }

private:
    char previous_ = '\0';
};

TerminalWriter echoWriter;
TerminalWriter receiveWriter;

}  // namespace

void begin(Device& device) {
    device.link.begin(config::kLinkBaud);
    console::printf("Serial terminal mode: %lu baud 8E1 over the laser.\r\n",
                    static_cast<unsigned long>(config::kLinkBaud));
    console::println("Everything typed here appears on the other unit's terminal.");
    console::println("Hold the key to aim the laser. Press reset to change mode.");
    console::println();
}

void update(Device& device) {
    const uint32_t now = millis();
    device.button.update(now);
    device.link.setForceOn(device.button.pressed());

    // PC -> laser. Stop reading while the transmit queue is full; the USB
    // side then buffers until the 9600-baud link catches up.
    char c;
    while (device.link.txSpace() > 0 && console::readByte(c)) {
        device.link.write(static_cast<uint8_t>(c));
        if (config::kSerialLocalEcho) echoWriter.write(c);
    }

    // Laser -> PC.
    uint8_t byte;
    while (device.link.read(byte)) {
        receiveWriter.write(static_cast<char>(byte));
        digitalWrite(LMC_PIN_STATUS_LED, HIGH);
        ledOffAtMs = now + kActivityLedMs;
    }
    if (static_cast<int32_t>(now - ledOffAtMs) >= 0) digitalWrite(LMC_PIN_STATUS_LED, LOW);

    // Chirp when a corrupted byte was dropped.
    const uint32_t parityErrors = device.link.receiver().parityErrors();
    if (parityErrors != lastParityErrors) {
        lastParityErrors = parityErrors;
        device.buzzer.set(true);
        buzzerOffAtMs = now + kErrorChirpMs;
    }
    if (device.buzzer.isOn() && static_cast<int32_t>(now - buzzerOffAtMs) >= 0) {
        device.buzzer.set(false);
    }
}

}  // namespace serial_mode
