#include <Wire.h>
#include <Adafruit_VL6180X.h>

Adafruit_VL6180X vl = Adafruit_VL6180X();

unsigned long startTime;

void setup() {
  Serial.begin(115200);
  startTime = millis();

  while (!Serial) { delay(1); }
  Serial.println("Time,Distance");

  if (!vl.begin()) {
    Serial.println("Failed to find sensor");
    while (1);
  }
}

void loop() {
  unsigned long currentTime = millis();
  float elapsedTime = (currentTime - startTime) / 1000.0;

  if (elapsedTime > 30) {
    Serial.println("Measurement complete.");
    while (1);
  }

  uint8_t range = vl.readRange();
  Serial.print(elapsedTime);
  Serial.print(",");
  Serial.println(range);

  delay(100);
}
