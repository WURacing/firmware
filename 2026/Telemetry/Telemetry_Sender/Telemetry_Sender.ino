/**
 * @file Telemetry_Sender.ino
 * @brief Telemetry Sender firmware implementation, WUFR26.
 *
 * CAN frames on the bus and their data points (2025 layout). Frames marked
 * "yes" are copied into the telemetry buffer by CanFiller().
 *
 *   ID  Sent  Data points
 *   16        ShockPotFL, ShockPotFR, null, null
 *   17        null, null, null, null
 *   18  yes   null, BrkPrsF, null, SteeringPot
 *   19  yes   null, BrakeTempFR, null, BrakeTempFL
 *   20  yes   null, null, null, Pitot
 *   21  yes   FAccX, FAccY, FAccZ, FGyroX
 *   22  yes   FGyroY, FGyroZ, FMagX, FMagY
 *   23        FMagZ, FrRideHeight1, FrRideHeight2
 *   32  yes   ShockPotRL, BrakeTempRL, null, null
 *   33        InletTemp2R, null, InletTemp1R, null
 *   34        null, ShockPotRR, InletTempL, null
 *   35  yes   null, BrakeTempRR, BrkPrsR, null
 *   36        OutletTempL, OutletTempR, null, null
 *   37  yes   RAccX, RAccY, RAccZ, RGyroX
 *   38  yes   RGyroY, RGyroZ, RMagX, RMagy
 *   39        RMagZ, ReRideHeight1, ReRideHeight2
 *   49  yes   ClutchPos, ClutchL, ClutchR
 *   50        null, null, null, null
 *   69        Endurance (Fuel Pressure, GPS Speed, Throttle Position,
 *             Coolant Temp)
 *
 * @see Telemetry_Sender.h for an overview of the board.
 */

#include "Telemetry_Sender.h"

#include <CAN.h>
#include <RH_RF95.h>
#include <SPI.h>

// Pins. Letters are the RFM95 FeatherWing jumper labels.
constexpr int RFM95_RST_PIN = 11;  // "A"
constexpr int RFM95_CS_PIN = 10;   // "B"
constexpr int RFM95_INT_PIN = 6;   // "D"
constexpr int LED_PIN = 13;

// Timing.
constexpr uint32_t blink_interval_ms = 1000;
constexpr uint32_t tel_period_ms = 1000;
constexpr uint32_t loop_delay_ms = 10;
constexpr uint32_t radio_reset_delay_ms = 10;
constexpr uint32_t serial_start_delay_ms = 100;

// Communication.
constexpr uint32_t serial_baud_rate = 115200;
constexpr int can_baud_rate = 1000000;
constexpr double rf95_freq = 915.0;  // MHz
constexpr int8_t rf95_tx_power_dbm = 23;

// Bytes of telemetry sent per radio packet.
constexpr int dbc_matrix_len = 88;

// Prints every converted value over serial.
constexpr bool print_bool = true;

uint32_t blink_previous_millis = 0;
bool led_state = LOW;
uint32_t cur_time = millis();
uint32_t prev_time = 0;

uint8_t dbc_matrix[dbc_matrix_len];
int16_t s_dbc_matrix[dbc_matrix_len / 2];

RH_RF95 rf95(RFM95_CS_PIN, RFM95_INT_PIN);

