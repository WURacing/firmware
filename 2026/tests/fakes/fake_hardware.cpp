/**
 * @file fake_hardware.cpp
 * @brief State behind the fake Arduino core, CAN, I2C, and flash libraries.
 */

#include <Adafruit_SPIFlashBase.h>
#include <Arduino.h>
#include <CANSAME5x.h>
#include <Wire.h>

#include <map>

FakeSerial Serial;
FakeSerial Serial1;
TwoWire Wire;

namespace {

int pin_modes[fake::pin_count];
int digital_inputs[fake::pin_count];
bool digital_input_set[fake::pin_count];
int analog_inputs[fake::pin_count];
int outputs[fake::pin_count];
int analog_resolution_bits = 10;
uint64_t now_us = 0;
uint32_t auto_advance_us = 0;
std::vector<fake::PinWrite> digital_writes;

// I2C devices by address. Channel fake::main_bus is outside the mux.
struct AttachedDevice {
  fake::I2cDevice* device;
  int mux_channel;
};
std::multimap<int, AttachedDevice> i2c_devices;
int i2c_mux_address = -1;
uint8_t i2c_mux_selection = 0;

fake::FakeFlash flash;

bool ValidPin(int pin) { return pin >= 0 && pin < fake::pin_count; }

uint32_t NowMs() { return static_cast<uint32_t>(now_us / 1000); }

// Returns the device that answers at `address` right now, or nullptr.
fake::I2cDevice* ReachableDevice(int address) {
  const auto range = i2c_devices.equal_range(address);
  for (auto it = range.first; it != range.second; ++it) {
    const int channel = it->second.mux_channel;
    if (channel == fake::main_bus ||
        (i2c_mux_address >= 0 && (i2c_mux_selection >> channel) & 1)) {
      return it->second.device;
    }
  }
  return nullptr;
}

}  // namespace

// Arduino core.

void pinMode(int pin, int mode) {
  if (ValidPin(pin)) {
    pin_modes[pin] = mode;
  }
}

void digitalWrite(int pin, int value) {
  if (ValidPin(pin)) {
    outputs[pin] = value ? HIGH : LOW;
    digital_writes.push_back({NowMs(), pin, outputs[pin]});
  }
}

int digitalRead(int pin) {
  if (!ValidPin(pin)) {
    return LOW;
  }
  if (pin_modes[pin] == OUTPUT) {
    return outputs[pin];
  }
  if (!digital_input_set[pin]) {
    return pin_modes[pin] == INPUT_PULLUP ? HIGH : LOW;
  }
  return digital_inputs[pin];
}

int analogRead(int pin) { return ValidPin(pin) ? analog_inputs[pin] : 0; }

void analogReadResolution(int bits) { analog_resolution_bits = bits; }

uint32_t millis() {
  now_us += auto_advance_us;
  return NowMs();
}

uint32_t micros() {
  now_us += auto_advance_us;
  return static_cast<uint32_t>(now_us);
}

void delay(uint32_t ms) { now_us += static_cast<uint64_t>(ms) * 1000; }

void delayMicroseconds(uint32_t us) { now_us += us; }

// CAN.

int CANSAME5x::begin(int32_t baud_rate) {
  baud_rate_ = baud_rate;
  begin_time_ms_ = NowMs();
  return fail_begin_ ? 0 : 1;
}

int CANSAME5x::filter(int id, int mask) {
  has_filter_ = true;
  filter_id_ = id;
  filter_mask_ = mask;
  return 1;
}

