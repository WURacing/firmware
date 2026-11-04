/**
 * @file FSB_CODE.h
 * @brief Front Sensor Board (FSB) firmware interface, WUFR26.
 *
 * Reads the BMX160 IMU and two VL6180X VCSEL ride-height sensors through a
 * TCA9548A I2C mux, averages the IMU over 10 samples (one every 4 ms), and
 * broadcasts everything on the CAN bus as frames of four int16_t values,
 * using IDs 0x10 through 0x17. The 20 analog sensor channels (16 through an
 * analog multiplexer and 4 wired directly) have slots in those frames but are
 * not sampled right now.
 *
 * Hardware: Adafruit Feather M4 CAN.
 */

#ifndef FSB_CODE_FSB_CODE_H_
#define FSB_CODE_FSB_CODE_H_

#include <Arduino.h>
// BMX160.h has no include guard, so it is included only here.
#include <BMX160.h>

// Toggles the status LED once per blink interval.
void Blink();

// Reads the IMU through I2C mux channel 0, rotates the readings into the car
// frame, and adds them to the running sums for the next CAN send.
void ReadImu();

// Averages the accumulated samples, sends CAN frames 0x10-0x17, and clears
// the sums for the next batch.
void SendAveragedFrames();

// Writes `data` scaled by `scale` into the 3-element array `out`.
void ImuUpdate(const sBmx160SensorData_t& data, float scale, float* out);

// Sets `out` to `matrix` (3x3, row-major) times `input`, then multiplies each
// component by `g_scale`.
void Transform(const float* input, const float* matrix, float g_scale,
               float* out);

// Reads the four analog inputs that bypass the analog mux (A0-A3) into
// `readings[16]` through `readings[19]`, in millivolts.
void ReadAnalogsMan(int16_t* readings);

// Selects analog mux `channel` (0-15) and returns its raw ADC reading.
uint16_t Mux(uint8_t channel);

// Writes `data` to the open CAN packet as two bytes, little-endian.
void CanWriteShort(int16_t data);

// Sends `values[start_index]` through `values[start_index + 3]` as a single
// 8-byte CAN frame with ID `frame_id`.
void CanShortFrame(const int16_t* values, int start_index, int frame_id);

// Swaps the X and Y components of a 3-axis vector {x, y, z} in place.
void SwapXyInPlace(float* vec);

// Looks for the I2C mux at 0x70-0x77 and remembers its address. Returns true
// if it was found.
bool FindI2cMux();

// Selects I2C mux `channel` (0-7). Returns true if the mux acknowledged.
bool SelectI2cMuxChannel(uint8_t channel);

// Deselects every I2C mux channel.
void DeselectI2cMux();

// Reads the VL6180X register `reg` into `value`. Returns true on success.
bool ReadVl6180xRegister(uint16_t reg, uint8_t& value);

// Writes `value` to the VL6180X register `reg`. Returns true on success.
bool WriteVl6180xRegister(uint16_t reg, uint8_t value);

// Starts the IMU on I2C mux channel 0. On failure the IMU readings stay zero
// and the board keeps running.
void InitImu();

// Checks and configures each VCSEL on its I2C mux channel.
void InitVcsels();

// Advances the VCSEL measurements by one step: starts a ranging cycle on
// every working sensor, or collects the results of the running one. Call
// every loop.
void RunVcsels();

// Clocks SCL to release a stuck I2C peripheral, then restarts Wire.
void RecoverI2cBus();

#endif  // FSB_CODE_FSB_CODE_H_
