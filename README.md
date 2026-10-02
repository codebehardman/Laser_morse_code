# Laser Morse Code Transceiver

Firmware for the ECE 198 laser transceiver: two identical STM32F103C8T6 units
that talk to each other in Morse code over a visible laser beam, without any
radio emissions (so they can be used inside the National Radio Quiet Zone).

Both units run the **same firmware**, and each one can both send and receive.

## Modes

When a unit powers up, the on-board LED (PC13) gives a short blip every half
second while it waits. The **first key press** chooses the mode:

| Press | Mode | LED confirmation |
|-------|------|------------------|
| Short (< 2 s) | **Morse code** | one long flash |
| Hold ≥ 2 s    | **Serial terminal** | two long flashes (they start while you're still holding) |

The mode stays until you press reset or cut the power. Set both units to the
same mode.

### Morse code mode
- **Send:** the key turns the laser on while pressed. Text typed in the
  serial console is also keyed out as Morse.
- **Receive:** the LED lights while the laser hits the phototransistor, and
  the pulses are decoded to text in the serial console.
- Holding the key for more than 1.5 s keeps the laser on for aiming. The
  receiver ignores holds that long, so aiming never shows up as text.

### Serial terminal mode
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
  console.{hpp,cpp}       serial console over the chip's USB port
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
| Laser (via NPN transistor Q1) | PA1 | high = laser on |
| Phototransistor + pull-up resistor | PA0 | low = light detected. **Pull-up to 3.3 V, not 5 V** (see below) |
| Key button to GND | PA2 | internal pull-up |
| Status LED | PC13 (on board) | lights when PC13 is low |
| USB | on-board connector | power + serial console |

Keep PA11/PA12 (USB) and PA13/PA14 (SWD programming) free. To use different
pins, add `-D LMC_PIN_...=Pxx` flags in `platformio.ini`. If your board's LED
is elsewhere or lights when the pin is high, set `LMC_PIN_STATUS_LED` and
`LMC_STATUS_LED_ACTIVE_LOW=0`.

## Flashing

The F103C8T6 has no programmer built in, and its USB port can't be used for
flashing until you've installed a bootloader. Flash it **once per board**
using one of these:

### Option A: ST-LINK V2 dongle (recommended)

Connect the dongle to the 4-pin SWD header at the end of the board:

| ST-LINK | Board |
|---------|-------|
| SWDIO | DIO (PA13) |
| SWCLK | CLK / DCLK (PA14) |
| GND | GND |
| 3.3V | 3.3 |

Unplug the board's own USB cable while flashing (the dongle powers it), then:

```sh
pio run -e bluepill -t upload
```

If the upload fails with "init mode failed", hold the board's RESET button,
start the upload, and release RESET when OpenOCD starts printing. This is
usually only needed the first time, if the factory demo firmware has turned
off the SWD pins.

### Option B: USB-serial adapter (3.3 V FTDI/CP2102/CH340)

| Adapter | Board |
|---------|-------|
| TX | PA10 (RX1) |
| RX | PA9 (TX1) |
| GND | GND |
| 3.3V | 3.3 |

1. Move the **BOOT0** jumper to **1**, press RESET.
2. Run `pio run -e bluepill_uart -t upload`.
3. Move BOOT0 back to **0** and press RESET to run the firmware.

### After flashing

Plug the board's own USB connector into the PC. It shows up as a serial
port (`/dev/cu.usbmodem*` on macOS, `COMx` on Windows). Open it with:

```sh
pio device monitor
```

Flash both units the same way.

## Using the serial console

Open the board's USB serial port in any terminal (the baud rate setting
doesn't matter over USB; 115200 is fine). In Morse mode, each line
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

- **USB not detected?** Many F103C8T6 boards have the wrong USB pull-up
  resistor (R10 = 10 kΩ instead of 1.5 kΩ). Most PCs still work. If yours
  doesn't, replace R10 or add 1.8 kΩ between PA12 and 3.3 V.
- **Phototransistor speed in serial mode.** At 9600 baud each bit lasts
  104 µs. A phototransistor's switching time grows with its load resistor,
  and the 47 kΩ pull-up may be too slow at this speed. If serial mode drops
  bytes while Morse mode works, try a 4.7–10 kΩ pull-up (a lower value
  responds faster but needs more light). You can also lower
  `config::kLinkBaud` on both units.
- **PA0 is not 5 V tolerant on the F103.** The schematic ties the
  phototransistor's pull-up to 5 V; on this chip that pushes current into
  PA0's protection diode. Connect the pull-up to **3.3 V** instead.
- **Q1 base resistor.** The schematic seems to drive the laser transistor's
  base straight from PA1. Add a resistor of about 1 kΩ.
