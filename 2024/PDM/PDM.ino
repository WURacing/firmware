#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <SPI.h>
// #include <CAN.h>
#include "PDM.h"

#define SPI_SPEED_FEATHER 3000000
#define SPI_SPEED_ADC 1500000
#define BAUD_RATE 1000000

// feather pins
#define MUX_A0 11
#define MUX_A1 12
#define MUX_A2 13
#define MUX_A3 A0
#define ADC_CS A5
#define RD_CS A4
#define TS1_CS 10
#define TS2_CS 9
#define TS3_CS 6
#define TS4_CS 5
#define EN 22
#define ADC_EOC 0
#define TS5_CS 4
#define TS6_CS 1
#define TS7_CS A3
#define TS8_CS A2
#define MUX_OUT_FB A1

// relay pin indices
#define ENGRD 0
#define PE3FPRD 1
#define AUX1RD 2
#define PE3FANRD 3
#define CANRD 4
#define AUX2RD 5
#define STRRD 6
#define WTPRD 7

// ADC pins
#define ETHF_PIN 0b0000
#define FPF_PIN 0b0001
#define PE3F_PIN 0b0010
#define FANF_PIN 0b0100 
#define ENGF_PIN 0b0101
#define CANF_PIN 0b0110
#define AUX1F_PIN 0b0111
#define STRF_PIN 0b1000
#define WTPF_PIN 0b1001
#define AUX2F_PIN 0b1010

// MUX indices
#define FAN 0
#define ENG 1
#define BAT122 2
#define AUX1 3
#define GPIO 4
#define CAN 5
#define STR 6
#define BAT123 7
#define FP 8
#define PE3 9
#define BAT121 10
#define ETH 11
#define AUX2 12
#define STRIN 13
#define PE3FP 14
#define PE3FAN 15

// Current limits
#define ETHF_LIMIT 15
#define FPF_LIMIT 15
#define PE3F_LIMIT 15
#define FANF_LIMIT 15
#define ENGF_LIMIT 15
#define CANF_LIMIT 15
#define AUX1F_LIMIT 15
#define STRF_LIMIT 15
#define WTPF_LIMIT 15
#define AUX2F_LIMIT 15

#define ACCEPTED_ERROR 15
#define ANALOG_LOW 5.0
#define LOW_VOLTAGE 8.0
#define LED 13
#define BLINK_INTERVAL 1000
#define VREF 3.3
#define ADC_RES 4095
#define RUN_INTERVAL 100

// Mux voltage divider resistors
#define R1 3000.0 // ohms
#define R2 840.0  // ohms

#define DEBUG
#ifdef DEBUG
#define printDebug(message) Serial.print(message)
#else
#define printDebug(message)
#endif

int coolant_temp;

float aux1_error = 0;
float aux2_error = 0;
float pe3_error = 0;
float eth_error = 0;
float eng_error = 0;
float fp_error = 0;
float fan_error = 0;
float can_error = 0;
float wtp_error = 0;
float str_error = 0;

int ledState = LOW;
unsigned long blinkCurrentMillis = millis();
unsigned long blinkPreviousMillis = 0;
unsigned long runPreviousMillis = 0;

bool str_state = true;
bool fp_state = false;
bool fan_state = false;
bool low_battery = false;

