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
const float factor[40] PROGMEM = {
  /*18*/ 1,       0.25,    1,        0.83044,
  /*19*/ 1,       1.656,   1,        1.656,
  /*20*/ 1,       1,       1,        0.00326,
  /*21*/ 0.01,    0.01,    0.01,     0.01,
  /*22*/ 0.01,    0.01,    0.01,     0.01,
  /*32*/ 0.013986,1.656,   1,        1,
  /*35*/ 1,       1.656,   0.25,     1,
  /*37*/ 0.01,    0.01,    0.01,     0.01,
  /*38*/ 0.01,    0.01,    0.01,     0.01,
  /*49*/ 1,       0.00024414,0.00024414,1
};
const float offset[40] PROGMEM = {
  /*18*/   0,   -125,      0,     -315.013,
  /*19*/   0,   -968,      0,     -968,
  /*20*/   0,      0,      0,       -1.63,
  /*21*/   0,      0,      0,        0,
  /*22*/   0,      0,      0,        0,
  /*32*/-0.0112,-968,      0,        0,
  /*35*/   0,   -968,   -125,        0,
  /*37*/   0,      0,      0,        0,
  /*38*/   0,      0,      0,        0,
  /*49*/   0,      0,      0,        0
};
/* ─────────────────────────────────────────────────────────────── */
uint8_t  rxBytes[80];      // raw 80-byte packet
int16_t  rawShorts[40];    // after byte-to-short conversion
int16_t  scaledShorts[40]; // final, gain+offset applied
inline int16_t bytesToShort(uint8_t hi, uint8_t lo)
{ return (int16_t)((hi << 8) | lo); }
void setup()
{
  Serial.begin(115200);
  delay(100);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  if (!rf95.init())  { while (1); }
  rf95.setFrequency(RF95_FREQ);
  rf95.setTxPower(23, false);
  Serial.println(F("Receiver ready"));
}
void loop()
{
  if (!rf95.available()) return;
  uint8_t len = sizeof(rxBytes);
  if (!rf95.recv(rxBytes, &len)) return;
  if (len != 80) return;                   // ensure full packet
  /* ── raw bytes → raw shorts ──────────────────────────────── */
  for (uint8_t i = 0; i < 40; ++i)
    rawShorts[i] = bytesToShort(rxBytes[i*2+1], rxBytes[i*2]);
  /* ── apply gain + offset, store back into shorts array ───── */
  for (uint8_t i = 0; i < 40; ++i) {
    float  f = pgm_read_float(&factor[i]);
    float  o = pgm_read_float(&offset[i]);
    float  scaled = rawShorts[i] * f + o;
    scaledShorts[i] = (int16_t)roundf(scaled);
  }
  /* ── print the scaled array ──────────────────────────────── */
  for (uint8_t i = 0; i < 40; ++i) {
    Serial.print(scaledShorts[i]);
    if (i < 39) Serial.write(',');
  }
  Serial.println();
}