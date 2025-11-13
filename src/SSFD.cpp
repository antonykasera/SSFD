/**
 * @file SSFD.cpp
 * @brief Seven-Segment Four-Digit Display Library Implementation
 */

#include "SSFD.h"
#include <avr/pgmspace.h>
#include <string.h>
#include <math.h>

// ========== PROGMEM LOOKUP TABLE ==========
/**
 * 7-segment mappings for digits and symbols
 * Bit layout: 7=a, 6=b, 5=c, 4=d, 3=e, 2=f, 1=g, 0=dp
 *
 *     a
 *  f     b
 *     g
 *  e     c
 *     d    dp
 */
const uint8_t SEGMENT_PATTERNS[] PROGMEM = {

    // Digits 0-9
    0b11111100, // 0: a,b,c,d,e,f
    0b01100000, // 1: b,c
    0b11011010, // 2: a,b,d,e,g
    0b11110010, // 3: a,b,c,d,g
    0b01100110, // 4: b,c,f,g
    0b10110110, // 5: a,c,d,f,g
    0b10111110, // 6: a,c,d,e,f,g
    0b11100000, // 7: a,b,c
    0b11111110, // 8: all except dp
    0b11110110, // 9: a,b,c,d,f,g
    0b00000000, // 10: BLANK
    0b00000001, // 11: dp only

    // Extended characters
    0b11101110, // 12: A (a,b,c,e,f,g)
    0b00111110, // 13: b (c,d,e,f,g)
    0b10011100, // 14: C (a,d,e,f) — uppercase C
    0b01111010, // 15: d (b,c,d,e,g)
    0b10011110, // 16: E (a,d,e,f,g)
    0b10001110, // 17: F (a,e,f,g)
    0b10111100, // 18: G (a,c,d,e,f)
    0b01101110, // 19: H (b,c,e,f,g)
    0b01100000, // 20: I (b,c)
    0b01111000, // 21: J (b,c,d,e)
    0b00001110, // 22: K (e,f,g) — approximation
    0b00011100, // 23: L (d,e,f)
    0b10101000, // 24: M (a,c,e) — approximation
    0b00101010, // 25: n (c,e,g)
    0b11111100, // 26: O (a,b,c,d,e,f)
    0b11001110, // 27: P (a,b,e,f,g)
    0b11110110, // 28: Q (a,b,c,d,f,g)
    0b00001010, // 29: r (e,g)
    0b10110110, // 30: S (a,c,d,f,g)
    0b00011110, // 31: T (d,e,f,g)
    0b01111100, // 32: U (b,c,d,e,f)
    0b01110000, // 33: V (b,c,d,e) — approximation
    0b01010100, // 34: W (b,c,d,f) — approximation
    0b01001110, // 35: X (b,c,e,f,g) — approximation
    0b01110110, // 36: Y (b,c,d,f,g)
    0b11011010, // 37: Z (a,b,d,e,g)

    // Extended symbols
    0b00000000, // 38: SPACE
    0b00000010, // 39: dash/minus (g)
    0b11000000, // 40: equals (b,c,d,e) — approximation
};

// Static instance pointer for ISR
static SevenSegment *_isrInstance = nullptr;

// ========== ISR HANDLER ==========
/**
 * Timer1 Compare Match ISR
 */
ISR(TIMER1_COMPA_vect)
{
    if (_isrInstance != nullptr && _isrInstance->_isrActive)
    {
        _isrInstance->multiplex();
    }
}

// ========== CONSTRUCTOR ==========
SevenSegment::SevenSegment(const uint8_t *segmentPins, const uint8_t *digitPins)
    : _isrActive(false),
      _segmentPins(segmentPins),
      _digitPins(digitPins),
      _leadingZeros(true),
      _currentDigit(0),
      _refreshIntervalMs(3),
      _lastRefreshTime(0),
      _blinkEnabled(false),
      _blinkStateOn(true),
      _blinkInterval(500),
      _blinkLastToggle(0),
      _lastError(Error::OK)
{
    memset((void *)_displayPatterns, 0, NUM_DIGITS);
    _isrInstance = this;
}

// ========== VALIDATION HELPERS ==========
bool SevenSegment::isPinValid(uint8_t pin)
{
    return pin <= MAX_PIN;
}

