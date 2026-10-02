# Laser Morse Code Transceiver

Firmware for the ECE 198 laser transceiver: two identical NUCLEO-F401RE units
that talk to each other in Morse code over a visible laser beam, without any
radio emissions (so they can be used inside the National Radio Quiet Zone).

Both units run the **same firmware**. Each unit sends and receives:

| Direction | How |
|-----------|-----|
| Send      | Press the key button (the laser stays on while it is pressed), or type text in the serial console and it is keyed out automatically |
| Receive   | The buzzer and the green LED (LD2) follow the incoming light, and the pulses are decoded to text in the serial console |

Holding the key for more than 1.5 s keeps the laser on for aiming. The
receiver ignores holds that long, so aiming never shows up as text.

## Repository layout

```
platformio.ini            build environments (firmware + PC unit tests)
include/board_config.hpp  pin assignments and tunable constants
src/
  main.cpp                setup()/loop(): ties all the modules together
  commands.{hpp,cpp}      serial console commands (/help, /test, /wpm, ...)
  diagnostics.{hpp,cpp}   hardware tests: laser, sensor, button, buzzer, link
  console.{hpp,cpp}       USB serial I/O (no-ops in the standalone build)
  device.hpp              references to all modules, shared by commands/tests
  drivers/                laser, phototransistor, button, buzzer drivers
lib/morse/src/            hardware-independent Morse code library
  morse_code.{hpp,cpp}        ITU alphabet: A-Z, 0-9, punctuation
  morse_transmitter.{hpp,cpp} text -> laser on/off timing (non-blocking)
  morse_decoder.{hpp,cpp}     light timing -> text, adapts to the sender's speed
test/test_morse/          unit tests for lib/morse, run on your PC
```

## Wiring

Pins from the design-document schematic:

| Part | Pin | Notes |
|------|-----|-------|
| Laser (via NPN transistor Q1) | PA1 (A1) | high = laser on |
| Phototransistor + 47 kΩ pull-up | PA0 (A0) | low = light detected |
| Key button to GND | PA2 (D1) | internal pull-up |
| Active buzzer | PA3 (D0) | high = sounding |
| Status LED | PA5 (LD2, on board) | follows the receiver |

### PA2/PA3 vs. the USB serial port

On the Nucleo board, **PA2 and PA3 are the ST-LINK USB serial (virtual COM
port) lines**. With the button and buzzer on those pins, the board cannot also
talk to a PC. There are two build environments to choose from:

| Environment | Button | Buzzer | USB serial console |
|-------------|--------|--------|--------------------|
| `standalone` (default) | PA2 | PA3 | no: key with the button, listen on the buzzer |
| `usb_serial` | **PA4 (A2)** | **PB0 (A3)** | yes: type text, see decoded text, run tests |

To use `usb_serial`, move the button wire from D1 to A2 and the buzzer wire
from D0 to A3. To use other pins, edit the `-D LMC_PIN_...` flags in
`platformio.ini`. The firmware won't compile if the serial console is enabled
while the button or buzzer is still on PA2/PA3.

## Building and flashing

Install [PlatformIO](https://platformio.org/install) (the VS Code extension or
`pip install platformio`), plug in the Nucleo, then:

```sh
pio run -e standalone -t upload     # or: -e usb_serial
pio device monitor -b 115200        # usb_serial only
```

Flash both units with the same environment.

## Using the serial console (`usb_serial`)

Open any serial terminal at **115200 baud**. Each line you type is sent as
Morse code when you press Enter. Received text shows up as `RX< ...`.

```
/help               list commands
/status             settings, receiver state, decoder speed estimate
/wpm <n>            transmit speed, 5-30 words per minute (default 12)
/sidetone on|off    beep locally while transmitting
/aim on|off         hold the laser on to align the units
/stop               abort the current transmission
/table              print the Morse alphabet
```

## Hardware tests

### Self-test at power-up (both builds)

Hold the key button while pressing reset (black button on the Nucleo):

1. The buzzer beeps (two short beeps in `standalone`).
2. The laser blinks 5 times. Check it by eye.
3. For 10 seconds the buzzer and LD2 follow the phototransistor. Point the
   other unit's laser at it (hold its key) and listen for the buzzer.
4. One long beep: the test is done.

With `usb_serial`, the results are also printed to the console.

### Console tests (`usb_serial`)

| Command | What it checks |
|---------|----------------|
| `/test laser`  | Blinks the laser 5 times. If you reflect the beam back with a mirror, it also counts the pulses at its own sensor |
| `/test sensor` | Prints every light/dark change for 10 s with timings. Use it to check aim and ambient-light interference |
| `/test button` | Prints key presses for 10 s |
| `/test buzzer` | Beep pattern |
| `/test all`    | All of the above |
| `/test rx` then `/test tx` | **Link test across two units.** Run `/test rx` on the receiving unit first, then `/test tx` on the sending unit. The sender transmits 100 pulses of 20 ms. The receiver reports how many arrived and their widths, and gives PASS if the error rate is ≤ 10% (the design requirement) |

The link test is the quick way to run the distance (Test 2, 30 m) and
interference (Test 3, lamp at 1 m) checks from the design document.

## Unit tests (no hardware needed)

```sh
pio test -e native
```

These check the Morse alphabet tables, the transmitter's exact timing, and
transmitter → decoder loopback, including speed mismatch, ±20% human timing
jitter, aim holds and unknown patterns.

## Morse timing

Standard ITU timing based on a unit of `1200 / WPM` ms: dot = 1 unit,
dash = 3, gap between elements = 1, between letters = 3, between words = 7.
The decoder estimates the sender's unit from the incoming marks, so people
keying at different speeds are decoded without any setup. Undecodable
patterns appear as `*`.