void setup()
{
  SPI.begin();

  pinMode(MUX_A0, OUTPUT);
  pinMode(MUX_A1, OUTPUT);
  pinMode(MUX_A2, OUTPUT);
  pinMode(MUX_A3, OUTPUT);
  pinMode(ADC_CS, OUTPUT);
  pinMode(RD_CS, OUTPUT);
  pinMode(TS1_CS, OUTPUT);
  pinMode(TS2_CS, OUTPUT);
  pinMode(TS3_CS, OUTPUT);
  pinMode(TS4_CS, OUTPUT);
  pinMode(TS5_CS, OUTPUT);
  pinMode(TS6_CS, OUTPUT);
  pinMode(TS7_CS, OUTPUT);
  pinMode(TS8_CS, OUTPUT);
  pinMode(EN, OUTPUT);

  pinMode(ADC_EOC, INPUT);
  pinMode(MUX_OUT_FB, INPUT);
  analogReadResolution(12);

  // set CS pins high
  digitalWrite(ADC_CS, HIGH);
  digitalWrite(RD_CS, HIGH);
  digitalWrite(TS1_CS, HIGH);
  digitalWrite(TS2_CS, HIGH);
  digitalWrite(TS3_CS, HIGH);
  digitalWrite(TS4_CS, HIGH);
  digitalWrite(TS5_CS, HIGH);
  digitalWrite(TS6_CS, HIGH);
  digitalWrite(TS7_CS, HIGH);
  digitalWrite(TS8_CS, HIGH);

  Serial.begin(9600); // start Serial monitor to display current read on monitor
  // if (CAN.begin(BAUD_RATE))
  // {
  //   Serial.printfln("Starting CAN failed");
  // }


  // decouple input mapping pins 0 and 1 from outputs 2 and 3
  digitalWrite(RD_CS, HIGH);
  SPI.beginTransaction(SPISettings(SPI_SPEED_FEATHER, MSBFIRST, SPI_MODE3));
  digitalWrite(RD_CS, LOW);
  SPI.transfer16(0b1000010000000000);
  digitalWrite(RD_CS, HIGH);
  delay(1);
  digitalWrite(RD_CS, LOW);
  SPI.transfer16(0b1000010100000000);
  digitalWrite(RD_CS, HIGH);
  SPI.endTransaction();

  // Disable relays
  relay(false, ENGRD); // good
  relay(false, AUX1RD); // good
  relay(false, CANRD); // good
  relay(false, AUX2RD); // good
  relay(false, WTPRD); // good
  relay(false, PE3FPRD); // good
  relay(false, STRRD); // good
  relay(false, PE3FANRD); // good

  // Enable relays
  relay(true, ENGRD); // good
  relay(true, AUX1RD); // good
  relay(true, CANRD); // good
  relay(false, AUX2RD); // good
  relay(true, WTPRD); // good


  delay(1000); // TODO: Remove or minimize this later



  runPreviousMillis = millis();
}

