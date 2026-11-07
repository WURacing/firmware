/**
 * @file Telemetry_Sender.ino
 * @brief Telemetry Sender firmware implementation, WUFR27.
 *
 * Fuel rate (L/h) = fuel_gain x RPM x injector pulse value. The total is
 * integrated every loop, held for at most the stale time of the older input,
 * and saved to flash once per second by FuelStorage.
 *
 * @see Telemetry_Sender.h for an overview of the board and the radio packet.
 */

#include "Telemetry_Sender.h"

#include <CANSAME5x.h>
#include <math.h>

#include "FuelStorage.h"

// Pins.
constexpr int RESET_BUTTON_PIN = 5;

// CAN IDs. The AiM dash rebroadcasts the PE3's engine channels as 0x69-0x6E.
constexpr int aim_frame_ids[] = {0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E};
constexpr int aim_frame_count = 6;
constexpr int rpm_frame_index = 1;          // 0x6A: RPM in bytes 6-7, uint16.
constexpr int pulse_frame_index = 5;        // 0x6E: pulse in bytes 4-7, int32.
constexpr int fuel_frame_id = 0x33;         // Sent by this board.
constexpr int front_reset_frame_id = 0x6F;  // One byte: 0 released, 1 held.

// Fuel calculation.
constexpr double fuel_gain = 2.5012e-5;  // L/h per RPM per pulse unit.
constexpr double pulse_unit_ms = 0.1;    // Pulse value 55 means 5.5 ms.

// Timing.
constexpr uint32_t rpm_stale_ms = 500;
constexpr uint32_t pulse_stale_ms = 1500;
constexpr uint32_t fuel_frame_period_ms = 100;
constexpr uint32_t radio_period_ms = 200;
constexpr uint32_t save_period_ms = 1000;
constexpr uint32_t callsign_period_ms = 540000;  // Every 9 minutes.

// Radio packet. See Telemetry_Sender.h for the layout.
constexpr uint8_t radio_packet_version = 1;
constexpr int radio_sent_aim_frames = 5;  // 0x69-0x6D; 0x6E stays on board.
constexpr int radio_packet_len = 53;
constexpr char callsign[] = "\r\nDE KQ9L\r\n";

// Communication.
constexpr uint32_t radio_baud_rate = 19200;
constexpr int32_t can_baud_rate = 1000000;

CANSAME5x CAN;
Adafruit_FlashTransport_QSPI flash_transport;
FuelStorage storage(&flash_transport);

uint8_t aim_data[aim_frame_count * 8] = {0};
uint8_t sequence = 0;
double rpm = NAN;
double pulse_value = NAN;
double liters = 0;
double rate = 0;
double last_saved = -1;
uint32_t rpm_at = 0;
uint32_t pulse_at = 0;
uint32_t last_integrate = 0;
uint32_t last_radio = 0;
uint32_t last_fuel_frame = 0;
uint32_t last_save = 0;
uint32_t last_callsign = 0;
bool have_rpm = false;
bool have_pulse = false;
bool total_ready = false;
bool resetting = false;
double reset_start_liters = 0;
bool front_reset = false;
bool front_reset_armed = false;
uint32_t front_reset_at = 0;

uint8_t Crc8(const uint8_t* data, unsigned length) {
  uint8_t crc = 0;
  for (unsigned i = 0; i < length; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07)
                         : static_cast<uint8_t>(crc << 1);
    }
  }
  return crc;
}

// Both inputs arrived recently enough to trust.
bool InputsFresh(uint32_t now) {
  return have_rpm && have_pulse && now - rpm_at < rpm_stale_ms &&
         now - pulse_at < pulse_stale_ms;
}

// Inputs are fresh and physically plausible.
bool InputsValid(uint32_t now) {
  return InputsFresh(now) && isfinite(rpm) && rpm >= 0 && rpm <= 30000 &&
         isfinite(pulse_value) && pulse_value >= 0 &&
         pulse_value * pulse_unit_ms <= 30 &&
         rpm * pulse_value * pulse_unit_ms <= 120000;
}

