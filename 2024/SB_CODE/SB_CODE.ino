#include <Adafruit_NeoPixel.h>
#include <CAN.h>
#include <SPI.h>
#include <DFRobot_BMX160.h>

#include "SB_CODE.h"
// #include "GoblinMode.h"

#define LEDPIN 8
#define LED 13
#define BLINK_INTERVAL 1000
#define BAUD_RATE 1000000
#define ANLG_RES 4096
#define ANLG_VRANGE 3.3
#define CAN_INTERVAL 1
#define EN 9
#define MUX_A0 10
#define MUX_A1 11
#define MUX_A2 12
#define MUX_A3 13
#define MUX_OUT A4

#define BIT_RESOLUTION 12
#define BRIGHTNESS 20
#define DIMENSIONS 3

unsigned long datacount = 0;

short analogs[20];
short accel[DIMENSIONS];
short gyro[DIMENSIONS];
short magn[DIMENSIONS];
double average_matrix[29];

#define BLINK_INTERVAL 1000
#define LEDPIN 8
#define ANLG_RES 4096
#define ANLG_VRANGE 3.3

unsigned long blinkCurrentMillis = millis();
unsigned long blinkPreviousMillis = 0;
unsigned long canCurrentMillis = millis();
unsigned long canPreviousMillis = 0;
bool LEDState = LOW;
int test = 0;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LEDPIN, NEO_GRB + NEO_KHZ800);

// accel/gyro sensor definition
DFRobot_BMX160 bmx160;

// to make using the mux space easier
// using namespace admux;

void setup()
{
  // Pin Definitions
  // Serial.println("setup");
  pinMode(PIN_NEOPIXEL_POWER, OUTPUT);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(EN, OUTPUT);
  pinMode(MUX_A0, OUTPUT);
  pinMode(MUX_A1, OUTPUT);
  pinMode(MUX_A2, OUTPUT);
  pinMode(MUX_A3, OUTPUT);
  pinMode(MUX_OUT, INPUT);
  analogReadResolution(BIT_RESOLUTION); // for our board --> bit resolution

  Serial.begin(9600);

  // LED SETUP
  // Serial.println("led");
  strip.begin();
  strip.clear();
  strip.setBrightness(BRIGHTNESS);
  strip.show();

  // CAN SETUP
  // Serial.println("CAN");
  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false);
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true);

  // CAN error message
  if (!CAN.begin(BAUD_RATE))
  {
    Serial.println("Starting CAN failed!");
  }

  // Accel error message
  if (bmx160.begin() != true)
  {
    Serial.println("init false");
    while (1)
      ;
  }
  else
  {
    Serial.println("init true");
  }
}

void loop()
{
  delay(100);
  // inputs are R,G,B
  // Serial.println("blink");
  // blink(200, 0, 100, strip);
  // blink();

  // MUX Update
  // Serial.println("mux update");
  mux_update(analogs);

  // Manual Data (The hard wired Analog Inputs)
  // Serial.println("read analogs");
  readAnalogsMan(analogs);

  // New Sensor Event
  // Taking in Accel, Gyro, Magnetometer results
  sBmx160SensorData_t Omagn, Ogyro, Oaccel;
  // these are all objects with x,y,z attributes
  // we source this from the attached library

  // Serial.println("get imu");
  bmx160.getAllData(&Omagn, &Ogyro, &Oaccel);

  // updating accel, gyro, magn arrays
  // Serial.println("update imu");
  accel_update(accel, Oaccel);
  accel_update(gyro, Ogyro);
  accel_update(magn, Omagn);

  // Serial.println(accel[0]);

  // 9 Sensor Values taken in here (3 seperate arrays)

  // each column of average_matrix will accumulate the average value over 10 entries

  // Serial.println("averaging");
  for (int i = 0; i < 20; i++)
  {
    average_matrix[i] += analogs[i];
  }

  for (int i = 0; i < 3; i++)
  {
    // Serial.println("imu data:");
    average_matrix[i + 20] += accel[i];
    average_matrix[i + 23] += gyro[i];
    average_matrix[i + 26] += magn[i];
  }

  short avg_send[29];

  // Serial.println("data");
  if (datacount % 10)
  {
    // Serial.println("asdf");
    // divide by averaging sampling size
    // convert to short for CAN frame
    for (int i = 0; i < 29; i++)
    {
      average_matrix[i] = average_matrix[i] / 10.0;
      avg_send[i] = (short)average_matrix[i];
      // Serial.print(i);
      // Serial.print(": ");
      // Serial.println(avg_send[20]);
    }
    // Serial.println("asdf2");

    // clear average_matrix for next summation. All other arrays don't have to be cleared since they are set using '=' declarations and not '+='

    // Send CAN Frame
    canShortFrame(avg_send, 0, 0x10);
    canShortFrame(avg_send, 4, 0x11);
    canShortFrame(avg_send, 8, 0x12);
    canShortFrame(avg_send, 12, 0x13);
    canShortFrame(avg_send, 16, 0x14);
    canShortFrame(avg_send, 20, 0x15);
    canShortFrame(avg_send, 24, 0x16);

    CAN.beginPacket(0x17);
    canWriteShort(test++);
    // Serial.println(test);
    if (test > 65536)
    {
      test = 0;
    }
    CAN.endPacket();

    // clear the avg_send and average_matrix arrays
    // Serial.println("asdf3");
    for (int i = 0; i < 29; i++)
    {
      avg_send[i] = 0;
      average_matrix[i] = 0;
    }
    // Serial.println("asdf4");
  }

  ++datacount;
}

