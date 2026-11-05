/**
 * @file control_board_test.cpp
 * @brief Host tests for the 2026 Control Board firmware.
 *
 * Runs the real ControlBoard.ino against the fake Arduino core: sets paddle
 * inputs, calls setup() and loop(), and checks which pins fire, for how long,
 * what the clutch servo is told, and what goes out on CAN.
 *
 * Tests compare against the wiring written out below, not the sketch's own
 * pin constants, so a wrong pin edit in the sketch makes a test fail.
 */

#include "ControlBoard.ino"  // NOLINT(build/include)

#include <gtest/gtest.h>

namespace {

// Match these pins with the pins seen on the Arduino schematic. The shifting
// behavior below was validated on the car (dev, November 2026).
constexpr int UP_PADDLE_PIN = 1;
constexpr int DOWN_PADDLE_PIN = 4;
constexpr int UPSHIFT_SOLENOID_PIN = 22;
constexpr int DOWNSHIFT_SOLENOID_PIN = 5;
constexpr int LEFT_CLUTCH_PADDLE_PIN = A0;
constexpr int RIGHT_CLUTCH_PADDLE_PIN = A1;
constexpr int CLUTCH_SERVO_PIN = A2;
constexpr int SHIFTCUT_PIN = A3;
constexpr int STATUS_LED_PIN = 13;

constexpr int adc_full_scale = 4096;
// Comfortably past the 0.8 "pulled" threshold.
constexpr int clutch_pulled_reading = 4000;
// Loops a shift paddle must be held before a shift fires (one per history
// bit).
constexpr int debounce_loops = 32;
// Solenoid pulse lengths: a normal shift, and the short pulse used with both
// clutch paddles pulled to find neutral.
constexpr uint32_t shift_pulse_length_ms = 65;
constexpr uint32_t neutral_pulse_length_ms = 14;
// Loops before the clutch average first updates the servo.
constexpr int clutch_average_loops = 11;

class ControlBoardTest : public ::testing::Test {
 protected:
  void SetUp() override {
    fake::Reset();
    ResetSketchGlobals();
    // Paddle inputs are active low, so released reads HIGH.
    fake::SetDigitalInput(UP_PADDLE_PIN, HIGH);
    fake::SetDigitalInput(DOWN_PADDLE_PIN, HIGH);
    SetClutchPaddles(0, 0);
    setup();
    fake::ClearDigitalWrites();
  }

  // Puts the sketch's globals back to their starting values, for when every
  // test runs in one process. Keep in sync with ControlBoard.ino.
  static void ResetSketchGlobals() {
    up_history = 0;
    down_history = 0;
    history_bit = 0;
    shifting = false;
    for (double& position : clutch_positions) {
      position = 0;
    }
    clutch_sample_count = 0;
    clutch_angle_commanded = 135;
    blink_previous_millis = 0;
    led_state = LOW;
    can_previous_millis = 0;
    clutch_servo = Servo();
  }

  // Runs loop() `count` times, 1 ms apart.
  static void RunLoops(int count) {
    for (int i = 0; i < count; ++i) {
      loop();
      fake::AdvanceTime(1);
    }
  }

  static void SetClutchPaddles(int left_reading, int right_reading) {
    fake::SetAnalogInput(LEFT_CLUTCH_PADDLE_PIN, left_reading);
    fake::SetAnalogInput(RIGHT_CLUTCH_PADDLE_PIN, right_reading);
  }

