#include <Adafruit_NeoPixel.h>
#include <CAN.h>
#include <SPI.h>
#include <BMX160.h>

#include "FSB_CODE.h"
// #include "GoblinMode.h"

// #define DEBUG
#ifdef DEBUG
#define printDebug(message) Serial.print(message)
#else
#define printDebug(message)
#endif

// Initializations
#define LEDPIN 8
#define LED 13
#define BLINK_INTERVAL 1000
#define BAUD_RATE 1000000
#define ANLG_RES 4096
#define SAMPLE_INTERVAL 4
#define NUM_SAMPLES 10
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

// Data storage mechanism
short analogs[20];
short accel[DIMENSIONS];
short gyro[DIMENSIONS];
short magn[DIMENSIONS];
double accel_out[DIMENSIONS];
double gyro_out[DIMENSIONS];
double magn_out[DIMENSIONS];
double average_matrix[29];

#define BLINK_INTERVAL 1000
#define LEDPIN 8
#define ANLG_RES 4096
#define ANLG_VRANGE 5

// Delta Time Loop Setup
unsigned long blinkCurrentMillis = millis();
unsigned long blinkPreviousMillis = 0;
bool LEDState = LOW;
unsigned long current_millis = millis();

// LED Setup
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LEDPIN, NEO_GRB + NEO_KHZ800);

// accel/gyro sensor definition
BMX160 bmx160;

float scale = 1000 * ANLG_VRANGE * 1.342 / float(ANLG_RES); // Added 1.342 to linearize with weird voltage drop

void setup()
{
  // Pin Definitions
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
  strip.begin();
  strip.clear();
  strip.setBrightness(BRIGHTNESS);
  strip.show();

  // CAN SETUP
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
    // Set the accelerometer range to 2G
    bmx160.setAccelRange(eAccelRange_2G);

    // Set the gyroscope range to 250DPS
    bmx160.setGyroRange(eGyroRange_250DPS);
  }
  current_millis = millis();
}

void loop()
{
  ++datacount;
  // Wait for the next sample interval
  while (millis() - current_millis < SAMPLE_INTERVAL)
  {
  }
  current_millis = millis();

  // Oscillate through all 16 channels of the multiplexer
  mux_update(analogs);

  // Add in the extra 4 analog channels
  readAnalogsMan(analogs);

  // Taking in Accel, Gyro, Magnetometer results
  sBmx160SensorData_t Omagn, Ogyro, Oaccel;

  bmx160.getAllData(&Omagn, &Ogyro, &Oaccel);

  // updating accel, gyro, magn arrays
  accel_update(accel, Oaccel);
  accel_update(gyro, Ogyro);
  accel_update(magn, Omagn);

  transform(accel, accel_out);
  transform(gyro, gyro_out);
  transform(magn, magn_out);

  // each column of average_matrix will accumulate the average value over 10 entries
  for (int i = 0; i < 20; i++)
  {
    average_matrix[i] += analogs[i];
  }

  // Accelerometer on SB data
  for (int i = 0; i < 3; i++)
  {
    average_matrix[i + 20] += accel_out[i];
    average_matrix[i + 23] += gyro_out[i];
    average_matrix[i + 26] += magn_out[i];
  }

  short avg_send[29];

  // Average out the matrices used and convert to short for CAN
  if (datacount >= NUM_SAMPLES)
  {
    for (int i = 0; i < 29; i++)
    {
      average_matrix[i] = average_matrix[i] / (float)NUM_SAMPLES;
      avg_send[i] = (short)average_matrix[i];
    }

    // Send CAN Frame
    canShortFrame(avg_send, 0, 0x10);
    canShortFrame(avg_send, 4, 0x11);
    canShortFrame(avg_send, 8, 0x12);
    canShortFrame(avg_send, 12, 0x13);
    canShortFrame(avg_send, 16, 0x14);
    canShortFrame(avg_send, 20, 0x15);
    canShortFrame(avg_send, 24, 0x16);

    CAN.beginPacket(0x17);
    CAN.endPacket();

    // clear the avg_send and average_matrix arrays of all previous values
    for (int i = 0; i < 29; i++)
    {
      avg_send[i] = 0;
      average_matrix[i] = 0;
    }
  }

  if (datacount >= NUM_SAMPLES)
  {
    datacount = 0;
  }
}
// Oscillate through all mux values and add to matrix
void mux_update(short *analogs)
{
  unsigned short data;
  for (byte i = 0; i < 16; i++)
  {
    data = mux(i);
    analogs[i] = data * scale;
    printDebug("Channel: ");
    printDebug(i);
    printDebug(" Data: ");
    printDebug(analogs[i]);
    printDebug("\t");
  }
  // Pin 27 -> A11 -> S12
  printDebug('\n');
}
// Add 3 dimensions of accel, gyro, magn to matrix
void accel_update(short *accel, sBmx160SensorData_t Oaccel)
{
  accel[0] = Oaccel.x * 100;
  accel[1] = Oaccel.y * 100;
  accel[2] = Oaccel.z * 100;
}

