/*
 * Example: 03_AdvancedFeatures
 *
 * Demonstrates advanced features of the SSFD library:
 * - Text display with character support
 * - Decimal point positioning
 * - Blinking mode
 * - Error handling
 * - Integer hundredths API (avoids float math)
 *
 * Press different button combinations (simulated with timing) to:
 * - Display text ("HELP", "GOOD", "FAIL")
 * - Display numbers with custom decimal positions
 * - Enable/disable blinking
 * - Switch between display modes
 */

#include <Arduino.h>
#include <avr/pgmspace.h>
#include "SSFD.h"

// ========== PIN CONFIGURATION ==========
const uint8_t digitPins[] PROGMEM = {10, 11, 12, 13};
const uint8_t segmentPins[] PROGMEM = {2, 3, 4, 5, 6, 7, 8, 9};

// Optional button pins (for interactive control)
// const uint8_t BTN_MODE = A0;
// const uint8_t BTN_BLINK = A1;

// ========== DISPLAY INSTANCE ==========
SevenSegment display(segmentPins, digitPins);

// ========== FORWARD DECLARATIONS ==========
void runDemoMode(unsigned long now);
void runCounterMode(unsigned long now);
void runTextMode(unsigned long now);
void runFixedPointMode(unsigned long now);
void switchMode();

// ========== STATE MACHINE ==========
enum DisplayMode : uint8_t
{
    MODE_COUNTER,     // Float counter (like example 02)
    MODE_TEXT,        // Display text
    MODE_FIXED_POINT, // Use integer hundredths (fast, no float)
    MODE_DEMO,        // Cycle through examples
};

DisplayMode currentMode = MODE_DEMO;
unsigned long modeStartTime = 0;
unsigned long demoStepTime = 0;
const unsigned long DEMO_STEP_INTERVAL = 2000; // 2 seconds per demo step

// ========== COUNTERS ==========
float floatCounter = 0.0f;
unsigned long lastCounterUpdate = 0;

// ========== SETUP ==========
void setup()
{
    Serial.begin(9600);
    delay(500);

    Serial.println("\n========================================");
    Serial.println("   SSFD: Advanced Features Demo");
    Serial.println("========================================\n");

    // Initialize display
    SevenSegment::Error err = display.begin();
    if (err != SevenSegment::Error::OK)
    {
        Serial.println("Display init failed!");
        while (1)
            ;
    }

    Serial.println("Display initialized\n");
    Serial.println("Running demo mode...\n");

    display.setLeadingZeros(false);
    modeStartTime = millis();
    demoStepTime = millis();
}

// ========== LOOP ==========
void loop()
{
    unsigned long now = millis();

    switch (currentMode)
    {
    case MODE_DEMO:
        runDemoMode(now);
        break;

    case MODE_COUNTER:
        runCounterMode(now);
        break;

    case MODE_TEXT:
        runTextMode(now);
        break;

    case MODE_FIXED_POINT:
        runFixedPointMode(now);
        break;
    }

    // Display refresh (handles blink state)
    display.refresh();

    // Optional: check buttons to switch modes
    // if (digitalRead(BTN_MODE) == LOW) {
    //   delay(20); // Debounce
    //   switchMode();
    // }
}

// ========== MODE: DEMO ==========
void runDemoMode(unsigned long now)
{
    if (now - demoStepTime >= DEMO_STEP_INTERVAL)
    {
        demoStepTime = now;

        static uint8_t step = 0;
        switch (step)
        {
        case 0:
            Serial.println("Demo Step 1: Display number 1234");
            display.setNumber(1234);
            break;

        case 1:
            Serial.println("Demo Step 2: Display float 56.78");
            display.setFloat(56.78f);
            break;

        case 2:
            Serial.println("Demo Step 3: Display text 'HELP'");
            display.setText("HELP");
            break;

        case 3:
            Serial.println("Demo Step 4: Display text 'GOOD'");
            display.setText("GOOD");
            break;

        case 4:
            Serial.println("Demo Step 5: Number with decimal at position 0");
            display.setNumber(5678, 0);
            break;

        case 5:
            Serial.println("Demo Step 6: Start blinking");
            display.startBlink(300);
            break;

        case 6:
            Serial.println("Demo Step 7: Stop blinking");
            display.stopBlink();
            display.setText("END");
            break;

        case 7:
            Serial.println("Demo complete! Looping...\n");
            step = 255; // Will wrap to 0
            break;
        }

        step++;
    }
}

// ========== MODE: COUNTER ==========
void runCounterMode(unsigned long now)
{
    if (now - lastCounterUpdate >= 100)
    {
        lastCounterUpdate = now;
        floatCounter += 0.01f;
        if (floatCounter > 99.99f)
            floatCounter = 0.0f;
        display.setFloat(floatCounter);
    }
}

// ========== MODE: TEXT ==========
void runTextMode(unsigned long now)
{
    static unsigned long textSwitchTime = 0;
    static uint8_t textIndex = 0;

    if (now - textSwitchTime >= 3000)
    {
        textSwitchTime = now;

        const char *texts[] = {"SSFD", "TEST", "GOOD", "HELP"};
        const uint8_t textCount = 4;

        display.setText(texts[textIndex]);
        Serial.print("Text: ");
        Serial.println(texts[textIndex]);

        textIndex = (textIndex + 1) % textCount;
    }
}

// ========== MODE: FIXED POINT (INTEGER HUNDREDTHS) ==========
void runFixedPointMode(unsigned long now)
{
    static unsigned long fpUpdateTime = 0;
    static uint16_t hundredths = 0;

    if (now - fpUpdateTime >= 50)
    {
        fpUpdateTime = now;
        hundredths += 10; // Increment by 0.10
        if (hundredths > 9999)
            hundredths = 0;

        // Use integer API (no float math needed!)
        display.setHundredths(hundredths, 2);
    }
}

// ========== HELPER: SWITCH MODE ==========
void switchMode()
{
    currentMode = (DisplayMode)((currentMode + 1) % 4);
    Serial.print("Mode switched to: ");
    Serial.println((uint8_t)currentMode);
    modeStartTime = millis();
}