/**
 * @file telemetry_sender_test.cpp
 * @brief Host tests for the 2026 Telemetry Sender (fuel consumption) sketch.
 *
 * Runs the real Telemetry_Sender.ino and FuelStorage.h against the fake
 * Arduino core, CAN bus, and QSPI flash chip. Tests feed it AiM RPM and
 * injector-pulse frames and check the fuel total, the 0x33 CAN frame, the
 * 53-byte radio packet on Serial1, the flash journal, and the reset button.
 *
 * Expected IDs, layouts, and formulas are written out below, not taken from
 * the sketch, so a wrong edit in the sketch fails a test.
 */

// The Arduino builder puts this at the top of every .ino; the sketch has no
// header of its own that includes it.
#include <Arduino.h>

#include "Telemetry_Sender.ino"  // NOLINT(build/include)

#include <gtest/gtest.h>

#include <algorithm>

namespace {

// Expected wiring and IDs.
constexpr int RESET_BUTTON_PIN = 5;
constexpr int STATUS_LED_PIN = 13;
constexpr int aim_rpm_frame_id = 0x6A;    // RPM: bytes 6-7, uint16 LE.
constexpr int aim_pulse_frame_id = 0x6E;  // Pulse: bytes 4-7, int32 LE.
constexpr int front_reset_frame_id = 0x6F;
constexpr int fuel_frame_id = 0x33;
constexpr char callsign[] = "\r\nDE KQ9L\r\n";

// Fuel rate in L/h = gain * RPM * logged pulse value (0.1 ms units).
constexpr double fuel_gain = 2.5012e-5;
constexpr double test_rpm = 6000;
constexpr int32_t test_pulse = 55;  // 5.5 ms.
constexpr double test_rate_lph = fuel_gain * test_rpm * test_pulse;

// Radio packet: 55 AA, version, sequence, AiM frames 0x69-0x6D, fuel frame
// 0x33, CRC over bytes 2-51. GetDataFromCar.py reads the same layout.
constexpr size_t radio_packet_size = 53;
constexpr uint8_t radio_packet_version = 1;
constexpr size_t fuel_slot_offset = 4 + 5 * 8;

// Flash journal format (FuelStorage.h): 2048 64-byte records at the end of
// flash.
constexpr uint32_t journal_records = 2048;
constexpr uint32_t record_size = 64;

double Liters(double rate_lph, double ms) { return rate_lph * ms / 3600000.0; }

// CRC-8, polynomial 0x07, initial value 0 (what the base station expects).
uint8_t Crc8(const uint8_t* data, size_t length) {
  uint8_t crc = 0;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07)
                         : static_cast<uint8_t>(crc << 1);
    }
  }
  return crc;
}

uint32_t Le32(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) | static_cast<uint32_t>(p[1]) << 8 |
         static_cast<uint32_t>(p[2]) << 16 | static_cast<uint32_t>(p[3]) << 24;
}

// One valid record found in the flash journal.
struct JournalRecord {
  uint32_t address;
  uint32_t sequence;
  double liters;
};

// Returns every record with valid markers, oldest sequence first. Does not
// check the CRC, so a corrupted record still shows up.
std::vector<JournalRecord> ReadJournal() {
  const std::vector<uint8_t>& flash = fake::Flash().data;
  const uint32_t base =
      static_cast<uint32_t>(flash.size()) - journal_records * record_size;
  std::vector<JournalRecord> records;
  for (uint32_t slot = 0; slot < journal_records; ++slot) {
    const uint8_t* record = flash.data() + base + slot * record_size;
    if (Le32(record) != 0x4c554654 || Le32(record + 4) != 0x00400001 ||
        Le32(record + 60) != 0x51afc03d) {
      continue;
    }
    JournalRecord found = {base + slot * record_size, Le32(record + 8), 0};
    memcpy(&found.liters, record + 24, sizeof(found.liters));
    records.push_back(found);
  }
  std::sort(records.begin(), records.end(),
            [](const JournalRecord& a, const JournalRecord& b) {
              return a.sequence < b.sequence;
            });
  return records;
}

class TelemetrySenderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    fake::Reset();
    ResetSketchGlobals();
  }

  // Puts the sketch's globals back to their starting values, for when every
  // test runs in one process. CheckReset()'s function-local statics can't be
  // reset here; ctest runs each test in its own process for that reason.
  // Keep in sync with Telemetry_Sender.ino.
  static void ResetSketchGlobals() {
    front_reset = false;
    front_reset_armed = false;
    front_reset_at = 0;
    storage = FuelStorage(&flash_transport);
    memset(aim_data, 0, sizeof(aim_data));
    sequence = 0;
    rpm = NAN;
    pulse_value = NAN;
    liters = 0;
    rate = 0;
    last_saved = -1;
    rpm_at = pulse_at = last_integrate = last_radio = last_fuel_frame =
        last_save = last_callsign = 0;
    have_rpm = false;
    have_pulse = false;
    total_ready = false;
    resetting = false;
    reset_start_liters = 0;
  }

  // Simulates a power cycle: RAM is lost, flash is kept. CheckReset()'s
  // function-local statics survive this, unlike on the car, so don't use it
  // in reset-button tests.
  static void Reboot() {
    ResetSketchGlobals();
    setup();
  }

  static void SendFrame(int id, const std::vector<uint8_t>& data,
                        bool extended = false, bool rtr = false) {
    CANSAME5x::Frame frame;
    frame.id = id;
    frame.data = data;
    frame.extended = extended;
    frame.rtr = rtr;
    CAN.QueueReceive(frame);
  }

  static std::vector<uint8_t> RpmFrame(uint16_t value) {
    std::vector<uint8_t> data(8, 0);
    data[6] = static_cast<uint8_t>(value);
    data[7] = static_cast<uint8_t>(value >> 8);
    return data;
  }

  static std::vector<uint8_t> PulseFrame(int32_t value) {
    std::vector<uint8_t> data(8, 0);
    for (int i = 0; i < 4; ++i) {
      data[4 + i] =
          static_cast<uint8_t>(static_cast<uint32_t>(value) >> (8 * i));
    }
    return data;
  }

  // Runs loop() once per millisecond for `ms`. While `rpm_value` >= 0, sends
  // an RPM frame every 100 ms; while `pulse_value` >= 0, a pulse frame too,
  // like the AiM dash does.
  static void Run(uint32_t ms, int rpm_value = -1, int32_t pulse_value = -1) {
    for (uint32_t i = 0; i < ms; ++i) {
      if (millis() % 100 == 0) {
        if (rpm_value >= 0) {
          SendFrame(aim_rpm_frame_id, RpmFrame(rpm_value));
        }
        if (pulse_value >= 0) {
          SendFrame(aim_pulse_frame_id, PulseFrame(pulse_value));
        }
      }
      loop();
      fake::AdvanceTime(1);
    }
  }

  static void RunEngine(uint32_t ms) { Run(ms, test_rpm, test_pulse); }
  static void RunEngineStopped(uint32_t ms) { Run(ms, 0, 0); }

  static std::vector<CANSAME5x::Frame> FuelFrames() {
    std::vector<CANSAME5x::Frame> frames;
    for (const CANSAME5x::Frame& frame : CAN.sent_frames()) {
      if (frame.id == fuel_frame_id) {
        frames.push_back(frame);
      }
    }
    return frames;
  }

  // Serial1 write() calls that look like radio packets.
  static std::vector<std::vector<uint8_t>> RadioPackets() {
    std::vector<std::vector<uint8_t>> packets;
    for (const std::vector<uint8_t>& write : Serial1.writes()) {
      if (write.size() == radio_packet_size) {
        packets.push_back(write);
      }
    }
    return packets;
  }

  static size_t CountOf(const std::string& haystack, const std::string& text) {
    size_t count = 0;
    for (size_t at = haystack.find(text); at != std::string::npos;
         at = haystack.find(text, at + 1)) {
      ++count;
    }
    return count;
  }

  // Runs until millis() % 500 == `phase`. Starting a hold at phase 250
  // makes the reset fire (50 ms debounce + 3 s) while the LED's 250 ms
  // "holding" blink is off, so the confirmation flashes start cleanly.
  static void RunUntilPhase(uint32_t phase, int rpm_value, int32_t pulse) {
    while (millis() % 500 != phase) {
      Run(1, rpm_value, pulse);
    }
  }

  // Holds the reset button for `ms` while the engine is stopped, then lets go.
  static void HoldResetButton(uint32_t ms) {
    fake::SetDigitalInput(RESET_BUTTON_PIN, LOW);
    RunEngineStopped(ms);
    fake::SetDigitalInput(RESET_BUTTON_PIN, HIGH);
  }
};

