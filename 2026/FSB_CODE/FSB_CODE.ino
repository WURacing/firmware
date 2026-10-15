#include <Adafruit_NeoPixel.h>
#include <CANSAME5x.h>
#include <BMX160.h>
#include <math.h>
#include <Wire.h>

// I2C
#include <Adafruit_VL6180X.h>
#include <SPI.h>

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

// VCSEL definitions
#define SENSOR_ADDRESS      0x29
#define MAX_CONVERGENCE_MS  60      // <-- tested from protoboard
#define VCSEL_TIMEOUT_MS    100      // abandon a measurement cycle after this long
#define VCSEL_STALE_MS      200
#define VCSEL_CH_FR         1       // <-- mux channel the FR sensor is plugged into
#define VCSEL_CH_FL         2       // <-- mux channel the FL sensor is plugged into
#define VCSEL_COUNT         2      // 

// IMU definitions
#define IMU_MUX_CH          0       // IMU (SEN0373) is on SCL0/SDA0 = I2C mux channel 0

// I2C definitions
#define REG_MODEL_ID          0x0000   // expect 0xB4
#define REG_SYSRANGE_START    0x0018
#define REG_MAX_CONVERGENCE   0x001C
#define REG_INT_CLEAR         0x0015
#define REG_RANGE_STATUS      0x004D
#define REG_INT_STATUS        0x004F
#define REG_RANGE_VAL         0x0062
#define VL6180X_MODEL_ID      0xB4

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
short avg_send[32];
//set rotation to be the identity matrix
float rotation[9] = {
    1, 0, 0,
    0, 1, 0,
    0, 0, 1,
};

const uint32_t I2C_TIMEOUT_MS = 25;
Adafruit_VL6180X vl;
uint8_t tcaAddr = 0;             // 0 = mux not found
const uint8_t vcselCh[VCSEL_COUNT] = {VCSEL_CH_FR, VCSEL_CH_FL};
bool     vcselOk[VCSEL_COUNT]      = {};
bool     vcselPending[VCSEL_COUNT] = {};
uint32_t vcselLast[VCSEL_COUNT]     = {0xFFFF, 0xFFFF};
uint32_t  vcselLastMs[VCSEL_COUNT]   = {};
bool     vcselCycleActive = false;
uint32_t vcselCycleStart  = 0;

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

Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LEDPIN, NEO_GRB + NEO_KHZ800);

BMX160 bmx160;
bool imuOk = false;              // set by initImu(); IMU data is zero when false

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

  delay(4000);

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
  // I2C mux address pins: A0 = D4 (also PIN_CAN_BOOSTEN, kept HIGH for CAN),
  // A1 = D5, A2 = D6. Drive A1/A2 so the mux address can't float mid-run.
  pinMode(5, OUTPUT); digitalWrite(5, LOW);
  pinMode(6, OUTPUT); digitalWrite(6, LOW);
  delay(5);

  Wire.begin();               // make sure SERCOM is alive
  findMux();
  initImu();
  initVcsels();
  Wire.setClock(400000);      // bmx160.begin()/vl.begin() re-run Wire.begin(), which resets to 100 kHz

  selectChecked(IMU_MUX_CH);
  while (1) {
    Wire.beginTransmission(0x68);
    Wire.endTransmission();
  }

// Serial.println("Scanning I2C...");
// int found = 0;

// for (uint8_t address = 1; address < 127; address++) {
//   Wire.beginTransmission(address);
//   uint8_t error = Wire.endTransmission();

//   if (error == 0) {
//     Serial.print("Device found at 0x");
//     if (address < 16) Serial.print("0");
//     Serial.println(address, HEX);
//     found++;
//   }
// }

// Serial.print("Devices found: ");
// Serial.println(found);
  // Accel error message
  // if (bmx160.begin() != true)
  // {
  //   Serial.println("init false");
  //   while (1)
  //     ;
  // }
  // else
  // {
  //   Serial.println("init true");
    
  // }
  // current_millis = millis();
}

