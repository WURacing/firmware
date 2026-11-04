/**
 * @file Adafruit_VL6180X.h
 * @brief Fake Adafruit VL6180X library for tests.
 *
 * Only begin() is faked: it succeeds when a device answers at 0x29. Sketches
 * that talk to the sensor's registers directly go through the fake Wire bus.
 */

#ifndef TESTS_FAKES_ADAFRUIT_VL6180X_H_
#define TESTS_FAKES_ADAFRUIT_VL6180X_H_

#include <Wire.h>

class Adafruit_VL6180X {
 public:
  static constexpr int address = 0x29;

  bool begin(TwoWire* /*wire*/ = &Wire) {
    ++begin_count_;
    return fake::I2cReachable(address);
  }
  int begin_count() const { return begin_count_; }

 private:
  int begin_count_ = 0;
};

#endif  // TESTS_FAKES_ADAFRUIT_VL6180X_H_
