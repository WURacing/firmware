/**
 * @file Telemetry_Sender.h
 * @brief Telemetry Sender firmware interface, WUFR26.
 *
 * Collects selected CAN frames from the car into a single 88-byte buffer and
 * transmits it over LoRa (RFM95, 915 MHz) once per telemetry period.
 *
 * Hardware: Adafruit Feather M4 CAN with an RFM95 LoRa FeatherWing.
 */

#ifndef TELEMETRY_TELEMETRY_SENDER_TELEMETRY_SENDER_H_
#define TELEMETRY_TELEMETRY_SENDER_TELEMETRY_SENDER_H_

#include <Arduino.h>

// Toggles the status LED once per blink interval. Call every loop.
void Blink();

// Reads the payload of the CAN packet `packet_id` (`packet_size` bytes) into
// its slot in `dbc_matrix`. Packets without a slot are ignored.
void CanFiller(uint8_t packet_id, int packet_size, uint8_t* dbc_matrix);

// Combines each little-endian byte pair of `byte_array` into one entry of
// `short_array`, which must hold `byte_array_size / 2` values.
void ConvertBytesToShorts(const uint8_t* byte_array, int16_t* short_array,
                          int byte_array_size);

#endif  // TELEMETRY_TELEMETRY_SENDER_TELEMETRY_SENDER_H_