void loop()
{

  if (millis() - runPreviousMillis < RUN_INTERVAL)
  {
    return;
  }
  else
  {
    runPreviousMillis += RUN_INTERVAL;
  }

  // Sense current on each pin
  float aux1_c = currSense(AUX1F_PIN);
  float aux2_c = currSense(AUX2F_PIN);
  float pe3_c = currSense(PE3F_PIN);
  float eth_c = currSense(ETHF_PIN);
  float eng_c = currSense(ENGF_PIN);
  float fp_c = currSense(FPF_PIN);
  float fan_c = currSense(FANF_PIN);
  float can_c = currSense(CANF_PIN);
  float wtp_c = currSense(WTPF_PIN);
  float str_c = currSense(STRF_PIN);

  // if (aux1_c > 10 || aux1_c < 0){
  //   Serial.print("WARNING: aux1 current is " + aux1_c);
  // }
  // if (aux2_c > 10 || aux2_c < 0){
  //   Serial.print("WARNING: pe3 current is " + pe3_c);
  // }
  // if (pe3_c > 2 || pe3_c < 0){
  //   Serial.print("WARNING: aux2 current is " + aux2_c);
  // }
  // if (eth_c > 10 || eth_c < 0){
  //   Serial.print("WARNING: ethrottle current is " + eth_c);
  // }
  // if (eng_c > 2 || eng_c < 0){
  //   Serial.print("WARNING: engine current is " + eng_c);
  // }
  // if (fp_c > 7 || fp_c < 0){
  //   Serial.print("WARNING: fuel pump current is " + fp_c);
  // }
  // if (fan_c > 12 || fan_c < 0){
  //   Serial.print("WARNING: fan current is " + fan_c);
  // }
  // if (can_c > 5 || can_c < 0){
  //   Serial.print("WARNING: CAN current is " + can_c);
  // }
  // if (wtp_c > 5 || wtp_c < 0){
  //   Serial.print("WARNING: water pump current is " + wtp_c);
  // }
  // if (str_c > 1 || str_c < 0){
  //   Serial.print("WARNING: starter current is " + str_c);
  // }


  // Sense voltage on each pin
  float fan_v = mux(FAN);
  float eng_v = mux(ENG);
  float bat122_v = mux(BAT122);
  float aux1_v = mux(AUX1);
  float gpio_v = mux(GPIO);
  float can_v = mux(CAN);
  float str_v = mux(STR);
  float bat123_v = mux(BAT123);
  float fp_v = mux(FP);
  float pe3_v = mux(PE3);
  float eth_v = mux(ETH);
  float aux2_v = mux(AUX2);
  float strin_v = mux(STRIN);
  float pe3fp_v = mux(PE3FP);
  float pe3fan_v = mux(PE3FAN);
  float bat121_v = mux(BAT121);


  printDebug("Aux1: ");
  printDebug(aux1_c);
  printDebug("A\tAux2: ");
  printDebug(aux2_c);
  printDebug("A\tPE3: ");
  printDebug(pe3_c);
  printDebug("A\tETH: ");
  printDebug(eth_c);
  printDebug("A\tENG: ");
  printDebug(eng_c);
  printDebug("A\tFP: ");
  printDebug(fp_c);
  printDebug("A\tFAN: ");
  printDebug(fan_c);
  printDebug("A\tCAN: ");
  printDebug(can_c);
  printDebug("A\tSTR: ");
  printDebug(str_c);
  printDebug("A\tWTP: ");
  printDebug(wtp_c);
  printDebug("A\n");

  printDebug("Aux1: ");
  printDebug(aux1_v);
  printDebug("V\tAux2: ");
  printDebug(aux2_v);
  printDebug("V\tPE3: ");
  printDebug(pe3_v);
  printDebug("V\tETH: ");
  printDebug(eth_v);
  printDebug("V\tENG: ");
  printDebug(eng_v);
  printDebug("V\tFP: ");
  printDebug(fp_v);
  printDebug("V\tFAN: ");
  printDebug(fan_v);
  printDebug("V\tCAN: ");
  printDebug(can_v);
  printDebug("V\tSTR: ");
  printDebug(str_v);
  printDebug("V\tBat121: ");
  printDebug(bat121_v);
  printDebug("V\tBat122: ");
  printDebug(bat122_v);
  printDebug("V\tBat123: ");
  printDebug(bat123_v);
  printDebug("V\tSTRIN: ");
  printDebug(strin_v);
  printDebug("V\tPE3FP: ");
  printDebug(pe3fp_v);
  printDebug("V\tPE3FAN: ");
  printDebug(pe3fan_v);
  printDebug("V\tGPIO: ");
  printDebug(gpio_v);
  printDebug("V\n");

  // if (aux1_c > AUX1F_LIMIT)
  // {
  //   aux1_error += aux1_c - AUX1F_LIMIT;
  // }
  // if (aux2_c > AUX2F_LIMIT)
  // {
  //   aux2_error += aux2_c - AUX2F_LIMIT;
  // }
  // if (pe3_c > PE3F_LIMIT)
  // {
  //   pe3_error += pe3_c - PE3F_LIMIT;
  // }
  // if (eth_c > ETHF_LIMIT)
  // {
  //   eth_error += eth_c - ETHF_LIMIT;
  // }
  // if (eng_c > ENGF_LIMIT)
  // {
  //   eng_error += eng_c - ENGF_LIMIT;
  // }
  // if (fp_c > FPF_LIMIT)
  // { // relay(true, ENGRD);
  //   // relay(true, AUX1RD);
  //   // relay(true, CANRD);
  //   // relay(true, AUX2RD);
  //   fp_error += fp_c - FPF_LIMIT;
  // }
  // if (fan_c > FANF_LIMIT)
  // {
  //   fan_error += fan_c - FANF_LIMIT;
  // }
  // if (can_c > CANF_LIMIT)
  // {
  //   can_error += can_c - CANF_LIMIT;
  // }
  // if (wtp_c > WTPF_LIMIT)
  // {
  //   wtp_error += wtp_c - WTPF_LIMIT;
  // }
  // if (str_c > STRF_LIMIT)
  // {
  //   str_error += str_c - STRF_LIMIT;
  // }

  // Digital circuit breaking
  // if (aux1_c < 0 || aux1_error > ACCEPTED_ERROR)
  // {
  //   Serial.print("Aux1 error: ");
  //   Serial.println(aux1_error);
  //   relay(false, AUX1RD); // disable relay
  // }
  // if (aux2_c < 0 || aux2_error > ACCEPTED_ERROR)
  // {
  //   Serial.print("Aux2 error: ");
  //   Serial.println(aux2_error);
  //   relay(false, AUX2RD);
  // }
  // if (pe3_c < 0 || pe3_error > ACCEPTED_ERROR)
  // {
  //   Serial.print("PE3 error: ");
  //   Serial.println(pe3_error);
  //   relay(false, PE3FPRD);
  // }
  // if (eth_c < 0 || eth_error > ACCEPTED_ERROR)
  // {
  //   Serial.print("ETH error: ");
  //   Serial.println(eth_error);
  //   relay(false, ENGRD);
  // }
  // if (eng_c < 0 || eng_error > ACCEPTED_ERROR)
  // {
  //   Serial.print("ENG error: ");
  //   Serial.println(eng_error);
  //   relay(false, ENGRD);
  // }
  // if (fp_c < 0 || fp_error > ACCEPTED_ERROR)
  // {
  //   Serial.print("FP error: ");
  //   Serial.println(fp_error);
  //   relay(false, PE3FPRD);
  // }
  // if (fan_c < 0 || fan_error > ACCEPTED_ERROR)
  // {
  //   Serial.print("FAN error: ");
  //   Serial.println(fan_error);
  //   relay(false, PE3FANRD);
  // }
  // if (can_c < 0 || can_error > ACCEPTED_ERROR)
  // {
  //   Serial.print("CAN error: ");
  //   Serial.println(can_error);
  //   relay(false, CANRD);
  // }
  // if (wtp_c < 0 || wtp_error > ACCEPTED_ERROR)
  // {
  //   Serial.print("WTP error: ");
  //   Serial.println(wtp_error);
  //   relay(false, WTPRD);
  // }
  // if (str_c < 0 || str_error > ACCEPTED_ERROR)
  // {
  //   Serial.print("STR error: ");
  //   Serial.println(str_error);
  //   relay(false, STRRD);
  // }

  // //read signals from PE3 and turn on related relays
  if (pe3fan_v > ANALOG_LOW && fan_state)
  {
    relay(false, PE3FANRD);
    //relay(false, WTPRD);
    fan_state = false;
  }
  if (pe3fan_v <= ANALOG_LOW && !fan_state)
  {
    relay(true, PE3FANRD);
    //relay(true, WTPRD);
    fan_state = true;
  }

  if (pe3fp_v > ANALOG_LOW && fp_state)
  {
    relay(false, PE3FPRD);
    fp_state = false;
  }
  if (pe3fp_v <= ANALOG_LOW && !fp_state)
  {
    relay(true, PE3FPRD);
    fp_state = true;
  }

  // // TODO: Switch to push to start
  if (strin_v >= ANALOG_LOW && str_state)
  {
    relay(false, STRRD);
    str_state = false;
  }
  if (strin_v < ANALOG_LOW && !str_state)
  {
    relay(true, STRRD);
    str_state = true;
  }


  // TODO: water pump: start whenever engine is turned on, stop when coolant temp gets low enough
  // get coolant temp from CAN
  // CAN.filter(2365584712); // coolant temp ID from DBC file
  // CAN.onReceive(onReceive);

  // push to start
  // timeout after 2s
  // enable starter relay, stop when RPM >1000
  // get RPM from CAN

  // ethrottle (ETHF)

  // read voltage of battery
  // datasheet says minimum preferred is 8V
  // added 20% factor of safety
  // if (bat123_v < LOW_VOLTAGE && strin_v > ANALOG_LOW)
  // {
  //   Serial.print("Battery voltage too low: ");
  //   Serial.println(bat123_v);
  //   for (int i = 0; i < 8; i++)
  //   {
  //     relay(false, i);
  //   }
  //   low_battery = true;
  // }

  // TODO: Send CAN data
  // TODO: Ethrottle protections
}

