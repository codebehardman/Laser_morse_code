# Laser Morse Code Transceiver

Firmware for the ECE 198 laser transceiver: two identical units that talk
to each other in Morse code over a visible laser beam, without any
radio emissions (so they can be used inside the National Radio Quiet Zone).

Both units run the **same firmware** on an **ESP32-WROOM-32 DevKit**, and
each one can both send and receive. The ESP32's Wi-Fi and Bluetooth are never
switched on by this firmware, so the board does not transmit radio.

(Earlier versions targeted the NUCLEO-F401RE and STM32F103C8T6; see the git
history, e.g. commit `f61c9a8`, if you need them.)

## Modes

When a unit powers up, the on-board LED gives a short blip every half
second while it waits. The **first key press** chooses the mode:

| Press | Mode | LED confirmation |
|-------|------|------------------|
| Tap (< 1 s) | **Morse code** | one long flash |
| Hold ≥ 1 s  | **Serial terminal** | two long flashes (they start while you're still holding, so let go then) |

The serial monitor shows the chosen mode (`===== Mode: ... =====`). Press
**EN** to reset and choose again.

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
  100 kΩ pull-up is too slow for that (see Hardware notes). A dedicated task
  on the ESP32's second core sends and receives the bits, sampling the
  sensor 8 times per bit with a 3-sample majority vote.
- Backspace works while typing. Of the commands, only `/level` and
  `/threshold` work in this mode; the rest need Morse mode.
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
                          laser_uart: serial link over the laser (core-0 task)
lib/morse/src/            hardware-independent Morse code library
  morse_code.{hpp,cpp}        ITU alphabet: A-Z, 0-9, punctuation
  morse_transmitter.{hpp,cpp} text -> laser on/off timing (non-blocking)
  morse_decoder.{hpp,cpp}     light timing -> text, adapts to the sender's speed
lib/optical_uart/src/     hardware-independent 8E1 software UART + line framing
test/                     unit tests for both libraries, run on your PC
```

## Wiring

| Part | ESP32 pin | Notes |
|------|-----------|-------|
| Laser (via NPN transistor Q1) | **GPIO 25** | high = laser on |
| Phototransistor + 100 kΩ pull-up | **GPIO 26** | read with the ADC. **Pull-up to 3V3, never 5 V** |
| Key button to GND | **GPIO 27** | internal pull-up |
| Status LED | GPIO 2 (on board) | |
| Laser supply (R1 + laser diode) | **VIN / 5V** | USB 5 V while plugged in |
| Pull-up supply | **3V3** | |
| USB | on-board connector | power, serial console and flashing |

```
ESP32 DevKit
  VIN  ──[100Ω R1]──▶|── laser diode ── Q1 collector
  GPIO25 ──[~1kΩ]──────────────────────── Q1 base   (Q1 emitter → GND)
  3V3  ──[100kΩ]──┬── GPIO26
                  └── phototransistor collector   (emitter → GND)
  GPIO27 ──[button]── GND
```

ESP32 pins are 3.3 V only. Avoid GPIO 0, 2, 12 and 15 (they affect booting)
for external parts. To use different pins, add `-D LMC_PIN_...=` flags in
`platformio.ini`.

## Light threshold (ambient light)

The phototransistor is read with the ADC: **0 = very bright, 4095 = dark**. A
reading below the **threshold** counts as "laser detected". Ambient light
also lowers the reading, so a bright room needs a lower threshold than a
dark one.

**In the code**, set the room type in `include/board_config.hpp`:

```cpp
constexpr Lighting kLighting = Lighting::Dark;   // or Lighting::Bright
constexpr uint16_t kThresholdDarkRoom = 2000;
constexpr uint16_t kThresholdBrightRoom = 700;
```

**At runtime** (both modes, lost on reset):

| Command | Does |
|---------|------|
| `/level` | shows the live reading for 5 s, with a bar and LASER/dark |
| `/threshold` | shows the current threshold and reading |
| `/threshold 1500` | sets it to 1500 |
| `/threshold dark` / `bright` | uses the preset from the config |
| `/threshold auto` | measures the room (other laser off), then the other unit's laser (hold its button), and sets the threshold halfway between |

Once you've found a value that works, put it in `board_config.hpp` so it
survives a reset.

## Flashing

Plug the ESP32 into the computer and run:

```sh
pio run -e esp32 -t upload
pio device monitor
```

Or in VS Code: PlatformIO sidebar → **esp32** → **Upload**, then
**Monitor**. The board shows up as `/dev/cu.usbserial-*` on macOS (`COMx` on
Windows). Close any serial monitor on that port first. If the upload stops
at "Connecting...", hold the board's **BOOT** button until the upload starts.
If it fails with "serial noise or corruption", lower `upload_speed` in
`platformio.ini` (e.g. to 115200).

Flash both units the same way.

## Using the serial console

Open the board's USB serial port in any terminal at **115200 baud**. Opening
the port restarts the board, so you'll see the mode prompt. In Morse mode,
each line you type is sent as Morse code when you press Enter. Received text
shows up as `RX< ...`.

```
/help               list commands
/status             settings, receiver state, decoder speed estimate
/wpm <n>            transmit speed, 5-40 words per minute (default 20)
/aim on|off         hold the laser on to align the units
/stop               abort the current transmission
/table              print the Morse alphabet
/level, /threshold  light sensor (see "Light threshold"; also in serial mode)
```

## Hardware tests

### Console tests (Morse mode)

| Command | What it checks |
|---------|----------------|
| `/test laser`  | Blinks the laser 5 times. If you reflect the beam back with a mirror, it also counts the pulses at its own sensor |
| `/test sensor` | Prints every light/dark change for 10 s with timings and ADC levels. Use it to check aim and ambient-light interference |
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
- **Line framing:** CRC, round trips, and that damaged or partial messages
  are always reported, never shown wrong or dropped silently.

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
- **Phototransistor pull-up: sensitivity vs. speed.** A larger pull-up is
  more sensitive but slower to return high after the light goes off. With
  100 kΩ the laser is detected, but 9600 baud came through garbled; with
  10 kΩ the laser was no longer detected at all. The link therefore runs at
  1200 baud (`config::kLinkBaud`). To go faster, try 50 kΩ (2 × 100 kΩ in
  parallel) at 2400 baud, on both units.
- **Q1 base resistor.** The design-doc schematic drives the laser
  transistor's base straight from the microcontroller pin. Add a resistor of
  about 1 kΩ.