void loop()
{ 
  ++datacount;
  // Wait for the next sample interval
  while (millis() - current_millis < SAMPLE_INTERVAL)
  {
  }
  current_millis = millis();

  // // Oscillate through all 16 channels of the multiplexer
  // mux_update(analogs);

  // // Add in the extra 4 analog channels
  // readAnalogsMan(analogs);

  // Taking in Accel, Gyro, Magnetometer results
  // sBmx160SensorData_t Omagn, Ogyro, Oaccel;

  // IMU read: the IMU is behind the I2C mux, so select its channel first.
  // Must run before runVcsels(), which switches mux channels.
  sBmx160SensorData_t Omagn = {}, Ogyro = {}, Oaccel = {};
  if (imuOk && selectChecked(IMU_MUX_CH))
  {
    bmx160.getAllData(&Omagn, &Ogyro, &Oaccel);
    deselectAll();
  }

  // updating accel, gyro, magn arrays (x100 to match the DBC's 0.01 scale)
  imu_update(accel, Oaccel, 98.0f);    // m/s^2 x100 (x0.98 undoes the library's g = 10 at 4 g range)
  imu_update(gyro,  Ogyro,  100.0f);   // deg/s x100
  imu_update(magn,  Omagn,  100.0f);   // uT x100

  //PRINTING OUT ACCEL VALUES TO SEE WHAT IS GOING ON
  // Serial.println("BEFORE");
  // Serial.print("A0: ");
  // Serial.println(accel[0]);
  // Serial.print("A1: ");
  // Serial.println(accel[1]);
  // Serial.print("A2: ");
  // Serial.println(accel[2]);


  // ACCEL/IMU CODE COMMENTED
  // if(startingUp){
  //   startingUp = false;
  //  float roll = atan2f(accel[1],accel[2]);
  //  float denom = sqrtf(accel[1]*accel[1]+accel[2]*accel[2]);
  //  float pitch = atan2f(-accel[0],denom);
  //   float roll = 0.35f+PI;
  //   float pitch = 0.05f;  
    
  //   float s_r = sinf(roll);
  //   float c_r = cosf(roll);
  //   float s_p = sinf(pitch);
  //   float c_p = cosf(pitch);

  //   rotation[0] = c_p;
  //   rotation[1] = s_p * s_r;
  //   rotation[2] = s_p * c_r;
  //   rotation[3] = 0.0f;
  //   rotation[4] = c_r;
  //   rotation[5] = -s_r;
  //   rotation[6] = -s_p;
  //   rotation[7] = c_p*s_r;
  //   rotation[8] = c_p*c_r;
  //   Serial.println("Rotation Matrix Calculated");
  //   if(zmagcali){
  //     float declination = 0.214;
  //     float m_x = rotation[0]*magn[0] + rotation[1]*magn[1] + rotation[2]*magn[2];
  //     float m_y = rotation[3]*magn[0] + rotation[4]*magn[1] + rotation[5]*magn[2];
  //     float psi = atan2f(m_y,m_x+declination);
  //     float c_y = cosf(-psi);
  //     float s_y = sinf(-psi);



  //     rotation[0] =  c_y * c_p;
  //     rotation[1] =  c_y * s_p * s_r + s_y * c_r;
  //     rotation[2] =  c_y * s_p * c_r - s_y * s_r;
      
  //     rotation[3] = -s_y * c_p;
  //     rotation[4] = -s_y * s_p * s_r + c_y * c_r;
  //     rotation[5] = -s_y * s_p * c_r - c_y * s_r;
      
  //     rotation[6] = -s_p;
  //     rotation[7] =  c_p * s_r;
  //     rotation[8] =  c_p * c_r;
      
  //   }
  //   if(manualcal){
  //     float s_z = sinf(psi);
  //     float c_z = cosf(psi);
  //     float Rz[9] = {
  //       c_z, -s_z, 0.0f,
  //       s_z,  c_z, 0.0f,
  //       0.0f, 0.0f, 1.0f
  //     };
  //     // multiply: newRot = Rz * rotation
  //     float newRot[9];
  //     for(int i = 0; i < 3; ++i) {
  //       for(int j = 0; j < 3; ++j) {
  //         // newRot[i][j] = sum_k Rz[i][k] * rotation[k][j]
  //         newRot[3*i + j] =
  //           Rz[3*i + 0]*rotation[0*3 + j] +
  //           Rz[3*i + 1]*rotation[1*3 + j] +
  //           Rz[3*i + 2]*rotation[2*3 + j];
  //       }
  //     }

  //     // copy back into rotation[]
  //     memcpy(rotation, newRot, 9*sizeof(float));

  //   }

  // }



  // rotation[] is the identity until the mounting calibration is re-done,
  // so this sends the IMU's own axes.
  transform2(accel, rotation, accel_out, 1.0);
  transform2(gyro, rotation, gyro_out, 1.0);
  transform2(magn, rotation, magn_out, 1.0);

//
//  Serial.println("AFTER");


  //swap the x and y
  // swapXYInPlace(accel_out);
  // swapXYInPlace(gyro_out);
  // swapXYInPlace(magn_out);



  // each column of average_matrix will accumulate the average value over 10 entries
  // for (int i = 0; i < 20; i++)
  // {
  //   average_matrix[i] += analogs[i];
  // }

  // Accelerometer on SB data
  for (int i = 0; i < 3; i++)
  {
    average_matrix[i + 20] += (double)accel_out[i];
    average_matrix[i + 23] += (double)gyro_out[i];
    average_matrix[i + 26] += (double)magn_out[i];
  }

  runVcsels();

  // Average out the matrices used and convert to short for CAN
  if (datacount >= NUM_SAMPLES) 
  {
    for (int i = 0; i < 29; i++)
    {
      average_matrix[i] = average_matrix[i] / (float)NUM_SAMPLES;
      avg_send[i] = (short)average_matrix[i];
    }
    // IMU CODE COMMENTED
    // Serial.print("X:");
    // Serial.print(avg_send[20]);
    // Serial.print('\t');
    // Serial.print("Y:");
    // Serial.print(avg_send[21]);
    // Serial.print('\t');
    // Serial.print("Z:");
    // Serial.println(avg_send[22]);

    // Adding VCSELs to CAN
    // This sends the average of all readings since CAN frames were last sent
    // If there are no valid readings, it sends 0xFFFF
    for (uint8_t i = 0; i < VCSEL_COUNT; i++) {
      bool fresh = (millis() - vcselLastMs[i]) < VCSEL_STALE_MS;
      avg_send[29 + i] = fresh ? (short)vcselLast[i] : (short)0xFFFF;
      Serial.print("VCSEL: ");
      Serial.print(vcselLast[i]);
      Serial.print(" VCSEL: ");
      Serial.println(i);
    }
    avg_send[31] = 0;

    // Send CAN Frame
    canShortFrame(avg_send, 0, 0x10);
    canShortFrame(avg_send, 4, 0x11);
    canShortFrame(avg_send, 8, 0x12);
    canShortFrame(avg_send, 12, 0x13);
    canShortFrame(avg_send, 16, 0x14);
    canShortFrame(avg_send, 20, 0x15);
    canShortFrame(avg_send, 24, 0x16);
    canShortFrame(avg_send, 28, 0x17);

    // clear the avg_send and average_matrix arrays of all previous values
    for (int i = 0; i < 29; i++) {
      average_matrix[i] = 0;
    }

    for (int i=0; i<32; i++) {
      avg_send[i] = 0;
    }

    datacount = 0;
  }

// Oscillate through all mux values and add to matrix
// void mux_update(short *analogs)
// {
//   unsigned short data;
//   for (byte i = 0; i < 16; i++)
//   {
//     data = mux(i);
//     analogs[i] = data * scale;
//     printDebug("Channel: ");
//     printDebug(i);
//     printDebug(" Data: ");
//     printDebug(analogs[i]);
//     printDebug("\t");
//   }
//   // Pin 27 -> A11 -> S12
//   printDebug('\n');
// }
}

