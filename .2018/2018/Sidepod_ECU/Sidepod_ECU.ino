//#include <Wire.h>
#include <SoftwareSerial.h>
//#include <Adafruit_GPS.h>
#include <mcp_can.h>
//#include <Adafruit_MMA8451.h>
//#include <Adafruit_Sensor.h>
#include <SD.h>

#include "CANBus18.h"
#include "PacketSender.h"

#define CAN0_INT 4
MCP_CAN CAN0(10);

const int sd_select = 7;

const int rxPinXBee = A3;
const int txPinXBee = A4;

const int INNER_PRESSURE = A0;
const int acc1Pin = A1;
const int acc2Pin = A2;
const int acc3Pin = A5;

SoftwareSerial XBee_Serial(rxPinXBee,txPinXBee);
PacketSender XBee(XBee_Serial, 9600);

File logfile;

const int deltaTime = 4000;
unsigned long accumulator = 0;

/*double rpm;           //key: 0x30
double load;          //key: 0x31
double throttle;      //key: 0x32
double coolantF;      //key: 0x33
double o2;            //key: 0x34
double vehicleSpeed;  //key: 0x35
byte gear;            //key: 0x36
double volts; //key: 0x37
float DispFB; // 0x40
float DispBB; // 0x41*/

//intake: 44
//ingition: 45
//seat acceleration x: 46
//y: 47
//z: 48

//MAP: 52
//Inner MAP: 53

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];

void setup() {
  //Serial.begin(9600);
  // put your setup code here, to run once:
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
      Serial.println("MCP2515 Initialized Successfully!");
  }
  else {
      Serial.println("Error Initializing MCP2515...");
  }
  CAN0.setMode(MCP_NORMAL); // send acknowledgements to recieved data
  pinMode(CAN0_INT, INPUT);

  delay(500);
  
  /*OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);*/

  delay(500);

  /*if (mma.begin()) {
    mmaWorking = true;
  }
  else {
    Serial.println("Couldnt start");
    mmaWorking = false;
  }*/
  
  //Serial.println("MMA8451 found!");
  
  //mma.setRange(MMA8451_RANGE_8_G);

  delay(500);

  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(sd_select, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(sd_select)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  char filename[] = "LOG000.CSV";
  for (uint16_t i = 0; i < 1000; i++) {
    filename[3] = i / 100 + '0';
    filename[4] = (i%100)/10 + '0';
    filename[5] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      Serial.println(filename);
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
  }
  
  if (! logfile) {
    Serial.println("couldnt create file");
  }

  delay(500);
}