// void onReceive()
// {
//   uint16_t msg;
//   for (int i = 0; i < 6; i++)
//   {
//     int b = CAN.read();
//     if (i == 4)
//     {
//       msg = b << 8; // left shift first byte
//     }
//     if (i == 5)
//     {
//       msg |= b; // or second byte
//     }
//   }
//   coolant_temp = msg;
// }

float currSense(int pin)
{
  // Wait for conversion to finish
  while (digitalRead(ADC_EOC) == LOW)
  {
    delay(1);
  }

  digitalWrite(ADC_CS, HIGH);
  SPI.beginTransaction(SPISettings(SPI_SPEED_ADC, MSBFIRST, SPI_MODE0));
  digitalWrite(ADC_CS, LOW);
  delay(1);

  // uint8_t mesg = 0b00000101;
  // mesg |= pin << 4; // bitwise operator
  // uint8_t output = SPI.transfer(mesg);

  uint16_t mesg = 0b00001101 << 8;
  mesg |= pin << 12; // bitwise operator
  // mesg |= 0b1011 << 12;
  uint16_t output = SPI.transfer(mesg);

  // uint16_t mesg = 0b00110000; // params: buffer (0b - binary, 0 - unipolar binary, 0 - MSB out first, 11 - 16-bit output length, XXXX - pin command), return size
  // mesg |= pin;                // bitwise operator
  // uint16_t output = SPI.transfer16(mesg);
  digitalWrite(ADC_CS, HIGH);

  SPI.endTransaction();

  return (output >> 4) * (VREF / (float)ADC_RES); //* 30.303 - 50; // Linearize
  // return output >> 2;
}

