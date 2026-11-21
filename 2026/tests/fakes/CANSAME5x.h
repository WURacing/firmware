/**
 * @file CANSAME5x.h
 * @brief Fake Adafruit CAN library (CANSAME5x) for tests.
 *
 * Records each frame the sketch sends instead of putting it on a bus, and
 * hands the sketch frames the test queues with QueueReceive(). The hardware
 * acceptance filter set with filter() is applied to received frames.
 */

// NOLINT(build/header_guard)
// No include guard, like the real library header: including it twice
// (for example <CAN.h> plus <CANSAME5x.h>) fails here as it does on the
// board.

#include <stddef.h>
#include <stdint.h>

#include <deque>
#include <vector>

class CANSAME5x {
 public:
  // One CAN frame, sent or received.
  struct Frame {
    uint32_t time_ms = 0;
    int id = 0;
    std::vector<uint8_t> data;
    bool extended = false;
    bool rtr = false;
  };

  // Returns 1 (success) unless the test called FailBegin().
  int begin(int32_t baud_rate);
  int filter(int id, int mask);

  int beginPacket(int id, int dlc = -1, bool rtr = false);
  size_t write(uint8_t byte);
  size_t write(const uint8_t* data, size_t length);
  int endPacket();

  // Moves to the next queued frame that passes the filter. Returns its data
  // length, or 0 when there are none.
  int parsePacket();
  int32_t packetId() const { return rx_packet_.id; }
  bool packetExtended() const { return rx_packet_.extended; }
  bool packetRtr() const { return rx_packet_.rtr; }
  int available() const {
    return static_cast<int>(rx_packet_.data.size() - rx_read_index_);
  }
  // Returns the next byte of the current packet, or -1.
  int read();

  // Test controls and records.
  void QueueReceive(const Frame& frame) { rx_queue_.push_back(frame); }
  int32_t baud_rate() const { return baud_rate_; }
  uint32_t begin_time_ms() const { return begin_time_ms_; }
  bool has_filter() const { return has_filter_; }
  int filter_id() const { return filter_id_; }
  int filter_mask() const { return filter_mask_; }
  const std::vector<Frame>& sent_frames() const { return sent_frames_; }
  void FailBegin() { fail_begin_ = true; }
  void Reset() { *this = CANSAME5x(); }

 private:
  int32_t baud_rate_ = 0;
  uint32_t begin_time_ms_ = 0;
  bool fail_begin_ = false;
  bool has_filter_ = false;
  int filter_id_ = 0;
  int filter_mask_ = 0;
  bool packet_open_ = false;
  Frame packet_;
  std::vector<Frame> sent_frames_;
  std::deque<Frame> rx_queue_;
  Frame rx_packet_;
  size_t rx_read_index_ = 0;
};

// The library's global CAN object. Sketches that declare their own
// `CANSAME5x CAN;` are built without the fake's definition.
extern CANSAME5x CAN;
