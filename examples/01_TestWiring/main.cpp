/*
 * Example: 01_TestWiring
 *
 * Runs a blocking diagnostic test to verify correct wiring of the 7-segment display.
 * 
 * This sketch illuminates segments sequentially (a-g, then dp) on all four digits
 * simultaneously. Use this to:
 * - Verify all segments light up
 * - Identify broken segments or digits
 * - Confirm GPIO pin assignments
 * - Test transistor/driver connections
 *
 * **Expected Behavior:**
 * - Segment 'a' lights for 1 second across all 4 digits
 * - Segment 'b' lights for 1 second across all 4 digits
 * - ... and so on for c, d, e, f, g, dp
 * - Serial monitor prints progress
 *
 * **Troubleshooting:**
 * - If a segment doesn't light: check the corresponding GPIO pin and resistor
 * - If a digit doesn't light: check the digit transistor and base resistor
 * - If nothing lights: verify power supply to display
 */

#include <Arduino.h>
#include <avr/pgmspace.h>
#include "SSFD.h"

// ========== PIN CONFIGURATION ==========
// Define pins in PROGMEM (saves RAM on small microcontrollers)
const uint8_t digitPins[] PROGMEM = {10, 11, 12, 13};     // Digit control pins
const uint8_t segmentPins[] PROGMEM = {2, 3, 4, 5, 6, 7, 8, 9}; // Segment a-g, dp

// ========== DISPLAY INSTANCE ==========
SevenSegment display(segmentPins, digitPins);

// ========== SETUP ==========
void setup() {
  Serial.begin(9600);
  delay(500); // Wait for Serial to stabilize
  
  Serial.println("\n========================================");
  Serial.println("   SSFD: 7-Segment Wiring Test");
  Serial.println("========================================\n");

  // Initialize display with error checking
  SevenSegment::Error err = display.begin();
  if (err != SevenSegment::Error::OK) {
    Serial.print("❌ ERROR: Display initialization failed (code ");
    Serial.print((uint8_t)err);
    Serial.println(")");
    Serial.println("   Check pin assignments and power supply");
    while (1); // Halt on initialization failure
  }

  Serial.println("✓ Display initialized successfully\n");
  Serial.println("Starting wiring diagnostic...");
  Serial.println("Each segment will light for 1 second.\n");

  // Run the blocking wiring test
  // This illuminates each segment sequentially (a-g, then dp)
  display.testWiring(1000); // 1000 ms = 1 second per segment

  Serial.println("\n✓ Wiring test complete!");
  Serial.println("\nInterpret results:");
  Serial.println("  • All segments lit? → Wiring is correct");
  Serial.println("  • Some segments dark? → Check that GPIO pin");
  Serial.println("  • Whole digit dark? → Check digit transistor");
  Serial.println("\nYou can now use the FloatCounter example.\n");
}

// ========== LOOP ==========
void loop() {
  // This example only runs once in setup()
  // Nothing to do here
}