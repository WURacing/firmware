 /**
 * @file FSB_CODE.ino
 * @brief Front Sensor Board (FSB) firmware implementation, WUFR26.
 *
 * Every sample_interval_ms, reads the IMU (behind I2C mux channel 0) and
 * steps the non-blocking VCSEL ranging cycle. Every num_samples samples, the
 * accumulated IMU values are averaged, the latest VCSEL ranges are added, and
 * all 32 values go out as eight CAN frames.
 *
 * @note The analog mux and direct analog inputs are not sampled, so CAN
 *       values 0-19 are always 0.
 *
 * @see FSB_CODE.h for an overview of the board.
 */

#include "FSB_CODE.h"

#include <Adafruit_NeoPixel.h>
#include <Adafruit_VL6180X.h>
#include <CANSAME5x.h>
#include <SPI.h>
#include <Wire.h>
#include <math.h>

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
// I2C mux address pins: A0 = D4 (also PIN_CAN_BOOSTEN, kept HIGH for CAN),
// A1 = D5, A2 = D6.
constexpr int I2C_MUX_A1_PIN = 5;
constexpr int I2C_MUX_A2_PIN = 6;
constexpr int SCL_PIN = 21;  // Feather M4 CAN: SCL = PA13.
constexpr int num_mux_select_pins = 4;
constexpr int mux_select_pins[num_mux_select_pins] = {MUX_A0_PIN, MUX_A1_PIN,
                                                      MUX_A2_PIN, MUX_A3_PIN};
constexpr int num_manual_analog_pins = 4;
constexpr int manual_analog_pins[num_manual_analog_pins] = {A0, A1, A2, A3};

// Timing.
constexpr uint32_t blink_interval_ms = 1000;
constexpr uint32_t sample_interval_ms = 4;
constexpr uint32_t startup_delay_ms = 4000;

// Communication.
constexpr uint32_t serial_baud_rate = 9600;
constexpr int can_baud_rate = 1000000;
constexpr int can_base_id = 0x10;
constexpr int num_can_frames = 8;  // IDs 0x10 through 0x17.
constexpr int shorts_per_frame = 4;
constexpr uint32_t i2c_clock_hz = 400000;

// Analog conversion.
constexpr int adc_bits = 12;
constexpr int analog_resolution = 4096;
constexpr int analog_voltage_range = 5;

// Sampling and CAN layout.
constexpr int num_samples = 10;
constexpr int num_analog_channels = 20;
constexpr int first_manual_channel = 16;
constexpr int dimensions = 3;
constexpr int accel_index = 20;
constexpr int gyro_index = 23;
constexpr int magn_index = 26;
constexpr int num_averaged_values = 29;  // 20 analog + 3 accel/gyro/magn.
constexpr int vcsel_index = 29;
constexpr int num_can_values = num_can_frames * shorts_per_frame;  // 32.

// IMU (SEN0373) is on SCL0/SDA0 = I2C mux channel 0.
constexpr uint8_t imu_mux_channel = 0;
// Scales match the DBC's 0.01 resolution. Accel uses 98 instead of 100 to
// undo the library's g = 10 at the 4 g range.
constexpr float accel_scale = 98.0f;  // m/s^2 x100.
constexpr float gyro_scale = 100.0f;  // deg/s x100.
constexpr float magn_scale = 100.0f;  // uT x100.

// VCSEL (VL6180X) settings.
constexpr uint8_t vcsel_address = 0x29;
constexpr uint8_t max_convergence_ms = 60;  // Tested on the protoboard.
constexpr uint32_t vcsel_timeout_ms = 100;  // Abandon a cycle after this.
constexpr uint32_t vcsel_stale_ms = 200;
constexpr uint8_t vcsel_count = 2;
constexpr uint8_t vcsel_channels[vcsel_count] = {1, 2};  // FR, FL mux channel.
constexpr int16_t vcsel_invalid = static_cast<int16_t>(0xFFFF);

