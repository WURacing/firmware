/**
 * @file FSB_CODE.ino
 * @brief Front Sensor Board (FSB) firmware implementation, WUFR26.
 *
 * @note The IMU on multiplexer channel 0 is disabled pending hardware
 *       debugging. RSB_CODE.ino contains the working IMU implementation.
 *
 * @see FSB_CODE.h for an overview of the board.
 */

#include "FSB_CODE.h"

#include <Adafruit_NeoPixel.h>
#include <BMX160.h>
#include <CAN.h>
#include <CANSAME5x.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>
#include <Adafruit_VL6180X.h>


// #define DEBUG
#ifdef DEBUG
#define PRINT_DEBUG(message) Serial.print(message)
#else
#define PRINT_DEBUG(message)
#endif

// Pins.
constexpr int NEOPIXEL_PIN = 8;
constexpr int LED_PIN = 13;
constexpr int MUX_EN_PIN = 9;
constexpr int MUX_A0_PIN = 10;
constexpr int MUX_A1_PIN = 11;
constexpr int MUX_A2_PIN = 12;
constexpr int MUX_A3_PIN = 13;
constexpr int MUX_OUT_PIN = A4;
constexpr int num_mux_select_pins = 4;
constexpr int mux_select_pins[num_mux_select_pins] = {MUX_A0_PIN, MUX_A1_PIN,
                                                      MUX_A2_PIN, MUX_A3_PIN};
constexpr int num_manual_analog_pins = 4;
constexpr int manual_analog_pins[num_manual_analog_pins] = {A0, A1, A2, A3};
constexpr int SCL_PIN = 21;  // Feather M4 CAN: SCL = PA13.

// Timing.
constexpr uint32_t blink_interval_ms = 1000;
constexpr uint32_t sample_interval_ms = 4;
constexpr uint32_t i2c_timeout_ms = 25;
constexpr uint32_t startup_delay_ms = 4000;

// Communication.
constexpr uint32_t serial_baud_rate = 9600;
constexpr int can_baud_rate = 1000000;
constexpr int can_base_id = 0x10;
constexpr int shorts_per_frame = 4;

// Analog conversion.
constexpr int adc_bits = 12;
constexpr int analog_resolution = 4096;
constexpr int analog_voltage_range = 5;

// Sampling.
constexpr int num_samples = 10;
constexpr int num_analog_channels = 20;
constexpr int first_manual_channel = 16;
constexpr int num_can_values = 29;  // 20 analog + 3 accel + 3 gyro + 3 magn.

constexpr uint8_t led_brightness = 20;

uint32_t data_count = 0;
uint32_t current_millis = millis();
uint32_t blink_previous_millis = 0;
bool led_state = LOW;

int16_t analogs[num_analog_channels];
double average_matrix[num_can_values];
int16_t avg_send[num_can_values];

CANSAME5x CAN;
Adafruit_NeoPixel strip(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
BMX160 bmx160;  // Unused while the IMU path is disabled.

void setup() {
  pinMode(PIN_NEOPIXEL_POWER, OUTPUT);
  for (int pin : manual_analog_pins) {
    pinMode(pin, INPUT);
  }
  pinMode(MUX_EN_PIN, OUTPUT);
  for (int pin : mux_select_pins) {
    pinMode(pin, OUTPUT);
  }
  pinMode(MUX_OUT_PIN, INPUT);
  analogReadResolution(adc_bits);

  Serial.begin(serial_baud_rate);

  strip.begin();
  strip.clear();
  strip.setBrightness(led_brightness);
  strip.show();

  delay(startup_delay_ms);

  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false);
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true);
  if (!CAN.begin(can_baud_rate)) {
    Serial.println("Starting CAN failed!");
  }

  Wire.begin();
  Wire.setTimeout(i2c_timeout_ms);
}

void loop() {
  ++data_count;
  // Wait for the next sample interval.
  while (millis() - current_millis < sample_interval_ms) {
  }
  current_millis = millis();

  // TODO(henry): Potential issue with data_count here -- may be the
  // cause of vcsel data dropping
  if (data_count >= num_samples) {
    for (int i = 0; i < num_can_values; ++i) {
      average_matrix[i] = average_matrix[i] / static_cast<float>(num_samples);
      avg_send[i] = static_cast<int16_t>(average_matrix[i]);
    }

    for (int i=0; i<32; i++) {
      avg_send[i] = 0;
    }

    datacount = 0;
  }

  for (int frame = 0; frame * shorts_per_frame < num_can_values - 1; ++frame) {
    CanShortFrame(avg_send, frame * shorts_per_frame, can_base_id + frame);
  }
  CAN.beginPacket(can_base_id + 7);
  CAN.endPacket();

  for (int i = 0; i < num_can_values; ++i) {
    avg_send[i] = 0;
    average_matrix[i] = 0;
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

void ReadAnalogsMan(int16_t* readings) {
  for (int i = 0; i < num_manual_analog_pins; ++i) {
    readings[first_manual_channel + i] =
        (analogRead(manual_analog_pins[i]) /
         static_cast<float>(analog_resolution)) *
        1000 * analog_voltage_range;
  }
  PRINT_DEBUG("Channel: ");
  PRINT_DEBUG(19);
  PRINT_DEBUG(" Data: ");
  PRINT_DEBUG(readings[19]);
  PRINT_DEBUG("\t");
}

void CanWriteShort(int16_t data) {
  CAN.write(data & 0xFF);
  CAN.write(data >> 8);
}

void CanShortFrame(const int16_t* values, int start_index, int frame_id) {
  CAN.beginPacket(frame_id);
  for (int i = start_index; i < start_index + shorts_per_frame; ++i) {
    CanWriteShort(values[i]);
    Serial.print("Wrote: ");
    Serial.println(i);
  }
  CAN.endPacket();
}

uint16_t Mux(uint8_t channel) {
  digitalWrite(MUX_EN_PIN, HIGH);
  for (int bit = 0; bit < num_mux_select_pins; ++bit) {
    digitalWrite(mux_select_pins[bit], (channel >> bit) & 1 ? HIGH : LOW);
  }
  const uint16_t reading = analogRead(MUX_OUT_PIN);
  digitalWrite(MUX_EN_PIN, LOW);
  return reading;
}

void SwapXyInPlace(float* vec) {
  const float x = vec[0];
  vec[0] = vec[1];
  vec[1] = x;
}

void RecoverI2cBus() {
  pinMode(SCL_PIN, OUTPUT);
  for (int i = 0; i < 9; ++i) {
    digitalWrite(SCL_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(SCL_PIN, LOW);
    delayMicroseconds(5);
  }
  pinMode(SCL_PIN, INPUT_PULLUP);
  Wire.end();
  Wire.begin();
}
