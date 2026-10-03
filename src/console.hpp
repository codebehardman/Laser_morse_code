#pragma once

#include <stddef.h>

// Serial console over the board's USB port.
namespace console {

void begin();

void print(const char* text);
void print(char c);
void println(const char* text = "");
void printf(const char* format, ...) __attribute__((format(printf, 1, 2)));

// Raw byte I/O, used by the serial terminal mode.
bool readByte(char& c);
void write(char c);

// Non-blocking line reader. Returns true and fills `line` (NUL-terminated,
// without the line ending) once a full line has been received.
bool readLine(char* line, size_t size);

}  // namespace console
