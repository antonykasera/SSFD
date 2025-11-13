#ifndef SSFD_H
#define SSFD_H

#include <Arduino.h>
#include <avr/pgmspace.h>

/**
 * @file SSFD.h
 * @brief Seven-Segment Four-Digit Display Library
 * @author Antony Kasera
 * @version 1.0.0
 * 
 * A robust, non-blocking library for controlling a common-cathode 7-segment
 * 4-digit display via multiplexing with Timer1 ISR. Supports integers, floats,
 * text, and custom segments with full bounds checking and safety guards.
 * 
 * **Requirements:**
 * - ATmega328P (Arduino Uno/Nano) or compatible
 * - 16 MHz clock frequency (Timer1 calibrated for this)
 * - Timer1 availability (not used by other code)
 * 
 * **Usage:**
 * ```cpp
 * const uint8_t digitPins[] PROGMEM = {10, 11, 12, 13};
 * const uint8_t segmentPins[] PROGMEM = {2, 3, 4, 5, 6, 7, 8, 9};
 * SevenSegment display(segmentPins, digitPins);
 * 
 * void setup() {
 *   if (!display.begin()) Serial.println("Init failed!");
 *   display.setNumber(1234);
 * }
 * 
 * void loop() {
 *   display.refresh(); // Non-blocking multiplexing
 * }
 * ```
 */

class SevenSegment {
public:
  // ========== CONSTANTS ==========
  static constexpr uint8_t NUM_DIGITS = 4;
  static constexpr uint8_t NUM_SEGMENTS = 8;
  static constexpr uint8_t MAX_PIN = 53;     // Arduino Uno max pin
  static constexpr uint16_t MAX_VALUE = 9999;
  static constexpr float MAX_FLOAT = 99.99f;

  // Error codes
  enum class Error : uint8_t {
    OK = 0,
    NULL_POINTER = 1,
    INVALID_PIN = 2,
    TIMER_INIT_FAILED = 3,
    NOT_INITIALIZED = 4,
    INVALID_ARGUMENT = 5
  };

  // ========== CONSTRUCTOR ==========
  /**
   * @brief Construct a SevenSegment display handler
   * @param segmentPins PROGMEM array of 8 GPIO pins for segments (a-g, dp)
   * @param digitPins PROGMEM array of 4 GPIO pins for digit control
   * @warning Both arrays MUST be in PROGMEM and contain exactly the expected values
   */
  SevenSegment(const uint8_t* segmentPins, const uint8_t* digitPins);

  volatile bool _isrActive;

  // ========== CORE FUNCTIONS ==========
  /**
   * @brief Initialize display pins and Timer1 ISR for multiplexing
   * @return Error code (Error::OK on success)
   * @note MUST be called in setup(); display will not work without this
   */
  Error begin();

  /**
   * @brief Disable Timer1 and perform cleanup
   * @note Call before reinitializing or if Timer1 is needed elsewhere
   */
  void end();

  /**
   * @brief Refresh the display (non-blocking multiplexing step)
   * @note Must be called frequently in loop() for stable display
   * @warning If called from ISR context, this is a no-op
   */
  void refresh();

  /**
   * @brief Clear all digits (display blank)
   */
  void clear();

  /**
   * @brief Test wiring and detect faults (BLOCKING)
   * @param delayMs Duration (ms) to illuminate each segment
   * @note Call only in setup(); blocks execution
   */
  void testWiring(unsigned int delayMs = 500);

  /**
   * @brief Check if display is properly initialized
   * @return true if begin() succeeded
   */
  bool isInitialized() const { return _isrActive; }

  /**
   * @brief Get last initialization error
   * @return Error code from most recent begin() call
   */
  Error getLastError() const { return _lastError; }

  // ========== DISPLAY DATA FUNCTIONS ==========
  /**
   * @brief Display a 4-digit unsigned integer
   * @param value 0..9999 (values > 9999 clamped)
   * @param dpPosition Position of decimal point: -1 (none), 0..3 (after digit N)
   * @note dpPosition=1 displays as "XXX.X" (decimal after 2nd digit from left)
   */
  void setNumber(uint16_t value, int8_t dpPosition = -1);

  /**
   * @brief Display a floating-point number with auto-decimal placement
   * @param value 0.00..99.99 (clamped and rounded)
   * @return Error code if NaN/Inf detected
   * @note Decimal point position chosen based on magnitude (e.g., 1.23 vs 12.34)
   */
  Error setFloat(float value);

  /**
   * @brief Display text (up to 4 ASCII characters)
   * @param text String to display; strlen must be <= 4
   * @return Error code if text invalid
   * @note Supported: digits 0-9, A-Z (uppercase), space, dash, equals
   */
  Error setText(const char* text);

  /**
   * @brief Display raw 7-segment patterns (advanced)
   * @param patterns Array of 4 bytes; bit 7=a, 6=b, ..., 0=dp
   */
  void setSegments(const uint8_t patterns[NUM_DIGITS]);

  /**
   * @brief Set integer value using hundredths (avoids float math)
   * @param hundredths Value in hundredths: 0..9999 = 0.00..99.99
   * @param dpPosition Decimal point position (-1..3)
   */
  void setHundredths(uint16_t hundredths, int8_t dpPosition = 2);

  // ========== DISPLAY MODES ==========
  /**
   * @brief Enable/disable leading zero suppression
   * @param enabled true = "  1.2"; false = "001.2"
   */
  void setLeadingZeros(bool enabled);

  /**
   * @brief Set refresh interval (multiplexing speed)
   * @param ms Interval 1..255 (milliseconds between digit cycles)
   * @note Lower = faster refresh, brighter; higher = dimmer but less ISR load
   */
  void setRefreshInterval(uint8_t ms);

  /**
   * @brief Start blinking the display
   * @param intervalMs Blink period in milliseconds (typically 500)
   * @note Calls blinkEnable(true) automatically
   */
  void startBlink(unsigned long intervalMs = 500);

  /**
   * @brief Stop blinking the display
   */
  void stopBlink();

  /**
   * @brief Check if display is currently blinking
   * @return true if blinking
   */
  bool isBlinking() const { return _blinkEnabled; }

   /**
   * @brief Perform one multiplexing cycle (called by ISR or refresh())
   */
  void multiplex();

  // ========== PRIVATE MEMBERS ==========
private:
  // Pin arrays (stored as PROGMEM pointers)
  const uint8_t* _segmentPins;
  const uint8_t* _digitPins;

  // Display state
  volatile uint8_t _displayPatterns[NUM_DIGITS];
  bool _leadingZeros;

  // Multiplexing
  volatile uint8_t _currentDigit;
  uint8_t _refreshIntervalMs;
  unsigned long _lastRefreshTime;

  // Blinking
  bool _blinkEnabled;
  volatile bool _blinkStateOn;
  unsigned long _blinkInterval;
  unsigned long _blinkLastToggle;

  // ISR safety
  Error _lastError;

  // Helper functions
  /**
   * @brief Convert ASCII character to 7-segment pattern
   * @return Segment pattern, or 0 if character not supported
   */
  uint8_t getPattern(char c);

  /**
   * @brief Validate pin value
   * @return true if pin in valid range [0..MAX_PIN]
   */
  static bool isPinValid(uint8_t pin);

};

#endif // SSFD_H