void transform(short *inp, double *out)
{
  // out[0] = (short)(0.61146138 * inp[0] + 0.09275305 * inp[1] + 0.76278828 * inp[2]);
  // out[1] = (short)(0.09275305 * inp[0] + 0.95878757 * inp[1] + -0.19093815 * inp[2]);
  // out[2] = (short)(-0.76278828 * inp[0] + 0.19093815 * inp[1] + 0.5882438 * inp[2]);

  double angle = 80;
  double c = cos(angle / 180.0 * 3.14);
  double s = sin(angle / 180.0 * 3.14);

  out[0] = (1 * inp[0] + 0 * inp[1] + 0 * inp[2]) / 40;
  out[1] = (0 * inp[0] + c * inp[1] + (-s) * inp[2]) / 40;
  out[2] = (0 * inp[0] + s * inp[1] + c * inp[2]) / 40;
}

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

// Read in the analog inputs not on the mux
void readAnalogsMan(short *analogs)
{
  analogs[16] = (analogRead(A0) / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
  analogs[17] = (analogRead(A1) / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
  analogs[18] = (analogRead(A2) / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
  analogs[19] = (analogRead(A3) / (float)ANLG_RES) * 1000 * ANLG_VRANGE;
  printDebug("Channel: ");
  printDebug(19);
  printDebug(" Data: ");
  printDebug(analogs[19]);
  printDebug("\t");
}

// Writing a short to CAN
void canWriteShort(short data)
{
  CAN.write(data & 0xFF);
  CAN.write(data >> 8);
}
// Writing a frame full of shorts to CAN
void canShortFrame(short *send, int i, int Hex)
{
  CAN.beginPacket(Hex);
  for (int j = i; j < i + 4; j++)
  {
    canWriteShort(send[j]);
  }
  CAN.endPacket();
}

// params:
// index: number between 0-15 on the mux
unsigned short mux(unsigned int index)
{
  digitalWrite(EN, HIGH);
  // set multiplexer pins
  if ((index & 0b0001) > 0)
  {
    digitalWrite(MUX_A0, HIGH);  }
  else
  {
    digitalWrite(MUX_A0, LOW);
  }

  if ((index & 0b0010) > 0)
  {
    digitalWrite(MUX_A1, HIGH);
  }
  else
  {
    digitalWrite(MUX_A1, LOW);
  }

  if ((index & 0b0100) > 0)
  {
    digitalWrite(MUX_A2, HIGH);
  }
  else
  {
    digitalWrite(MUX_A2, LOW);
  }

  if ((index & 0b1000) > 0)
  {
    digitalWrite(MUX_A3, HIGH);
  }
  else
  {
    digitalWrite(MUX_A3, LOW);
  }

  unsigned short reading = analogRead(MUX_OUT);
  digitalWrite(EN, LOW);
  return reading;
}
