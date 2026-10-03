# Laser Morse Code Transceiver

Firmware for the ECE 198 laser transceiver: two identical units that talk
to each other in Morse code over a visible laser beam, without any
radio emissions (so they can be used inside the National Radio Quiet Zone).

Both units run the **same firmware**, and each one can both send and receive.
The firmware supports two boards:

| Board | PlatformIO environment | Flashing |
|-------|------------------------|----------|
| **ESP32-WROOM-32 DevKit** (current) | `esp32` (default) | over its USB port, no extra hardware |
| STM32F103C8T6 "Blue Pill" | `bluepill` / `bluepill_uart` | needs an ST-LINK V2 or USB-serial adapter |

The ESP32's Wi-Fi and Bluetooth are never switched on by this firmware, so
the board does not transmit radio.

## Modes

When a unit powers up, the on-board LED gives a short blip every half
second while it waits. The **first key press** chooses the mode:

| Press | Mode | LED confirmation |
|-------|------|------------------|
| Tap (< 1 s) | **Morse code** | one long flash |
| Hold ≥ 1 s  | **Serial terminal** | two long flashes (they start while you're still holding, so let go then) |

The serial monitor shows the chosen mode (`===== Mode: ... =====`). On the
ESP32, press **EN** to reset and choose again.

The mode stays until you press reset or cut the power. Set both units to the
same mode.

### Morse code mode
- **Send:** the key turns the laser on while pressed. Text typed in the
  serial console is also keyed out as Morse, at 20 words per minute by
  default (`/wpm` changes it). Morse is slow: roughly 0.6 s per letter at
  20 WPM, so the console shows how long each message will take.
- **Receive:** the LED lights while the laser hits the phototransistor, and
  the pulses are decoded to text in the serial console.
- Holding the key for more than 1.5 s keeps the laser on for aiming. The
  receiver ignores holds that long, so aiming never shows up as text.

### Serial terminal mode
A text link between two PCs. Type a line and press **Enter**: the whole line
is sent over the laser and appears in the other unit's terminal as
`RX< ...`. It works in both directions at once.
- Each line travels as one frame (start marker, text, CRC-8 checksum, end
  marker). The receiver shows a line only if it arrived intact, and prints
  `[message corrupted - ask to resend]` otherwise.
- The laser link runs at **1200 baud, 8 data bits, even parity, 1 stop bit**.
  The design document specifies 9600 baud, but the phototransistor with its
  100 kΩ pull-up is too slow for that (see Hardware notes). A timer
  interrupt sends and receives the bits, sampling 8 times per bit with a
  3-sample majority vote.
- Backspace works while typing. Commands (`/...`) only work in Morse mode.
- Holding the key keeps the laser on for aiming. The other unit sees this as
  a line break and prints nothing.
- The LED flashes when data arrives.

## Repository layout

```
platformio.ini            build environments (firmware + PC unit tests)
include/board_config.hpp  pin assignments and tunable constants
src/
  main.cpp                setup()/loop(): mode selection, then runs the mode
  modes/
    mode_select.{hpp,cpp}   power-up key press: short = Morse, long = serial
    morse_mode.{hpp,cpp}    Morse code mode
    serial_mode.{hpp,cpp}   serial terminal mode
  commands.{hpp,cpp}      serial console commands (/help, /test, /wpm, ...)
  diagnostics.{hpp,cpp}   hardware tests: laser, sensor, button, link
  console.{hpp,cpp}       serial console over the board's USB port
  device.hpp              references to all modules, shared by commands/tests
  drivers/                laser, phototransistor, button, status LED drivers,
                          laser_uart: timer-interrupt serial link over the laser
lib/morse/src/            hardware-independent Morse code library
  morse_code.{hpp,cpp}        ITU alphabet: A-Z, 0-9, punctuation
  morse_transmitter.{hpp,cpp} text -> laser on/off timing (non-blocking)
  morse_decoder.{hpp,cpp}     light timing -> text, adapts to the sender's speed
lib/optical_uart/src/     hardware-independent 8E1 software UART framing
test/                     unit tests for both libraries, run on your PC
```

## Wiring

| Part | ESP32 DevKit | STM32F103C8T6 | Notes |
|------|--------------|---------------|-------|
| Laser (via NPN transistor Q1) | **GPIO 25** | PA1 | high = laser on |
| Phototransistor + pull-up resistor | **GPIO 26** | PA0 | low = light detected. **Pull-up to 3.3 V, never 5 V** |
| Key button to GND | **GPIO 27** | PA2 | internal pull-up |
| Status LED | GPIO 2 (on board) | PC13 (on board) | |
| Laser supply (R1 + laser diode) | **VIN / 5V** | 5V | USB 5 V while plugged in |
| Pull-up supply | **3V3** | 3.3 | |
| USB | on-board connector | on-board connector | power + serial console |

```
ESP32 DevKit
  VIN  ──[100Ω R1]──▶|── laser diode ── Q1 collector
  GPIO25 ─────────────────────────────── Q1 base   (Q1 emitter → GND)
  3V3  ──[47kΩ]──┬── GPIO26
                 └── phototransistor collector   (emitter → GND)
  GPIO27 ──[button]── GND
```

All of these boards' pins are 3.3 V only. On the ESP32 avoid GPIO 0, 2, 12
and 15 (they affect booting) and GPIO 34–39 (input-only) for external parts.
To use different pins, add `-D LMC_PIN_...=` flags in `platformio.ini`.

## Flashing

### ESP32

Plug the ESP32 into the computer and run:

```sh
pio run -e esp32 -t upload
pio device monitor
```

Or in VS Code: PlatformIO sidebar → **esp32** → **Upload**, then
**Monitor**. The board shows up as `/dev/cu.usbserial-*` on macOS (`COMx` on
Windows). If the upload stops at "Connecting...", hold the board's **BOOT**
button until the upload starts. If it fails with "serial noise or
corruption", lower `upload_speed` in `platformio.ini` (e.g. to 115200).

Flash both units the same way.

### STM32F103C8T6

This chip has no programmer built in, and its USB port can't be used for
flashing until you've installed a bootloader. Flash it **once per board**
using one of these.

**ST-LINK V2 (recommended).** Connect SWDIO→DIO, SWCLK→CLK, GND→GND and
3.3V→3.3 on the 4-pin header at the end of the board. Unplug the board's own
USB, then run `pio run -e bluepill -t upload`. A Nucleo board's built-in
ST-LINK also works: remove its two CN2 jumpers and use CN4 (pin 2 SWCLK,
pin 3 GND, pin 4 SWDIO).

**USB-serial adapter.** Connect TX→PA10, RX→PA9 and GND. Set **BOOT0 = 1**,
press RESET, run `pio run -e bluepill_uart -t upload`, then set BOOT0 back
to 0 and press RESET.

After flashing, plug in the board's own USB and run `pio device monitor`.

## Using the serial console

Open the board's USB serial port in any terminal at **115200 baud**. On the
ESP32, opening the port restarts the board, so you'll see the mode prompt. In Morse mode, each line
you type is sent as Morse code when you press Enter. Received text shows up as `RX< ...`.

```
/help               list commands
/status             settings, receiver state, decoder speed estimate
/wpm <n>            transmit speed, 5-40 words per minute (default 20)
/aim on|off         hold the laser on to align the units
/stop               abort the current transmission
/table              print the Morse alphabet
```

These commands only work in Morse mode. In serial terminal mode every
character goes over the link.

## Hardware tests

### Console tests (Morse mode)

| Command | What it checks |
|---------|----------------|
| `/test laser`  | Blinks the laser 5 times. If you reflect the beam back with a mirror, it also counts the pulses at its own sensor |
| `/test sensor` | Prints every light/dark change for 10 s with timings. Use it to check aim and ambient-light interference |
| `/test button` | Prints key presses for 10 s |
| `/test all`    | All of the above |
| `/test rx` then `/test tx` | **Link test across two units.** Run `/test rx` on the receiving unit first, then `/test tx` on the sending unit. The sender transmits 100 pulses of 20 ms. The receiver reports how many arrived and their widths, and gives PASS if the error rate is ≤ 10% (the design requirement) |

The link test is the quick way to run the distance (Test 2, 30 m) and
interference (Test 3, lamp at 1 m) checks from the design document.

## Unit tests (no hardware needed)

```sh
pio test -e native
```

- **Morse:** the alphabet tables, the transmitter's exact timing, and
  transmitter → decoder loopback, including speed mismatch, ±20% human timing
  jitter, aim holds and unknown patterns.
- **Serial link:** frame layout, all 256 byte values, any sampling phase,
  ±3% clock mismatch, detection of a flipped bit, glitches, and a held-on
  laser.

## Morse timing

Standard ITU timing based on a unit of `1200 / WPM` ms: dot = 1 unit,
dash = 3, gap between elements = 1, between letters = 3, between words = 7.
The decoder estimates the sender's unit from the incoming marks, so people
keying at different speeds are decoded without any setup. Undecodable
patterns appear as `*`.

## Hardware notes

- **Laser brightness.** The laser circuit is designed for 5 V. Take it from
  the ESP32's VIN/5V pin, which carries USB 5 V while plugged in. From 3.3 V
  the laser gets only about a third of its current; if you must use 3.3 V,
  change R1 from 100 Ω to about 33 Ω (check the laser's datasheet first).
- **USB not detected (STM32)?** Many F103C8T6 boards have the wrong USB pull-up
  resistor (R10 = 10 kΩ instead of 1.5 kΩ). Most PCs still work. If yours
  doesn't, replace R10 or add 1.8 kΩ between PA12 and 3.3 V.
- **Phototransistor pull-up: sensitivity vs. speed.** A larger pull-up is
  more sensitive but slower to return high after the light goes off. With
  100 kΩ the laser is detected, but 9600 baud came through garbled; with
  10 kΩ the laser was no longer detected at all. The link therefore runs at
  1200 baud (`config::kLinkBaud`). To go faster, try 50 kΩ (2 × 100 kΩ in
  parallel) at 2400 baud, on both units.
- **PA0 is not 5 V tolerant on the F103.** The schematic ties the
  phototransistor's pull-up to 5 V; on this chip that pushes current into
  PA0's protection diode. Connect the pull-up to **3.3 V** instead.
- **Q1 base resistor.** The schematic seems to drive the laser transistor's
  base straight from PA1. Add a resistor of about 1 kΩ.
