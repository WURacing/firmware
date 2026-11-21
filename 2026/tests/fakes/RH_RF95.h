/**
 * @file RH_RF95.h
 * @brief Fake RadioHead RFM95 LoRa driver for tests.
 *
 * Records sent packets and hands the sketch packets the test queues with
 * QueueReceive(), instead of using a radio.
 */

#ifndef TESTS_FAKES_RH_RF95_H_
#define TESTS_FAKES_RH_RF95_H_

#include <stdint.h>
#include <string.h>

#include <deque>
#include <vector>

constexpr uint8_t RH_RF95_MAX_MESSAGE_LEN = 251;

class RH_RF95 {
 public:
  RH_RF95(uint8_t chip_select_pin, uint8_t interrupt_pin)
      : chip_select_pin_(chip_select_pin), interrupt_pin_(interrupt_pin) {}

  bool init() { return init_ok_; }
  bool setFrequency(float mhz) {
    frequency_mhz_ = mhz;
    return true;
  }
  void setTxPower(int8_t dbm, bool use_rfo = false) {
    tx_power_dbm_ = dbm;
    use_rfo_ = use_rfo;
  }
  bool send(const uint8_t* data, uint8_t length) {
    sent_.emplace_back(data, data + length);
    return true;
  }
  bool waitPacketSent() { return true; }
  bool available() const { return !rx_queue_.empty(); }
  // Copies the next queued packet into `buffer` (up to `*length` bytes) and
  // sets `*length` to its size.
  bool recv(uint8_t* buffer, uint8_t* length) {
    if (rx_queue_.empty()) {
      return false;
    }
    const std::vector<uint8_t> packet = rx_queue_.front();
    rx_queue_.pop_front();
    const uint8_t count =
        packet.size() < *length ? static_cast<uint8_t>(packet.size()) : *length;
    memcpy(buffer, packet.data(), count);
    *length = count;
    return true;
  }
  int16_t lastRssi() const { return last_rssi_; }

  // Test controls and records.
  void FailInit() { init_ok_ = false; }
  void QueueReceive(const std::vector<uint8_t>& packet) {
    rx_queue_.push_back(packet);
  }
  void SetLastRssi(int16_t rssi) { last_rssi_ = rssi; }
  uint8_t chip_select_pin() const { return chip_select_pin_; }
  uint8_t interrupt_pin() const { return interrupt_pin_; }
  float frequency_mhz() const { return frequency_mhz_; }
  int8_t tx_power_dbm() const { return tx_power_dbm_; }
  const std::vector<std::vector<uint8_t>>& sent() const { return sent_; }

 private:
  uint8_t chip_select_pin_;
  uint8_t interrupt_pin_;
  bool init_ok_ = true;
  float frequency_mhz_ = 0;
  int8_t tx_power_dbm_ = 0;
  bool use_rfo_ = false;
  int16_t last_rssi_ = 0;
  std::vector<std::vector<uint8_t>> sent_;
  std::deque<std::vector<uint8_t>> rx_queue_;
};

#endif  // TESTS_FAKES_RH_RF95_H_