// params:
// enable (true if enabling, false if disabling)
// relay (number between 0-7 corresponding to the relay number)
void relay(bool enable, uint8_t relay)
{
  // if battery voltage is too low, stop relays from being reenabled
  // if (low_battery)
  // {
  //   return;
  // }

  printDebug("Relay: ");
  printDebug(relay);
  printDebug("\tEnable: ");
  printDebug(enable);
  printDebug("\n");

  // read relay state first
  uint16_t read = 0b0100000000000000;

  digitalWrite(RD_CS, HIGH);
  SPI.beginTransaction(SPISettings(SPI_SPEED_FEATHER, MSBFIRST, SPI_MODE3));
  digitalWrite(RD_CS, LOW);
  uint16_t read_output = SPI.transfer16(read); // returns a 16 bit, convert it to 8

  uint8_t data = read_output & 0b11111111; // get data

  uint8_t relaybit = 1 << relay;       // bit that's being set
  uint8_t disabled = ~relaybit & data; // normal data, but disable relay bit

  uint16_t command_data = disabled; // if it doesn't work, change this to uint8_t

  if (enable)
  {
    command_data = disabled | relaybit; // enable relay bit
  }

  uint16_t mesg = 0b1000000000000000 | command_data; // put command data in message
  SPI.transfer16(mesg);
  digitalWrite(RD_CS, HIGH);
  SPI.endTransaction();
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

  float voltage = analogRead(MUX_OUT_FB)*0.0039; //* 3.3 * 4.84  / (float)4095;
  digitalWrite(EN, LOW);
  digitalWrite(MUX_A0, LOW);
  digitalWrite(MUX_A1, LOW);
  digitalWrite(MUX_A2, LOW);
  digitalWrite(MUX_A3, LOW);
  // float voltage = analogRead(MUX_OUT_FB);
  //  voltage divider equation
  //voltage = (2700 + 840) * voltage / 840;

  return voltage;
}