// VL6180X registers.
constexpr uint16_t reg_model_id = 0x0000;
constexpr uint16_t reg_sysrange_start = 0x0018;
constexpr uint16_t reg_max_convergence = 0x001C;
constexpr uint16_t reg_int_clear = 0x0015;
constexpr uint16_t reg_range_status = 0x004D;
constexpr uint16_t reg_int_status = 0x004F;
constexpr uint16_t reg_range_val = 0x0062;
constexpr uint8_t vl6180x_model_id = 0xB4;

constexpr uint8_t led_brightness = 20;

uint32_t data_count = 0;
uint32_t current_millis = millis();
uint32_t blink_previous_millis = 0;
bool led_state = LOW;

int16_t analogs[num_analog_channels];
float accel[dimensions];
float gyro[dimensions];
float magn[dimensions];
float accel_out[dimensions];
float gyro_out[dimensions];
float magn_out[dimensions];
double average_matrix[num_averaged_values];
int16_t avg_send[num_can_values];
// Identity until the mounting calibration is re-done, so the IMU's own axes
// are sent.
float rotation[9] = {
    1, 0, 0,  //
    0, 1, 0,  //
    0, 0, 1,  //
};

Adafruit_NeoPixel strip(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
BMX160 bmx160;
Adafruit_VL6180X vl;

bool imu_ok = false;      // Set by InitImu(); IMU data is zero when false.
uint8_t tca_address = 0;  // 0 = I2C mux not found.
bool vcsel_ok[vcsel_count] = {};
bool vcsel_pending[vcsel_count] = {};
uint32_t vcsel_last[vcsel_count] = {0xFFFF, 0xFFFF};
uint32_t vcsel_last_ms[vcsel_count] = {};
bool vcsel_cycle_active = false;
uint32_t vcsel_cycle_start = 0;

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

  // Drive the I2C mux address pins so the address can't float mid-run.
  pinMode(I2C_MUX_A1_PIN, OUTPUT);
  digitalWrite(I2C_MUX_A1_PIN, LOW);
  pinMode(I2C_MUX_A2_PIN, OUTPUT);
  digitalWrite(I2C_MUX_A2_PIN, LOW);
  delay(5);

  Wire.begin();
  FindMux();
  InitImu();
  InitVcsels();
  // bmx160.begin() and vl.begin() re-run Wire.begin(), which resets the
  // clock to 100 kHz.
  Wire.setClock(i2c_clock_hz);
}

void loop() {
  ++data_count;
  // Wait for the next sample interval.
  while (millis() - current_millis < sample_interval_ms) {
  }
  current_millis = millis();

  // The IMU is behind the I2C mux, so select its channel first. Must run
  // before RunVcsels(), which switches mux channels.
  sBmx160SensorData_t magn_data = {};
  sBmx160SensorData_t gyro_data = {};
  sBmx160SensorData_t accel_data = {};
  if (imu_ok && SelectChecked(imu_mux_channel)) {
    bmx160.getAllData(&magn_data, &gyro_data, &accel_data);
    DeselectAll();
  }

  ImuUpdate(accel, accel_data, accel_scale);
  ImuUpdate(gyro, gyro_data, gyro_scale);
  ImuUpdate(magn, magn_data, magn_scale);

  Transform2(accel, rotation, accel_out, 1.0);
  Transform2(gyro, rotation, gyro_out, 1.0);
  Transform2(magn, rotation, magn_out, 1.0);

  for (int i = 0; i < dimensions; ++i) {
    average_matrix[accel_index + i] += static_cast<double>(accel_out[i]);
    average_matrix[gyro_index + i] += static_cast<double>(gyro_out[i]);
    average_matrix[magn_index + i] += static_cast<double>(magn_out[i]);
  }

  RunVcsels();

  if (data_count >= num_samples) {
    for (int i = 0; i < num_averaged_values; ++i) {
      average_matrix[i] = average_matrix[i] / static_cast<float>(num_samples);
      avg_send[i] = static_cast<int16_t>(average_matrix[i]);
    }

    // Sends the latest valid range, or 0xFFFF if none is recent enough.
    for (uint8_t i = 0; i < vcsel_count; ++i) {
      const bool fresh = (millis() - vcsel_last_ms[i]) < vcsel_stale_ms;
      avg_send[vcsel_index + i] =
          fresh ? static_cast<int16_t>(vcsel_last[i]) : vcsel_invalid;
      Serial.print("VCSEL: ");
      Serial.print(vcsel_last[i]);
      Serial.print(" VCSEL: ");
      Serial.println(i);
    }
    avg_send[num_can_values - 1] = 0;

    for (int frame = 0; frame < num_can_frames; ++frame) {
      CanShortFrame(avg_send, frame * shorts_per_frame, can_base_id + frame);
    }

    for (int i = 0; i < num_averaged_values; ++i) {
      average_matrix[i] = 0;
    }
    for (int i = 0; i < num_can_values; ++i) {
      avg_send[i] = 0;
    }

    data_count = 0;
  }
}

