#pragma once

#include "drivers/button.hpp"
#include "drivers/buzzer.hpp"
#include "drivers/laser.hpp"
#include "drivers/photo_sensor.hpp"
#include "morse_decoder.hpp"
#include "morse_transmitter.hpp"

// User-adjustable runtime settings.
struct Settings {
    uint32_t wpm;
    bool sidetone;  // beep locally while transmitting
    bool aim;       // hold the laser on continuously for alignment
};

// Everything the command handler and diagnostics need access to.
struct Device {
    Laser& laser;
    PhotoSensor& sensor;
    Button& button;
    Buzzer& buzzer;
    morse::MorseTransmitter& transmitter;
    morse::MorseDecoder& decoder;
    Settings& settings;

    // Sample the inputs; used by blocking routines to keep them fresh.
    void updateInputs() {
        const uint32_t now = millis();
        sensor.update(now);
        button.update(now);
    }
};
