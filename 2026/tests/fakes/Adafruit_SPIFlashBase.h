/**
 * @file Adafruit_SPIFlashBase.h
 * @brief Fake Adafruit SPI flash library for tests.
 *
 * Backs every flash object with one in-memory NOR flash chip (fake::Flash()):
 * erasing a 4 KB sector sets it to 0xFF, and writes can only clear bits. A
 * test can make the chip report busy for a while after each erase or write,
 * or never become ready, to exercise a sketch's error handling.
 */

#ifndef TESTS_FAKES_ADAFRUIT_SPIFLASHBASE_H_
#define TESTS_FAKES_ADAFRUIT_SPIFLASHBASE_H_

#include <stdint.h>

#include <vector>

class Adafruit_FlashTransport {
 public:
  virtual ~Adafruit_FlashTransport() = default;
};

class Adafruit_FlashTransport_QSPI : public Adafruit_FlashTransport {};

namespace fake {

// The fake flash chip shared by every Adafruit_SPIFlashBase.
struct FakeFlash {
  static constexpr uint32_t sector_size = 4096;

  bool present = true;
  std::vector<uint8_t> data;
  // readStatus() reports busy this many times after each erase or write.
  int busy_polls_per_operation = 0;
  int busy_polls_remaining = 0;
  int erase_count = 0;
  int write_count = 0;
};

FakeFlash& Flash();

// Replaces the chip with a blank (all 0xFF) one of `size` bytes. The Feather
// M4 CAN has 2 MB.
void ResetFlash(uint32_t size = 2 * 1024 * 1024);

}  // namespace fake

class Adafruit_SPIFlashBase {
 public:
  explicit Adafruit_SPIFlashBase(Adafruit_FlashTransport* /*transport*/) {}

  bool begin() { return fake::Flash().present; }
  uint32_t size() const {
    return static_cast<uint32_t>(fake::Flash().data.size());
  }
  // Returns the bytes read: `length`, or 0 if out of range.
  uint32_t readBuffer(uint32_t address, uint8_t* buffer, uint32_t length);
  // Returns the bytes written: `length`, or 0 if out of range.
  uint32_t writeBuffer(uint32_t address, const uint8_t* buffer,
                       uint32_t length);
  bool eraseSector(uint32_t sector_number);
  // Bit 0 is set while the chip is busy.
  uint8_t readStatus();
};

#endif  // TESTS_FAKES_ADAFRUIT_SPIFLASHBASE_H_