// Copy one BMX160 reading into a 3-element array, scaled for CAN
void imu_update(float *out, sBmx160SensorData_t d, float scale)
{
  out[0] = d.x * scale;
  out[1] = d.y * scale;
  out[2] = d.z * scale;
}

// Rotate a vector into the car frame: out = rotation * inp, then scale
void transform2(float *inp, float *rotation, float *out, float g_scale)
{
  out[0] = rotation[0] * inp[0] + rotation[1] * inp[1] + rotation[2] * inp[2];
  out[1] = rotation[3] * inp[0] + rotation[4] * inp[1] + rotation[5] * inp[2];
  out[2] = rotation[6] * inp[0] + rotation[7] * inp[1] + rotation[8] * inp[2];

  for (int i = 0; i < 3; ++i)   // was i < 2, which skipped z
  {
    out[i] = g_scale * out[i];
  }
}

// ---------------------------
//      Helper Functions
// ---------------------------
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
    // Serial.print("Wrote: ");
    // Serial.println(j);
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

/*
  I2C and VCSEL helper functions
*/

// Find which pin the I2C mux is connected to and use it 
bool findMux() {
  for (uint8_t addr = 0x70; addr <= 0x77; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      tcaAddr = addr;
      Serial.print("Mux found at 0x");
      Serial.println(addr, HEX);
      return true;
    }
  }
  tcaAddr = 0;
  Serial.println("FAIL: mux not found at 0x70-0x77 (check SDA/SCL pullups, power, RESET)");
  return false;
}

bool selectChecked(uint8_t channel) {
  if (tcaAddr == 0 || channel > 7) return false;
  Wire.beginTransmission(tcaAddr);
  Wire.write(uint8_t(1 << channel));
  return Wire.endTransmission() == 0;
}

void deselectAll() {
  if (tcaAddr == 0) return;
  Wire.beginTransmission(tcaAddr);
  Wire.write(uint8_t(0));
  Wire.endTransmission();
}

bool readReg(uint16_t reg, uint8_t &value) {
  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(uint8_t(reg >> 8)); Wire.write(uint8_t(reg));
  if (Wire.endTransmission(false)) return false;
  if (Wire.requestFrom(SENSOR_ADDRESS, uint8_t(1)) != 1) return false;
  value = Wire.read();
  return true;
}

