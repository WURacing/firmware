/**
 * @file Wire.h
 * @brief Fake Arduino I2C (Wire) library for tests.
 *
 * Transactions go to fake devices the test attaches, either directly on the
 * bus or behind a TCA9548A-style I2C mux. Writing one byte to the mux selects
 * its channels (bit n = channel n); a device behind the mux only answers
 * while its channel is selected. An address with no reachable device does
 * not acknowledge, as on real hardware.
 */

#ifndef TESTS_FAKES_WIRE_H_
#define TESTS_FAKES_WIRE_H_

#include <stddef.h>
#include <stdint.h>

#include <vector>

class TwoWire {
 public:
  void begin();
  void end();
  void setClock(uint32_t hz) { clock_hz_ = hz; }
  void setTimeout(uint32_t ms) { timeout_ms_ = ms; }

  void beginTransmission(int address);
  size_t write(uint8_t byte);
  // Returns 0 when the device acknowledged, 2 (address NACK) otherwise.
  uint8_t endTransmission(bool send_stop = true);
  // Returns the number of bytes received.
  uint8_t requestFrom(int address, int quantity, bool send_stop = true);
  int available() const {
    return static_cast<int>(rx_buffer_.size() - rx_index_);
  }
  int read();

  uint32_t clock_hz() const { return clock_hz_; }
  uint32_t timeout_ms() const { return timeout_ms_; }
  int begin_count() const { return begin_count_; }
  void Reset() { *this = TwoWire(); }

 private:
  uint32_t clock_hz_ = 100000;
  uint32_t timeout_ms_ = 0;
  int begin_count_ = 0;
  int tx_address_ = -1;
  std::vector<uint8_t> tx_buffer_;
  std::vector<uint8_t> rx_buffer_;
  size_t rx_index_ = 0;
};

extern TwoWire Wire;

namespace fake {

// An I2C peripheral. Write() receives each completed write transaction's
// bytes; Read() returns the bytes for a read of `count` bytes.
class I2cDevice {
 public:
  virtual ~I2cDevice() = default;
  virtual void Write(const std::vector<uint8_t>& bytes) = 0;
  virtual std::vector<uint8_t> Read(size_t count) = 0;
};

// A device that acknowledges its address and otherwise does nothing.
class AckOnlyDevice : public I2cDevice {
 public:
  void Write(const std::vector<uint8_t>& /*bytes*/) override {}
  std::vector<uint8_t> Read(size_t count) override {
    return std::vector<uint8_t>(count, 0);
  }
};

constexpr int main_bus = -1;

// Puts `device` at `address`, on the main bus or behind I2C mux channel
// `mux_channel`. The test keeps ownership of `device`.
void AttachI2cDevice(int address, I2cDevice* device,
                     int mux_channel = main_bus);

// Puts a TCA9548A-style I2C mux at `address` (0x70-0x77).
void AttachI2cMux(int address);

// Returns the mux's channel bits (bit n = channel n selected).
uint8_t I2cMuxSelection();

// Returns true if a device at `address` would acknowledge right now.
bool I2cReachable(int address);

// Removes every device and the mux.
void ResetI2c();

}  // namespace fake

#endif  // TESTS_FAKES_WIRE_H_
