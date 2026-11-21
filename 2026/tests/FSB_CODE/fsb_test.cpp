/**
 * @file fsb_test.cpp
 * @brief Host tests for the 2026 Front Sensor Board firmware.
 *
 * Runs the real FSB_CODE.ino against the fake Arduino core with a fake I2C
 * mux, IMU, and two VCSELs attached, and checks what goes out on CAN.
 *
 * Tests compare against the wiring and CAN layout written out below, not
 * the sketch's own constants, so a wrong edit in the sketch fails a test.
 */

#include "FSB_CODE.ino"  // NOLINT(build/include)

#include <gtest/gtest.h>

#include <map>

namespace {

// Expected wiring. Change these only when the car's wiring changes.
// The I2C mux's A0 is tied to D4 (held HIGH for CAN); A1 and A2 are LOW.
constexpr int I2C_MUX_ADDRESS = 0x71;
constexpr int IMU_ADDRESS = 0x68;
constexpr int IMU_MUX_CHANNEL = 0;
constexpr int VCSEL_ADDRESS = 0x29;
constexpr int VCSEL_FR_MUX_CHANNEL = 1;
constexpr int VCSEL_FL_MUX_CHANNEL = 2;
constexpr int ANALOG_MUX_EN_PIN = 9;
constexpr int ANALOG_MUX_SELECT_PINS[4] = {10, 11, 12, 13};
constexpr int ANALOG_MUX_OUT_PIN = A4;
constexpr int I2C_MUX_ADDR_PINS[2] = {5, 6};
constexpr int SCL_PIN = 21;

// Expected CAN layout: frames 0x10-0x17, four little-endian int16 each.
constexpr int first_frame_id = 0x10;
constexpr int num_frames = 8;
constexpr int imu_accel_frame = 0x15;  // AccX, AccY, AccZ, GyroX.
constexpr int imu_gyro_frame = 0x16;   // GyroY, GyroZ, MagX, MagY.
constexpr int imu_magn_frame = 0x17;   // MagZ, VCSEL FR, VCSEL FL, unused.
constexpr int16_t no_reading = -1;     // 0xFFFF.

constexpr int loops_per_send = 10;
constexpr uint32_t sample_period_ms = 4;

// A VL6180X: the registers FSB_CODE reads and writes, addressed with a
// 16-bit register number.
class FakeVl6180x : public fake::I2cDevice {
 public:
  uint8_t model_id = 0xB4;
  uint8_t range_mm = 100;
  uint8_t error_code = 0;  // RANGE_STATUS bits 7:4; 0 = valid.
  bool finishes_ranging = true;

  void Write(const std::vector<uint8_t>& bytes) override {
    if (bytes.size() < 2) {
      return;
    }
    register_ = static_cast<uint16_t>(bytes[0] << 8 | bytes[1]);
    if (bytes.size() >= 3) {
      WriteRegister(register_, bytes[2]);
    }
  }
  std::vector<uint8_t> Read(size_t count) override {
    return std::vector<uint8_t>(count, ReadRegister(register_));
  }

  int ranging_starts() const { return ranging_starts_; }
  // Returns the last value written to `reg`, or -1.
  int Written(uint16_t reg) const {
    const auto it = written_.find(reg);
    return it == written_.end() ? -1 : it->second;
  }

 private:
  static constexpr uint16_t model_id_reg = 0x0000;
  static constexpr uint16_t int_clear_reg = 0x0015;
  static constexpr uint16_t sysrange_start_reg = 0x0018;
  static constexpr uint16_t range_status_reg = 0x004D;
  static constexpr uint16_t int_status_reg = 0x004F;
  static constexpr uint16_t range_val_reg = 0x0062;