void ImuUpdate(float* out, sBmx160SensorData_t data, float scale) {
  out[0] = data.x * scale;
  out[1] = data.y * scale;
  out[2] = data.z * scale;
}

void Transform2(const float* input, const float* rotation, float* out,
                float g_scale) {
  out[0] =
      rotation[0] * input[0] + rotation[1] * input[1] + rotation[2] * input[2];
  out[1] =
      rotation[3] * input[0] + rotation[4] * input[1] + rotation[5] * input[2];
  out[2] =
      rotation[6] * input[0] + rotation[7] * input[1] + rotation[8] * input[2];

  for (int i = 0; i < dimensions; ++i) {
    out[i] = g_scale * out[i];
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

bool FindMux() {
  for (uint8_t address = 0x70; address <= 0x77; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      tca_address = address;
      Serial.print("Mux found at 0x");
      Serial.println(address, HEX);
      return true;
    }
  }
  tca_address = 0;
  Serial.println(
      "FAIL: mux not found at 0x70-0x77 (check SDA/SCL pullups, power, "
      "RESET)");
  return false;
}

bool SelectChecked(uint8_t channel) {
  if (tca_address == 0 || channel > 7) return false;
  Wire.beginTransmission(tca_address);
  Wire.write(static_cast<uint8_t>(1 << channel));
  return Wire.endTransmission() == 0;
}

void DeselectAll() {
  if (tca_address == 0) return;
  Wire.beginTransmission(tca_address);
  Wire.write(static_cast<uint8_t>(0));
  Wire.endTransmission();
}

bool ReadReg(uint16_t reg, uint8_t& value) {
  Wire.beginTransmission(vcsel_address);
  Wire.write(static_cast<uint8_t>(reg >> 8));
  Wire.write(static_cast<uint8_t>(reg));
  if (Wire.endTransmission(false)) return false;
  if (Wire.requestFrom(vcsel_address, static_cast<uint8_t>(1)) != 1) {
    return false;
  }
  value = Wire.read();
  return true;
}

bool WriteReg(uint16_t reg, uint8_t value) {
  Wire.beginTransmission(vcsel_address);
  Wire.write(static_cast<uint8_t>(reg >> 8));
  Wire.write(static_cast<uint8_t>(reg));
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

void InitImu() {
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("CH0 device at 0x");
      Serial.println(address, HEX);
    }
  }

  if (!SelectChecked(imu_mux_channel)) {
    Serial.println("IMU: mux select failed");
    return;
  }
  if (!bmx160.begin()) {
    Serial.println("IMU: no ACK at 0x68 on mux CH0");
    DeselectAll();
    return;
  }
  // The chip resets to +/-2000 dps, but the library scales for 250 unless
  // told.
  bmx160.setGyroRange(eGyroRange_250DPS);
  // The default +/-2 g clips under braking and bumps.
  bmx160.setAccelRange(eAccelRange_4G);
  imu_ok = true;
  Serial.println("IMU: OK");
  DeselectAll();
}

