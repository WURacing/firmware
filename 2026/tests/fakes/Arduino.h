/**
 * @file Arduino.h
 * @brief Fake Arduino core for running sketches on a computer in tests.
 *
 * Provides the parts of the Arduino API our sketches use (pins, timing,
 * Serial) and records what the sketch does with them, so a test can set the
 * inputs, call setup() and loop(), and check the outputs. Time only moves
 * when the sketch calls delay(), the test calls fake::AdvanceTime(), or the
 * test turns on fake::SetAutoAdvanceMicros() for sketches that busy-wait on
 * millis().
 *
 * Add to this file when a sketch needs another Arduino function.
 */

#ifndef TESTS_FAKES_ARDUINO_H_
#define TESTS_FAKES_ARDUINO_H_

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#define PI 3.1415926535897932384626433832795

constexpr int LOW = 0;
constexpr int HIGH = 1;
constexpr int INPUT = 0;
constexpr int OUTPUT = 1;
constexpr int INPUT_PULLUP = 2;
constexpr int DEC = 10;
constexpr int HEX = 16;

// Feather M4 CAN pin numbers. Only their being distinct matters to the tests.
constexpr uint8_t A0 = 14;
constexpr uint8_t A1 = 15;
constexpr uint8_t A2 = 16;
constexpr uint8_t A3 = 17;
constexpr uint8_t A4 = 18;
constexpr uint8_t A5 = 19;
constexpr uint8_t LED_BUILTIN = 13;
constexpr uint8_t PIN_CAN_STANDBY = 40;
constexpr uint8_t PIN_CAN_BOOSTEN = 41;
constexpr uint8_t PIN_NEOPIXEL_POWER = 42;

using std::max;
using std::min;

void pinMode(int pin, int mode);
void digitalWrite(int pin, int value);
int digitalRead(int pin);
int analogRead(int pin);
void analogReadResolution(int bits);
uint32_t millis();
uint32_t micros();
void delay(uint32_t ms);
void delayMicroseconds(uint32_t us);

// Records everything printed or written, instead of sending it anywhere.
class FakeSerial {
 public:
  void begin(uint32_t baud_rate) { baud_rate_ = baud_rate; }
  template <typename T>
  void print(const T& value) {
    std::ostringstream stream;
    stream << value;
    output_ += stream.str();
  }
  // Arduino prints uint8_t as a number; a C++ stream would print a character.
  void print(uint8_t value) { print(static_cast<unsigned>(value)); }
  // Prints an integer in `base` (DEC or HEX), like Arduino: uppercase hex,
  // no prefix or leading zeros.
  template <typename T>
  void print(const T& value, int base) {
    std::ostringstream stream;
    if (base == HEX) {
      stream << std::uppercase << std::hex;
    }
    stream << static_cast<uint32_t>(value);
    output_ += stream.str();
  }
  template <typename T>
  void println(const T& value) {
    print(value);
    output_ += "\r\n";
  }
  template <typename T>
  void println(const T& value, int base) {
    print(value, base);
    output_ += "\r\n";
  }
  void println() { output_ += "\r\n"; }
  size_t write(uint8_t byte) { return write(&byte, 1); }
  size_t write(const uint8_t* data, size_t length) {
    writes_.emplace_back(data, data + length);
    return length;
  }

  uint32_t baud_rate() const { return baud_rate_; }
  // Everything passed to print() and println().
  const std::string& output() const { return output_; }
  // Each write() call's bytes, oldest first.
  const std::vector<std::vector<uint8_t>>& writes() const { return writes_; }
  void Reset() { *this = FakeSerial(); }

 private:
  uint32_t baud_rate_ = 0;
  std::string output_;
  std::vector<std::vector<uint8_t>> writes_;
};

extern FakeSerial Serial;
extern FakeSerial Serial1;

// Test-side controls and records for the fake hardware.
namespace fake {

constexpr int pin_count = 64;
constexpr int unset_mode = -1;

// One call to digitalWrite().
struct PinWrite {
  uint32_t time_ms;
  int pin;
  int value;
};

// A stretch of time during which a pin was HIGH.
struct Pulse {
  uint32_t start_ms;
  uint32_t end_ms;
  uint32_t length_ms() const { return end_ms - start_ms; }
};

// Puts every pin, the clock, Serial, CAN, I2C, and flash back to power-on
// state.
void Reset();

// Sets what digitalRead(pin) returns. Until this is called, an INPUT_PULLUP
// pin reads HIGH and any other input reads LOW.
void SetDigitalInput(int pin, int value);

// Sets what analogRead(pin) returns.
void SetAnalogInput(int pin, int value);

// Moves the clock forward without running any sketch code.
void AdvanceTime(uint32_t ms);

// Makes every millis() and micros() call move the clock forward by `us`, so
// a sketch that busy-waits on millis() can make progress. Default 0 (off).
void SetAutoAdvanceMicros(uint32_t us);

// Returns the mode last passed to pinMode(pin), or unset_mode.
int PinModeOf(int pin);

// Returns the value last written to `pin` with digitalWrite(), or LOW.
int OutputOf(int pin);

// Returns the bits passed to analogReadResolution(), or 10 (Arduino default).
int AnalogResolutionBits();

// Returns every digitalWrite() since the last Reset(), oldest first.
const std::vector<PinWrite>& DigitalWrites();

// Returns each time `pin` was driven HIGH and then LOW again.
std::vector<Pulse> HighPulses(int pin);

// Clears the digitalWrite() history, keeping pin states.
void ClearDigitalWrites();

}  // namespace fake

#endif  // TESTS_FAKES_ARDUINO_H_