  void WriteRegister(uint16_t reg, uint8_t value) {
    written_[reg] = value;
    if (reg == sysrange_start_reg && (value & 0x01)) {
      ++ranging_starts_;
      result_ready_ = finishes_ranging;
    } else if (reg == int_clear_reg) {
      result_ready_ = false;
    }
  }
  uint8_t ReadRegister(uint16_t reg) const {
    switch (reg) {
      case model_id_reg:
        return model_id;
      case range_status_reg:
        return static_cast<uint8_t>(error_code << 4 | 0x01);  // Ready.
      case int_status_reg:
        return result_ready_ ? 0x04 : 0x00;
      case range_val_reg:
        return range_mm;
      default:
        return 0;
    }
  }

  uint16_t register_ = 0;
  bool result_ready_ = false;
  int ranging_starts_ = 0;
  std::map<uint16_t, uint8_t> written_;
};

sBmx160SensorData_t Vector(float x, float y, float z) {
  sBmx160SensorData_t data = {};
  data.x = x;
  data.y = y;
  data.z = z;
  return data;
}

class FsbTest : public ::testing::Test {
 protected:
  void SetUp() override {
    fake::Reset();
    ResetSketchGlobals();
    // loop() busy-waits on millis(), so let each call move time forward.
    fake::SetAutoAdvanceMicros(1);
  }

  // Puts the sketch's globals back to their starting values, for when every
  // test runs in one process. Keep in sync with FSB_CODE.ino.
  static void ResetSketchGlobals() {
    data_count = 0;
    for (int i = 0; i < 3; ++i) {
      accel[i] = gyro[i] = magn[i] = 0;
      accel_out[i] = gyro_out[i] = magn_out[i] = 0;
    }
    for (double& value : average_matrix) {
      value = 0;
    }
    for (int16_t& value : avg_send) {
      value = 0;
    }
    for (int i = 0; i < 9; ++i) {
      rotation[i] = (i % 4 == 0) ? 1 : 0;
    }
    for (int16_t& value : analogs) {
      value = 0;
    }
    vl = Adafruit_VL6180X();
    tca_address = 0;
    for (int i = 0; i < 2; ++i) {
      vcsel_ok[i] = false;
      vcsel_pending[i] = false;
      vcsel_last[i] = 0xFFFF;
      vcsel_last_ms[i] = 0;
    }
    vcsel_cycle_active = false;
    vcsel_cycle_start = 0;
    blink_previous_millis = 0;
    led_state = LOW;
    current_millis = 0;
    strip = Adafruit_NeoPixel(1, 8, NEO_GRB + NEO_KHZ800);
    bmx160 = BMX160();
    imu_ok = false;
  }

  // Attaches the mux, IMU, and both VCSELs as wired on the car.
  void AttachCarHardware() {
    fake::AttachI2cMux(I2C_MUX_ADDRESS);
    fake::AttachI2cDevice(IMU_ADDRESS, &imu_, IMU_MUX_CHANNEL);
    fake::AttachI2cDevice(VCSEL_ADDRESS, &vcsel_fr_, VCSEL_FR_MUX_CHANNEL);
    fake::AttachI2cDevice(VCSEL_ADDRESS, &vcsel_fl_, VCSEL_FL_MUX_CHANNEL);
  }

  static void RunLoops(int count) {
    for (int i = 0; i < count; ++i) {
      loop();
    }
  }

  static std::vector<CANSAME5x::Frame> FramesWithId(int id) {
    std::vector<CANSAME5x::Frame> frames;
    for (const CANSAME5x::Frame& frame : CAN.sent_frames()) {
      if (frame.id == id) {
        frames.push_back(frame);
      }
    }
    return frames;
  }

  // Returns value `index` (0-3) of the latest frame with `id`.
  static int16_t LatestValue(int id, int index) {
    const std::vector<CANSAME5x::Frame> frames = FramesWithId(id);
    EXPECT_FALSE(frames.empty()) << "no frame 0x" << std::hex << id;
    if (frames.empty() || frames.back().data.size() < 8) {
      return 0;
    }
    const std::vector<uint8_t>& data = frames.back().data;
    return static_cast<int16_t>(data[2 * index] | data[2 * index + 1] << 8);
  }