void InitVcsels() {
  if (!FindMux()) {
    Serial.println("VCSEL: mux not found");
    return;
  }
  Serial.print("VCSEL: mux at 0x");
  Serial.println(tca_address, HEX);
  for (uint8_t i = 0; i < vcsel_count; ++i) {
    uint8_t id = 0;
    // Only call vl.begin() after a good ID probe.
    vcsel_ok[i] = SelectChecked(vcsel_channels[i]) &&
                  ReadReg(reg_model_id, id) && id == vl6180x_model_id &&
                  vl.begin(&Wire) &&
                  WriteReg(reg_max_convergence, max_convergence_ms) &&
                  WriteReg(reg_int_clear, 0x07);
    Serial.print("VCSEL CH");
    Serial.print(vcsel_channels[i]);
    Serial.println(vcsel_ok[i] ? " OK" : " FAIL");
  }
  // TODO(henry): Remove this temporary scan of every I2C mux channel.
  for (uint8_t channel = 0; channel < 8; ++channel) {
    Serial.print("scan CH");
    Serial.print(channel);
    Serial.print(": ");
    if (!SelectChecked(channel)) {
      Serial.println("mux select failed");
      continue;
    }
    Wire.beginTransmission(vcsel_address);
    if (Wire.endTransmission() != 0) {
      Serial.println("nothing at 0x29");
      continue;
    }
    uint8_t id = 0;
    if (!ReadReg(reg_model_id, id)) {
      Serial.println("ACKs, but register read failed");
      continue;
    }
    Serial.print("model ID 0x");
    Serial.println(id, HEX);  // B4 = good.
  }
  DeselectAll();
}

void RunVcsels() {
  if (tca_address == 0) return;
  const uint32_t now = millis();

  if (!vcsel_cycle_active) {
    for (uint8_t i = 0; i < vcsel_count; ++i) {
      uint8_t status;
      // Start a measurement if the device is ready, clearing any stale
      // result first.
      vcsel_pending[i] = vcsel_ok[i] && SelectChecked(vcsel_channels[i]) &&
                         ReadReg(reg_range_status, status) && (status & 0x01) &&
                         WriteReg(reg_int_clear, 0x07) &&
                         WriteReg(reg_sysrange_start, 0x01);
    }
    vcsel_cycle_active = true;
    vcsel_cycle_start = now;
    DeselectAll();
    return;
  }

  bool all_done = true;
  for (uint8_t i = 0; i < vcsel_count; ++i) {
    if (!vcsel_pending[i]) continue;
    uint8_t int_status;
    if (!SelectChecked(vcsel_channels[i]) ||
        !ReadReg(reg_int_status, int_status)) {
      vcsel_pending[i] = false;  // I2C hiccup: drop this sample.
      continue;
    }
    if (!(int_status & 0x04)) {  // Still ranging.
      all_done = false;
      continue;
    }

    uint8_t range_mm;
    uint8_t status;
    const bool ok =
        ReadReg(reg_range_val, range_mm) && ReadReg(reg_range_status, status);
    WriteReg(reg_int_clear, 0x07);
    if (ok && (status >> 4) == 0) {  // Error code 0 = valid range.
      vcsel_last[i] = range_mm;
      vcsel_last_ms[i] = now;
    }
    vcsel_pending[i] = false;
  }
  DeselectAll();

  if (all_done || now - vcsel_cycle_start > vcsel_timeout_ms) {
    vcsel_cycle_active = false;
  }
}

void RecoverI2cBus() {
  pinMode(SCL_PIN, OUTPUT);
  for (int i = 0; i < 9; ++i) {  // Free the peripheral.
    digitalWrite(SCL_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(SCL_PIN, LOW);
    delayMicroseconds(5);
  }
  pinMode(SCL_PIN, INPUT_PULLUP);  // Release SCL.
  Wire.end();
  Wire.begin();
}