void mux_update(short *analogs)
{
  int data;
  for (byte i = 0; i < 16; i++)
  {
    data = mux(12);
    analogs[12] = (data / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
    Serial.print("Channel: ");
    Serial.print(12);
    Serial.print(" Data: ");
    Serial.print(analogs[12]);
    Serial.print("\t");
    delay(4);
    // digitalWrite(EN, LOW);
  }
  // Pin 27 -> A11 -> S12
  Serial.println();
}

void accel_update(short *accel, sBmx160SensorData_t Oaccel)
{
  accel[0] = Oaccel.x * 100;
  accel[1] = Oaccel.y * 100;
  accel[2] = Oaccel.z * 100;
}

// void blink(int r, int g, int b, Adafruit_NeoPixel &strip)
// {
//   blinkCurrentMillis = millis();
//   if (blinkCurrentMillis - blinkPreviousMillis >= BLINK_INTERVAL)
//   {
//     blinkPreviousMillis = blinkCurrentMillis;
//     if (LEDState == LOW)
//     {
//       Serial.println("high");
//       LEDState = HIGH;
//       strip.setPixelColor(0, r, g, b);
//       strip.show();
//     }
//     else
//     {
//       Serial.println("low");
//       LEDState = LOW;
//       strip.clear();
//       strip.show();
//     }
//   }
// }

void blink()
{
  blinkCurrentMillis = millis();
  if (blinkCurrentMillis - blinkPreviousMillis >= BLINK_INTERVAL)
  {
    blinkPreviousMillis = blinkCurrentMillis;
    if (LEDState == LOW)
    {
      LEDState = HIGH;
    }
    else
    {
      LEDState = LOW;
    }
    digitalWrite(LED, LEDState);
  }
}

void readAnalogsMan(short *analogs)
{
  analogs[16] = (analogRead(A0) / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
  analogs[17] = (analogRead(A1) / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
  analogs[18] = (analogRead(A2) / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
  analogs[19] = (analogRead(A3) / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
}

void canWriteShort(short data)
{
  CAN.write(data & 0xFF);
  CAN.write(data >> 8);
}

void canShortFrame(short *send, int i, int Hex)
{
  CAN.beginPacket(Hex);
  for (int j = i; j < i + 4; j++)
  {
    canWriteShort(send[j]);
    // Serial.println(send[i]);
  }
  CAN.endPacket();
}

// params:
// index: number between 0-15 (DOUBLE CHECK)
float mux(unsigned int index)
{
  digitalWrite(EN, HIGH);
  // Serial.println(index, HEX);
  // set multiplexer pins
  if ((index & 0b0001) > 0)
  {
    digitalWrite(MUX_A0, HIGH);
    //   Serial.println("A0 high");
  }
  else
  {
    digitalWrite(MUX_A0, LOW);
  }

  if ((index & 0b0010) > 0)
  {
    digitalWrite(MUX_A1, HIGH);
    // Serial.println("A1 high");
  }
  else
  {
    digitalWrite(MUX_A1, LOW);
  }

  if ((index & 0b0100) > 0)
  {
    digitalWrite(MUX_A2, HIGH);
    // Serial.println("A2 high");
  }
  else
  {
    digitalWrite(MUX_A2, LOW);
  }

  if ((index & 0b1000) > 0)
  {
    digitalWrite(MUX_A3, HIGH);
    // Serial.println("A3 high");
  }
  else
  {
    digitalWrite(MUX_A3, LOW);
  }

  delay(1);
  digitalWrite(EN, LOW);
  // float voltage = analogRead(MUX_OUT_FB);
  //  voltage divider equation
  // voltage = (R1 + R2) * voltage / R2;
  return analogRead(MUX_OUT);
}