int CANSAME5x::beginPacket(int id, int /*dlc*/, bool rtr) {
  packet_ = Frame();
  packet_.time_ms = NowMs();
  packet_.id = id;
  packet_.rtr = rtr;
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

size_t CANSAME5x::write(const uint8_t* data, size_t length) {
  size_t written = 0;
  for (size_t i = 0; i < length; ++i) {
    written += write(data[i]);
  }
  return written;
}

int CANSAME5x::endPacket() {
  if (!packet_open_) {
    return 0;
  }
  sent_frames_.push_back(packet_);
  packet_open_ = false;
  return 1;
}

int CANSAME5x::parsePacket() {
  rx_packet_ = Frame();
  rx_read_index_ = 0;
  while (!rx_queue_.empty()) {
    const Frame frame = rx_queue_.front();
    rx_queue_.pop_front();
    if (has_filter_ &&
        (frame.id & filter_mask_) != (filter_id_ & filter_mask_)) {
      continue;
    }
    rx_packet_ = frame;
    return static_cast<int>(frame.data.size());
  }
  return 0;
}

int CANSAME5x::read() {
  if (rx_read_index_ >= rx_packet_.data.size()) {
    return -1;
  }
  return rx_packet_.data[rx_read_index_++];
}

// I2C.

void TwoWire::begin() {
  ++begin_count_;
  clock_hz_ = 100000;
}

void TwoWire::end() {}

void TwoWire::beginTransmission(int address) {
  tx_address_ = address;
  tx_buffer_.clear();
}

size_t TwoWire::write(uint8_t byte) {
  tx_buffer_.push_back(byte);
  return 1;
}

uint8_t TwoWire::endTransmission(bool /*send_stop*/) {
  constexpr uint8_t ack = 0;
  constexpr uint8_t address_nack = 2;
  const int address = tx_address_;
  tx_address_ = -1;
  if (address == i2c_mux_address) {
    if (!tx_buffer_.empty()) {
      i2c_mux_selection = tx_buffer_.back();
    }
    return ack;
  }
  fake::I2cDevice* device = ReachableDevice(address);
  if (device == nullptr) {
    return address_nack;
  }
  if (!tx_buffer_.empty()) {
    device->Write(tx_buffer_);
  }
  return ack;
}

uint8_t TwoWire::requestFrom(int address, int quantity, bool /*send_stop*/) {
  rx_buffer_.clear();
  rx_index_ = 0;
  if (address == i2c_mux_address) {
    rx_buffer_.assign(quantity, i2c_mux_selection);
  } else if (fake::I2cDevice* device = ReachableDevice(address)) {
    rx_buffer_ = device->Read(quantity);
  }
  return static_cast<uint8_t>(rx_buffer_.size());
}

int TwoWire::read() {
  if (rx_index_ >= rx_buffer_.size()) {
    return -1;
  }
  return rx_buffer_[rx_index_++];
}

// Flash.

uint32_t Adafruit_SPIFlashBase::readBuffer(uint32_t address, uint8_t* buffer,
                                           uint32_t length) {
  if (static_cast<uint64_t>(address) + length > flash.data.size()) {
    return 0;
  }
  memcpy(buffer, flash.data.data() + address, length);
  return length;
}

uint32_t Adafruit_SPIFlashBase::writeBuffer(uint32_t address,
                                            const uint8_t* buffer,
                                            uint32_t length) {
  if (static_cast<uint64_t>(address) + length > flash.data.size()) {
    return 0;
  }
  // NOR flash programming can only clear bits.
  for (uint32_t i = 0; i < length; ++i) {
    flash.data[address + i] &= buffer[i];
  }
  ++flash.write_count;
  flash.busy_polls_remaining = flash.busy_polls_per_operation;
  return length;
}

bool Adafruit_SPIFlashBase::eraseSector(uint32_t sector_number) {
  const uint64_t start =
      static_cast<uint64_t>(sector_number) * fake::FakeFlash::sector_size;
  if (start + fake::FakeFlash::sector_size > flash.data.size()) {
    return false;
  }
  memset(flash.data.data() + start, 0xFF, fake::FakeFlash::sector_size);
  ++flash.erase_count;
  flash.busy_polls_remaining = flash.busy_polls_per_operation;
  return true;
}

uint8_t Adafruit_SPIFlashBase::readStatus() {
  if (flash.busy_polls_remaining > 0) {
    --flash.busy_polls_remaining;
    return 1;
  }
  return 0;
}

// Test controls.

namespace fake {

void Reset() {
  for (int pin = 0; pin < pin_count; ++pin) {
    pin_modes[pin] = unset_mode;
    digital_inputs[pin] = LOW;
    digital_input_set[pin] = false;
    analog_inputs[pin] = 0;
    outputs[pin] = LOW;
  }
  analog_resolution_bits = 10;
  now_us = 0;
  auto_advance_us = 0;
  digital_writes.clear();
  Serial.Reset();
  Serial1.Reset();
  CAN.Reset();
  Wire.Reset();
  ResetI2c();
  ResetFlash();
}

void SetDigitalInput(int pin, int value) {
  if (ValidPin(pin)) {
    digital_inputs[pin] = value ? HIGH : LOW;
    digital_input_set[pin] = true;
  }
}

void SetAnalogInput(int pin, int value) {
  if (ValidPin(pin)) {
    analog_inputs[pin] = value;
  }
}

void AdvanceTime(uint32_t ms) { now_us += static_cast<uint64_t>(ms) * 1000; }

void SetAutoAdvanceMicros(uint32_t us) { auto_advance_us = us; }

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

void AttachI2cDevice(int address, I2cDevice* device, int mux_channel) {
  i2c_devices.insert({address, {device, mux_channel}});
}

void AttachI2cMux(int address) { i2c_mux_address = address; }

uint8_t I2cMuxSelection() { return i2c_mux_selection; }

bool I2cReachable(int address) {
  return address == i2c_mux_address || ReachableDevice(address) != nullptr;
}

void ResetI2c() {
  i2c_devices.clear();
  i2c_mux_address = -1;
  i2c_mux_selection = 0;
}

FakeFlash& Flash() { return flash; }

void ResetFlash(uint32_t size) {
  flash = FakeFlash();
  flash.data.assign(size, 0xFF);
}

}  // namespace fake