void Integrate(uint32_t now) {
  uint32_t elapsed = now - last_integrate;
  if (total_ready && InputsValid(last_integrate)) {
    // Count the held rate only until the older input goes stale.
    uint32_t remaining = rpm_stale_ms - (last_integrate - rpm_at);
    const uint32_t pulse_remaining =
        pulse_stale_ms - (last_integrate - pulse_at);
    if (remaining > pulse_remaining) {
      remaining = pulse_remaining;
    }
    if (elapsed > remaining) {
      elapsed = remaining;
    }
    liters += fuel_gain * rpm * pulse_value * elapsed / 3600000.0;
  }
  last_integrate = now;
  rate = InputsValid(now) ? fuel_gain * rpm * pulse_value : 0;
}

// Returns the index of `id` in aim_frame_ids, or -1.
int AimFrameIndex(int32_t id) {
  for (int i = 0; i < aim_frame_count; ++i) {
    if (id == aim_frame_ids[i]) {
      return i;
    }
  }
  return -1;
}

void ReceiveCan() {
  for (int count = 0; count < 16; ++count) {
    const int size = CAN.parsePacket();
    if (size == 0) {
      break;
    }
    const int32_t id = CAN.packetId();
    const bool standard = !CAN.packetExtended() && !CAN.packetRtr();
    uint8_t bytes[8];
    int length = 0;
    while (CAN.available()) {
      const int value = CAN.read();
      if (length < 8) {
        bytes[length++] = static_cast<uint8_t>(value);
      }
    }

    if (id == front_reset_frame_id && standard && size == 1 && length == 1 &&
        bytes[0] <= 1) {
      // A release must be seen before a hold counts.
      if (bytes[0] == 0) {
        front_reset_armed = true;
      }
      front_reset = front_reset_armed && bytes[0] == 1;
      front_reset_at = millis();
      continue;
    }

    const int index = AimFrameIndex(id);
    if (index < 0 || !standard || size != 8 || length != 8) {
      if (index == rpm_frame_index) {
        have_rpm = false;
      }
      if (index == pulse_frame_index) {
        have_pulse = false;
      }
      continue;
    }

    const uint32_t now = millis();
    Integrate(now);
    memcpy(aim_data + index * 8, bytes, 8);
    if (index == rpm_frame_index) {
      rpm = static_cast<uint16_t>(bytes[6] | bytes[7] << 8);
      rpm_at = now;
      have_rpm = true;
    }
    if (index == pulse_frame_index) {
      const uint32_t raw = static_cast<uint32_t>(bytes[4]) |
                           static_cast<uint32_t>(bytes[5]) << 8 |
                           static_cast<uint32_t>(bytes[6]) << 16 |
                           static_cast<uint32_t>(bytes[7]) << 24;
      pulse_value = static_cast<int32_t>(raw);
      pulse_at = now;
      have_pulse = true;
    }
  }
}

void FuelFrameBytes(uint8_t* frame) {
  // Clamp before converting so a value never wraps. Stale rate is zero.
  const uint32_t rate_x100 =
      static_cast<uint32_t>(fmin(65535.0, fmax(0.0, round(rate * 100.0))));
  const uint32_t total_ml =
      static_cast<uint32_t>(fmin(16777215.0, fmax(0.0, round(liters * 1000))));
  frame[0] = static_cast<uint8_t>(rate_x100);
  frame[1] = static_cast<uint8_t>(rate_x100 >> 8);
  frame[2] = static_cast<uint8_t>(total_ml);
  frame[3] = static_cast<uint8_t>(total_ml >> 8);
  frame[4] = static_cast<uint8_t>(total_ml >> 16);
  frame[5] = 0;
  frame[6] = 0;
  frame[7] = 0;
}

void BuildRadioPacket(uint8_t* packet) {
  packet[0] = 0x55;
  packet[1] = 0xAA;
  packet[2] = radio_packet_version;
  packet[3] = sequence++;
  memcpy(packet + 4, aim_data, radio_sent_aim_frames * 8);
  FuelFrameBytes(packet + 4 + radio_sent_aim_frames * 8);
  packet[radio_packet_len - 1] = Crc8(packet + 2, radio_packet_len - 3);
}

