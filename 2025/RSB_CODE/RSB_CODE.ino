#include <Adafruit_NeoPixel.h>
#include <CAN.h>
#include <SPI.h>
#include <BMX160.h>
#include <math.h>

#include "RSB_CODE.h"
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
float accel[DIMENSIONS];
float gyro[DIMENSIONS];
float magn[DIMENSIONS];
float accel_out[DIMENSIONS];
float gyro_out[DIMENSIONS];
float magn_out[DIMENSIONS];
double average_matrix[29];
short avg_send[29];
//set rotation to be the identity matrix
float rotation[9] = {
    1, 0, 0,
    0, 1, 0,
    0, 0, 1,
};


#define BLINK_INTERVAL 1000
#define LEDPIN 8
#define ANLG_RES 4096
#define ANLG_VRANGE 5

// Delta Time Loop Setup
unsigned long blinkCurrentMillis = millis();
unsigned long blinkPreviousMillis = 0;
bool LEDState = LOW;
unsigned long current_millis = millis();
short g_scale;

// LED Setup
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LEDPIN, NEO_GRB + NEO_KHZ800);

// accel/gyro sensor definition
BMX160 bmx160;

float scale = 1000 * ANLG_VRANGE / float(ANLG_RES); // Added 1.342 to linearize with weird voltage drop



//SETTINGS
bool startingUp = true;
bool zmagcali = false;
bool manualcal = false;
float psi = 0.0f;


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

  //PRINTING OUT ACCEL VALUES TO SEE WHAT IS GOING ON
  // Serial.println("BEFORE");
  // Serial.print("A0: ");
  // Serial.println(accel[0]);
  // Serial.print("A1: ");
  // Serial.println(accel[1]);
  // Serial.print("A2: ");
  // Serial.println(accel[2]);



  if(startingUp){
    startingUp = false;
//    float roll = atan2f(accel[1],accel[2]);
//    float denom = sqrtf(accel[1]*accel[1]+accel[2]*accel[2]);
//    float pitch = atan2f(-accel[0],denom);
    float roll = 0.05f;
    float pitch = -0.90f;  
    
    float s_r = sinf(roll);
    float c_r = cosf(roll);
    float s_p = sinf(pitch);
    float c_p = cosf(pitch);

    rotation[0] = c_p;
    rotation[1] = s_p * s_r;
    rotation[2] = s_p * c_r;
    rotation[3] = 0.0f;
    rotation[4] = c_r;
    rotation[5] = -s_r;
    rotation[6] = -s_p;
    rotation[7] = c_p*s_r;
    rotation[8] = c_p*c_r;
    Serial.println("Rotation Matrix Calculated");
    if(zmagcali){
      float declination = 0.214;
      float m_x = rotation[0]*magn[0] + rotation[1]*magn[1] + rotation[2]*magn[2];
      float m_y = rotation[3]*magn[0] + rotation[4]*magn[1] + rotation[5]*magn[2];
      float psi = atan2f(m_y,m_x+declination);
      float c_y = cosf(-psi);
      float s_y = sinf(-psi);



      rotation[0] =  c_y * c_p;
      rotation[1] =  c_y * s_p * s_r + s_y * c_r;
      rotation[2] =  c_y * s_p * c_r - s_y * s_r;
      
      rotation[3] = -s_y * c_p;
      rotation[4] = -s_y * s_p * s_r + c_y * c_r;
      rotation[5] = -s_y * s_p * c_r - c_y * s_r;
      
      rotation[6] = -s_p;
      rotation[7] =  c_p * s_r;
      rotation[8] =  c_p * c_r;
      
    }
    if(manualcal){
      float s_z = sinf(psi);
      float c_z = cosf(psi);
      float Rz[9] = {
        c_z, -s_z, 0.0f,
        s_z,  c_z, 0.0f,
        0.0f, 0.0f, 1.0f
      };
      // multiply: newRot = Rz * rotation
      float newRot[9];
      for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < 3; ++j) {
          // newRot[i][j] = sum_k Rz[i][k] * rotation[k][j]
          newRot[3*i + j] =
            Rz[3*i + 0]*rotation[0*3 + j] +
            Rz[3*i + 1]*rotation[1*3 + j] +
            Rz[3*i + 2]*rotation[2*3 + j];
        }
      }

      // copy back into rotation[]
      memcpy(rotation, newRot, 9*sizeof(float));

    }

  }



  transform2(accel, rotation, accel_out, 1.0);
  transform2(gyro, rotation, gyro_out, 1.0);
  transform2(magn, rotation, magn_out, 1.0);