void loop() {

  /*if (millis() > accumulator && mmaWorking){
    mma.read();
    sensors_event_t event;
    mma.getEvent(&event);

    payload outgoing;

    float accX = -1.0 * event.acceleration.x;
    outgoing.floatData = accX;
    XBee.sendPayloadTimestamp(outgoing, 0x46);
    //Serial.print("x: ");
    //Serial.println(accX);
    
    float accY = event.acceleration.y;
    outgoing.floatData = accY;
    XBee.sendPayloadTimestamp(outgoing, 0x47);
    //Serial.print("y: ");
    //Serial.println(accY);
    
    float accZ = event.acceleration.z;
    outgoing.floatData = accZ;
    XBee.sendPayloadTimestamp(outgoing, 0x48);
    //Serial.print("z: ");
    //Serial.println(accZ);
    
    accumulator += deltaTime;
  }*/

  /*if (micros() > accumulator){
    int reading = analogRead(INNER_PRESSURE);
    float pressure = reading * 0.0904 + 10.333;
    Serial.println(pressure);

    payload outgoing;
    outgoing.floatData = pressure;
    XBee.sendPayloadTimestamp(outgoing, 0x53);

    int acc1 = analogRead(acc1Pin);
    int acc2 = analogRead(acc2Pin);
    int acc3 = analogRead(acc3Pin);

    byte data[8];

    INIT_SidepodController0(data);
    SET_SidepodController0_InnerManifoldPressure(data, reading);
    SET_SidepodController0_CGAcc1(data, acc1);
    SET_SidepodController0_CGAcc2(data, acc2);
    SET_SidepodController0_CGAcc3(data, acc3);

    unsigned long timestamp = millis();    

    logfile.print(timestamp);
    logfile.print(",");
    logfile.print(ID_SidepodController0,HEX);
    logfile.print(",");
    char tmp[4];
    for (int i = 0; i < len; ++i){
      sprintf(tmp, "%02X",data[i]);
      logfile.print(tmp);
    }
    logfile.println();
    logfile.flush();

    accumulator += deltaTime;
  }*/

  if (!digitalRead(CAN0_INT)) {
    unsigned long timestamp = millis();
    CAN0.readMsgBuf(&rxId, &len, rxBuf);
    
    payload outgoing;

    logfile.print(timestamp);
    logfile.print(",");
    logfile.print(rxId,HEX);
    logfile.print(",");
    char tmp[4];
    for (int i = 0; i < len; ++i){
      sprintf(tmp, "%02X",rxBuf[i]);
      logfile.print(tmp);
    }
    logfile.println();
    logfile.flush();

    switch (rxId & 0x1FFFFFFF) {

      case ID_AEMEngine0: {

          int rawRPM = GET_AEMEngine0_EngineSpeed(rxBuf);
          float rpm = CALC_AEMEngine0_EngineSpeed(rawRPM, 1.0);

          outgoing.floatData = rpm;
          XBee.sendPayloadTimestamp(outgoing, 0x30);

          int rawLoad = GET_AEMEngine0_EngineLoad(rxBuf);
          float load = CALC_AEMEngine0_EngineLoad(rawLoad, 1.0);

          outgoing.floatData = load;
          XBee.sendPayloadTimestamp(outgoing, 0x31);
          //Display.sendPayload(outgoing, 0x31);

          //int rawMAP = GET_AEMEngine0_MAP(rxBuf);
          //float mapP = CALC_AEMEngine0_MAP(rawMAP, 1.0);

          //outgoing.floatData = mapP;
          //XBee.sendPayloadTimestamp(outgoing, 0x52);

          //log throttle position
          int rawThrottle = GET_AEMEngine0_ThrottlePos(rxBuf);
          float throttle = CALC_AEMEngine0_ThrottlePos(rawThrottle, 1.0);

          outgoing.floatData = throttle;
          XBee.sendPayloadTimestamp(outgoing, 0x32);

          //log coolant temp
          int coolantC = GET_AEMEngine0_CoolantTemp(rxBuf);
          float coolantF = CALC_AEMEngine0_CoolantTemp(coolantC, 1.0);

          outgoing.floatData = coolantF;
          XBee.sendPayloadTimestamp(outgoing, 0x33);

          int rawIntake = GET_AEMEngine0_IntakeManifoldAirTemp(rxBuf);
          float intake = CALC_AEMEngine0_IntakeManifoldAirTemp(rawIntake, 1.0);

          outgoing.floatData = intake;
          XBee.sendPayloadTimestamp(outgoing, 0x44);

          break;
        }

      case ID_AEMEngine3: {
          //dataLine = "O2_SPEED_GEAR_VOLTAGE, ";
          //log O2
          /*uint8_t rawo2 = (uint8_t)rxBuf[0];
          o2 = rawo2 * O2_SCALE + 0.5;*/
          //dataLine = dataLine + o2 +  ", ";

          int rawo2 = GET_AEMEngine3_AFR1(rxBuf);
          float o2 = CALC_AEMEngine3_AFR1(rawo2, 1.0);

          outgoing.floatData = o2;
          XBee.sendPayloadTimestamp(outgoing, 0x34);

          /*uint16_t rawSpeed = (uint16_t)rxBuf[2] << 8;
          rawSpeed |= rxBuf[3];
          vehicleSpeed = rawSpeed * SPEED_SCALE;*/
          //dataLine = dataLine + vehicleSpeed + ", ";

          int rawSpeed = GET_AEMEngine3_VehicleSpeed(rxBuf);
          float vehicleSpeed = CALC_AEMEngine3_VehicleSpeed(rawSpeed, 1.0);

          outgoing.floatData = vehicleSpeed;
          XBee.sendPayloadTimestamp(outgoing, 0x35);
          //Display.sendPayload(outgoing, 0x35);

          byte gear = rxBuf[4];
          XBee.sendByteTimestamp(gear, 0x36);
          //Display.sendByte(gear, 0x36);

          int rawVolts = GET_AEMEngine3_ECUBatteryVoltage(rxBuf);
          float volts = CALC_AEMEngine3_ECUBatteryVoltage(rawVolts, 1.0);

          outgoing.floatData = volts;
          XBee.sendPayloadTimestamp(outgoing, 0x37);

          int rawIgn = GET_AEMEngine3_IgnitionTiming(rxBuf);
          float ign = CALC_AEMEngine3_IgnitionTiming(rawIgn, 1.0);

          outgoing.floatData = ign;
          XBee.sendPayloadTimestamp(outgoing, 0x45);
          
          // log time (since arduino started)
          //dataLine = dataLine + (millis()/1000.0);
          //Serial.println(dataLine);
          //logFile.println(dataLine);
          break;
        }

      case ID_RightController0: {
          int rawRB = GET_RightController0_RearBrakePressure(rxBuf);
          float rearBrake = CALC_RightController0_RearBrakePressure(rawRB, 1.0);
          outgoing.floatData = rearBrake;
          XBee.sendPayloadTimestamp(outgoing, 0x41);

          int rawFB = GET_RightController0_FrontBrakePressure(rxBuf);
          float frontBrake = CALC_RightController0_FrontBrakePressure(rawFB, 1.0);
          outgoing.floatData = frontBrake;
          XBee.sendPayloadTimestamp(outgoing, 0x40);

          /*Serial.print("FRONT: ");
          Serial.print(frontBrake);
          Serial.print("\t");
          Serial.print("REAR:");
          Serial.println(rearBrake);*/
          
      }

      case ID_LeftController1: {
          int rawX = GET_LeftController1_WheelAccX(rxBuf);
          float accX = CALC_LeftController1_WheelAccX(rawX, 1.0);
          outgoing.floatData = accX;
          XBee.sendPayloadTimestamp(outgoing, 0x49);

          int rawY = GET_LeftController1_WheelAccY(rxBuf);
          float accY = CALC_LeftController1_WheelAccY(rawY, 1.0);
          outgoing.floatData = accY;
          XBee.sendPayloadTimestamp(outgoing, 0x4A);

          int rawZ = GET_LeftController1_WheelAccZ(rxBuf);
          float accZ = CALC_LeftController1_WheelAccZ(rawZ, 1.0);
          outgoing.floatData = accZ;
          XBee.sendPayloadTimestamp(outgoing, 0x4B);
      }
      
      default: {
          break;
      }
        
    }
  }
}
