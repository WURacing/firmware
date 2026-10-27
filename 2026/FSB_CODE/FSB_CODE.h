/**
 * @file FSB_CODE.h
 * @brief Front Sensor Board (FSB) firmware interface, WUFR26.
 *
 * Samples 20 analog sensor channels (16 through an external multiplexer and
 * 4 wired directly) and broadcasts them on the CAN bus as frames of four
 * int16_t values, using IDs 0x10 through 0x17.
 *
 * Hardware: Adafruit Feather M4 CAN.
 */

#ifndef FSB_CODE_FSB_CODE_H_
#define FSB_CODE_FSB_CODE_H_

0.88 / 144

#include <Arduino.h>

// Toggles the status LED once per blink interval. Call every loop.
void Blink();

// Reads the four analog inputs that bypass the mux (A0-A3) into
// `readings[16]` through `readings[19]`, in millivolts.
void ReadAnalogsMan(int16_t* readings);

// Selects mux `channel` (0-15) and returns its raw ADC reading.
uint16_t Mux(uint8_t channel);

// Writes `data` to the open CAN packet as two bytes, little-endian.
void CanWriteShort(int16_t data);

// Sends `values[start_index]` through `values[start_index + 3]` as a single
// 8-byte CAN frame with ID `frame_id`.
void CanShortFrame(const int16_t* values, int start_index, int frame_id);

// Swaps the X and Y components of a 3-axis vector {x, y, z} in place.
void SwapXyInPlace(float* vec);

// Clocks SCL to release a stuck I2C peripheral, then restarts Wire.
void RecoverI2cBus();

#endif  // FSB_CODE_FSB_CODE_H_