// Setup.

TEST_F(TelemetrySenderTest, SetupStartsRadioSerialAndSendsCallsign) {
  setup();
  EXPECT_EQ(Serial1.baud_rate(), 19200u);
  EXPECT_EQ(CountOf(Serial1.output(), callsign), 1u);
}

TEST_F(TelemetrySenderTest, SetupStartsCanAt1MbpsFilteredTo0x68Through0x6F) {
  setup();
  EXPECT_EQ(CAN.baud_rate(), 1000000);
  EXPECT_EQ(fake::OutputOf(PIN_CAN_STANDBY), LOW);
  EXPECT_EQ(fake::OutputOf(PIN_CAN_BOOSTEN), HIGH);
  ASSERT_TRUE(CAN.has_filter());
  for (int id = 0x68; id <= 0x6F; ++id) {
    EXPECT_EQ(id & CAN.filter_mask(), CAN.filter_id() & CAN.filter_mask());
  }
  EXPECT_NE(0x70 & CAN.filter_mask(), CAN.filter_id() & CAN.filter_mask());
  EXPECT_NE(0x67 & CAN.filter_mask(), CAN.filter_id() & CAN.filter_mask());
}

TEST_F(TelemetrySenderTest, SetupUsesPullupOnResetButton) {
  setup();
  EXPECT_EQ(fake::PinModeOf(RESET_BUTTON_PIN), INPUT_PULLUP);
  EXPECT_EQ(fake::PinModeOf(STATUS_LED_PIN), OUTPUT);
}

// Fuel counting.

TEST_F(TelemetrySenderTest, CountsFuelFromRpmAndInjectorPulse) {
  setup();
  RunEngine(10000);
  EXPECT_NEAR(rate, test_rate_lph, 1e-9);
  EXPECT_NEAR(liters, Liters(test_rate_lph, 10000),
              Liters(test_rate_lph, 10000) * 0.02);
}

TEST_F(TelemetrySenderTest, NothingCountedWithoutAimFrames) {
  setup();
  Run(5000);
  EXPECT_EQ(liters, 0);
  EXPECT_EQ(rate, 0);
}

TEST_F(TelemetrySenderTest, CountingStopsWhenPulseFramesStop) {
  setup();
  RunEngine(2000);
  const double before = liters;
  // RPM keeps coming but the pulse channel goes quiet; it is held for its
  // 1.5 s stale time, then the rate drops to zero.
  Run(5000, test_rpm);
  EXPECT_EQ(rate, 0);
  EXPECT_NEAR(liters - before, Liters(test_rate_lph, 1500),
              Liters(test_rate_lph, 1500) * 0.1);
  const double after = liters;
  Run(2000, test_rpm);
  EXPECT_EQ(liters, after);
}

TEST_F(TelemetrySenderTest, CountingStopsWhenRpmFramesStop) {
  setup();
  RunEngine(2000);
  const double before = liters;
  Run(3000, -1, test_pulse);  // RPM is stale after 0.5 s.
  EXPECT_EQ(rate, 0);
  EXPECT_NEAR(liters - before, Liters(test_rate_lph, 500),
              Liters(test_rate_lph, 500) * 0.25);
}

TEST_F(TelemetrySenderTest, MalformedRpmFrameStopsCounting) {
  setup();
  RunEngine(1000);
  SendFrame(aim_rpm_frame_id, {0, 0, 0, 0, 0, 0, 0x70});  // 7 bytes.
  Run(1);
  EXPECT_FALSE(have_rpm);
  EXPECT_EQ(rate, 0);
}

