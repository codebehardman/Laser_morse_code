# Laser Morse Code Transceiver

Firmware for the ECE 198 laser transceiver: two identical NUCLEO-F401RE units
that talk to each other in Morse code over a visible laser beam, without any
radio emissions (so they can be used inside the National Radio Quiet Zone).

Both units run the **same firmware**, and each one can both send and receive.

## Modes

When a unit powers up, LD2 (the green LED on the Nucleo) gives a short
blip every half second while it waits. The **first key press** chooses the
mode:

| Press | Mode | LD2 confirmation |
|-------|------|------------------|
| Short (< 2 s) | **Morse code** | one long flash |
| Hold ≥ 2 s    | **Serial terminal** | two long flashes (they start while you're still holding) |

The mode stays until you press reset or cut the power. Set both units to the
same mode.

### Morse code mode
- **Send:** the key turns the laser on while pressed. In the `usb_serial`
  build, text typed in the console is also keyed out as Morse.
- **Receive:** LD2 lights while the laser hits the phototransistor. In the
  `usb_serial` build, the pulses are decoded to text in the console.
- Holding the key for more than 1.5 s keeps the laser on for aiming. The
  receiver ignores holds that long, so aiming never shows up as text.

### Serial terminal mode (`usb_serial` build only)
A transparent link between two PCs: whatever is typed in one unit's serial
terminal comes out byte-for-byte in the other unit's terminal, and the link
works in both directions at once.
- The laser link runs at **9600 baud, 8 data bits, even parity, 1 stop bit**,
  as in the design document. A timer interrupt sends and receives the bits,
  sampling 8 times per bit with a 3-sample majority vote in the middle.
- A byte that fails the parity or stop-bit check is **dropped** rather than
  shown wrong.
- Your own typing is echoed locally (`config::kSerialLocalEcho`). Line
  endings are shown correctly whether your terminal sends CR, LF or CRLF.
- Holding the key keeps the laser on for aiming. The other unit sees this as
  a line break and prints nothing.
- LD2 flashes when data arrives.

In the `standalone` build there is no PC connection, so a long press runs
the [hardware self-test](#self-test-standalone-build-long-press) and then
enters Morse mode.

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
  console.{hpp,cpp}       USB serial I/O (no-ops in the standalone build)
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

Pins from the design-document schematic:

| Part | Pin | Notes |
|------|-----|-------|
| Laser (via NPN transistor Q1) | PA1 (A1) | high = laser on |
| Phototransistor + 47 kΩ pull-up | PA0 (A0) | low = light detected |
| Key button to GND | PA2 (D1) | internal pull-up |
| Status LED | PA5 (LD2, on board) | the only local indicator: received light, mode, self-test |

### PA2/PA3 vs. the USB serial port

On the Nucleo board, **PA2 and PA3 are the ST-LINK USB serial (virtual COM
port) lines**. With the button on PA2, the board cannot also talk to a PC.
There are two build environments to choose from:

| Environment | Button | USB serial console |
|-------------|--------|--------------------|
| `standalone` (default) | PA2 | no: Morse mode only; key with the button, watch LD2 |
| `usb_serial` | **PA4 (A2)** | yes: both modes, console, diagnostics |

To use `usb_serial`, move the button wire from D1 to A2. To use another pin,
edit the `-D LMC_PIN_BUTTON` flag in `platformio.ini`. The firmware won't
compile if the serial console is enabled while the button is still on
PA2/PA3.

## Building and flashing

Install [PlatformIO](https://platformio.org/install) (the VS Code extension or
`pip install platformio`), plug in the Nucleo, then:

```sh
pio run -e standalone -t upload     # or: -e usb_serial
pio device monitor -b 115200        # usb_serial only
```

Flash both units with the same environment.

## Using the serial console (`usb_serial`)

Open any serial terminal at **115200 baud** (this is the PC-to-board
speed, separate from the 9600-baud laser link). In Morse mode, each line
you type is sent as Morse code when you press Enter. Received text shows up as `RX< ...`.

```
/help               list commands
/status             settings, receiver state, decoder speed estimate
/wpm <n>            transmit speed, 5-30 words per minute (default 12)
/aim on|off         hold the laser on to align the units
/stop               abort the current transmission
/table              print the Morse alphabet
```

These commands only work in Morse mode. In serial terminal mode every
character goes over the link.

## Hardware tests

### Self-test (`standalone` build, long press)

In the `standalone` build, hold the key for 2 s at power-up:

1. After the two confirmation flashes, LD2 flashes 3 times quickly: the
   self-test is starting.
2. The laser blinks 5 times, with LD2 blinking in step. Check the laser by eye.
3. For the next 10 seconds LD2 follows the phototransistor. Point the other
   unit's laser at it (hold its key) and LD2 should light up.
4. One long (1 s) flash: the test is done, and the unit is now in Morse mode.

### Console tests (`usb_serial`, Morse mode)

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

- **Phototransistor speed in serial mode.** At 9600 baud each bit lasts
  104 µs. A phototransistor's switching time grows with its load resistor,
  and the 47 kΩ pull-up may be too slow at this speed. If serial mode drops
  bytes while Morse mode works, try a 4.7–10 kΩ pull-up (a lower value
  responds faster but needs more light). You can also lower
  `config::kLinkBaud` on both units.
- **Q1 base resistor.** The schematic seems to drive the laser transistor's
  base straight from PA1. Add a resistor of about 1 kΩ.
