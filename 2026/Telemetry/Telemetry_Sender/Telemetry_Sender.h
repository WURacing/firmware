/**
 * @file Telemetry_Sender.h
 * @brief Telemetry Sender firmware interface, WUFR27.
 *
 * Counts fuel used from the AiM dash's RPM (0x6A) and injector pulse (0x6E)
 * frames, keeps the total in flash, and broadcasts it on CAN as 0x33. Every
 * 200 ms it sends one radio packet to the Trimble radio on Serial1:
 *
 *   Offset  Size  Field
 *   0       2     0x55 0xAA (sync)
 *   2       1     Packet version (radio_packet_version)
 *   3       1     Sequence number, wraps at 255
 *   4       40    AiM frames 0x69, 0x6A, 0x6B, 0x6C, 0x6D (8 bytes each)
 *   44      8     Fuel frame 0x33 (same bytes as the CAN frame)
 *   52      1     CRC-8 (poly 0x07, init 0) over bytes 2-51
 *
 * TelemetryWebApp/telemetryBackend/GetDataFromCar.py reads this packet. Change
 * both together and bump radio_packet_version.
 *
 * Hardware: Adafruit Feather M4 CAN with a Trimble radio on Serial1.
 */

#ifndef TELEMETRY_TELEMETRY_SENDER_TELEMETRY_SENDER_H_
#define TELEMETRY_TELEMETRY_SENDER_TELEMETRY_SENDER_H_

#include <Arduino.h>

// Reads waiting CAN frames: stores the AiM frames, updates RPM and injector
// pulse, and handles the front board's reset button frame.
void ReceiveCan();

// Adds the fuel used since the last call to the total, then updates the
// current fuel rate.
void Integrate(uint32_t now);

// Fills `frame` (8 bytes) with the 0x33 fuel frame: rate x100 (L/h) in bytes
// 0-1, total in mL in bytes 2-4, bytes 5-7 zero.
void FuelFrameBytes(uint8_t* frame);

// Fills `packet` (radio_packet_len bytes) with the next radio packet.
void BuildRadioPacket(uint8_t* packet);

// Sends the next radio packet to the Trimble radio.
void SendRadio();

// Handles the reset button (hold 3 s with the engine stopped to zero the
// total) and drives the status LED.
void CheckReset(uint32_t now);

// CRC-8, polynomial 0x07, initial value 0.
uint8_t Crc8(const uint8_t* data, unsigned length);

#endif  // TELEMETRY_TELEMETRY_SENDER_TELEMETRY_SENDER_H_
