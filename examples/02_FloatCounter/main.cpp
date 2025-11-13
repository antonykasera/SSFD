/*
 * Example: 02_FloatCounter
 *
 * Demonstrates non-blocking float counter with real-time display updates.
 *
 * This sketch increments a counter by 0.01 every 100 ms and displays it
 * on the 7-segment display without blocking your main loop.
 *
 * The display refresh runs in the Timer1 ISR (Interrupt Service Routine),
 * so your code remains 100% responsive to sensors, buttons, and other tasks.
 *
 * **Expected Behavior:**
 * - Display shows "0.00" at startup
 * - Counter increments: 0.00 → 0.01 → 0.02 → ... → 99.99 → 0.00 (loops)
 * - Serial monitor prints counter value every ~500 ms
 * - Display remains bright and flicker-free (ISR multiplexing)
 *
 * **Customization:**
 * - Change increment: modify `currentValue += 0.01` (line ~X)
 * - Change update rate: modify `>= 100` timeout (milliseconds)
 * - Add blinking: call `display.startBlink(500)` in setup()
 */

#include <Arduino.h>
#include <avr/pgmspace.h>
#include "SSFD.h"

// ========== PIN CONFIGURATION ==========
const uint8_t digitPins[] PROGMEM = {10, 11, 12, 13};     // {D1, D2, D3, D4}
const uint8_t segmentPins[] PROGMEM = {2, 3, 4, 5, 6, 7, 8, 9}; // {a, b, c, d, e, f, g, dp}

// ========== DISPLAY INSTANCE ==========
SevenSegment display(segmentPins, digitPins);

// ========== COUNTER STATE ==========
float currentValue = 0.0f;         // Counter value (0.00 - 99.99)
unsigned long lastUpdateTime = 0;  // Timestamp of last counter update
const unsigned long UPDATE_INTERVAL = 100; // ms between increments (10 Hz)
const float INCREMENT = 0.01f;     // Value to add each update
const float MAX_COUNTER = 99.99f;  // Roll over at this value

// ========== SERIAL LOGGING ==========
unsigned long lastLogTime = 0;
const unsigned long LOG_INTERVAL = 500; // Log every 500 ms

// ========== SETUP ==========
void setup() {
  Serial.begin(9600);
  delay(500);

  Serial.println("\n========================================");
  Serial.println("   SSFD: Float Counter Example");
  Serial.println("========================================\n");

  // Initialize display with error checking
  SevenSegment::Error err = display.begin();
  if (err != SevenSegment::Error::OK) {
    Serial.print("❌ ERROR: Display initialization failed (code ");
    Serial.print((uint8_t)err);
    Serial.println(")");
    while (1);
  }

  Serial.println("✓ Display initialized");

  // Disable leading zeros for cleaner display ("  1.2" instead of "001.2")
  display.setLeadingZeros(false);
  Serial.println("✓ Leading zeros disabled");

  // Initialize counter to 0.00
  display.setFloat(currentValue);
  Serial.println("✓ Counter started at 0.00\n");

  Serial.println("Counter running... (non-blocking)\n");
}

// ========== LOOP ==========
void loop() {
  // ========== NON-BLOCKING COUNTER UPDATE ==========
  // Only increment if enough time has passed (non-blocking timing)
  if (millis() - lastUpdateTime >= UPDATE_INTERVAL) {
    lastUpdateTime = millis();

    // Increment counter by 0.01
    currentValue += INCREMENT;

    // Roll over at MAX_COUNTER
    if (currentValue > MAX_COUNTER) {
      currentValue = 0.0f;
      Serial.println("→ Counter reset to 0.00");
    }

    // Update display with new value
    // setFloat() handles the decimal point placement automatically
    display.setFloat(currentValue);
  }

  // ========== ISR-BASED DISPLAY REFRESH ==========
  // The display multiplexing is handled by Timer1 ISR automatically.
  // This call manages blink state (if enabled).
//   display.refresh();

  // ========== OPTIONAL: SERIAL LOGGING ==========
  // Print counter value every 500 ms (non-blocking)
  if (millis() - lastLogTime >= LOG_INTERVAL) {
    lastLogTime = millis();

    // Format and print counter value
    Serial.print("Counter: ");
    Serial.print(currentValue, 2); // Print with 2 decimal places
    Serial.println(" (ISR multiplexing active)");
  }

  // ========== YOUR CODE HERE ==========
  // Add button checks, sensor reads, or other tasks here.
  // The display will continue to update in the background via ISR!
  // Example:
  // if (digitalRead(BUTTON_PIN) == LOW) {
  //   currentValue = 0.0f;
  //   display.setFloat(currentValue);
  // }
}