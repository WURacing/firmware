/**
 * @file Arduino.h
 * @brief Fake Arduino core for running sketches on a computer in tests.
 *
 * Provides the parts of the Arduino API our sketches use (pins, timing,
 * Serial) and records what the sketch does with them, so a test can set the
 * inputs, call setup() and loop(), and check the outputs. Time only moves
 * when the sketch calls delay() or the test calls fake::AdvanceTime().
 *
 * Add to this file when a sketch needs another Arduino function.
 */

#ifndef TESTS_FAKES_ARDUINO_H_
#define TESTS_FAKES_ARDUINO_H_

#include <math.h>
#include <stdint.h>

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

// Feather M4 CAN pin numbers. Only their being distinct matters to the tests.
constexpr uint8_t A0 = 14;
constexpr uint8_t A1 = 15;
constexpr uint8_t A2 = 16;
constexpr uint8_t A3 = 17;
constexpr uint8_t A4 = 18;
constexpr uint8_t A5 = 19;
constexpr uint8_t PIN_CAN_STANDBY = 40;
constexpr uint8_t PIN_CAN_BOOSTEN = 41;

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

// Records everything printed, instead of sending it anywhere.
class FakeSerial {
 public:
  void begin(uint32_t baud_rate) { baud_rate_ = baud_rate; }
  template <typename T>
  void print(const T& value) {
    std::ostringstream stream;
    stream << value;
    output_ += stream.str();
  }
  template <typename T>
  void println(const T& value) {
    print(value);
    output_ += "\r\n";
  }
  void println() { output_ += "\r\n"; }

  uint32_t baud_rate() const { return baud_rate_; }
  const std::string& output() const { return output_; }
  void Reset() { *this = FakeSerial(); }

 private:
  uint32_t baud_rate_ = 0;
  std::string output_;
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

// Puts every pin, the clock, and Serial back to power-on state. Also resets
// the fake CAN bus.
void Reset();

// Sets what digitalRead(pin) returns.
void SetDigitalInput(int pin, int value);

// Sets what analogRead(pin) returns.
void SetAnalogInput(int pin, int value);

// Moves the clock forward without running any sketch code.
void AdvanceTime(uint32_t ms);

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
