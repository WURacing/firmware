#include <SPI.h>
#include <RH_RF95.h>
#define RF95_FREQ   915.0
#define RFM95_CS      8
#define RFM95_INT     3
#define RFM95_RST     4
RH_RF95 rf95(RFM95_CS, RFM95_INT);
/* ───── Frame-ordered lookup tables ──────────────────────────────
   The order exactly matches the 10 frames the Sender packs into
   its 80-byte payload:
        18,19,20,21,22,32,35,37,38,49               */
/*  factor[i] * raw + offset[i]  →  scaledShorts[i] */
/* ─── lookup tables (NO PROGMEM on SAMD) ─────────────────────── */
const float factor[40] = {
  .001, 0.25, .001, 0.83044,  .001, 1.656, .001, 1.656,
  .001, .001, .001, 0.00326,     0.01, 0.01, 0.01, 0.01,
  0.01, 0.01, 0.01, 0.01, 0.013986, 1.656, .001, .001,
  .001, 1.656, 0.25, .001,       0.01, 0.01, 0.01, 0.01,
  0.01, 0.01, 0.01, 0.01,  .001, 0.00024414, 0.00024414, .001
};
const float offset[40] = {
  0, -125, 0, -315.013,  0, -968, 0, -968,
  0, 0, 0, -1.63,        0, 0, 0, 0,
  0, 0, 0, 0,            -0.0112, -968, 0, 0,
  0, -968, -125, 0,      0, 0, 0, 0,
  0, 0, 0, 0,            0, 0, 0, 0
};
uint8_t  rxBytes[80];


void setup()
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);

  // Manual radio reset
  digitalWrite(RFM95_RST, LOW); delay(10);
  digitalWrite(RFM95_RST, HIGH); delay(10);

  if (!rf95.init())  while (1);          // halt if radio fails
  rf95.setFrequency(RF95_FREQ);
  rf95.setTxPower(23, false);

  Serial.println(F("Receiver ready"));
}


void loop() {
  if (!rf95.available()) return;

  uint8_t len = sizeof(rxBytes);
  if (!rf95.recv(rxBytes, &len)) return;
  if (len != 80) return;                // expect full frame

  Serial.print(F("Values: "));

  for (uint8_t i = 0; i < 40; ++i) {
    uint16_t raw  = (uint16_t)rxBytes[2 * i + 1] << 8 | rxBytes[2 * i];
    float    phys = raw * factor[i] + offset[i];

    Serial.print(phys, 2);              // two-decimal precision
    if (i < 39) Serial.print(',');      // comma-separate, no trailing comma
  }
  Serial.println();
}
