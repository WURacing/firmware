/**
 * @file ControlBoard.ino
 * @brief Control Board firmware implementation, WUFR26.
 *
 * Controls clutch and shifting for the car. Contains multiple clutch functions.
 * © WashU Racing FSAE
 * 
 * @see ControlBoard.h for an overview of the board.
 */

#include "ControlBoard.h"

#include <CAN.h>
#include <SPI.h>
#include <Servo.h>

// Pins.
constexpr int LED_PIN = 13;
constexpr int UP_IN_PIN = 1;
constexpr int DOWN_IN_PIN = 4;
constexpr int UP_OUT_PIN = 5;
constexpr int DOWN_OUT_PIN = 22;
constexpr int CLUTCH_IN1_PIN = A0;  // Left paddle.
constexpr int CLUTCH_IN2_PIN = A1;  // Right paddle.
constexpr int CLUTCH_OUT_PIN = A2;
// Held high for the duration of each upshift pulse.
constexpr int SMEET_PIN = A3; // Shiftcut signal

// Timing.
constexpr uint32_t blink_interval_ms = 1000;
constexpr uint32_t can_interval_ms = 1;
constexpr int shift_pulse_ms = 115;
// Short pulse used with the clutch pulled, to find neutral. Upshift() and
// Downshift() take whole milliseconds, so the original 12.5 ran as 12.
constexpr int neutral_shift_pulse_ms = 12;

// Communication.
constexpr uint32_t serial_baud_rate = 9600;
constexpr int can_baud_rate = 1000000;
constexpr int can_id = 0x31;

// Analog conversion.
constexpr int adc_bits = 12;
constexpr int analog_resolution = 4096;

// Paddle position (0-1) above which a clutch paddle counts as pulled.
constexpr double clutch_pulled = 0.8;
constexpr int clutch_samples = 10;
constexpr int history_bits = 32;
constexpr uint32_t paddle_held = UINT32_MAX;
constexpr uint32_t paddle_released = 0;

uint32_t up_history = 0;
uint32_t down_history = 0;
uint16_t history_bit = 0;
bool shifting = false;

double clutch_positions[clutch_samples];
uint32_t clutch_sample_count = 0;
double clutch_angle_commanded = 135;

uint32_t blink_previous_millis = 0;
int led_state = LOW;
uint32_t can_previous_millis = 0;

Servo clutch_servo;

void setup() {
  pinMode(UP_IN_PIN, INPUT);
  pinMode(DOWN_IN_PIN, INPUT);
  pinMode(CLUTCH_IN1_PIN, INPUT);
  pinMode(CLUTCH_IN2_PIN, INPUT);

  pinMode(UP_OUT_PIN, OUTPUT);
  pinMode(DOWN_OUT_PIN, OUTPUT);
  pinMode(SMEET_PIN, OUTPUT);
  digitalWrite(UP_OUT_PIN, LOW);
  digitalWrite(DOWN_OUT_PIN, LOW);
  digitalWrite(SMEET_PIN, LOW);
  analogReadResolution(adc_bits);

  clutch_servo.attach(CLUTCH_OUT_PIN);

  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false);
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true);

  Serial.begin(serial_baud_rate);
  if (!CAN.begin(can_baud_rate)) {
    Serial.println("Starting CAN failed!");
  }
}

void loop() {
  Blink();

  const ClutchPaddles paddles = GetClutchPaddlePositions();
  CheckShiftPaddles(history_bit, up_history, down_history);
  UpdateShifting(paddles);
  UpdateClutch();
  SendClutchState(paddles);

  ++history_bit;
  if (history_bit == history_bits) {
    history_bit = 0;
  }
}

// Fires at most one shift per paddle press. With both clutch paddles pulled,
// shifts use the short neutral pulse.
void UpdateShifting(const ClutchPaddles& paddles) {
  const bool both_clutches_pulled =
      paddles.left >= clutch_pulled && paddles.right >= clutch_pulled;

  if (up_history == paddle_held && !shifting && !both_clutches_pulled) {
    shifting = true;
    Upshift(shift_pulse_ms);
  }
  if (up_history == paddle_released && down_history == paddle_released) {
    shifting = false;
  }

  if (down_history == paddle_held && !shifting && !both_clutches_pulled) {
    shifting = true;
    Downshift(shift_pulse_ms);
  }
  if (down_history == paddle_released && up_history == paddle_released) {
    shifting = false;
  }

  if (both_clutches_pulled && down_history == paddle_held && !shifting) {
    shifting = true;
    Downshift(neutral_shift_pulse_ms);
  }
  if (both_clutches_pulled && up_history == paddle_held && !shifting) {
    shifting = true;
    Upshift(neutral_shift_pulse_ms);
  }
  if (down_history == paddle_released && up_history == paddle_released) {
    shifting = false;
  }
}

