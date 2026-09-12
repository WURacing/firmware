#include <Wire.h>
#include <Adafruit_VL6180X.h>
#include <DFRobot_BMX160.h>
// #include 'GoblinMode.h'

#define VL6180X_ADDR 0x29
#define TCA9548_ADDR 0x70
#define BMX160_ADDR 0x68
#define LEFT_VCSEL_CHANNEL 0 // Configure these based on wiring and pin layout
#define RIGHT_VCSEL_CHANNEL 1

DFRobot_BMX160 bmx160;
Adafruit_VL6180X vl_left = Adafruit_VL6180X();
Adafruit_VL6180X vl_right = Adafruit_VL6180X();

void tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(TCA9548_ADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}

void printRangeError(uint8_t status){
  switch (status) {
    case VL6180X_ERROR_SYSERR_1:
    case VL6180X_ERROR_SYSERR_5:
      Serial.println("System error");
      break;
    case VL6180X_ERROR_ECEFAIL:
      Serial.println("Early convergence estimate fail (no target?)");
      break;
    case VL6180X_ERROR_NOCONVERGE:
      Serial.println("No target detected");
      break;
    case VL6180X_ERROR_RAWUFLOW:
    case VL6180X_ERROR_RAWOFLOW:
      Serial.println("Raw range algorithm under/overflow");
      break;
    case VL6180X_ERROR_RANGEUFLOW:
    case VL6180X_ERROR_RANGEOFLOW:
      Serial.println("Range under/overflow (target too close/far)");
      break;
    default:
      Serial.print("Unknown status code: ");
      Serial.println(status);
      break;
  }
}

void setup() {
  // Begin serial port and I2C bus
  Serial.begin(115200);
  Wire.begin();
  delay(100);

  // Find I2C Switch
  Wire.beginTransmission(TCA9548_ADDR);
  if(!Wire.endTransmission()){
    Serial.println("Found I2C Switch");
  }
  else {
    Serial.println("I2C Switch not found, check wiring");
    while(1);
  }

  // Begin BMX160
  if(!bmx160.begin()){
    Serial.println("Failed to start BMX160");
    while(1);
  }

  // Find and begin both VCSELs
  tcaselect(LEFT_VCSEL_CHANNEL);
  if (!vl_left.begin()){
    Serial.println("Failed to find left VCSEL, check wiring");
    while(1);
  }

  tcaselect(RIGHT_VCSEL_CHANNEL);
  if(!vl_right.begin()){
    Serial.println("Failed to find right VCSEL, check wiring");
    while(1);
  }
}

void loop() {
  // Sample from BMX160
  sBmx160SensorData_t Omagn, Ogyro, Oaccel;
  bmx160.getAllData(&Omagn, &Ogyro, &Oaccel);

  // Magnetometer
  Serial.print("M X: "); Serial.print(Omagn.x);
  Serial.print(" Y: "); Serial.print(Omagn.y);
  Serial.print(" Z: "); Serial.print(Omagn.z);
  Serial.println(" uT");

  // Gyroscope
  Serial.print("G X: "); Serial.print(Ogyro.x);
  Serial.print(" Y: "); Serial.print(Ogyro.y);
  Serial.print(" Z: "); Serial.print(Ogyro.z);
  Serial.println(" g");

  // Accelerometer
  Serial.print("A X: "); Serial.print(Oaccel.x);
  Serial.print(" Y: "); Serial.print(Oaccel.y);
  Serial.print(" Z: "); Serial.print(Oaccel.z);
  Serial.println(" m/s^2");

  Serial.println("");

  // Sample from left VCSEL
  tcaselect(LEFT_VCSEL_CHANNEL);
  uint8_t range_left = vl_left.readRange();
  uint8_t status_left = vl_left.readRangeStatus();

  if (status_left == VL6180X_ERROR_NONE) {
    Serial.print("Range (left VCSEL): ");
    Serial.print(range_left);
    Serial.println(" mm");
  } else {
    Serial.print("Left VCSEL error - ");
    printRangeError(status_left);
  }

  // Sample from right VCSEL
  tcaselect(RIGHT_VCSEL_CHANNEL);
  uint8_t range_right = vl_right.readRange();
  uint8_t status_right = vl_right.readRangeStatus();

  if(status_right == VL6180X_ERROR_NONE) {
    Serial.print("Range (right VCSEL): ");
    Serial.print(range_right);
    Serial.println(" mm");
  } else {
    Serial.print("Right VCSEL error - ");
    printRangeError(status_right);
  }

  delay(500);
}
