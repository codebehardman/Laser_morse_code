#include "commands.hpp"

#include <stdlib.h>
#include <string.h>

#include "board_config.hpp"
#include "console.hpp"
#include "diagnostics.hpp"
#include "morse_code.hpp"

namespace commands {
namespace {

constexpr size_t kMaxArgs = 4;

// Split `line` (modified in place) into whitespace-separated arguments.
// (Not strtok: newlib's strtok drags in assert/fprintf, which needs a UART.)
size_t tokenize(char* line, char* argv[], size_t maxArgs) {
    size_t argc = 0;
    char* p = line;
    while (*p != '\0' && argc < maxArgs) {
        while (*p == ' ' || *p == '\t') *p++ = '\0';
        if (*p == '\0') break;
        argv[argc++] = p;
        while (*p != '\0' && *p != ' ' && *p != '\t') ++p;
    }
    return argc;
}

bool parseOnOff(const char* arg, bool& value) {
    if (arg == nullptr) return false;
    if (strcmp(arg, "on") == 0) {
        value = true;
        return true;
    }
    if (strcmp(arg, "off") == 0) {
        value = false;
        return true;
    }
    return false;
}

uint32_t parseNumber(const char* arg, uint32_t fallback) {
    if (arg == nullptr) return fallback;
    const long value = strtol(arg, nullptr, 10);
    return value > 0 ? static_cast<uint32_t>(value) : fallback;
}

void printStatus(Device& device) {
    console::printf("TX speed   : %lu WPM (%lu ms unit)\n",
                    static_cast<unsigned long>(device.settings.wpm),
                    static_cast<unsigned long>(device.transmitter.unitMs()));
    console::printf("RX unit    : %lu ms (adaptive, ~%lu WPM)\n",
                    static_cast<unsigned long>(device.decoder.unitMs()),
                    static_cast<unsigned long>(1200 / device.decoder.unitMs()));
    console::printf("Sidetone   : %s\n", device.settings.sidetone ? "on" : "off");
    console::printf("Aim mode   : %s\n", device.settings.aim ? "on" : "off");
    console::printf("Receiver   : %s\n", device.sensor.lightDetected() ? "LIGHT" : "dark");
    console::printf("TX queue   : %s\n", device.transmitter.busy() ? "sending" : "idle");
    console::printf("RX symbols : %lu marks, %lu unknown patterns\n",
                    static_cast<unsigned long>(device.decoder.marksSeen()),
                    static_cast<unsigned long>(device.decoder.unknownPatterns()));
}

void printMorseTable() {
    const char* symbols = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,?'!/()&:;=+-_\"$@";
    for (const char* s = symbols; *s != '\0'; ++s) {
        console::printf("  %c  %-8s", *s, morse::encode(*s));
        if ((s - symbols) % 4 == 3) console::println();
    }
    console::println();
}

void runTest(Device& device, size_t argc, char* argv[]) {
    const char* which = argc > 1 ? argv[1] : "";
    if (strcmp(which, "laser") == 0) {
        diagnostics::testLaser(device);
    } else if (strcmp(which, "sensor") == 0) {
        diagnostics::testSensor(device);
    } else if (strcmp(which, "button") == 0) {
        diagnostics::testButton(device);
    } else if (strcmp(which, "buzzer") == 0) {
        diagnostics::testBuzzer(device);
    } else if (strcmp(which, "tx") == 0) {
        diagnostics::sendPulses(device, parseNumber(argc > 2 ? argv[2] : nullptr, 100));
    } else if (strcmp(which, "rx") == 0) {
        diagnostics::countPulses(device, parseNumber(argc > 2 ? argv[2] : nullptr, 100));
    } else if (strcmp(which, "all") == 0) {
        diagnostics::testBuzzer(device);
        diagnostics::testLaser(device);
        diagnostics::testButton(device);
        diagnostics::testSensor(device);
    } else {
        console::println("Usage: /test laser|sensor|button|buzzer|all|tx [n]|rx [n]");
        return;
    }
    // Anything the decoder picked up during a test is noise for the chat.
    device.decoder.reset();
}

}  // namespace

void printHelp() {
    console::println("Laser Morse transceiver - type text and press Enter to send it.");
    console::println("Commands:");
    console::println("  /help               this help");
    console::println("  /status             show settings and receiver state");
    console::println("  /wpm <n>            transmit speed in words per minute (5-30)");
    console::println("  /sidetone on|off    beep locally while transmitting");
    console::println("  /aim on|off         hold the laser on to align the two units");
    console::println("  /stop               abort the current transmission");
    console::println("  /table              print the Morse alphabet");
    console::println("  /test laser         blink the laser 5 times");
    console::println("  /test sensor        report light/dark changes for 10 s");
    console::println("  /test button        report key presses for 10 s");
    console::println("  /test buzzer        beep pattern");
    console::println("  /test all           buzzer, laser, button and sensor tests");
    console::println("  /test rx [n]        link test: count pulses from the other unit");
    console::println("  /test tx [n]        link test: send n pulses (run rx on the other unit first)");
}

void applyWpm(Device& device, uint32_t wpm) {
    if (wpm < config::kMinWpm) wpm = config::kMinWpm;
    if (wpm > config::kMaxWpm) wpm = config::kMaxWpm;
    device.settings.wpm = wpm;
    device.transmitter.setUnitMs(morse::wpmToUnitMs(wpm));
}

void handleLine(Device& device, const char* line) {
    if (line[0] != '/') {
        const size_t queued = device.transmitter.enqueue(line);
        device.transmitter.enqueue(' ');  // word gap between lines
        console::printf("TX> %s\n", line);
        if (queued < strlen(line)) console::println("(transmit queue full - message truncated)");
        return;
    }

    char buffer[128];
    strncpy(buffer, line + 1, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    char* argv[kMaxArgs] = {};
    const size_t argc = tokenize(buffer, argv, kMaxArgs);
    if (argc == 0) return;
    const char* cmd = argv[0];

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        printHelp();
    } else if (strcmp(cmd, "status") == 0) {
        printStatus(device);
    } else if (strcmp(cmd, "wpm") == 0) {
        applyWpm(device, parseNumber(argc > 1 ? argv[1] : nullptr, device.settings.wpm));
        console::printf("TX speed set to %lu WPM\n", static_cast<unsigned long>(device.settings.wpm));
    } else if (strcmp(cmd, "sidetone") == 0) {
        if (!parseOnOff(argc > 1 ? argv[1] : nullptr, device.settings.sidetone)) {
            console::println("Usage: /sidetone on|off");
        }
    } else if (strcmp(cmd, "aim") == 0) {
        if (!parseOnOff(argc > 1 ? argv[1] : nullptr, device.settings.aim)) {
            console::println("Usage: /aim on|off");
        }
    } else if (strcmp(cmd, "stop") == 0) {
        device.transmitter.clear();
        console::println("Transmission aborted.");
    } else if (strcmp(cmd, "table") == 0) {
        printMorseTable();
    } else if (strcmp(cmd, "test") == 0) {
        device.laser.set(false);
        device.buzzer.set(false);
        runTest(device, argc, argv);
    } else {
        console::printf("Unknown command '/%s' - type /help\n", cmd);
    }
}

}  // namespace commands
