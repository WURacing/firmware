// Description: Front Sensor Board code for WUFR23
// Authors: Jonah Sachs, Hayden Schroeder

#include <CAN.h>
#include <SPI.h>

//#include <GoblinMode.h>

#define BAUD_RATE 1000000
#define ROLLING_AVG 64
#define ANLG_RES 4096

signed char x_acc;
signed char y_acc;
signed char z_acc;

signed char x_send;
signed char y_send;
signed char z_send;

unsigned long datacount = 0;

signed char x_avgs[ROLLING_AVG];
signed char y_avgs[ROLLING_AVG];
signed char z_avgs[ROLLING_AVG];

long x_tot;
long y_tot;
long z_tot;

const int ACCEL_PIN = 5;
const int LED = 13;

int ledState = LOW;
unsigned long currentMillis = millis();
unsigned long previousMillis = 0;
const long interval = 1000;

// Method for status indicator
void blink() {
  currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    digitalWrite(LED, ledState);
  }
}

void readAnalogs(float &anlg1, float &anlg2, float &anlg3, float &anlg4, float &anlg5, float &anlg6) {
  anlg1 = (float)analogRead(A0) / ANLG_RES;
  anlg2 = (float)analogRead(A1) / ANLG_RES;
  anlg3 = (float)analogRead(A2) / ANLG_RES;
  anlg4 = (float)analogRead(A3) / ANLG_RES;
  anlg5 = (float)analogRead(A4) / ANLG_RES;
  anlg6 = (float)analogRead(A5) / ANLG_RES;
}

// Configured for + or - 4g
void setup() {
  // Set up status indicator
  pinMode(LED, OUTPUT);

  // Set up SPI
  Serial.begin(11520);
  pinMode(ACCEL_PIN, OUTPUT);
  digitalWrite(ACCEL_PIN, HIGH);

  SPI.begin();
  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE3));

  digitalWrite(ACCEL_PIN, LOW);
  SPI.transfer(0x20);
  SPI.transfer(0x3F);
  digitalWrite(ACCEL_PIN, HIGH);

  digitalWrite(ACCEL_PIN, LOW);
  SPI.transfer(0x23);
  SPI.transfer(0x10);
  digitalWrite(ACCEL_PIN, HIGH);

  // Set up CAN
  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false);
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true);

  if (!CAN.begin(BAUD_RATE)) {
    Serial.println("Starting CAN failed!");
  }
}

void loop() {
  blink();

  // Receive SPI data
  digitalWrite(ACCEL_PIN, LOW);
  SPI.transfer(0xA9);
  x_acc = SPI.transfer(0x00);
  digitalWrite(ACCEL_PIN, HIGH);

  digitalWrite(ACCEL_PIN, LOW);
  SPI.transfer(0xAB);
  y_acc = SPI.transfer(0x00);
  digitalWrite(ACCEL_PIN, HIGH);

  digitalWrite(ACCEL_PIN, LOW);
  SPI.transfer(0xAD);
  z_acc = SPI.transfer(0x00);
  digitalWrite(ACCEL_PIN, HIGH);

  // Averaging
  x_avgs[datacount % ROLLING_AVG] = x_acc;
  y_avgs[datacount % ROLLING_AVG] = y_acc;
  z_avgs[datacount % ROLLING_AVG] = z_acc;

  x_tot = 0;
  y_tot = 0;
  z_tot = 0;

  for (int i = 0; i < ROLLING_AVG; i++) {
    x_tot += x_avgs[i];
    y_tot += y_avgs[i];
    z_tot += z_avgs[i];
  }

  x_send = (x_tot / ROLLING_AVG);
  y_send = (y_tot / ROLLING_AVG);
  z_send = (z_tot / ROLLING_AVG);

  datacount += 1;

  // Analog data
  float anlg1, anlg2, anlg3, anlg4, anlg5, anlg6;
  readAnalogs(anlg1, anlg2, anlg3, anlg4, anlg5, anlg6);

  // Delta time
  unsigned long averageTime = 0;
  if (millis() - averageTime > 1) {
    // Accelerometer data
    averageTime = millis();
    CAN.beginPacket(0x5);
    CAN.write(x_send);
    CAN.write(y_send);
    CAN.write(z_send);
    CAN.endPacket();

    // Analog data
    CAN.beginPacket(0x6);
    CAN.write(anlg1);
    CAN.write(anlg2);
    CAN.endPacket();

    CAN.beginPacket(0x7);
    CAN.write(anlg3);
    CAN.write(anlg4);
    CAN.endPacket();

    CAN.beginPacket(0x8);
    CAN.write(anlg5);
    CAN.write(anlg6);
    CAN.endPacket();
  }

  Serial.printf("X:%d\tY:%d\tZ:%d\n", x_send, y_send, z_send);
}