  fake::AckOnlyDevice imu_;
  FakeVl6180x vcsel_fr_;
  FakeVl6180x vcsel_fl_;
};

// Setup.

TEST_F(FsbTest, SetupConfiguresAnalogInputsAndAnalogMuxOutputs) {
  setup();
  for (int pin : {int{A0}, int{A1}, int{A2}, int{A3}, ANALOG_MUX_OUT_PIN}) {
    EXPECT_EQ(fake::PinModeOf(pin), INPUT) << "pin " << pin;
  }
  EXPECT_EQ(fake::PinModeOf(ANALOG_MUX_EN_PIN), OUTPUT);
  for (int pin : ANALOG_MUX_SELECT_PINS) {
    EXPECT_EQ(fake::PinModeOf(pin), OUTPUT) << "pin " << pin;
  }
  EXPECT_EQ(fake::AnalogResolutionBits(), 12);
}

TEST_F(FsbTest, SetupStartsCanAt1MbpsAfterFourSecondDelay) {
  setup();
  EXPECT_EQ(CAN.baud_rate(), 1000000);
  EXPECT_GE(CAN.begin_time_ms(), 4000u);
  EXPECT_EQ(fake::OutputOf(PIN_CAN_STANDBY), LOW);
  EXPECT_EQ(fake::OutputOf(PIN_CAN_BOOSTEN), HIGH);
}

TEST_F(FsbTest, SetupDrivesI2cMuxAddressPinsLow) {
  setup();
  for (int pin : I2C_MUX_ADDR_PINS) {
    EXPECT_EQ(fake::PinModeOf(pin), OUTPUT) << "pin " << pin;
    EXPECT_EQ(fake::OutputOf(pin), LOW) << "pin " << pin;
  }
}

TEST_F(FsbTest, SetupLeavesI2cAt400kHz) {
  AttachCarHardware();
  setup();
  EXPECT_EQ(Wire.clock_hz(), 400000u);
}

TEST_F(FsbTest, SetupTurnsOnNeoPixelAtLowBrightness) {
  setup();
  EXPECT_TRUE(strip.begun());
  EXPECT_EQ(strip.pin(), 8);
  EXPECT_EQ(strip.brightness(), 20);
  EXPECT_GE(strip.show_count(), 1);
}

TEST_F(FsbTest, SetupReturnsSoLoopCanRun) {
  // An old debugging loop in setup() once kept loop() from ever running.
  AttachCarHardware();
  setup();
  RunLoops(loops_per_send);
  EXPECT_EQ(FramesWithId(first_frame_id).size(), 1u);
}

TEST_F(FsbTest, FindsI2cMuxAtAnyAddressFrom0x70To0x77) {
  fake::AttachI2cMux(0x75);
  setup();
  EXPECT_NE(Serial.output().find("Mux found at 0x75"), std::string::npos);
}

// CAN output.

TEST_F(FsbTest, SendsFramesOx10To0x17EveryTenSamples) {
  AttachCarHardware();
  setup();
  RunLoops(3 * loops_per_send);

  for (int id = first_frame_id; id < first_frame_id + num_frames; ++id) {
    const std::vector<CANSAME5x::Frame> frames = FramesWithId(id);
    ASSERT_EQ(frames.size(), 3u) << "frame 0x" << std::hex << id;
    EXPECT_EQ(frames[0].data.size(), 8u);
  }
  EXPECT_EQ(CAN.sent_frames().size(), 3u * num_frames);
}

TEST_F(FsbTest, SendsOneBatchEvery40Ms) {
  AttachCarHardware();
  setup();
  RunLoops(3 * loops_per_send);
  const std::vector<CANSAME5x::Frame> frames = FramesWithId(first_frame_id);
  ASSERT_EQ(frames.size(), 3u);
  EXPECT_NEAR(frames[1].time_ms - frames[0].time_ms,
              loops_per_send * sample_period_ms, 1);
  EXPECT_NEAR(frames[2].time_ms - frames[1].time_ms,
              loops_per_send * sample_period_ms, 1);
}

TEST_F(FsbTest, AnalogChannelFramesAreZeroWhileSamplingIsOff) {
  // Documents current behavior: analog sampling is commented out.
  AttachCarHardware();
  for (int pin : {int{A0}, int{A1}, int{A2}, int{A3}, ANALOG_MUX_OUT_PIN}) {
    fake::SetAnalogInput(pin, 2048);
  }
  setup();
  RunLoops(loops_per_send);
  for (int id = first_frame_id; id < first_frame_id + 5; ++id) {
    for (int i = 0; i < 4; ++i) {
      EXPECT_EQ(LatestValue(id, i), 0) << "frame 0x" << std::hex << id;
    }
  }
}

// IMU.

TEST_F(FsbTest, ImuReadingsAreScaledIntoFrames0x15To0x17) {
  AttachCarHardware();
  bmx160.SetData(Vector(1.0f, -2.0f, 0.5f), Vector(10.0f, 20.0f, -30.0f),
                 Vector(0.25f, -0.5f, 1.0f));
  setup();
  RunLoops(loops_per_send);

  // Acceleration x98 (x100 for the DBC, x0.98 for the library's g = 10).
  EXPECT_EQ(LatestValue(imu_accel_frame, 0), 98);
  EXPECT_EQ(LatestValue(imu_accel_frame, 1), -196);
  EXPECT_EQ(LatestValue(imu_accel_frame, 2), 49);
  // Gyro and magnetometer x100.
  EXPECT_EQ(LatestValue(imu_accel_frame, 3), 1000);
  EXPECT_EQ(LatestValue(imu_gyro_frame, 0), 2000);
  EXPECT_EQ(LatestValue(imu_gyro_frame, 1), -3000);
  EXPECT_EQ(LatestValue(imu_gyro_frame, 2), 25);
  EXPECT_EQ(LatestValue(imu_gyro_frame, 3), -50);
  EXPECT_EQ(LatestValue(imu_magn_frame, 0), 100);
}

TEST_F(FsbTest, ImuIsAveragedOverTheTenSamples) {
  AttachCarHardware();
  setup();
  bmx160.SetData(Vector(1.0f, 0, 0), Vector(0, 0, 0), Vector(0, 0, 0));
  RunLoops(loops_per_send / 2);
  bmx160.SetData(Vector(3.0f, 0, 0), Vector(0, 0, 0), Vector(0, 0, 0));
  RunLoops(loops_per_send / 2);
  EXPECT_EQ(LatestValue(imu_accel_frame, 0), 196);  // Average 2.0 x 98.
}

TEST_F(FsbTest, EachBatchStartsFromZero) {
  AttachCarHardware();
  bmx160.SetData(Vector(1.0f, 0, 0), Vector(0, 0, 0), Vector(0, 0, 0));
  setup();
  RunLoops(loops_per_send);
  bmx160.SetData(Vector(2.0f, 0, 0), Vector(0, 0, 0), Vector(0, 0, 0));
  RunLoops(loops_per_send);
  EXPECT_EQ(LatestValue(imu_accel_frame, 0), 196);
}

TEST_F(FsbTest, ImuIsReadWithItsMuxChannelSelected) {
  AttachCarHardware();
  setup();
  RunLoops(2 * loops_per_send);
  EXPECT_TRUE(imu_ok);
  EXPECT_EQ(bmx160.read_count(), 2 * loops_per_send);
  EXPECT_EQ(bmx160.unreachable_read_count(), 0);
}

TEST_F(FsbTest, ImuIsSetTo250DpsAnd4g) {
  AttachCarHardware();
  setup();
  EXPECT_EQ(bmx160.gyro_range(), eGyroRange_250DPS);
  EXPECT_EQ(bmx160.accel_range(), eAccelRange_4G);
}

TEST_F(FsbTest, ImuOnAnotherMuxChannelIsNotFound) {
  fake::AttachI2cMux(I2C_MUX_ADDRESS);
  fake::AttachI2cDevice(IMU_ADDRESS, &imu_, 3);
  setup();
  EXPECT_FALSE(imu_ok);
}

TEST_F(FsbTest, MissingImuSendsZerosAndKeepsRunning) {
  fake::AttachI2cMux(I2C_MUX_ADDRESS);
  setup();
  RunLoops(loops_per_send);
  EXPECT_FALSE(imu_ok);
  EXPECT_NE(Serial.output().find("IMU: no ACK at 0x68 on mux CH0"),
            std::string::npos);
  EXPECT_EQ(bmx160.read_count(), 0);
  EXPECT_EQ(LatestValue(imu_accel_frame, 0), 0);
}

TEST_F(FsbTest, MissingI2cMuxStillSendsCan) {
  setup();
  RunLoops(loops_per_send);
  EXPECT_NE(Serial.output().find("FAIL: mux not found"), std::string::npos);
  EXPECT_EQ(CAN.sent_frames().size(), static_cast<size_t>(num_frames));
  EXPECT_EQ(LatestValue(imu_magn_frame, 1), no_reading);
  EXPECT_EQ(LatestValue(imu_magn_frame, 2), no_reading);
}

TEST_F(FsbTest, I2cMuxIsDeselectedAfterEachLoop) {
  AttachCarHardware();
  setup();
  for (int i = 0; i < 2 * loops_per_send; ++i) {
    loop();
    EXPECT_EQ(fake::I2cMuxSelection(), 0) << "after loop " << i;
  }
}

// VCSELs.

TEST_F(FsbTest, VcselDistancesGoInFrame0x17) {
  AttachCarHardware();
  vcsel_fr_.range_mm = 45;
  vcsel_fl_.range_mm = 80;
  setup();
  RunLoops(loops_per_send);
  EXPECT_EQ(LatestValue(imu_magn_frame, 1), 45);
  EXPECT_EQ(LatestValue(imu_magn_frame, 2), 80);
  EXPECT_EQ(LatestValue(imu_magn_frame, 3), 0);
}

TEST_F(FsbTest, VcselsAreConfiguredAtStartup) {
  AttachCarHardware();
  setup();
  for (const FakeVl6180x* vcsel : {&vcsel_fr_, &vcsel_fl_}) {
    EXPECT_EQ(vcsel->Written(0x001C), 60);    // Max convergence time, ms.
    EXPECT_EQ(vcsel->Written(0x0015), 0x07);  // Clear interrupts.
  }
  EXPECT_TRUE(vcsel_ok[0]);
  EXPECT_TRUE(vcsel_ok[1]);
}

TEST_F(FsbTest, VcselsRangeEveryOtherLoop) {
  // One loop starts a measurement, the next collects it.
  AttachCarHardware();
  setup();
  RunLoops(loops_per_send);
  EXPECT_EQ(vcsel_fr_.ranging_starts(), loops_per_send / 2);
  EXPECT_EQ(vcsel_fl_.ranging_starts(), loops_per_send / 2);
}

TEST_F(FsbTest, VcselWithWrongModelIdIsSkipped) {
  AttachCarHardware();
  vcsel_fr_.model_id = 0x00;
  vcsel_fl_.range_mm = 80;
  setup();
  RunLoops(loops_per_send);
  EXPECT_NE(Serial.output().find("VCSEL CH1 FAIL"), std::string::npos);
  EXPECT_NE(Serial.output().find("VCSEL CH2 OK"), std::string::npos);
  EXPECT_EQ(vcsel_fr_.ranging_starts(), 0);
  EXPECT_EQ(vl.begin_count(), 1);  // Only after a good ID probe.
  EXPECT_EQ(LatestValue(imu_magn_frame, 1), no_reading);
  EXPECT_EQ(LatestValue(imu_magn_frame, 2), 80);
}

TEST_F(FsbTest, VcselRangeErrorIsNotSent) {
  AttachCarHardware();
  vcsel_fr_.error_code = 0x0B;  // E.g. no target in range.
  setup();
  RunLoops(loops_per_send);
  EXPECT_EQ(LatestValue(imu_magn_frame, 1), no_reading);
}

TEST_F(FsbTest, VcselThatStopsRespondingGoesStaleAfter200Ms) {
  AttachCarHardware();
  vcsel_fr_.range_mm = 45;
  vcsel_fl_.range_mm = 80;
  setup();
  RunLoops(loops_per_send);
  ASSERT_EQ(LatestValue(imu_magn_frame, 1), 45);

  vcsel_fr_.finishes_ranging = false;
  RunLoops(4 * loops_per_send);  // 160 ms: still fresh.
  EXPECT_EQ(LatestValue(imu_magn_frame, 1), 45);
  RunLoops(2 * loops_per_send);  // 240 ms: stale.
  EXPECT_EQ(LatestValue(imu_magn_frame, 1), no_reading);
  EXPECT_EQ(LatestValue(imu_magn_frame, 2), 80);
}

// Helpers kept for when analog sampling returns.

TEST_F(FsbTest, MuxSelectsChannelAndReadsMuxOutput) {
  setup();
  fake::SetAnalogInput(ANALOG_MUX_OUT_PIN, 1234);
  fake::ClearDigitalWrites();
  EXPECT_EQ(Mux(0b1010), 1234);
  EXPECT_EQ(fake::OutputOf(ANALOG_MUX_SELECT_PINS[0]), LOW);
  EXPECT_EQ(fake::OutputOf(ANALOG_MUX_SELECT_PINS[1]), HIGH);
  EXPECT_EQ(fake::OutputOf(ANALOG_MUX_SELECT_PINS[2]), LOW);
  EXPECT_EQ(fake::OutputOf(ANALOG_MUX_SELECT_PINS[3]), HIGH);
  // Enabled for the read, then disabled.
  EXPECT_EQ(fake::HighPulses(ANALOG_MUX_EN_PIN).size(), 1u);
  EXPECT_EQ(fake::OutputOf(ANALOG_MUX_EN_PIN), LOW);
}

TEST_F(FsbTest, ReadAnalogsManConvertsToMillivoltsOn5VScale) {
  fake::SetAnalogInput(A0, 0);
  fake::SetAnalogInput(A1, 2048);
  fake::SetAnalogInput(A2, 4095);
  fake::SetAnalogInput(A3, 1024);
  int16_t readings[20] = {};
  ReadAnalogsMan(readings);
  EXPECT_EQ(readings[16], 0);
  EXPECT_EQ(readings[17], 2500);
  EXPECT_EQ(readings[18], 4998);
  EXPECT_EQ(readings[19], 1250);
}

TEST_F(FsbTest, SwapXyInPlaceSwapsOnlyXAndY) {
  float vec[3] = {1, 2, 3};
  SwapXyInPlace(vec);
  EXPECT_EQ(vec[0], 2);
  EXPECT_EQ(vec[1], 1);
  EXPECT_EQ(vec[2], 3);
}

TEST_F(FsbTest, RecoverI2cBusClocksSclNineTimes) {
  const int begins = Wire.begin_count();
  RecoverI2cBus();
  EXPECT_EQ(fake::HighPulses(SCL_PIN).size(), 9u);
  EXPECT_EQ(fake::PinModeOf(SCL_PIN), INPUT_PULLUP);
  EXPECT_EQ(Wire.begin_count(), begins + 1);
}

}  // namespace