TEST_F(TelemetrySenderTest, ExtendedAndRemoteFramesAreIgnored) {
  setup();
  SendFrame(aim_rpm_frame_id, RpmFrame(test_rpm), /*extended=*/true);
  SendFrame(aim_pulse_frame_id, PulseFrame(test_pulse), false, /*rtr=*/true);
  Run(10);
  EXPECT_FALSE(have_rpm);
  EXPECT_FALSE(have_pulse);
}

TEST_F(TelemetrySenderTest, FramesOutside0x68To0x6FNeverArrive) {
  setup();
  // 0x16A matches 0x6A in the low byte but not the filter.
  for (int i = 0; i < 20; ++i) {
    SendFrame(0x16A, RpmFrame(test_rpm));
    SendFrame(aim_pulse_frame_id, PulseFrame(test_pulse));
    Run(100);
  }
  EXPECT_FALSE(have_rpm);
  EXPECT_EQ(liters, 0);
}

TEST_F(TelemetrySenderTest, ImplausibleInjectorPulseIsNotCounted) {
  setup();
  Run(2000, test_rpm, 400);  // 40 ms pulse: over the 30 ms limit.
  EXPECT_EQ(rate, 0);
  EXPECT_EQ(liters, 0);
}

// Outputs.

TEST_F(TelemetrySenderTest, SendsFuelFrame0x33Every100Ms) {
  setup();
  RunEngine(10000);
  const std::vector<CANSAME5x::Frame> frames = FuelFrames();
  ASSERT_GE(frames.size(), 99u);
  EXPECT_LE(frames.size(), 101u);
  for (size_t i = 1; i < frames.size(); ++i) {
    EXPECT_EQ(frames[i].time_ms - frames[i - 1].time_ms, 100u);
  }

  // Bytes 0-1: rate x100 (L/h), 2-4: total x1000 (L), 5-7 zero.
  const std::vector<uint8_t>& data = frames.back().data;
  ASSERT_EQ(data.size(), 8u);
  EXPECT_EQ(data[0] | data[1] << 8, 825);  // 8.25 L/h.
  const int total_ml = data[2] | data[3] << 8 | data[4] << 16;
  EXPECT_NEAR(total_ml, liters * 1000, 1);
  EXPECT_EQ(data[5] | data[6] | data[7], 0);
}

TEST_F(TelemetrySenderTest, SendsRadioPacketEvery200Ms) {
  setup();
  RunEngine(2001);  // Packets at 200, 400, ..., 2000 ms.
  const std::vector<std::vector<uint8_t>> packets = RadioPackets();
  EXPECT_EQ(packets.size(), 10u);
  for (size_t i = 0; i < packets.size(); ++i) {
    EXPECT_EQ(packets[i][0], 0x55);
    EXPECT_EQ(packets[i][1], 0xAA);
    EXPECT_EQ(packets[i][2], radio_packet_version);
    EXPECT_EQ(packets[i][3], static_cast<uint8_t>(i));  // Sequence.
  }
}

TEST_F(TelemetrySenderTest, RadioPacketCarriesAimFramesAndFuelFrame) {
  setup();
  const std::vector<uint8_t> frame_0x69 = {1, 2, 3, 4, 5, 6, 7, 8};
  SendFrame(0x69, frame_0x69);
  RunEngine(1001);  // Ends right after the 1000 ms packet.
  const std::vector<uint8_t> packet = RadioPackets().back();

  // Bytes 4-43: AiM frames 0x69-0x6D in order.
  EXPECT_TRUE(
      std::equal(frame_0x69.begin(), frame_0x69.end(), packet.begin() + 4));
  const std::vector<uint8_t> rpm_frame = RpmFrame(test_rpm);
  EXPECT_TRUE(std::equal(rpm_frame.begin(), rpm_frame.end(),
                         packet.begin() + 4 + 8));  // 0x6A is frame 1.

  // Bytes 44-51: the same bytes as the latest 0x33 CAN frame.
  const std::vector<uint8_t> fuel = FuelFrames().back().data;
  EXPECT_TRUE(
      std::equal(fuel.begin(), fuel.end(), packet.begin() + fuel_slot_offset));
  const int total_ml = packet[fuel_slot_offset + 2] |
                       packet[fuel_slot_offset + 3] << 8 |
                       packet[fuel_slot_offset + 4] << 16;
  EXPECT_NEAR(total_ml, liters * 1000, 1);

  // Byte 52: CRC over version, sequence, and data.
  EXPECT_EQ(packet[52], Crc8(packet.data() + 2, 50));
}

