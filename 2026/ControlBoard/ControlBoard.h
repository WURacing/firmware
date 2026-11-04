/**
 * @file ControlBoard.h
 * @brief Control Board firmware interface, WUFR26.
 *
 * Reads the shift and clutch paddles on the steering wheel, actuates the
 * shift solenoids, drives the clutch servo, and reports clutch state on the
 * CAN bus.
 *
 * Hardware: Adafruit Feather M4 CAN.
 */

#ifndef CONTROLBOARD_CONTROLBOARD_H_
#define CONTROLBOARD_CONTROLBOARD_H_

#include <Arduino.h>

// Clutch paddle positions, each from 0 (released) to 1 (fully pulled).
struct ClutchPaddles {
  double left;
  double right;
};

// Fires at most one shift per paddle press, using the short neutral pulse
// when both clutch paddles are pulled. Call every loop.
void UpdateShifting(const ClutchPaddles& paddles);

// Averages the clutch paddle position over several loops and moves the clutch
// servo to match. Call every loop.
void UpdateClutch();

// Sends the commanded clutch angle and paddle positions on CAN once per CAN
// interval. Call every loop.
void SendClutchState(const ClutchPaddles& paddles);

// Toggles the status LED once per blink interval. Call every loop.
void Blink();

// Records the current state of both shift paddles as bit `bit` of
// `up_history` and `down_history` (1 = pressed).
void CheckShiftPaddles(uint16_t bit, uint32_t& up_history,
                       uint32_t& down_history);

// Fires the upshift solenoid for `pulse_ms`, then waits `pulse_ms`.
void Upshift(int pulse_ms);

// Fires the downshift solenoid for `pulse_ms`, then waits `pulse_ms`.
void Downshift(int pulse_ms);

// Returns the position of whichever clutch paddle is pulled further.
double GetClutchPaddlePosition();

// Returns the position of each clutch paddle.
ClutchPaddles GetClutchPaddlePositions();

// Converts an averaged clutch paddle position (0-1) to a servo angle.
// LinearClutchMap is the one in use; SCurveClutchMap is kept for tuning.
double LinearClutchMap(double paddle_position);
double SCurveClutchMap(double paddle_position);
double PiecewiseClutchMap(double paddle_position);
// Moves the clutch servo to `angle` degrees.
void SetClutchPosition(unsigned int angle);

// Sets bit `bit` of `bits` to `value`.
void ModifyBit(uint16_t bit, bool value, uint32_t& bits);

// Writes `data` to the open CAN packet as two bytes, little-endian.
void CanWriteShort(int16_t data);

// Sends `values[start_index]` through `values[start_index + 3]` as a single
// 8-byte CAN frame with ID `frame_id`.
void CanShortFrame(const int16_t* values, int start_index, int frame_id);

#endif  // CONTROLBOARD_CONTROLBOARD_H_