bool writeReg(uint16_t reg, uint8_t value) {
  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(uint8_t(reg >> 8)); Wire.write(uint8_t(reg)); Wire.write(value);
  return Wire.endTransmission() == 0;
}

// The IMU is on mux channel 0, so begin() only sees it with that channel selected.
// On failure the board keeps running (CAN still sends) instead of hanging.
void initImu() {
    for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { Serial.print("CH0 device at 0x"); Serial.println(a, HEX); }
  }

  if (!selectChecked(IMU_MUX_CH)) { Serial.println("IMU: mux select failed"); return; }
  if (!bmx160.begin()) {
    Serial.println("IMU: no ACK at 0x68 on mux CH0");
    deselectAll();
    return;
  }
  bmx160.setGyroRange(eGyroRange_250DPS);   // chip resets to +/-2000 dps; library scales for 250 unless told
  bmx160.setAccelRange(eAccelRange_4G);     // default +/-2 g clips under braking/bumps
  imuOk = true;
  Serial.println("IMU: OK");
  deselectAll();
}

void initVcsels() {
  if (!findMux()) { Serial.println("VCSEL: mux not found"); return; }
  Serial.print("VCSEL: mux at 0x"); Serial.println(tcaAddr, HEX);
  for (uint8_t i = 0; i < VCSEL_COUNT; i++) {
    uint8_t id = 0;
    vcselOk[i] = selectChecked(vcselCh[i])
              && readReg(REG_MODEL_ID, id) && id == VL6180X_MODEL_ID
              && vl.begin(&Wire)                       // only after a good ID probe
              && writeReg(REG_MAX_CONVERGENCE, MAX_CONVERGENCE_MS)
              && writeReg(REG_INT_CLEAR, 0x07);
    Serial.print("VCSEL CH"); Serial.print(vcselCh[i]);
    Serial.println(vcselOk[i] ? " OK" : " FAIL");
  }
  // TEMP: what's on every mux channel?
  for (uint8_t ch = 0; ch < 8; ch++) {
    Serial.print("scan CH"); Serial.print(ch); Serial.print(": ");
    if (!selectChecked(ch)) { Serial.println("mux select failed"); continue; }
    Wire.beginTransmission(SENSOR_ADDRESS);
    if (Wire.endTransmission() != 0) { Serial.println("nothing at 0x29"); continue; }
    uint8_t id = 0;
    if (!readReg(REG_MODEL_ID, id)) { Serial.println("ACKs, but register read failed"); continue; }
    Serial.print("model ID 0x"); Serial.println(id, HEX);   // B4 = good
  }
  deselectAll();
}

void runVcsels() {
  if (tcaAddr == 0) return;
  uint32_t now = millis();

  if (!vcselCycleActive) {
    for (uint8_t i = 0; i < VCSEL_COUNT; i++) {
      uint8_t st;
      vcselPending[i] = vcselOk[i]
                     && selectChecked(vcselCh[i])
                     && readReg(REG_RANGE_STATUS, st) && (st & 0x01)   // device ready
                     && writeReg(REG_INT_CLEAR, 0x07)                  // clear any stale result
                     && writeReg(REG_SYSRANGE_START, 0x01);
    }
    vcselCycleActive = true;
    vcselCycleStart = now;
    deselectAll();
    return;
  }

  bool allDone = true;
  for (uint8_t i = 0; i < VCSEL_COUNT; i++) {
    if (!vcselPending[i]) continue;
    uint8_t intStatus;
    if (!selectChecked(vcselCh[i]) || !readReg(REG_INT_STATUS, intStatus)) {
      vcselPending[i] = false;               // I2C hiccup: drop this sample
      continue;
    }
    if (!(intStatus & 0x04)) { allDone = false; continue; }   // still ranging

    uint8_t mm, st;
    bool ok = readReg(REG_RANGE_VAL, mm) && readReg(REG_RANGE_STATUS, st);
    writeReg(REG_INT_CLEAR, 0x07);
    if (ok && (st >> 4) == 0) {              // error code 0 = valid range
      vcselLast[i] = mm;
      vcselLastMs[i] = now;
    }
    vcselPending[i] = false;
  }
  deselectAll();

  if (allDone || now - vcselCycleStart > VCSEL_TIMEOUT_MS) vcselCycleActive = false;
}

void recoverI2CBus() {
  const uint8_t SCL_PIN = 21;   // Feather M4 CAN → SCL = PA13
  pinMode(SCL_PIN, OUTPUT);
  for (int i = 0; i < 9; i++) {           // free the slave
    digitalWrite(SCL_PIN, HIGH); delayMicroseconds(5);
    digitalWrite(SCL_PIN, LOW);  delayMicroseconds(5);
  }
  pinMode(SCL_PIN, INPUT_PULLUP);         // release SCL
  Wire.end();
  Wire.begin();                           // re‑init SERCOM
}
