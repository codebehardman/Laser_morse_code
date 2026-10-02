#include "console.hpp"

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "board_config.hpp"

namespace console {

#if LMC_USB_SERIAL

namespace {
constexpr size_t kLineSize = 128;
char lineBuffer[kLineSize];
size_t lineLength = 0;
}  // namespace

void begin() { Serial.begin(config::kConsoleBaud); }

bool enabled() { return true; }

void print(const char* text) { Serial.print(text); }

void print(char c) { Serial.print(c); }

void println(const char* text) { Serial.println(text); }

void printf(const char* format, ...) {
    char buffer[160];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.print(buffer);
}

bool readByte(char& c) {
    if (Serial.available() <= 0) return false;
    c = static_cast<char>(Serial.read());
    return true;
}

void write(char c) { Serial.write(static_cast<uint8_t>(c)); }

bool readLine(char* line, size_t size) {
    while (Serial.available() > 0) {
        const char c = static_cast<char>(Serial.read());
        if (c == '\r' || c == '\n') {
            if (lineLength == 0) continue;  // ignore empty lines / CRLF pairs
            lineBuffer[lineLength] = '\0';
            strncpy(line, lineBuffer, size - 1);
            line[size - 1] = '\0';
            lineLength = 0;
            return true;
        }
        if (c == '\b' || c == 0x7F) {  // backspace
            if (lineLength > 0) --lineLength;
            continue;
        }
        if (lineLength < kLineSize - 1) lineBuffer[lineLength++] = c;
    }
    return false;
}

#else  // !LMC_USB_SERIAL

void begin() {}
bool enabled() { return false; }
void print(const char*) {}
void print(char) {}
void println(const char*) {}
void printf(const char*, ...) {}
bool readByte(char&) { return false; }
void write(char) {}
bool readLine(char*, size_t) { return false; }

#endif

}  // namespace console
