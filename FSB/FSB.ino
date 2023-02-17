// Description: Front Sensor Board code for WUFR23
// Authors: Jonah Sachs, Hayden Schroeder

#include <CAN.h>
#include <SPI.h>

//#include <GoblinMode.h>

#define BAUD_RATE 1000000
#define ROLLING_AVG 64

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

// Configured for + or - 4g
void setup() {
  pinMode(LED, OUTPUT);

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

  // Receive data
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

  // Delta time
  unsigned long averageTime = 0;
  if (millis() - averageTime > 10) {
    averageTime = millis();
    CAN.beginPacket(0x2);
    CAN.write(x_send);
    CAN.write(y_send);
    CAN.write(z_send);
    CAN.endPacket();
  }

  // Prints :)
//  char x = x_send;
//  char y = y_send;
//  char z = z_send;
  Serial.printf("X:%d\tY:%d\tZ:%d\n", x_send, y_send, z_send);
}
