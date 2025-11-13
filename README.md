# SSFD — Seven-Segment Four-Digit Display Library

A professional, production-ready C++ library for Arduino (ATmega328P) that drives common-cathode 7-segment 4-digit displays via **non-blocking ISR-based multiplexing**.

## Features

✅ **Non-blocking** — Display refresh runs in Timer1 ISR; your code stays 100% responsive  
✅ **Robust** — Full input validation, bounds checking, and error codes  
✅ **Efficient** — Uses PROGMEM for lookup tables; ~80 Hz multiplexing for crisp display  
✅ **Flexible** — Display integers, floats, text, or custom 7-segment patterns  
✅ **Safe** — Volatile guards, atomic writes, graceful failure modes  
✅ **Well-documented** — Doxygen-ready headers, examples, and inline comments

## Hardware Requirements

- **MCU:** Arduino Uno, Nano, or compatible (ATmega328P @ 16 MHz)
- **Display:** Common-cathode 4-digit 7-segment display
- **Transistors:** 4× NPN BJTs (e.g., 2N2222) for digit multiplexing (optional if using ULN2003)
- **Resistors:**
  - 220Ω–470Ω in series with each segment (8 total)
  - 1kΩ–10kΩ base resistors for digit transistors (4 total)

### Pinout Example

```
Display Digit Pins (Common Cathode)      Arduino GPIO
    Digit 1 ————————————————————————————— Pin 10
    Digit 2 ————————————————————————————— Pin 11
    Digit 3 ————————————————————————————— Pin 12
    Digit 4 ————————————————————————————— Pin 13

Display Segment Pins (a-g, dp)           Arduino GPIO
    Segment a ——————————————————————————— Pin 2
    Segment b ——————————————————————————— Pin 3
    Segment c ——————————————————————————— Pin 4
    Segment d ——————————————————————————— Pin 5
    Segment e ——————————————————————————— Pin 6
    Segment f ——————————————————————————— Pin 7
    Segment g ——————————————————————————— Pin 8
    Segment dp ——————————————————————————— Pin 9
```

## Installation

1. **Clone or download** this repository into your Arduino libraries folder:

   ```bash
   ~/Arduino/libraries/SSFD/
   ```

2. **Restart** the Arduino IDE.

3. **Verify installation:** Check `Sketch → Include Library → SSFD` appears in the menu.

## Quick Start

```cpp
#include <Arduino.h>
#include "SSFD.h"

// Define pin arrays in PROGMEM
const uint8_t digitPins[] PROGMEM = {10, 11, 12, 13};
const uint8_t segmentPins[] PROGMEM = {2, 3, 4, 5, 6, 7, 8, 9};

// Create display instance
SevenSegment display(segmentPins, digitPins);

void setup() {
  Serial.begin(9600);

  // Initialize display (REQUIRED)
  SevenSegment::Error err = display.begin();
  if (err != SevenSegment::Error::OK) {
    Serial.println("Display init failed!");
    while (1); // Halt
  }

  // Optional: disable leading zeros
  display.setLeadingZeros(false);

  // Display a number
  display.setNumber(1234);
}

void loop() {
  // Required for blinking animations (multiplexing runs automatically in background)
  display.refresh();

  // Your code here — display stays active in background!
  delay(100);
}
```

## API Reference

### Core Functions

#### `Error begin()`

Initialize pins and Timer1. **Must be called in `setup()`.**

**Returns:** `Error::OK` on success; see error codes below.

```cpp
if (display.begin() != SevenSegment::Error::OK) {
  // Handle error
}
```

#### `void refresh()`

Refresh display state (non-blocking). Multiplexing is handled by ISR automatically, but this manages blink state.

#### `void clear()`

Display all blank (off).

#### `void end()`

Disable Timer1 and clean up. Call if you need Timer1 for other code.

#### `bool isInitialized()`

Check if `begin()` was successful.

#### `Error getLastError()`

Get error code from most recent operation.

---

### Display Functions

#### `void setNumber(uint16_t value, int8_t dpPosition = -1)`

Display a 4-digit integer (0–9999).

- **value:** 0–9999 (clamped if > 9999)
- **dpPosition:** Decimal point position: -1 (none), 0–3 (after digit N from left)

