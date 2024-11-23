#include <Wire.h>
#include <Adafruit_VL6180X.h>
#include <RecursiveUpdate.h>

Adafruit_VL6180X vl = Adafruit_VL6180X();
RecursiveUpdate filter;

unsigned long startTime;

// Constants
const int MAX_READINGS = 300; // Max readings to store for 30 seconds with 100ms intervals
const float ALPHA = 0.1;      // Smoothing factor for the filter

// Variables
float currentAverage = -1.0;  // Start as None (-1.0)
float rawReadings[MAX_READINGS];
float filteredReadings[MAX_READINGS];
int readingCount = 0;

void setup() {
  Serial.begin(115200);
  startTime = millis();

  while (!Serial) { delay(1); }
  Serial.println("Time,Distance,Filtered");

  if (!vl.begin()) {
    Serial.println("Failed to find sensor");
    while (1);
  }
}

void loop() {
  unsigned long currentTime = millis();
  float elapsedTime = (currentTime - startTime) / 1000.0;

  if (elapsedTime > 30 || readingCount >= MAX_READINGS) {
    // Finish and output the data as a CSV for later plotting
    Serial.println("Measurement complete.");
    Serial.println("Time,Distance,Filtered");

    for (int i = 0; i < readingCount; i++) {
      Serial.print(i * 0.1, 1); // Assuming a 100ms interval
      Serial.print(",");
      Serial.print(rawReadings[i]);
      Serial.print(",");
      Serial.println(filteredReadings[i]);
    }

    while (1); // Stop the program
  }

  // Read range
  uint8_t range = vl.readRange();

  // Update raw and filtered arrays
  if (readingCount < MAX_READINGS) {
    rawReadings[readingCount] = range;
    currentAverage = filter.updateRecursiveFilter(range, currentAverage, ALPHA);
    filteredReadings[readingCount] = currentAverage;
    readingCount++;
  }

  // Print raw and filtered values
  Serial.print(elapsedTime, 1);
  Serial.print(",");
  Serial.print(range);
  Serial.print(",");
  Serial.println(currentAverage);

  delay(100); // Wait 100ms before the next reading
}
