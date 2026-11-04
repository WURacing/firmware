/**
 * @file fake_hardware.cpp
 * @brief State behind the fake Arduino core and CAN library.
 */

#include <Arduino.h>
#include <CAN.h>

FakeSerial Serial;
FakeSerial Serial1;
CANSAME5x CAN;

namespace {

int pin_modes[fake::pin_count];
int digital_inputs[fake::pin_count];
int analog_inputs[fake::pin_count];
int outputs[fake::pin_count];
int analog_resolution_bits = 10;
uint32_t now_ms = 0;
std::vector<fake::PinWrite> digital_writes;

bool ValidPin(int pin) { return pin >= 0 && pin < fake::pin_count; }

}  // namespace

void pinMode(int pin, int mode) {
  if (ValidPin(pin)) {
    pin_modes[pin] = mode;
  }
}

void digitalWrite(int pin, int value) {
  if (ValidPin(pin)) {
    outputs[pin] = value ? HIGH : LOW;
    digital_writes.push_back({now_ms, pin, outputs[pin]});
  }
}

int digitalRead(int pin) {
  if (!ValidPin(pin)) {
    return LOW;
  }
  return pin_modes[pin] == OUTPUT ? outputs[pin] : digital_inputs[pin];
}

int analogRead(int pin) { return ValidPin(pin) ? analog_inputs[pin] : 0; }

void analogReadResolution(int bits) { analog_resolution_bits = bits; }

uint32_t millis() { return now_ms; }

uint32_t micros() { return now_ms * 1000; }

void delay(uint32_t ms) { now_ms += ms; }

int CANSAME5x::begin(int32_t baud_rate) {
  baud_rate_ = baud_rate;
  return fail_begin_ ? 0 : 1;
}

int CANSAME5x::beginPacket(int id, int /*dlc*/, bool /*rtr*/) {
  packet_ = Frame{millis(), id, {}};
  packet_open_ = true;
  return 1;
}

size_t CANSAME5x::write(uint8_t byte) {
  // A classic CAN frame holds at most 8 bytes.
  if (!packet_open_ || packet_.data.size() >= 8) {
    return 0;
  }
  packet_.data.push_back(byte);
  return 1;
}

int CANSAME5x::endPacket() {
  if (!packet_open_) {
    return 0;
  }
  sent_frames_.push_back(packet_);
  packet_open_ = false;
  return 1;
}

namespace fake {

void Reset() {
  for (int pin = 0; pin < pin_count; ++pin) {
    pin_modes[pin] = unset_mode;
    digital_inputs[pin] = LOW;
    analog_inputs[pin] = 0;
    outputs[pin] = LOW;
  }
  analog_resolution_bits = 10;
  now_ms = 0;
  digital_writes.clear();
  Serial.Reset();
  Serial1.Reset();
  CAN.Reset();
}

void SetDigitalInput(int pin, int value) {
  if (ValidPin(pin)) {
    digital_inputs[pin] = value ? HIGH : LOW;
  }
}

void SetAnalogInput(int pin, int value) {
  if (ValidPin(pin)) {
    analog_inputs[pin] = value;
  }
}

void AdvanceTime(uint32_t ms) { now_ms += ms; }

int PinModeOf(int pin) { return ValidPin(pin) ? pin_modes[pin] : unset_mode; }

int OutputOf(int pin) { return ValidPin(pin) ? outputs[pin] : LOW; }

int AnalogResolutionBits() { return analog_resolution_bits; }

const std::vector<PinWrite>& DigitalWrites() { return digital_writes; }

std::vector<Pulse> HighPulses(int pin) {
  std::vector<Pulse> pulses;
  bool high = false;
  uint32_t start_ms = 0;
  for (const PinWrite& write : digital_writes) {
    if (write.pin != pin) {
      continue;
    }
    if (write.value == HIGH && !high) {
      high = true;
      start_ms = write.time_ms;
    } else if (write.value == LOW && high) {
      high = false;
      pulses.push_back({start_ms, write.time_ms});
    }
  }
  return pulses;
}

void ClearDigitalWrites() { digital_writes.clear(); }

}  // namespace fake