TEST_F(TelemetrySenderTest, RadioPacketLeavesPulseFrame0x6EOnBoard) {
  setup();
  SendFrame(aim_pulse_frame_id, {9, 9, 9, 9, 9, 9, 9, 9});
  Run(201);
  const std::vector<uint8_t> packet = RadioPackets().back();
  ASSERT_EQ(packet.size(), radio_packet_size);
  for (size_t i = 4; i < fuel_slot_offset; ++i) {
    EXPECT_NE(packet[i], 9) << "byte " << i;
  }
}

TEST_F(TelemetrySenderTest, RadioPacketShowsZeroRateWithoutInputs) {
  setup();
  Run(1000);
  const std::vector<uint8_t> packet = RadioPackets().back();
  EXPECT_EQ(packet[fuel_slot_offset] | packet[fuel_slot_offset + 1] << 8, 0);
}

TEST_F(TelemetrySenderTest, CallsignRepeatsEveryNineMinutes) {
  setup();
  Run(9 * 60 * 1000 + 10);
  EXPECT_EQ(CountOf(Serial1.output(), callsign), 2u);
}

// Flash journal.

TEST_F(TelemetrySenderTest, SavesTotalToFlashOncePerSecond) {
  setup();
  RunEngine(3050);
  const std::vector<JournalRecord> records = ReadJournal();
  ASSERT_EQ(records.size(), 3u);
  EXPECT_LT(records[0].liters, records[1].liters);
  EXPECT_LT(records[1].liters, records[2].liters);
}

TEST_F(TelemetrySenderTest, DoesNotRewriteFlashWhileTotalIsUnchanged) {
  setup();
  RunEngine(1500);
  RunEngineStopped(1500);  // One more save for the last of the fuel.
  const int writes = fake::Flash().write_count;
  RunEngineStopped(5000);
  EXPECT_EQ(fake::Flash().write_count, writes);
}

TEST_F(TelemetrySenderTest, TotalSurvivesPowerCycle) {
  setup();
  RunEngine(5000);
  RunEngineStopped(1500);  // Let the final total be saved.
  const double saved = liters;
  ASSERT_GT(saved, 0);

  Reboot();
  EXPECT_DOUBLE_EQ(liters, saved);
  RunEngine(2000);
  EXPECT_GT(liters, saved);
}

TEST_F(TelemetrySenderTest, CorruptNewestRecordFallsBackToThePreviousOne) {
  setup();
  RunEngine(2050);
  std::vector<JournalRecord> records = ReadJournal();
  ASSERT_GE(records.size(), 2u);
  const JournalRecord previous = records[records.size() - 2];
  // Flip a bit in the newest record's value so its CRC no longer matches.
  fake::Flash().data[records.back().address + 24] ^= 0x01;

  Reboot();
  EXPECT_DOUBLE_EQ(liters, previous.liters);
}

TEST_F(TelemetrySenderTest, NeverErasesFlashThatHoldsAFilesystem) {
  fake::Flash().data[510] = 0x55;  // MBR / boot sector signature.
  fake::Flash().data[511] = 0xAA;
  setup();
  RunEngine(5000);
  EXPECT_FALSE(storage.ok);
  EXPECT_EQ(fake::Flash().erase_count, 0);
  EXPECT_EQ(fake::Flash().write_count, 0);
  // Still counts, in RAM only.
  EXPECT_GT(liters, 0);
}

TEST_F(TelemetrySenderTest, MissingFlashStillCountsInRam) {
  fake::Flash().present = false;
  setup();
  RunEngine(3000);
  EXPECT_FALSE(storage.ok);
  EXPECT_NEAR(liters, Liters(test_rate_lph, 3000),
              Liters(test_rate_lph, 3000) * 0.05);
}

TEST_F(TelemetrySenderTest, StuckFlashGivesUpAfterTwoSecondsAndKeepsCounting) {
  fake::Flash().busy_polls_per_operation = 1 << 30;  // Never ready again.
  setup();
  RunEngine(5000);
  EXPECT_FALSE(storage.ok);
  EXPECT_EQ(fake::Flash().write_count, 1);
  EXPECT_NEAR(liters, Liters(test_rate_lph, 5000),
              Liters(test_rate_lph, 5000) * 0.05);
}