// ========== begin() ==========
SevenSegment::Error SevenSegment::begin()
{
    // Validate pointers
    if (_segmentPins == nullptr || _digitPins == nullptr)
    {
        _lastError = Error::NULL_POINTER;
        return _lastError;
    }

    // Validate all pins before initializing
    for (uint8_t i = 0; i < NUM_SEGMENTS; i++)
    {
        uint8_t pin = pgm_read_byte(&_segmentPins[i]);
        if (!isPinValid(pin))
        {
            _lastError = Error::INVALID_PIN;
            return _lastError;
        }
    }

    for (uint8_t i = 0; i < NUM_DIGITS; i++)
    {
        uint8_t pin = pgm_read_byte(&_digitPins[i]);
        if (!isPinValid(pin))
        {
            _lastError = Error::INVALID_PIN;
            return _lastError;
        }
    }

    // Initialize segment pins
    for (uint8_t i = 0; i < NUM_SEGMENTS; i++)
    {
        uint8_t pin = pgm_read_byte(&_segmentPins[i]);
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    // Initialize digit pins
    for (uint8_t i = 0; i < NUM_DIGITS; i++)
    {
        uint8_t pin = pgm_read_byte(&_digitPins[i]);
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    // Setup Timer1 for ~80 Hz multiplexing (16MHz / 64 / 3125)
    cli(); 
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    OCR1A = 499;
    TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10); // CTC mode, /64 prescale
    TIMSK1 |= (1 << OCIE1A);
    _isrActive = true;
    sei(); 

    _lastError = Error::OK;
    return _lastError;
}

// ========== end() ==========
void SevenSegment::end()
{
    cli();
    _isrActive = false;
    TIMSK1 &= ~(1 << OCIE1A);
    sei();
    clear();
}

// ========== refresh() ==========
void SevenSegment::refresh()
{
    // Non-blocking refresh — allows manual multiplexing if not using ISR
    // (Currently ISR handles it, but kept for API flexibility)
    if (_blinkEnabled)
    {
        unsigned long now = millis();
        if (now - _blinkLastToggle >= _blinkInterval)
        {
            _blinkStateOn = !_blinkStateOn;
            _blinkLastToggle = now;
        }
    }
}

// ========== multiplex() ==========
void SevenSegment::multiplex()
{

    uint8_t prevDigit = _currentDigit;
    digitalWrite(pgm_read_byte(&_digitPins[prevDigit]), LOW);

    _currentDigit = (_currentDigit + 1) % NUM_DIGITS;

    if (_blinkEnabled && !_blinkStateOn)
    {
        return;
    }

    // Read pattern for current digit (volatile safe)
    uint8_t pattern = _displayPatterns[_currentDigit];

    // Set all segment pins according to pattern
    for (uint8_t bit = 0; bit < NUM_SEGMENTS; bit++)
    {
        uint8_t pin = pgm_read_byte(&_segmentPins[bit]);
        if (pattern & (1 << (7 - bit)))
        {
            digitalWrite(pin, HIGH);
        }
        else
        {
            digitalWrite(pin, LOW);
        }
    }

    digitalWrite(pgm_read_byte(&_digitPins[_currentDigit]), HIGH);
}

// ========== clear() ==========
void SevenSegment::clear()
{
    uint8_t blankPattern = pgm_read_byte(&SEGMENT_PATTERNS[10]);
    cli();
    for (uint8_t i = 0; i < NUM_DIGITS; i++)
    {
        _displayPatterns[i] = blankPattern;
    }
    sei();
}

// ========== testWiring() ==========
void SevenSegment::testWiring(unsigned int delayMs)
{
    if (!_isrActive)
    {
        return;
    }

    cli();
    TIMSK1 &= ~(1 << OCIE1A);

    for (uint8_t d = 0; d < NUM_DIGITS; d++)
    {
        digitalWrite(pgm_read_byte(&_digitPins[d]), HIGH);
    }

    for (uint8_t s = 0; s < NUM_SEGMENTS; s++)
    {
        digitalWrite(pgm_read_byte(&_segmentPins[s]), HIGH);
        sei();
        delay(delayMs);
        cli();
        digitalWrite(pgm_read_byte(&_segmentPins[s]), LOW);
    }

    for (uint8_t d = 0; d < NUM_DIGITS; d++)
    {
        digitalWrite(pgm_read_byte(&_digitPins[d]), LOW);
    }

    TIMSK1 |= (1 << OCIE1A);
    sei();
}

// ========== setNumber() ==========
void SevenSegment::setNumber(uint16_t value, int8_t dpPosition)
{
    // Bounds checking
    if (value > MAX_VALUE)
    {
        value = MAX_VALUE;
    }
    if (dpPosition < -1 || dpPosition > 3)
    {
        dpPosition = -1;
    }

    // Extract digits
    uint8_t digits[NUM_DIGITS];
    uint16_t temp = value;
    for (int8_t i = NUM_DIGITS - 1; i >= 0; i--)
    {
        digits[i] = temp % 10;
        temp /= 10;
    }

    // Build patterns with leading zero suppression
    bool isLeading = true;
    cli();
    for (uint8_t i = 0; i < NUM_DIGITS; i++)
    {
        uint8_t digit = digits[i];
        uint8_t pattern = pgm_read_byte(&SEGMENT_PATTERNS[digit]);

        // Suppress leading zeros if enabled
        if (!_leadingZeros && digit == 0 && isLeading && i < NUM_DIGITS - 1)
        {
            pattern = pgm_read_byte(&SEGMENT_PATTERNS[10]);
        }
        else if (digit != 0 || !isLeading || i == NUM_DIGITS - 1)
        {
            isLeading = false;
        }

        if (i == dpPosition && dpPosition >= 0)
        {
            pattern |= pgm_read_byte(&SEGMENT_PATTERNS[11]); // OR with DP
        }

        _displayPatterns[i] = pattern;
    }
    sei();
}

// ========== setFloat() ==========
SevenSegment::Error SevenSegment::setFloat(float value)
{
    // Check for NaN or Inf
    if (isnan(value) || isinf(value))
    {
        setText("Err ");
        return _lastError = Error::INVALID_ARGUMENT;
    }

    // Handle negative numbers
    if (value < 0.0f)
    {
        // We can only show 3 digits and a negative sign
        if (value <= -100.0f)
        {
            setText("-999"); 
        }
        else
        {
            uint16_t numToDisplay;
            int8_t dpPosition;
            if (value > -10.0f)
            {
                numToDisplay = (uint16_t)roundf(fabs(value) * 100.0f);
                dpPosition = 1;
            }
            else
            {
                numToDisplay = (uint16_t)roundf(fabs(value) * 10.0f);
                dpPosition = 2;
            }
            setNumber(numToDisplay, dpPosition);
            cli();
            _displayPatterns[0] = getPattern('-');
            sei();
        }
        return _lastError = Error::OK;
    }

    // Handle positive numbers
    uint16_t numToDisplay;
    int8_t dpPosition = -1;
    int intPart = (int)value;
    int numIntDigits = 1;

    if (intPart >= 1000)
        numIntDigits = 4;
    else if (intPart >= 100)
        numIntDigits = 3;
    else if (intPart >= 10)
        numIntDigits = 2;

    // The number of decimals we can show is 4 - numIntDigits
    int numDecimals = 4 - numIntDigits;
    if (numDecimals < 0)
        numDecimals = 0;

    dpPosition = numIntDigits - 1; // 0="X.XXX", 1="XX.XX", 2="XXX.X", 3="XXXX"
    if (dpPosition == 3)
        dpPosition = -1;

    double multiplier = pow(10, numDecimals);
    numToDisplay = (uint16_t)round(value * multiplier);

    if (numToDisplay > 9999)
    {
        numToDisplay = 9999;
    }

    setNumber(numToDisplay, dpPosition);
    return _lastError = Error::OK;
}

// ========== setText() ==========
SevenSegment::Error SevenSegment::setText(const char *text)
{
    if (text == nullptr)
    {
        return Error::NULL_POINTER;
    }

    uint8_t len = strlen(text);
    if (len > NUM_DIGITS)
    {
        return Error::INVALID_ARGUMENT;
    }

    char padded[NUM_DIGITS + 1] = {' ', ' ', ' ', ' ', '\0'};
    strncpy(padded, text, len);

    cli();
    for (uint8_t i = 0; i < NUM_DIGITS; i++)
    {
        _displayPatterns[i] = getPattern(padded[i]);
    }
    sei();

    return Error::OK;
}

// ========== setSegments() ==========
void SevenSegment::setSegments(const uint8_t patterns[NUM_DIGITS])
{
    if (patterns == nullptr)
    {
        return;
    }

    cli();
    for (uint8_t i = 0; i < NUM_DIGITS; i++)
    {
        _displayPatterns[i] = patterns[i];
    }
    sei();
}

// ========== setHundredths() ==========
void SevenSegment::setHundredths(uint16_t hundredths, int8_t dpPosition)
{
    if (hundredths > 9999)
    {
        hundredths = 9999;
    }
    if (dpPosition < -1 || dpPosition > 3)
    {
        dpPosition = 2; // Default to 2 decimal places
    }

    setNumber(hundredths, dpPosition);
}

// ========== getPattern() ==========
uint8_t SevenSegment::getPattern(char c)
{
    // Map character to pattern index
    uint8_t index = 10;

    if (c >= '0' && c <= '9')
    {
        index = c - '0'; // 0-9
    }
    else if (c >= 'A' && c <= 'Z')
    {
        index = 12 + (c - 'A'); // A-Z (indices 12-37)
    }
    else if (c >= 'a' && c <= 'z')
    {
        index = 12 + (c - 'a'); // Treat lowercase same as uppercase
    }
    else if (c == ' ')
    {
        index = 38; // SPACE
    }
    else if (c == '-')
    {
        index = 39; // DASH
    }
    else if (c == '=')
    {
        index = 40; // EQUALS
    }
    else if (c == '.')
    {
        index = 11; // DP only
    }

    // Bounds check
    if (index >= (sizeof(SEGMENT_PATTERNS) / sizeof(SEGMENT_PATTERNS[0])))
    {
        index = 10;
    }

    return pgm_read_byte(&SEGMENT_PATTERNS[index]);
}

// ========== setLeadingZeros() ==========
void SevenSegment::setLeadingZeros(bool enabled)
{
    _leadingZeros = enabled;
}

// ========== setRefreshInterval() ==========
void SevenSegment::setRefreshInterval(uint8_t ms)
{
    if (ms < 1)
        ms = 1;
    if (ms > 255)
        ms = 255;
    _refreshIntervalMs = ms;
}

// ========== startBlink() ==========
void SevenSegment::startBlink(unsigned long intervalMs)
{
    _blinkInterval = intervalMs;
    _blinkEnabled = true;
    _blinkStateOn = true;
    _blinkLastToggle = millis();
}

// ========== stopBlink() ==========
void SevenSegment::stopBlink()
{
    _blinkEnabled = false;
    _blinkStateOn = true;
}