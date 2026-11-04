/**
 * @file CAN.h
 * @brief Fake Adafruit CAN library (CANSAME5x) for tests.
 *
 * Records each frame the sketch sends instead of putting it on a bus.
 */

#ifndef TESTS_FAKES_CAN_H_
#define TESTS_FAKES_CAN_H_

#include <stddef.h>
#include <stdint.h>

#include <vector>

class CANSAME5x {
 public:
  // One frame sent with beginPacket(), write(), and endPacket().
  struct Frame {
    uint32_t time_ms;
    int id;
    std::vector<uint8_t> data;
  };

  // Returns 1 (success) unless the test called FailBegin().
  int begin(int32_t baud_rate);
  int beginPacket(int id, int dlc = -1, bool rtr = false);
  size_t write(uint8_t byte);
  int endPacket();

  int32_t baud_rate() const { return baud_rate_; }
  const std::vector<Frame>& sent_frames() const { return sent_frames_; }
  void FailBegin() { fail_begin_ = true; }
  void Reset() { *this = CANSAME5x(); }

 private:
  int32_t baud_rate_ = 0;
  bool fail_begin_ = false;
  bool packet_open_ = false;
  Frame packet_;
  std::vector<Frame> sent_frames_;
};

extern CANSAME5x CAN;

#endif  // TESTS_FAKES_CAN_H_