// Reset button.

TEST_F(TelemetrySenderTest, HoldingResetForThreeSecondsWithEngineOffZeroes) {
  setup();
  RunEngine(5000);
  RunEngineStopped(1500);
  ASSERT_GT(liters, 0);

  RunUntilPhase(250, 0, 0);
  // The reset fires after the 50 ms debounce plus a 3 s hold.
  const uint32_t fires_at = millis() + 3050;
  HoldResetButton(3200);
  RunEngineStopped(1500);

  EXPECT_NEAR(liters, 0, 1e-12);
  EXPECT_DOUBLE_EQ(ReadJournal().back().liters, 0);
  // Three slow flashes (400 ms period) confirm the reset.
  int flashes = 0;
  for (const fake::Pulse& pulse : fake::HighPulses(STATUS_LED_PIN)) {
    if (pulse.start_ms >= fires_at) {
      ++flashes;
      EXPECT_EQ(pulse.length_ms(), 200u);
    }
  }
  EXPECT_EQ(flashes, 3);
}

TEST_F(TelemetrySenderTest, ResetIsRefusedWhileTheEngineIsRunning) {
  setup();
  RunEngine(3000);
  RunUntilPhase(250, test_rpm, test_pulse);
  const uint32_t fires_at = millis() + 3050;
  fake::SetDigitalInput(RESET_BUTTON_PIN, LOW);
  RunEngine(3200);
  fake::SetDigitalInput(RESET_BUTTON_PIN, HIGH);
  const double before = liters;
  RunEngine(1500);

  EXPECT_GT(liters, before);  // Still counting from where it was.
  // Six fast flashes (160 ms period) mean "refused".
  int flashes = 0;
  for (const fake::Pulse& pulse : fake::HighPulses(STATUS_LED_PIN)) {
    if (pulse.start_ms >= fires_at) {
      ++flashes;
      EXPECT_EQ(pulse.length_ms(), 80u);
    }
  }
  EXPECT_EQ(flashes, 6);
}

TEST_F(TelemetrySenderTest, ShortPressDoesNotReset) {
  setup();
  RunEngine(3000);
  RunEngineStopped(1500);
  const double before = liters;
  HoldResetButton(2500);
  RunEngineStopped(1500);
  EXPECT_DOUBLE_EQ(liters, before);
}

TEST_F(TelemetrySenderTest, ButtonHeldSinceBootDoesNotReset) {
  // The button must be seen released before a hold counts, so a button
  // stuck down (or held while powering on) never resets the total.
  fake::SetDigitalInput(RESET_BUTTON_PIN, LOW);
  setup();
  RunEngine(3000);
  const double before = liters;
  ASSERT_GT(before, 0);
  RunEngineStopped(4000);
  EXPECT_NEAR(liters, before, Liters(test_rate_lph, 500));
}

TEST_F(TelemetrySenderTest, FrontBoardButtonResetsOverCan) {
  setup();
  RunEngine(3000);
  RunEngineStopped(1500);
  ASSERT_GT(liters, 0);

  // Released (0), then held (1), repeated every 100 ms like the front board.
  SendFrame(front_reset_frame_id, {0});
  for (int i = 0; i < 32; ++i) {
    SendFrame(front_reset_frame_id, {1});
    RunEngineStopped(100);
  }
  SendFrame(front_reset_frame_id, {0});
  RunEngineStopped(1500);
  EXPECT_NEAR(liters, 0, 1e-12);
}

TEST_F(TelemetrySenderTest, FrontBoardHoldWithoutReleaseIsIgnored) {
  setup();
  RunEngine(3000);
  RunEngineStopped(1500);
  const double before = liters;
  for (int i = 0; i < 40; ++i) {  // Never sent 0 first.
    SendFrame(front_reset_frame_id, {1});
    RunEngineStopped(100);
  }
  RunEngineStopped(1500);
  EXPECT_DOUBLE_EQ(liters, before);
}

}  // namespace
