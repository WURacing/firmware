/**
 * @file Adafruit_NeoPixel.h
 * @brief Fake Adafruit NeoPixel library for tests. Records what was shown.
 */

#ifndef TESTS_FAKES_ADAFRUIT_NEOPIXEL_H_
#define TESTS_FAKES_ADAFRUIT_NEOPIXEL_H_

#include <stdint.h>

#include <vector>

constexpr uint16_t NEO_GRB = 0x52;
constexpr uint16_t NEO_KHZ800 = 0x0000;

class Adafruit_NeoPixel {
 public:
  Adafruit_NeoPixel(uint16_t count, int16_t pin, uint16_t type)
      : pin_(pin), type_(type), colors_(count, 0) {}

  void begin() { begun_ = true; }
  void clear() {
    for (uint32_t& color : colors_) {
      color = 0;
    }
  }
  void setBrightness(uint8_t brightness) { brightness_ = brightness; }
  void setPixelColor(uint16_t index, uint32_t color) {
    if (index < colors_.size()) {
      colors_[index] = color;
    }
  }
  void show() { ++show_count_; }

  int16_t pin() const { return pin_; }
  uint16_t type() const { return type_; }
  bool begun() const { return begun_; }
  uint8_t brightness() const { return brightness_; }
  int show_count() const { return show_count_; }
  uint32_t color(uint16_t index) const { return colors_[index]; }

 private:
  int16_t pin_;
  uint16_t type_;
  std::vector<uint32_t> colors_;
  bool begun_ = false;
  uint8_t brightness_ = 255;
  int show_count_ = 0;
};

#endif  // TESTS_FAKES_ADAFRUIT_NEOPIXEL_H_