void setup() {
  pinMode(RFM95_RST_PIN, OUTPUT);
  digitalWrite(RFM95_RST_PIN, HIGH);

  Serial.begin(serial_baud_rate);
  delay(serial_start_delay_ms);

  // Manual radio reset.
  digitalWrite(RFM95_RST_PIN, LOW);
  delay(radio_reset_delay_ms);
  digitalWrite(RFM95_RST_PIN, HIGH);
  delay(radio_reset_delay_ms);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println(
        "Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug "
        "info");
    while (true) {
    }
  }
  Serial.println("LoRa radio init OK!");

  if (!rf95.setFrequency(rf95_freq)) {
    Serial.println("setFrequency failed");
    while (true) {
    }
  }
  Serial.print("Set Freq to: ");
  Serial.println(rf95_freq);

  rf95.setTxPower(rf95_tx_power_dbm, false);

  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false);
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true);
  if (!CAN.begin(can_baud_rate)) {
    Serial.println("Starting CAN failed!");
    while (true) {
    }
  }

  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  Blink();
  delay(loop_delay_ms);
  cur_time = millis();
  for (int packet_size = CAN.parsePacket(); packet_size;
       packet_size = CAN.parsePacket()) {
    CanFiller(CAN.packetId(), packet_size, dbc_matrix);
  }

  // Convert to shorts so the values can be checked over serial.
  ConvertBytesToShorts(dbc_matrix, s_dbc_matrix, dbc_matrix_len);
  if (print_bool) {
    Serial.print("Shorts: ");
    for (int i = 0; i < dbc_matrix_len / 2; ++i) {
      Serial.print(s_dbc_matrix[i]);
      Serial.print(",");
    }
    Serial.println(" ");
  }

  if (cur_time - prev_time >= tel_period_ms) {
    prev_time = cur_time;
    if (print_bool) {
      Serial.println("Sending...");
    }
    rf95.send(dbc_matrix, dbc_matrix_len);
  }
}

// Reads all `packet_size` bytes of the current CAN packet into `dbc_matrix`,
// starting at `offset`.
void ReadPacket(int offset, int packet_size, uint8_t* dbc_matrix) {
  for (int i = 0; i < packet_size; ++i) {
    dbc_matrix[offset + i] = CAN.read();
  }
}

// TODO: packet_id is 8 bits, so CAN IDs above 0xFF wrap around (e.g.
// 0x112 is read as 18).
void CanFiller(uint8_t packet_id, int packet_size, uint8_t* dbc_matrix) {
  if (packet_id == 18) {
    ReadPacket(0, packet_size, dbc_matrix);
  } else if (packet_id == 19) {
    ReadPacket(8, packet_size, dbc_matrix);
  } else if (packet_id == 20) {
    ReadPacket(16, packet_size, dbc_matrix);
  } else if (packet_id == 21) {
    ReadPacket(24, packet_size, dbc_matrix);
  } else if (packet_id == 22) {
    ReadPacket(32, packet_size, dbc_matrix);
  } else if (packet_id == 32) {
    ReadPacket(40, packet_size, dbc_matrix);
  } else if (packet_id == 35) {
    ReadPacket(48, packet_size, dbc_matrix);
  } else if (packet_id == 37) {
    ReadPacket(56, packet_size, dbc_matrix);
  } else if (packet_id == 38) {
    ReadPacket(64, packet_size, dbc_matrix);
  } else if (packet_id == 49) {
    // TODO: The Control Board sends ClutchPos as 2 bytes, so reading
    // it as 1 byte shifts ClutchL and ClutchR by one byte.
    dbc_matrix[72] = CAN.read();  // ClutchPos (8-bit).
    dbc_matrix[73] = 0;           // Padding.
    dbc_matrix[74] = CAN.read();  // ClutchL LSB.
    dbc_matrix[75] = CAN.read();  // ClutchL MSB.
    dbc_matrix[76] = CAN.read();  // ClutchR LSB.
    dbc_matrix[77] = CAN.read();  // ClutchR MSB.
  } else if (packet_id == 105) {
    ReadPacket(80, packet_size, dbc_matrix);
  }
}

void ConvertBytesToShorts(const uint8_t* byte_array, int16_t* short_array,
                          int byte_array_size) {
  for (int i = 0; i < byte_array_size / 2; ++i) {
    short_array[i] =
        (static_cast<int16_t>(byte_array[2 * i + 1]) << 8) | byte_array[2 * i];
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
