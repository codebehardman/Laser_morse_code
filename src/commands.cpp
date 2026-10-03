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
    console::printf("Aim mode   : %s\n", device.settings.aim ? "on" : "off");
    console::printf("Receiver   : %s (level %u, threshold %u)\n",
                    device.sensor.lightDetected() ? "LIGHT" : "dark", device.sensor.readLevel(),
                    device.sensor.threshold());
    console::printf("TX queue   : %s\n", device.transmitter.busy() ? "sending" : "idle");
    console::printf("RX symbols : %lu marks, %lu unknown patterns\n",
                    static_cast<unsigned long>(device.decoder.marksSeen()),
                    static_cast<unsigned long>(device.decoder.unknownPatterns()));
}

// Split "/cmd args..." into argv (pointing into `buffer`). Returns argc.
size_t parseCommand(const char* line, char* buffer, size_t bufferSize, char* argv[]) {
    strncpy(buffer, line + 1, bufferSize - 1);
    buffer[bufferSize - 1] = '\0';
    return tokenize(buffer, argv, kMaxArgs);
}

void printThreshold(Device& device) {
    console::printf("Threshold %u (level below it = laser). Current level %u. "
                    "Presets: dark room %u, bright room %u.\n",
                    device.sensor.threshold(), device.sensor.readLevel(),
                    config::kThresholdDarkRoom, config::kThresholdBrightRoom);
}

// /level and /threshold, available in both modes. Returns false if `cmd`
// is not one of them.
bool handleSensorCommand(Device& device, size_t argc, char* argv[]) {
    const char* cmd = argv[0];
    if (strcmp(cmd, "level") == 0) {
        diagnostics::showLevel(device);
        return true;
    }
    if (strcmp(cmd, "threshold") != 0) return false;

    const char* arg = argc > 1 ? argv[1] : nullptr;
    if (arg == nullptr) {
        printThreshold(device);
    } else if (strcmp(arg, "dark") == 0) {
        device.sensor.setThreshold(config::kThresholdDarkRoom);
        printThreshold(device);
    } else if (strcmp(arg, "bright") == 0) {
        device.sensor.setThreshold(config::kThresholdBrightRoom);
        printThreshold(device);
    } else if (strcmp(arg, "auto") == 0) {
        diagnostics::calibrateThreshold(device);
    } else {
        const uint32_t value = parseNumber(arg, 0);
        if (value == 0 || value > PhotoSensor::kMaxLevel) {
            console::println("Usage: /threshold [1-4095 | dark | bright | auto]");
        } else {
            device.sensor.setThreshold(static_cast<uint16_t>(value));
            printThreshold(device);
        }
    }
    return true;
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
    } else if (strcmp(which, "tx") == 0) {
        diagnostics::sendPulses(device, parseNumber(argc > 2 ? argv[2] : nullptr, 100),
                                parseNumber(argc > 3 ? argv[3] : nullptr, 20));
    } else if (strcmp(which, "edge") == 0) {
        diagnostics::measureEdges(device);
    } else if (strcmp(which, "rx") == 0) {
        diagnostics::countPulses(device, parseNumber(argc > 2 ? argv[2] : nullptr, 100));
    } else if (strcmp(which, "all") == 0) {
        diagnostics::testLaser(device);
        diagnostics::testButton(device);
        diagnostics::testSensor(device);
    } else {
        console::println("Usage: /test laser|sensor|button|all|tx [n] [ms]|rx [n]|edge");
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
    console::println("  /wpm <n>            transmit speed in words per minute (5-40)");
    console::println("  /aim on|off         hold the laser on to align the two units");
    console::println("  /stop               abort the current transmission");
    console::println("  /table              print the Morse alphabet");
    console::println("  /level              show the live light-sensor reading for 5 s");
    console::println("  /threshold [n]      show/set the light threshold (0-4095, below = laser)");
    console::println("  /threshold dark|bright   use the dark-room or bright-room preset");
    console::println("  /threshold auto     measure the room and the other laser, set it in between");
    console::println("  /test laser         blink the laser 5 times");
    console::println("  /test sensor        report light/dark changes for 10 s");
    console::println("  /test button        report key presses for 10 s");
    console::println("  /test all           laser, button and sensor tests");
    console::println("  /test rx [n]        link test: count pulses from the other unit");
    console::println("  /test tx [n] [ms]   link test: send n pulses of ms each (default 100 x 20 ms)");
    console::println("  /test edge          receiver speed: run this, then '/test tx 200 1' on the other unit");
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
        // Messages start with the start prosign, then a letter gap.
        const uint32_t units =
            morse::kStartProsignUnits + morse::kCharGapUnits + morse::messageUnits(line);
        const uint32_t seconds = (units * device.transmitter.unitMs() + 999) / 1000;
        console::printf("TX> %s  (sending as Morse at %lu WPM, ~%lu s)\n", line,
                        static_cast<unsigned long>(device.settings.wpm),
                        static_cast<unsigned long>(seconds));
        if (queued < strlen(line)) console::println("(transmit queue full - message truncated)");
        return;
    }

    char buffer[128];
    char* argv[kMaxArgs] = {};
    const size_t argc = parseCommand(line, buffer, sizeof(buffer), argv);
    if (argc == 0) return;
    const char* cmd = argv[0];

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        printHelp();
    } else if (strcmp(cmd, "status") == 0) {
        printStatus(device);
    } else if (strcmp(cmd, "wpm") == 0) {
        applyWpm(device, parseNumber(argc > 1 ? argv[1] : nullptr, device.settings.wpm));
        console::printf("TX speed set to %lu WPM\n", static_cast<unsigned long>(device.settings.wpm));
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
        device.led.set(false);
        runTest(device, argc, argv);
    } else if (!handleSensorCommand(device, argc, argv)) {
        console::printf("Unknown command '/%s' - type /help\n", cmd);
    }
}

bool handleSensorLine(Device& device, const char* line) {
    char buffer[128];
    char* argv[kMaxArgs] = {};
    const size_t argc = parseCommand(line, buffer, sizeof(buffer), argv);
    return argc > 0 && handleSensorCommand(device, argc, argv);
}

}  // namespace commands
