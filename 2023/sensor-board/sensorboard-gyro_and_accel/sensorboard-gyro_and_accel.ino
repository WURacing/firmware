#include <CAN.h>
#include <SPI.h>

//#include <GoblinMode.h>

#define BAUD_RATE 1000000
#define ROLLING_AVG 64

byte x_acc;
byte y_acc;
byte z_acc;

byte x_gyr;
byte y_gyr;
byte z_gyr;

byte whoami;


byte x_senda;
byte y_senda;
byte z_senda;

byte x_sendg;
byte y_sendg;
byte z_sendg;

unsigned long datacount = 0;

byte xacc_avgs[ROLLING_AVG];
byte yacc_avgs[ROLLING_AVG];
byte zacc_avgs[ROLLING_AVG];

byte xgyr_avgs[ROLLING_AVG];
byte ygyr_avgs[ROLLING_AVG];
byte zgyr_avgs[ROLLING_AVG];

long xacc_tot;
long yacc_tot;
long zacc_tot;

long xgyr_tot;
long ygyr_tot;
long zgyr_tot;

const int ACCEL_PIN = 5;
const int GYRO_PIN = 6; 
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

// Configured for + or - 2g
void setup() {
  pinMode(LED, OUTPUT);

  Serial.begin(11520);
  pinMode(ACCEL_PIN, OUTPUT);
  pinMode(GYRO_PIN, OUTPUT);
  digitalWrite(ACCEL_PIN, HIGH);
  digitalWrite(GYRO_PIN, HIGH);
  

  SPI.begin();
  SPI.beginTransaction(SPISettings(1000, MSBFIRST, SPI_MODE3));
//
//  digitalWrite(ACCEL_PIN, LOW);
//  SPI.transfer(0x20);
//  SPI.transfer(0x3F);
//  digitalWrite(ACCEL_PIN, HIGH);

  

  
  digitalWrite(GYRO_PIN, LOW);
  SPI.transfer(0x20);
  SPI.transfer(0xFF);
  digitalWrite(GYRO_PIN, HIGH);
  

  


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

//  digitalWrite(GYRO_PIN, LOW);
//  SPI.transfer(0x0F);
//  whoami = SPI.transfer(0x00);
//  digitalWrite(GYRO_PIN, HIGH);

  // Receive data
//  digitalWrite(ACCEL_PIN, LOW);
//  SPI.transfer(0xA9);
//  x_acc = SPI.transfer(0x00);
//  digitalWrite(ACCEL_PIN, HIGH);
//
//  digitalWrite(ACCEL_PIN, LOW);
//  SPI.transfer(0xAB);
//  y_acc = SPI.transfer(0x00);
//  digitalWrite(ACCEL_PIN, HIGH);
//
//  digitalWrite(ACCEL_PIN, LOW);
//  SPI.transfer(0xAD);
//  z_acc = SPI.transfer(0x00);
//  digitalWrite(ACCEL_PIN, HIGH);



  digitalWrite(GYRO_PIN, LOW);
  SPI.transfer(0xA9);
  x_gyr = SPI.transfer(0x00);
  digitalWrite(GYRO_PIN, HIGH);

  digitalWrite(GYRO_PIN, LOW);
  SPI.transfer(0xAB);
  y_gyr = SPI.transfer(0x00);
  digitalWrite(GYRO_PIN, HIGH);

  digitalWrite(GYRO_PIN, LOW);
  SPI.transfer(0xAD);
  z_gyr = SPI.transfer(0x00);
  digitalWrite(GYRO_PIN, HIGH);

  // Averaging
  xacc_avgs[datacount % ROLLING_AVG] = x_acc;
  yacc_avgs[datacount % ROLLING_AVG] = y_acc;
  zacc_avgs[datacount % ROLLING_AVG] = z_acc;

  xgyr_avgs[datacount % ROLLING_AVG] = x_gyr;
  ygyr_avgs[datacount % ROLLING_AVG] = y_gyr;
  zgyr_avgs[datacount % ROLLING_AVG] = z_gyr;
  

  xacc_tot = 0;
  yacc_tot = 0;
  zacc_tot = 0;

  xgyr_tot = 0;
  ygyr_tot = 0;
  zgyr_tot = 0;

  for (int i = 0; i < ROLLING_AVG; i++) {
    xacc_tot += xacc_avgs[i];
    yacc_tot += yacc_avgs[i];
    zacc_tot += zacc_avgs[i];

    xgyr_tot += xgyr_avgs[i];
    ygyr_tot += ygyr_avgs[i];
    zgyr_tot += zgyr_avgs[i];
  }

  x_senda = (byte)(xacc_tot / ROLLING_AVG);
  y_senda = (byte)(yacc_tot / ROLLING_AVG);
  z_senda = (byte)(zacc_tot / ROLLING_AVG);

  x_sendg = (byte)(xgyr_tot / ROLLING_AVG);
  y_sendg = (byte)(ygyr_tot / ROLLING_AVG);
  z_sendg = (byte)(zgyr_tot / ROLLING_AVG);
  

  datacount += 1;

  // Delta time
  unsigned long averageTime = 0;
  if (millis() - averageTime > 10) {
    averageTime = millis();
    CAN.beginPacket(0x1);
    CAN.write(x_senda);
    CAN.write(y_senda);
    CAN.write(z_senda);
    CAN.write(x_sendg);
    CAN.write(y_sendg);
    CAN.write(z_sendg);
    CAN.endPacket();
  }

  // Prints :)
  int x1 = (signed char)x_senda;
  int y1 = (signed char)y_senda;
  int z1 = (signed char)z_senda;
  int x2 = (signed char)x_sendg;
  int y2 = (signed char)y_sendg;
  int z2 = (signed char)z_sendg;
//  Serial.printf("X:%d\tY:%d\tZ:%d\n", x1, y1, z1);
  Serial.printf("X:%d\tY:%d\tZ:%d\n", x_gyr, y_gyr, z_gyr);
//    Serial.print(whoami);
//    Serial.print("\n");
    
}
