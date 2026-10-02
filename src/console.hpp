#pragma once

#include <stddef.h>

// USB serial console (ST-LINK virtual COM port). When the firmware is built
// without LMC_USB_SERIAL every function is a no-op, so callers never need to
// check whether the console exists.
namespace console {

void begin();
bool enabled();

void print(const char* text);
void print(char c);
void println(const char* text = "");
void printf(const char* format, ...) __attribute__((format(printf, 1, 2)));

// Non-blocking line reader. Returns true and fills `line` (NUL-terminated,
// without the line ending) once a full line has been received.
bool readLine(char* line, size_t size);

}  // namespace console
