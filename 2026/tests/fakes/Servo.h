/**
 * @file Servo.h
 * @brief Fake Arduino Servo library for tests.
 *
 * Records the angle the sketch asks for and the angle the real library would
 * actually drive: values below 544 are degrees, clamped to 0-180.
 */

#ifndef TESTS_FAKES_SERVO_H_
#define TESTS_FAKES_SERVO_H_

#include <stdint.h>

#include <vector>

class Servo {
 public:
  uint8_t attach(int pin) {
    pin_ = pin;
    return 0;
  }
  void write(int value) {
    requested_angles_.push_back(value);
    // Same rule as the Arduino Servo library: below 544 means degrees.
    if (value < min_pulse_width_us) {
      angle_ = value < 0 ? 0 : (value > 180 ? 180 : value);
    }
  }
  int read() const { return angle_; }

  bool attached() const { return pin_ >= 0; }
  int pin() const { return pin_; }
  // The value passed to the latest write(), before clamping.
  int last_requested_angle() const { return requested_angles_.back(); }
  const std::vector<int>& requested_angles() const { return requested_angles_; }

 private:
  static constexpr int min_pulse_width_us = 544;

  int pin_ = -1;
  int angle_ = 90;
  std::vector<int> requested_angles_;
};

#endif  // TESTS_FAKES_SERVO_H_
