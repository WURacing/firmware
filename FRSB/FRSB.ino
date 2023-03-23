// Description: Front right sensor board code for WUFR23. It reads accelerometer, gyro, and temp data over SPI, as well as analog data over analog pins. 
//              It then sends the data over CAN to the DAQ.
// Authors: Hayden Schroeder, Jonah Sachs

#include <CAN.h>
#include <SPI.h>

//#include <GoblinMode.h>

#define BAUD_RATE 1000000
#define ROLLING_AVG 64
#define ANLG_RES 4096
#define ANLG_LEN 6
#define BLINK_INTERVAL 1000
#define CAN_INTERVAL 1

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
unsigned long blinkCurrentMillis = millis();
unsigned long blinkPreviousMillis = 0;
unsigned long canCurrentMillis = millis();
unsigned long canPreviousMillis = 0;

/*
* Blinks the LED on the board to indicate that the board is running.
*/
void blink() {
  blinkCurrentMillis = millis();
  if (blinkCurrentMillis - blinkPreviousMillis >= BLINK_INTERVAL) {
    blinkPreviousMillis = blinkCurrentMillis;
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    digitalWrite(LED, ledState);
  }
}

/*
* Reads the analog values from the analog pins. The values are stored as voltage * 1000.
* @param analogs An array of shorts to store the analog values in
*/
void readAnalogs(short *analogs) {
  for (int i = 0; i < ANLG_LEN; i++) {
    analogs[i] = (analogRead(i) / ANLG_RES) * 1000;
  }
}

void setup() {
  // Set up status indicator
  pinMode(LED, OUTPUT);

  // Set up SPI. Accelerometer is configure for +/- 4g.
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
  short analogs[ANLG_LEN];
  readAnalogs(analogs);

  // Delta time
  canCurrentMillis = millis();
  if (canCurrentMillis - canPreviousMillis > CAN_INTERVAL) {
    // Accelerometer data
    canPreviousMillis = millis();
    CAN.beginPacket(0x11);
    CAN.write(x_send);
    CAN.write(y_send);
    CAN.write(z_send);
    CAN.endPacket();

    // Analog data
    CAN.beginPacket(0x12);
    CAN.write(analogs[0]);
    CAN.write(analogs[1]);
    CAN.write(analogs[2]);
    CAN.write(analogs[3]);
    CAN.endPacket();

    CAN.beginPacket(0x13);
    CAN.write(analogs[4]);
    CAN.write(analogs[5]);
    CAN.endPacket();
  }
}