```cpp
display.setNumber(1234);        // "1234"
display.setNumber(42, 1);       // "0042" → "004.2"
display.setNumber(5, -1);       // "0005"
```

#### `Error setFloat(float value)`

Display a float (0.00–99.99) with auto-decimal placement.

- **value:** 0–99.99 (clamped; NaN/Inf → "ERR ")
- **Returns:** Error code if invalid

```cpp
display.setFloat(12.34);   // "12.34"
display.setFloat(5.6);     // " 5.60"
display.setFloat(NAN);     // "ERR "
```

#### `Error setText(const char* text)`

Display up to 4 ASCII characters. Supported: digits, A–Z, space, dash, equals.

```cpp
display.setText("HELP");   // "HELP"
display.setText("HI");     // "HI  " (padded)
```

#### `void setSegments(const uint8_t patterns[4])`

Display raw 7-segment patterns (advanced).

Bit layout: `0b abcdefgx` where a–g = segments, x = decimal point.

```cpp
uint8_t patterns[] = {0xFF, 0x00, 0xAA, 0x55};
display.setSegments(patterns);
```

#### `void setHundredths(uint16_t hundredths, int8_t dpPosition = 2)`

Display using integer hundredths (avoids float math).

- **hundredths:** 0–9999 (0.00–99.99)

```cpp
display.setHundredths(1234, 2);  // "12.34"
display.setHundredths(560, 2);   // " 5.60"
```

---

### Display Modes

#### `void setLeadingZeros(bool enabled)`

Enable/disable leading zero suppression.

```cpp
display.setLeadingZeros(false);
display.setNumber(5);    // " 5  " (with blanks)
```

#### `void startBlink(unsigned long intervalMs = 500)`

Start blinking the display.

```cpp
display.startBlink(300);  // Blink every 300 ms
```

#### `void stopBlink()`

Stop blinking.

#### `bool isBlinking()`

Check if currently blinking.

---

## Error Codes

| Code | Name                | Meaning                      |
| ---- | ------------------- | ---------------------------- |
| 0    | `OK`                | No error                     |
| 1    | `NULL_POINTER`      | Null pin array passed        |
| 2    | `INVALID_PIN`       | Pin number > 53              |
| 3    | `TIMER_INIT_FAILED` | Timer1 initialization failed |
| 4    | `NOT_INITIALIZED`   | `begin()` not called         |
| 5    | `INVALID_ARGUMENT`  | Invalid function argument    |

---

## Examples

See the `examples/` folder for complete sketches:

- **01_TestWiring** — Verify all segments and digits light up
- **02_FloatCounter** — Count and display floats in real time
- **03_AdvancedFeatures** — Display characters, symbols and some sequences like blinking

---

## Performance

- **Refresh Rate:** ~125 Hz (Timer1 @ 16 MHz, prescaler 64)
- **Flash Usage:** ~6 KB
- **RAM Usage:** ~60 bytes
- **ISR Time:** <100 µs per interrupt

---

## Limitations

- **ATmega328P only** (Timer1 specific; other MCUs require modifications)
- **Single instance** (static ISR pointer; use singleton pattern for safety)
- **Timer1 exclusivity** — Cannot be used if other code needs Timer1

---

## Troubleshooting

### Display is blank

1. Verify `begin()` returned `OK`
2. Call `display.testWiring()` in setup to verify segment/digit connections
3. Check transistor base resistor values and wiring
4. Verify display is common-cathode (not common-anode)

### Display is too dim

- Reduce segment resistor values (try 220Ω)
- Check Timer1 interrupt frequency (lower = brighter but slower)

### Display flickers

- Check for loose wiring
- Verify power supply is stable (add 100µF capacitor across display)

### Some segments don't light

- Use `testWiring()` to identify which segment is broken
- Check corresponding GPIO pin and resistor

---

## License

MIT License — See LICENSE file for details.

---

## Contributing

Contributions welcome! Please:

1. Fork this repository
2. Create a feature branch
3. Add tests and documentation
4. Submit a pull request

---

## Author

Antony Kasera — Created 13/11/2025

**Support:** Open an issue on GitHub for bugs or feature requests.