//
//  Serial.println("AFTER");


  //swap the x and y
  swapXYInPlace(accel_out);
  swapXYInPlace(gyro_out);
  swapXYInPlace(magn_out);

  makeineg(accel_out, 0);
  makeineg(gyro_out, 0);
  makeineg(magn_out, 0);

  makeineg(accel_out, 1);
  makeineg(gyro_out, 1);
  makeineg(magn_out, 1);

  // each column of average_matrix will accumulate the average value over 10 entries
  for (int i = 0; i < 20; i++)
  {
    average_matrix[i] += analogs[i];
  }

  // Accelerometer on SB data
  for (int i = 0; i < 3; i++)
  {
    average_matrix[i + 20] += (double)accel_out[i];
    average_matrix[i + 23] += (double)gyro_out[i];
    average_matrix[i + 26] += (double)magn_out[i];
  }

  

  // Average out the matrices used and convert to short for CAN
  if (datacount >= NUM_SAMPLES)
  {
    for (int i = 0; i < 29; i++)
    {
      average_matrix[i] = average_matrix[i] / (float)NUM_SAMPLES;
      avg_send[i] = (short)average_matrix[i];
    }

    Serial.print("X:");
    Serial.print(avg_send[20]);
    Serial.print('\t');
    Serial.print("Y:");
    Serial.print(avg_send[21]);
    Serial.print('\t');
    Serial.print("Z:");
    Serial.println(avg_send[22]);

    // Send CAN Frame
    canShortFrame(avg_send, 0, 0x20);
    canShortFrame(avg_send, 4, 0x21);
    canShortFrame(avg_send, 8, 0x22);
    canShortFrame(avg_send, 12, 0x23);
    canShortFrame(avg_send, 16, 0x24);
    canShortFrame(avg_send, 20, 0x25);
    canShortFrame(avg_send, 24, 0x26);

    CAN.beginPacket(0x27);
    canWriteShort(avg_send[28]);
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
void accel_update(float *accel, sBmx160SensorData_t Oaccel)
{
  accel[0] = Oaccel.x * 100;
  accel[1] = Oaccel.y * 100;
  accel[2] = Oaccel.z * 100;
}

void transform2(float *inp, float *rotation, float* out, float g_scale){
  out[0] = rotation[0] * inp[0] + rotation[1] * inp[1] + rotation[2]*inp[2];
  out[1] = rotation[3] * inp[0] + rotation[4] * inp[1] + rotation[5]*inp[2];
  out[2] = rotation[6] * inp[0] + rotation[7] * inp[1] + rotation[8]*inp[2];

  //scale by g_scale
  for(int i=0; i<2; ++i){
    out[i] = g_scale*out[i];
  }
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

  unsigned short reading = analogRead(MUX_OUT);
  digitalWrite(EN, LOW);
  return reading;
}


// Swaps the X and Y components of the vector in place.
// vec[0]=X, vec[1]=Y, vec[2]=Z
void swapXYInPlace(float vec[3]) {
  float tmp   = vec[0];
  vec[0]       = vec[1];
  vec[1]       = tmp;
}


void makeineg(float vec[3], unsigned int i) {
  vec[i] = -1.0 * vec[i];
}
