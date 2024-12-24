#include <SPI.h>
#include <RH_RF95.h>

#define RF95_FREQ 915.0
#define RFM95_CS    8
#define RFM95_INT   3
#define RFM95_RST   4

RH_RF95 rf95(RFM95_CS, RFM95_INT);

uint8_t Entry_Matrix[72]; //255 - 4(header)
short short_Matrix[36];
uint8_t Small_Entry[8];

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);
  delay(100);

  // Manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  rf95.setTxPower(23, false);
}

void loop() {
  if (rf95.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len)) {
      digitalWrite(LED_BUILTIN, HIGH);
      RH_RF95::printBuffer("Received: ", buf, len);
      Serial.print("Got: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);

      // Copy received data to Entry_Matrix
      memcpy(Entry_Matrix, buf, min(len, sizeof(Entry_Matrix)));

      // Convert bytes to shorts
      for (int i = 0; i < 36; i++) {
        short_Matrix[i] = bytesToShort(Entry_Matrix[i * 2 + 1], Entry_Matrix[i * 2]);
      }

      // Print short_Matrix
      Serial.print("Shorts: ");
      for (int i = 0; i < 36; i++) {
        Serial.print(short_Matrix[i]);
        Serial.print(",");
      }
      Serial.println();

      uint8_t data[] = "1";
      rf95.send(data, sizeof(data));
      rf95.waitPacketSent();
      Serial.println("Sent a reply");
      digitalWrite(LED_BUILTIN, LOW);
    } else {
      Serial.println("Failed the Receive");
    }
  }
}

short bytesToShort(byte highByte, byte lowByte) {
  return (short)((highByte << 8) | lowByte);
}