void SendRadio() {
  uint8_t packet[radio_packet_len];
  BuildRadioPacket(packet);
  Serial1.write(packet, sizeof(packet));
}

void CheckReset(uint32_t now) {
  static bool released = false;
  static bool holding = false;
  static bool allowed = false;
  static uint32_t hold_at = 0;
  static uint32_t flash_at = 0;
  static uint8_t flashes = 0;
  static uint16_t flash_period_ms = 400;
  static bool first = true;
  static bool raw = false;
  static bool stable = false;
  static uint32_t changed_at = 0;

  if (resetting && !storage.busy()) {
    const bool saved = storage.ok;
    if (saved) {
      liters -= reset_start_liters;
      last_saved = 0;
    }
    resetting = false;
    flash_at = now;
    flashes = saved ? 3 : 6;  // 3 slow flashes: done. 6 fast: refused.
    flash_period_ms = saved ? 400 : 160;
  }

  // Debounce the button (or the front board's held frame) for 50 ms.
  if (now - front_reset_at >= 350) {
    front_reset = false;
    front_reset_armed = false;
  }
  const bool input = digitalRead(RESET_BUTTON_PIN) == LOW || front_reset;
  if (first) {
    raw = input;
    stable = input;
    changed_at = now;
    first = false;
  }
  if (input != raw) {
    raw = input;
    changed_at = now;
  }
  if (now - changed_at >= 50) {
    stable = raw;
  }

  const bool stopped = have_rpm && now - rpm_at < rpm_stale_ms && rpm == 0;
  if (!stable) {
    if (!raw && now - changed_at >= 50) {
      released = true;
    }
    holding = false;
  } else if (released) {
    if (!holding) {
      holding = true;
      hold_at = now;
      allowed = true;
    }
    allowed &= stopped && total_ready && storage.ok && !resetting;
    if (now - hold_at >= 3000) {
      released = false;
      holding = false;
      resetting = allowed && storage.save(0, now);
      if (resetting) {
        reset_start_liters = liters;
      } else {
        flash_at = now;
        flashes = 6;
        flash_period_ms = 160;
      }
    }
  }

  const uint32_t led_now = millis();
  bool lit = false;
  if (flashes && led_now - flash_at < flashes * flash_period_ms) {
    lit = (led_now - flash_at) % flash_period_ms < flash_period_ms / 2;
  } else {
    flashes = 0;
    lit = holding && (led_now / 250) % 2 == 0;
  }
  digitalWrite(LED_BUILTIN, lit);
}

void setup() {
  Serial1.begin(radio_baud_rate);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  total_ready = storage.begin(liters);
  if (total_ready) {
    last_saved = storage.savedLiters;  // Don't rewrite an unchanged total.
  } else {
    // Count in RAM only; never erase flash we don't recognize.
    liters = 0;
    total_ready = true;
  }

  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, LOW);
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, HIGH);
  if (!CAN.begin(can_baud_rate)) {
    while (true) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
    }
  }
  CAN.filter(0x68, 0x7F8);  // Only 0x68-0x6F.

  Serial1.print(callsign);
  last_integrate = last_radio = last_fuel_frame = last_save = last_callsign =
      millis();
}

void loop() {
  ReceiveCan();
  storage.poll(millis());  // One short flash step; CAN is read again next loop.
  uint32_t now = millis();
  Integrate(now);
  CheckReset(now);
  now = millis();
  Integrate(now);

  if (now - last_fuel_frame >= fuel_frame_period_ms) {
    last_fuel_frame = now;
    uint8_t frame[8];
    FuelFrameBytes(frame);
    CAN.beginPacket(fuel_frame_id, 8);
    CAN.write(frame, 8);
    CAN.endPacket();
  }
  if (now - last_radio >= radio_period_ms) {
    last_radio = now;
    SendRadio();
  }
  if (now - last_save >= save_period_ms) {
    last_save = now;
    last_saved = storage.savedLiters;
    if (total_ready && !resetting && !storage.busy() && liters != last_saved) {
      storage.save(liters, now);
    }
  }
  if (now - last_callsign >= callsign_period_ms) {
    last_callsign = now;
    Serial1.print(callsign);
  }
}
