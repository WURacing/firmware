/**
 * @file FSB_CODE.ino
 * @brief Front Sensor Board (FSB) firmware implementation, WUFR26.
 *
 * Each loop starts 4 ms after the last. It reads the IMU, then advances the
 * VCSEL state machine, which starts a ranging cycle on one loop and collects
 * the results on later ones so the loop never waits on a sensor. Every 10th
 * loop sends the averaged IMU readings and the latest VCSEL distances.
 *
 * @note The analog channels are not sampled: the calls to the analog mux and
 *       ReadAnalogsMan() were commented out before this refactor, so frames
 *       0x10-0x14 carry zeros. Mux() and ReadAnalogsMan() are kept for when
 *       they are turned back on.
 * @note The IMU mounting rotation is the identity until the calibration is
 *       redone. The old calibration code is in git history (commit d57cf13).
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
constexpr int num_mux_select_pins = 4;
constexpr int mux_select_pins[num_mux_select_pins] = {MUX_A0_PIN, MUX_A1_PIN,
                                                      MUX_A2_PIN, MUX_A3_PIN};
constexpr int num_manual_analog_pins = 4;
constexpr int manual_analog_pins[num_manual_analog_pins] = {A0, A1, A2, A3};
// I2C mux address pins. Its A0 is D4, which is also PIN_CAN_BOOSTEN and is
// held HIGH for CAN. A1 and A2 are driven LOW so the address can't float.
constexpr int I2C_MUX_ADDR_A1_PIN = 5;
constexpr int I2C_MUX_ADDR_A2_PIN = 6;
constexpr int SCL_PIN = 21;  // Feather M4 CAN: SCL = PA13.

// Timing.
constexpr uint32_t blink_interval_ms = 1000;
constexpr uint32_t sample_interval_ms = 4;
constexpr uint32_t startup_delay_ms = 4000;
constexpr uint32_t i2c_mux_settle_ms = 5;
// Abandon a VCSEL ranging cycle after this long.
constexpr uint32_t vcsel_timeout_ms = 100;
// Send "no reading" for a VCSEL whose last valid reading is older than this.
constexpr uint32_t vcsel_stale_ms = 200;

// Communication.
constexpr uint32_t serial_baud_rate = 9600;
constexpr int can_baud_rate = 1000000;
constexpr int can_base_id = 0x10;
constexpr int num_can_frames = 8;
constexpr int shorts_per_frame = 4;
// bmx160.begin() and vl.begin() restart Wire, which resets it to 100 kHz.
constexpr uint32_t i2c_clock_hz = 400000;

// I2C devices.
constexpr uint8_t i2c_mux_first_address = 0x70;
constexpr uint8_t i2c_mux_last_address = 0x77;
constexpr uint8_t imu_mux_channel = 0;  // IMU (SEN0373) is on SCL0/SDA0.
constexpr int num_vcsels = 2;
// Mux channels of the front-right and front-left VCSELs.
constexpr uint8_t vcsel_mux_channels[num_vcsels] = {1, 2};
constexpr uint8_t vl6180x_address = 0x29;
constexpr uint8_t vl6180x_model_id = 0xB4;
constexpr uint8_t vcsel_max_convergence_ms = 60;  // Tested on the protoboard.
constexpr uint8_t vl6180x_clear_all_interrupts = 0x07;

// VL6180X registers.
constexpr uint16_t reg_model_id = 0x0000;
constexpr uint16_t reg_sysrange_start = 0x0018;
constexpr uint16_t reg_max_convergence = 0x001C;
constexpr uint16_t reg_int_clear = 0x0015;
constexpr uint16_t reg_range_status = 0x004D;
constexpr uint16_t reg_int_status = 0x004F;
constexpr uint16_t reg_range_val = 0x0062;

// Analog conversion.
constexpr int adc_bits = 12;
constexpr int analog_resolution = 4096;
constexpr int analog_voltage_range = 5;

// Sampling and CAN layout. avg_send holds 20 analog channels, then accel,
// gyro, and magn (x, y, z each), then the VCSELs and one unused slot.
constexpr int num_samples = 10;
constexpr int first_manual_channel = 16;
constexpr int dimensions = 3;
constexpr int accel_index = 20;
constexpr int gyro_index = 23;
constexpr int magn_index = 26;
constexpr int num_averaged_values = 29;
constexpr int vcsel_index = 29;
constexpr int num_send_values = 32;
constexpr uint32_t vcsel_no_reading = 0xFFFF;

// IMU scaling to match the DBC's 0.01 scale. The 0.98 on acceleration undoes
// the library's g = 10 at the 4 g range.
constexpr float accel_scale = 98.0f;  // m/s^2 x100.
constexpr float gyro_scale = 100.0f;  // deg/s x100.
constexpr float magn_scale = 100.0f;  // uT x100.

constexpr uint8_t led_brightness = 20;

uint32_t data_count = 0;

float accel[dimensions];
float gyro[dimensions];
float magn[dimensions];
float accel_out[dimensions];
float gyro_out[dimensions];
float magn_out[dimensions];
double average_matrix[num_averaged_values];
int16_t avg_send[num_send_values];
// IMU mounting rotation, row-major. Identity until recalibrated.
float rotation[9] = {
    1, 0, 0,  //
    0, 1, 0,  //
    0, 0, 1,
};

Adafruit_VL6180X vl;
uint8_t i2c_mux_address = 0;  // 0 = mux not found.
bool vcsel_ok[num_vcsels] = {};
bool vcsel_pending[num_vcsels] = {};
uint32_t vcsel_last[num_vcsels] = {vcsel_no_reading, vcsel_no_reading};
uint32_t vcsel_last_ms[num_vcsels] = {};
bool vcsel_cycle_active = false;
uint32_t vcsel_cycle_start = 0;

uint32_t blink_previous_millis = 0;
bool led_state = LOW;
uint32_t current_millis = millis();

Adafruit_NeoPixel strip(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

BMX160 bmx160;
bool imu_ok = false;  // Set by InitImu(). IMU data is zero when false.

void setup() {
  pinMode(PIN_NEOPIXEL_POWER, OUTPUT);
  for (int pin : manual_analog_pins) {
    pinMode(pin, INPUT);
  }
  pinMode(MUX_OUT_PIN, INPUT);
  pinMode(MUX_EN_PIN, OUTPUT);
  for (int pin : mux_select_pins) {
    pinMode(pin, OUTPUT);
  }
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

  pinMode(I2C_MUX_ADDR_A1_PIN, OUTPUT);
  digitalWrite(I2C_MUX_ADDR_A1_PIN, LOW);
  pinMode(I2C_MUX_ADDR_A2_PIN, OUTPUT);
  digitalWrite(I2C_MUX_ADDR_A2_PIN, LOW);
  delay(i2c_mux_settle_ms);

  Wire.begin();
  FindI2cMux();
  InitImu();
  InitVcsels();
  Wire.setClock(i2c_clock_hz);
}

void loop() {
  ++data_count;
  // Wait for the next sample interval.
  while (millis() - current_millis < sample_interval_ms) {
  }
  current_millis = millis();

  // Must run before RunVcsels(), which switches mux channels.
  ReadImu();
  RunVcsels();

  if (data_count >= num_samples) {
    SendAveragedFrames();
    data_count = 0;
  }
}

void ReadImu() {
  sBmx160SensorData_t magn_data = {};
  sBmx160SensorData_t gyro_data = {};
  sBmx160SensorData_t accel_data = {};
  if (imu_ok && SelectI2cMuxChannel(imu_mux_channel)) {
    bmx160.getAllData(&magn_data, &gyro_data, &accel_data);
    DeselectI2cMux();
  }

  ImuUpdate(accel_data, accel_scale, accel);
  ImuUpdate(gyro_data, gyro_scale, gyro);
  ImuUpdate(magn_data, magn_scale, magn);

  Transform(accel, rotation, 1.0, accel_out);
  Transform(gyro, rotation, 1.0, gyro_out);
  Transform(magn, rotation, 1.0, magn_out);

  for (int i = 0; i < dimensions; ++i) {
    average_matrix[accel_index + i] += static_cast<double>(accel_out[i]);
    average_matrix[gyro_index + i] += static_cast<double>(gyro_out[i]);
    average_matrix[magn_index + i] += static_cast<double>(magn_out[i]);
  }
}

void SendAveragedFrames() {
  for (int i = 0; i < num_averaged_values; ++i) {
    average_matrix[i] = average_matrix[i] / static_cast<float>(num_samples);
    avg_send[i] = static_cast<int16_t>(average_matrix[i]);
  }

  // Each VCSEL sends its latest valid reading, or 0xFFFF if it's stale.
  for (uint8_t i = 0; i < num_vcsels; ++i) {
    const bool fresh = (millis() - vcsel_last_ms[i]) < vcsel_stale_ms;
    avg_send[vcsel_index + i] = fresh ? static_cast<int16_t>(vcsel_last[i])
                                      : static_cast<int16_t>(vcsel_no_reading);
    Serial.print("VCSEL: ");
    Serial.print(vcsel_last[i]);
    Serial.print(" VCSEL: ");
    Serial.println(i);
  }
  avg_send[num_send_values - 1] = 0;

  for (int frame = 0; frame < num_can_frames; ++frame) {
    CanShortFrame(avg_send, frame * shorts_per_frame, can_base_id + frame);
  }

  for (double& value : average_matrix) {
    value = 0;
  }
  for (int16_t& value : avg_send) {
    value = 0;
  }
}

void ImuUpdate(const sBmx160SensorData_t& data, float scale, float* out) {
  out[0] = data.x * scale;
  out[1] = data.y * scale;
  out[2] = data.z * scale;
}

void Transform(const float* input, const float* matrix, float g_scale,
               float* out) {
  out[0] = matrix[0] * input[0] + matrix[1] * input[1] + matrix[2] * input[2];
  out[1] = matrix[3] * input[0] + matrix[4] * input[1] + matrix[5] * input[2];
  out[2] = matrix[6] * input[0] + matrix[7] * input[1] + matrix[8] * input[2];
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

bool FindI2cMux() {
  for (uint8_t address = i2c_mux_first_address; address <= i2c_mux_last_address;
       ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      i2c_mux_address = address;
      Serial.print("Mux found at 0x");
      Serial.println(address, HEX);
      return true;
    }
  }
  i2c_mux_address = 0;
  Serial.println(
      "FAIL: mux not found at 0x70-0x77 (check SDA/SCL pullups, power, "
      "RESET)");
  return false;
}

bool SelectI2cMuxChannel(uint8_t channel) {
  if (i2c_mux_address == 0 || channel > 7) {
    return false;
  }
  Wire.beginTransmission(i2c_mux_address);
  Wire.write(static_cast<uint8_t>(1 << channel));
  return Wire.endTransmission() == 0;
}

void DeselectI2cMux() {
  if (i2c_mux_address == 0) {
    return;
  }
  Wire.beginTransmission(i2c_mux_address);
  Wire.write(static_cast<uint8_t>(0));
  Wire.endTransmission();
}

bool ReadVl6180xRegister(uint16_t reg, uint8_t& value) {
  Wire.beginTransmission(vl6180x_address);
  Wire.write(static_cast<uint8_t>(reg >> 8));
  Wire.write(static_cast<uint8_t>(reg));
  if (Wire.endTransmission(false)) {
    return false;
  }
  if (Wire.requestFrom(vl6180x_address, static_cast<uint8_t>(1)) != 1) {
    return false;
  }
  value = Wire.read();
  return true;
}

bool WriteVl6180xRegister(uint16_t reg, uint8_t value) {
  Wire.beginTransmission(vl6180x_address);
  Wire.write(static_cast<uint8_t>(reg >> 8));
  Wire.write(static_cast<uint8_t>(reg));
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

void InitImu() {
  // Lists every address that answers with the current mux selection.
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("CH0 device at 0x");
      Serial.println(address, HEX);
    }
  }

  if (!SelectI2cMuxChannel(imu_mux_channel)) {
    Serial.println("IMU: mux select failed");
    return;
  }
  if (!bmx160.begin()) {
    Serial.println("IMU: no ACK at 0x68 on mux CH0");
    DeselectI2cMux();
    return;
  }
  // The chip resets to +/-2000 dps, but the library scales for 250 unless
  // told. The default +/-2 g clips under braking and bumps.
  bmx160.setGyroRange(eGyroRange_250DPS);
  bmx160.setAccelRange(eAccelRange_4G);
  imu_ok = true;
  Serial.println("IMU: OK");
  DeselectI2cMux();
}

void InitVcsels() {
  if (!FindI2cMux()) {
    Serial.println("VCSEL: mux not found");
    return;
  }
  Serial.print("VCSEL: mux at 0x");
  Serial.println(i2c_mux_address, HEX);
  for (uint8_t i = 0; i < num_vcsels; ++i) {
    uint8_t id = 0;
    // vl.begin() only runs after a good ID probe.
    vcsel_ok[i] =
        SelectI2cMuxChannel(vcsel_mux_channels[i]) &&
        ReadVl6180xRegister(reg_model_id, id) && id == vl6180x_model_id &&
        vl.begin(&Wire) &&
        WriteVl6180xRegister(reg_max_convergence, vcsel_max_convergence_ms) &&
        WriteVl6180xRegister(reg_int_clear, vl6180x_clear_all_interrupts);
    Serial.print("VCSEL CH");
    Serial.print(vcsel_mux_channels[i]);
    Serial.println(vcsel_ok[i] ? " OK" : " FAIL");
  }

  // Prints what answers at the VCSEL address on every mux channel, for
  // checking the wiring.
  for (uint8_t channel = 0; channel < 8; ++channel) {
    Serial.print("scan CH");
    Serial.print(channel);
    Serial.print(": ");
    if (!SelectI2cMuxChannel(channel)) {
      Serial.println("mux select failed");
      continue;
    }
    Wire.beginTransmission(vl6180x_address);
    if (Wire.endTransmission() != 0) {
      Serial.println("nothing at 0x29");
      continue;
    }
    uint8_t id = 0;
    if (!ReadVl6180xRegister(reg_model_id, id)) {
      Serial.println("ACKs, but register read failed");
      continue;
    }
    Serial.print("model ID 0x");
    Serial.println(id, HEX);  // B4 is a VL6180X.
  }
  DeselectI2cMux();
}

void RunVcsels() {
  if (i2c_mux_address == 0) {
    return;
  }
  const uint32_t now = millis();

  if (!vcsel_cycle_active) {
    for (uint8_t i = 0; i < num_vcsels; ++i) {
      uint8_t status = 0;
      // Start a measurement on each ready sensor, clearing any stale result.
      vcsel_pending[i] =
          vcsel_ok[i] && SelectI2cMuxChannel(vcsel_mux_channels[i]) &&
          ReadVl6180xRegister(reg_range_status, status) && (status & 0x01) &&
          WriteVl6180xRegister(reg_int_clear, vl6180x_clear_all_interrupts) &&
          WriteVl6180xRegister(reg_sysrange_start, 0x01);
    }
    vcsel_cycle_active = true;
    vcsel_cycle_start = now;
    DeselectI2cMux();
    return;
  }

  bool all_done = true;
  for (uint8_t i = 0; i < num_vcsels; ++i) {
    if (!vcsel_pending[i]) {
      continue;
    }
    uint8_t int_status = 0;
    if (!SelectI2cMuxChannel(vcsel_mux_channels[i]) ||
        !ReadVl6180xRegister(reg_int_status, int_status)) {
      vcsel_pending[i] = false;  // I2C hiccup: drop this sample.
      continue;
    }
    if (!(int_status & 0x04)) {  // Still ranging.
      all_done = false;
      continue;
    }

    uint8_t range_mm = 0;
    uint8_t status = 0;
    const bool read_ok = ReadVl6180xRegister(reg_range_val, range_mm) &&
                         ReadVl6180xRegister(reg_range_status, status);
    WriteVl6180xRegister(reg_int_clear, vl6180x_clear_all_interrupts);
    if (read_ok && (status >> 4) == 0) {  // Error code 0 = valid range.
      vcsel_last[i] = range_mm;
      vcsel_last_ms[i] = now;
    }
    vcsel_pending[i] = false;
  }
  DeselectI2cMux();

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
  Wire.begin();  // Re-init SERCOM.
}
