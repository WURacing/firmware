/**
 * @file FSB_CODE.h
 * @brief Front Sensor Board (FSB) firmware interface, WUFR26.
 *
 * Reads the BMX160 IMU and two VL6180X VCSEL ride-height sensors through an
 * I2C mux, averages the IMU over num_samples readings, and broadcasts the
 * results on the CAN bus as frames of four int16_t values, using IDs 0x10
 * through 0x17. The 20 analog channels are reserved in the frames but are
 * not currently sampled.
 *
 * Hardware: Adafruit Feather M4 CAN.
 */

#ifndef FSB_CODE_FSB_CODE_H_
#define FSB_CODE_FSB_CODE_H_

#include <Arduino.h>
#include <BMX160.h>

// Copies one BMX160 reading into `out` {x, y, z}, multiplied by `scale`.
void ImuUpdate(float* out, sBmx160SensorData_t data, float scale);

// Rotates `input` into the car frame (`out` = `rotation` * `input`), then
// multiplies every component by `g_scale`.
void Transform2(const float* input, const float* rotation, float* out,
                float g_scale);

// Toggles the status LED once per blink interval. Call every loop.
void Blink();

// Reads the four analog inputs that bypass the mux (A0-A3) into
// `readings[16]` through `readings[19]`, in millivolts.
void ReadAnalogsMan(int16_t* readings);

// Writes `data` to the open CAN packet as two bytes, little-endian.
void CanWriteShort(int16_t data);

// Sends `values[start_index]` through `values[start_index + 3]` as a single
// 8-byte CAN frame with ID `frame_id`.
void CanShortFrame(const int16_t* values, int start_index, int frame_id);

// Selects analog mux `channel` (0-15) and returns its raw ADC reading.
uint16_t Mux(uint8_t channel);

// Swaps the X and Y components of a 3-axis vector {x, y, z} in place.
void SwapXyInPlace(float* vec);

// Scans I2C addresses 0x70-0x77 for the I2C mux and stores the address it
// finds in `tca_address`. Returns false if no mux answers.
bool FindMux();

// Enables only I2C mux `channel` (0-7). Returns false if the mux is missing
// or does not acknowledge.
bool SelectChecked(uint8_t channel);

// Disables every I2C mux channel.
void DeselectAll();

// Reads one byte from VL6180X register `reg` into `value`. Returns false on
// an I2C error.
bool ReadReg(uint16_t reg, uint8_t& value);

// Writes `value` to VL6180X register `reg`. Returns false on an I2C error.
bool WriteReg(uint16_t reg, uint8_t value);

// Starts the IMU on I2C mux channel 0 and sets `imu_ok`. On failure the board
// keeps running (CAN still sends) instead of hanging.
void InitImu();

// Probes and configures each VCSEL on its I2C mux channel and sets
// `vcsel_ok`.
void InitVcsels();

// Advances the non-blocking VCSEL ranging cycle by one step. Call every loop.
void RunVcsels();

// Clocks SCL to release a stuck I2C peripheral, then restarts Wire.
void RecoverI2cBus();

#endif  // FSB_CODE_FSB_CODE_H_