// Averages the clutch paddle over clutch_samples loops and moves the servo.
void UpdateClutch() {
  clutch_positions[clutch_sample_count % clutch_samples] =
      GetClutchPaddlePosition();
  if (clutch_sample_count < clutch_samples) {
    ++clutch_sample_count;
  } else {
    clutch_sample_count = 0;
    double sum = 0;
    for (double position : clutch_positions) {
      sum += position;
    }
    const double averaged_position = sum / static_cast<double>(clutch_samples);
    clutch_angle_commanded = LinearClutchMap(averaged_position);
  }
  SetClutchPosition(clutch_angle_commanded);
}

// Broadcasts the commanded servo angle and paddle positions every
// can_interval_ms.
void SendClutchState(const ClutchPaddles& paddles) {
  const uint32_t now = millis();
  if (now - can_previous_millis >= can_interval_ms) {
    // TODO(henry): The paddle positions are 0-1, so the casts below send only
    // 0 or 1. CanShortFrame() also sends a 4th value, read from past the
    // end of this 3-value array.
    const int16_t send_data[3] = {
        static_cast<int16_t>(clutch_angle_commanded),
        static_cast<int16_t>(paddles.left),
        static_cast<int16_t>(paddles.right),
    };
    CanShortFrame(send_data, 0, can_id);
    can_previous_millis = now;
  }
}

void Blink() {
  const uint32_t now = millis();
  if (now - blink_previous_millis >= blink_interval_ms) {
    blink_previous_millis = now;
    led_state = (led_state == LOW) ? HIGH : LOW;
    digitalWrite(LED_PIN, led_state);
  }
}

void CheckShiftPaddles(uint16_t bit, uint32_t& up_history,
                       uint32_t& down_history) {
  // The paddle inputs are active low.
  const bool up_pressed = !digitalRead(UP_IN_PIN);
  const bool down_pressed = !digitalRead(DOWN_IN_PIN);
  ModifyBit(bit, up_pressed, up_history);
  ModifyBit(bit, down_pressed, down_history);
}

void Upshift(int pulse_ms) {
  digitalWrite(SMEET_PIN, HIGH);
  digitalWrite(UP_OUT_PIN, HIGH);
  delay(pulse_ms);
  digitalWrite(UP_OUT_PIN, LOW);
  digitalWrite(SMEET_PIN, LOW);
  delay(pulse_ms);
  Serial.println("Upshift pressed");
}

void Downshift(int pulse_ms) {
  digitalWrite(DOWN_OUT_PIN, HIGH);
  delay(pulse_ms);
  digitalWrite(DOWN_OUT_PIN, LOW);
  delay(pulse_ms);
  Serial.println("Downshift pressed");
}

double GetClutchPaddlePosition() {
  const int left = analogRead(CLUTCH_IN1_PIN);
  const int right = analogRead(CLUTCH_IN2_PIN);
  return static_cast<double>(max(left, right)) / analog_resolution;
}

ClutchPaddles GetClutchPaddlePositions() {
  ClutchPaddles paddles;
  paddles.left =
      static_cast<double>(analogRead(CLUTCH_IN1_PIN)) / analog_resolution;
  paddles.right =
      static_cast<double>(analogRead(CLUTCH_IN2_PIN)) / analog_resolution;
  return paddles;
}

double LinearClutchMap(double paddle_position) {
  if (paddle_position < 0.37) {
    return 21;
  }
  if (paddle_position > 0.93) {
    return 165;
  }
  return 105.76923 * paddle_position + 11.63462;
}

// S-curve that tapers at both ends: (sin(pi * (x - 0.5)) + 1) / 2, scaled to
// the 21-165 degree servo range.
double SCurveClutchMap(double paddle_position) {
  if (paddle_position < 0.1) {
    return 21;
  }
  if (paddle_position > 0.9) {
    return 165;
  }
  const double normalized = (paddle_position - 0.145) / (0.735 - 0.145);
  const double s_value = (sin(PI * (normalized - 0.5)) + 1) / 2;
  return 21 + (144 * s_value);
}

void SetClutchPosition(unsigned int angle) { clutch_servo.write(angle); }

void ModifyBit(uint16_t bit, bool value, uint32_t& bits) {
  const uint32_t mask = static_cast<uint32_t>(1) << bit;
  bits = (bits & ~mask) | (static_cast<uint32_t>(value) << bit);
}

void CanWriteShort(int16_t data) {
  CAN.write(data & 0xFF);
  CAN.write(data >> 8);
}

void CanShortFrame(const int16_t* values, int start_index, int frame_id) {
  CAN.beginPacket(frame_id);
  for (int i = start_index; i < start_index + 4; ++i) {
    CanWriteShort(values[i]);
  }
  CAN.endPacket();
}