  // Holds a shift paddle for `loops` loops, then releases it and runs until
  // the debounce history has cleared.
  static void PressPaddle(int paddle_pin, int loops) {
    fake::SetDigitalInput(paddle_pin, LOW);
    RunLoops(loops);
    fake::SetDigitalInput(paddle_pin, HIGH);
    RunLoops(debounce_loops);
  }
};

// Setup.

TEST_F(ControlBoardTest, SetupConfiguresPaddleInputs) {
  EXPECT_EQ(fake::PinModeOf(UP_PADDLE_PIN), INPUT);
  EXPECT_EQ(fake::PinModeOf(DOWN_PADDLE_PIN), INPUT);
  EXPECT_EQ(fake::PinModeOf(LEFT_CLUTCH_PADDLE_PIN), INPUT);
  EXPECT_EQ(fake::PinModeOf(RIGHT_CLUTCH_PADDLE_PIN), INPUT);
  EXPECT_EQ(fake::AnalogResolutionBits(), 12);
}

TEST_F(ControlBoardTest, SetupConfiguresSolenoidOutputsOff) {
  for (int pin : {UPSHIFT_SOLENOID_PIN, DOWNSHIFT_SOLENOID_PIN, SHIFTCUT_PIN}) {
    EXPECT_EQ(fake::PinModeOf(pin), OUTPUT) << "pin " << pin;
    EXPECT_EQ(fake::OutputOf(pin), LOW) << "pin " << pin;
  }
}

TEST_F(ControlBoardTest, SetupAttachesClutchServo) {
  EXPECT_TRUE(clutch_servo.attached());
  EXPECT_EQ(clutch_servo.pin(), CLUTCH_SERVO_PIN);
}

TEST_F(ControlBoardTest, SetupStartsCanAt1MbpsWithTransceiverOn) {
  EXPECT_EQ(CAN.baud_rate(), 1000000);
  EXPECT_EQ(fake::OutputOf(PIN_CAN_STANDBY), LOW);
  EXPECT_EQ(fake::OutputOf(PIN_CAN_BOOSTEN), HIGH);
}

TEST_F(ControlBoardTest, SetupReportsCanFailureOnSerial) {
  fake::Reset();
  CAN.FailBegin();
  setup();
  EXPECT_NE(Serial.output().find("Starting CAN failed!"), std::string::npos);
}

// Shifting.

TEST_F(ControlBoardTest, NoShiftWithoutPaddlePress) {
  RunLoops(500);
  EXPECT_TRUE(fake::HighPulses(UPSHIFT_SOLENOID_PIN).empty());
  EXPECT_TRUE(fake::HighPulses(DOWNSHIFT_SOLENOID_PIN).empty());
  EXPECT_TRUE(fake::HighPulses(SHIFTCUT_PIN).empty());
}

TEST_F(ControlBoardTest, UpPaddleFiresUpshiftSolenoid) {
  PressPaddle(UP_PADDLE_PIN, debounce_loops);

  const std::vector<fake::Pulse> pulses =
      fake::HighPulses(UPSHIFT_SOLENOID_PIN);
  ASSERT_EQ(pulses.size(), 1u);
  EXPECT_EQ(pulses[0].length_ms(), shift_pulse_length_ms);
  EXPECT_TRUE(fake::HighPulses(DOWNSHIFT_SOLENOID_PIN).empty());
}

TEST_F(ControlBoardTest, DownPaddleFiresDownshiftSolenoid) {
  PressPaddle(DOWN_PADDLE_PIN, debounce_loops);

  const std::vector<fake::Pulse> pulses =
      fake::HighPulses(DOWNSHIFT_SOLENOID_PIN);
  ASSERT_EQ(pulses.size(), 1u);
  EXPECT_EQ(pulses[0].length_ms(), shift_pulse_length_ms);
  EXPECT_TRUE(fake::HighPulses(UPSHIFT_SOLENOID_PIN).empty());
}

TEST_F(ControlBoardTest, ShiftcutIsHeldForTheWholeUpshiftPulse) {
  PressPaddle(UP_PADDLE_PIN, debounce_loops);

  const std::vector<fake::Pulse> solenoid =
      fake::HighPulses(UPSHIFT_SOLENOID_PIN);
  const std::vector<fake::Pulse> shiftcut = fake::HighPulses(SHIFTCUT_PIN);
  ASSERT_EQ(solenoid.size(), 1u);
  ASSERT_EQ(shiftcut.size(), 1u);
  EXPECT_LE(shiftcut[0].start_ms, solenoid[0].start_ms);
  EXPECT_GE(shiftcut[0].end_ms, solenoid[0].end_ms);
}

TEST_F(ControlBoardTest, DownshiftDoesNotCutIgnition) {
  PressPaddle(DOWN_PADDLE_PIN, debounce_loops);
  ASSERT_EQ(fake::HighPulses(DOWNSHIFT_SOLENOID_PIN).size(), 1u);
  EXPECT_TRUE(fake::HighPulses(SHIFTCUT_PIN).empty());
}

TEST_F(ControlBoardTest, ShortPaddleBlipDoesNotShift) {
  PressPaddle(UP_PADDLE_PIN, debounce_loops - 1);
  PressPaddle(DOWN_PADDLE_PIN, debounce_loops - 1);
  EXPECT_TRUE(fake::HighPulses(UPSHIFT_SOLENOID_PIN).empty());
  EXPECT_TRUE(fake::HighPulses(DOWNSHIFT_SOLENOID_PIN).empty());
}

TEST_F(ControlBoardTest, HoldingUpPaddleUpshiftsOnlyOnce) {
  PressPaddle(UP_PADDLE_PIN, 1000);
  EXPECT_EQ(fake::HighPulses(UPSHIFT_SOLENOID_PIN).size(), 1u);
  EXPECT_EQ(fake::HighPulses(SHIFTCUT_PIN).size(), 1u);
}

TEST_F(ControlBoardTest, HoldingDownPaddleDownshiftsOnlyOnce) {
  PressPaddle(DOWN_PADDLE_PIN, 1000);
  EXPECT_EQ(fake::HighPulses(DOWNSHIFT_SOLENOID_PIN).size(), 1u);
}

TEST_F(ControlBoardTest, EachSeparatePressShiftsAgain) {
  PressPaddle(UP_PADDLE_PIN, debounce_loops);
  PressPaddle(UP_PADDLE_PIN, debounce_loops);
  PressPaddle(DOWN_PADDLE_PIN, debounce_loops);
  PressPaddle(DOWN_PADDLE_PIN, debounce_loops);
  PressPaddle(DOWN_PADDLE_PIN, debounce_loops);
  EXPECT_EQ(fake::HighPulses(UPSHIFT_SOLENOID_PIN).size(), 2u);
  EXPECT_EQ(fake::HighPulses(DOWNSHIFT_SOLENOID_PIN).size(), 3u);
}

TEST_F(ControlBoardTest, OneClutchPulledStillGivesFullPulse) {
  // Either clutch paddle alone, in either direction.
  for (const int left : {clutch_pulled_reading, 0}) {
    SetClutchPaddles(left, clutch_pulled_reading - left);
    fake::ClearDigitalWrites();
    PressPaddle(UP_PADDLE_PIN, debounce_loops);
    PressPaddle(DOWN_PADDLE_PIN, debounce_loops);

    const std::vector<fake::Pulse> up = fake::HighPulses(UPSHIFT_SOLENOID_PIN);
    const std::vector<fake::Pulse> down =
        fake::HighPulses(DOWNSHIFT_SOLENOID_PIN);
    ASSERT_EQ(up.size(), 1u) << (left ? "left" : "right") << " clutch";
    ASSERT_EQ(down.size(), 1u) << (left ? "left" : "right") << " clutch";
    EXPECT_EQ(up[0].length_ms(), shift_pulse_length_ms);
    EXPECT_EQ(down[0].length_ms(), shift_pulse_length_ms);
  }
}

TEST_F(ControlBoardTest, BothClutchesPulledUpshiftsWithNeutralPulse) {
  SetClutchPaddles(clutch_pulled_reading, clutch_pulled_reading);
  PressPaddle(UP_PADDLE_PIN, debounce_loops);

  const std::vector<fake::Pulse> up = fake::HighPulses(UPSHIFT_SOLENOID_PIN);
  ASSERT_EQ(up.size(), 1u);
  EXPECT_EQ(up[0].length_ms(), neutral_pulse_length_ms);
  EXPECT_TRUE(fake::HighPulses(DOWNSHIFT_SOLENOID_PIN).empty());
}

TEST_F(ControlBoardTest, BothClutchesPulledDownshiftsWithNeutralPulse) {
  SetClutchPaddles(clutch_pulled_reading, clutch_pulled_reading);
  PressPaddle(DOWN_PADDLE_PIN, debounce_loops);

  const std::vector<fake::Pulse> down =
      fake::HighPulses(DOWNSHIFT_SOLENOID_PIN);
  ASSERT_EQ(down.size(), 1u);
  EXPECT_EQ(down[0].length_ms(), neutral_pulse_length_ms);
  EXPECT_TRUE(fake::HighPulses(UPSHIFT_SOLENOID_PIN).empty());
}

// Clutch.

TEST_F(ControlBoardTest, ServoHoldsStartingAngleUntilFirstAverage) {
  RunLoops(1);
  EXPECT_EQ(clutch_servo.last_requested_angle(), 135);
}

TEST_F(ControlBoardTest, ReleasedClutchCommandsMinimumAngle) {
  RunLoops(clutch_average_loops);
  EXPECT_EQ(clutch_servo.last_requested_angle(), 21);
}

TEST_F(ControlBoardTest, HalfPulledClutchFollowsLinearMap) {
  SetClutchPaddles(adc_full_scale / 2, 0);
  RunLoops(clutch_average_loops);
  // LinearClutchMap(0.5) = 107.8, truncated by SetClutchPosition().
  EXPECT_EQ(clutch_servo.last_requested_angle(), 107);
}

TEST_F(ControlBoardTest, ClutchFollowsWhicheverPaddleIsPulledFurther) {
  SetClutchPaddles(0, adc_full_scale / 2);
  RunLoops(clutch_average_loops);
  const int right_only = clutch_servo.last_requested_angle();

  SetClutchPaddles(adc_full_scale / 2, adc_full_scale / 4);
  RunLoops(clutch_average_loops);
  EXPECT_EQ(clutch_servo.last_requested_angle(), right_only);
}

TEST_F(ControlBoardTest, ClutchAveragesOutASingleSpike) {
  SetClutchPaddles(adc_full_scale / 2, 0);
  RunLoops(clutch_average_loops);
  const int steady = clutch_servo.last_requested_angle();

  // One loop's reading jumps to full pull; the average should barely move.
  SetClutchPaddles(adc_full_scale - 1, 0);
  RunLoops(1);
  SetClutchPaddles(adc_full_scale / 2, 0);
  RunLoops(clutch_average_loops);
  EXPECT_NEAR(clutch_servo.last_requested_angle(), steady, 10);
}

TEST(ClutchMapTest, LinearMapHasDeadZoneBelowQuarterPull) {
  EXPECT_DOUBLE_EQ(LinearClutchMap(0.0), 21);
  EXPECT_DOUBLE_EQ(LinearClutchMap(0.24), 21);
}

TEST(ClutchMapTest, LinearMapIncreasesWithPull) {
  double previous = LinearClutchMap(0.25);
  for (double position = 0.26; position <= 1.0; position += 0.01) {
    const double angle = LinearClutchMap(position);
    EXPECT_GT(angle, previous) << "at " << position;
    previous = angle;
  }
}

TEST(ClutchMapTest, PiecewiseMapClampsBothEnds) {
  EXPECT_DOUBLE_EQ(PiecewiseClutchMap(0.0), 21);
  EXPECT_DOUBLE_EQ(PiecewiseClutchMap(0.36), 21);
  EXPECT_DOUBLE_EQ(PiecewiseClutchMap(0.88), 185);
  EXPECT_DOUBLE_EQ(PiecewiseClutchMap(1.0), 185);
}

TEST(ClutchMapTest, SCurveMapClampsBothEnds) {
  EXPECT_DOUBLE_EQ(SCurveClutchMap(0.0), 21);
  EXPECT_DOUBLE_EQ(SCurveClutchMap(0.9), 185);
}

// CAN.

TEST_F(ControlBoardTest, SendsClutchFrameOnId0x31EveryMillisecond) {
  RunLoops(10);
  const std::vector<CANSAME5x::Frame>& frames = CAN.sent_frames();
  // No frame at t = 0, then one per 1 ms loop.
  ASSERT_EQ(frames.size(), 9u);
  for (const CANSAME5x::Frame& frame : frames) {
    EXPECT_EQ(frame.id, 0x31);
    EXPECT_EQ(frame.data.size(), 8u);
  }
}

TEST_F(ControlBoardTest, ClutchFrameStartsWithCommandedAngleLittleEndian) {
  RunLoops(clutch_average_loops + 1);
  const CANSAME5x::Frame& frame = CAN.sent_frames().back();
  ASSERT_GE(frame.data.size(), 2u);
  // Released clutch: 21 degrees.
  EXPECT_EQ(frame.data[0], 21);
  EXPECT_EQ(frame.data[1], 0);
}

// Status LED.

TEST_F(ControlBoardTest, StatusLedTogglesOncePerSecond) {
  RunLoops(3500);
  const std::vector<fake::Pulse> pulses = fake::HighPulses(STATUS_LED_PIN);
  // On at 1 s, off at 2 s, on at 3 s (still on).
  ASSERT_EQ(pulses.size(), 1u);
  EXPECT_EQ(pulses[0].start_ms, 1000u);
  EXPECT_EQ(pulses[0].end_ms, 2000u);
  EXPECT_EQ(fake::OutputOf(STATUS_LED_PIN), HIGH);
}

}  // namespace
