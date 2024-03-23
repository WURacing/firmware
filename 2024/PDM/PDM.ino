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
#define ETHF_LIMIT 10
#define FPF_LIMIT 10
#define PE3F_LIMIT 10
#define FANF_LIMIT 10
#define ENGF_LIMIT 10
#define CANF_LIMIT 10
#define AUX1F_LIMIT 10
#define STRF_LIMIT 10
#define WTPF_LIMIT 10
#define AUX2F_LIMIT 10

#define ACCEPTED_ERROR 10
#define ANALOG_LOW 0.5
#define LOW_VOLTAGE 9.6
#define LED 13
#define BLINK_INTERVAL 1000
#define VREF 3.3
#define ADC_RES 4095
#define MAX_CURRENT 50
#define RUN_INTERVAL 1000

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

void blink()
{
  blinkCurrentMillis = millis();
  if (blinkCurrentMillis - blinkPreviousMillis >= BLINK_INTERVAL)
  {
    blinkPreviousMillis = blinkCurrentMillis;
    if (ledState == LOW)
    {
      ledState = HIGH;
    }
    else
    {
      ledState = LOW;
    }
    digitalWrite(LED, ledState);
  }
}

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

  delay(5000); // TODO: Remove or minimize this later

  // Enable relays
  printDebug("Enabling relays\n");
  relay(true, ENGRD);
  relay(true, AUX1RD);
  relay(true, CANRD);
  relay(true, AUX2RD);
  relay(true, WTPRD); // TODO: Disable this later

  runPreviousMillis = millis();
}

void loop()
{
  blink();

  if (millis() - runPreviousMillis < RUN_INTERVAL)
  {
    return;
  }
  else
  {
    runPreviousMillis += RUN_INTERVAL;
  }

  // Sense current on each pin
  float aux1 = currSense(AUX1F_PIN);
  float aux2 = currSense(AUX2F_PIN);
  float pe3 = currSense(PE3F_PIN);
  float eth = currSense(ETHF_PIN);
  float eng = currSense(ENGF_PIN);
  float fp = currSense(FPF_PIN);
  float fan = currSense(FANF_PIN);
  float can = currSense(CANF_PIN);
  float wtp = currSense(WTPF_PIN);
  float str = currSense(STRF_PIN);

  printDebug("Aux1: ");
  printDebug(aux1);
  printDebug("\tAux2: ");
  printDebug(aux2);
  printDebug("\tPE3: ");
  printDebug(pe3);
  printDebug("\tETH: ");
  printDebug(eth);
  printDebug("\tENG: ");
  printDebug(eng);
  printDebug("\tFP: ");
  printDebug(fp);
  printDebug("\tFAN: ");
  printDebug(fan);
  printDebug("\tCAN: ");
  printDebug(can);
  printDebug("\tWTP: ");
  printDebug(wtp);
  printDebug("\tSTR: ");
  printDebug(str);
  printDebug("\n");

  if (aux1 > AUX1F_LIMIT)
  {
    aux1_error += aux1 - AUX1F_LIMIT;
  }
  if (aux2 > AUX2F_LIMIT)
  {
    aux2_error += aux2 - AUX2F_LIMIT;
  }
  if (pe3 > PE3F_LIMIT)
  {
    pe3_error += pe3 - PE3F_LIMIT;
  }
  if (eth > ETHF_LIMIT)
  {
    eth_error += eth - ETHF_LIMIT;
  }
  if (eng > ENGF_LIMIT)
  {
    eng_error += eng - ENGF_LIMIT;
  }
  if (fp > FPF_LIMIT)
  {
    fp_error += fp - FPF_LIMIT;
  }
  if (fan > FANF_LIMIT)
  {
    fan_error += fan - FANF_LIMIT;
  }
  if (can > CANF_LIMIT)
  {
    can_error += can - CANF_LIMIT;
  }
  if (wtp > WTPF_LIMIT)
  {
    wtp_error += wtp - WTPF_LIMIT;
  }
  if (str > STRF_LIMIT)
  {
    str_error += str - STRF_LIMIT;
  }

  // Digital circuit breaking
  if (aux1 < 0 || aux1_error > ACCEPTED_ERROR)
  {
    Serial.print("Aux1 error: ");
    Serial.println(aux1_error);
    relay(false, AUX1RD); // disable relay
  }
  if (aux2 < 0 || aux2_error > ACCEPTED_ERROR)
  {
    Serial.print("Aux2 error: ");
    Serial.println(aux2_error);
    relay(false, AUX2RD);
  }
  if (pe3 < 0 || pe3_error > ACCEPTED_ERROR)
  {
    Serial.print("PE3 error: ");
    Serial.println(pe3_error);
    relay(false, PE3FPRD);
  }
  if (eth < 0 || eth_error > ACCEPTED_ERROR)
  {
    Serial.print("ETH error: ");
    Serial.println(eth_error);
    relay(false, ENGRD);
  }
  if (eng < 0 || eng_error > ACCEPTED_ERROR)
  {
    Serial.print("ENG error: ");
    Serial.println(eng_error);
    relay(false, ENGRD);
  }
  if (fp < 0 || fp_error > ACCEPTED_ERROR)
  {
    Serial.print("FP error: ");
    Serial.println(fp_error);
    relay(false, PE3FPRD);
  }
  if (fan < 0 || fan_error > ACCEPTED_ERROR)
  {
    Serial.print("FAN error: ");
    Serial.println(fan_error);
    relay(false, PE3FANRD);
  }
  if (can < 0 || can_error > ACCEPTED_ERROR)
  {
    Serial.print("CAN error: ");
    Serial.println(can_error);
    relay(false, CANRD);
  }
  if (wtp < 0 || wtp_error > ACCEPTED_ERROR)
  {
    Serial.print("WTP error: ");
    Serial.println(wtp_error);
    relay(false, WTPRD);
  }
  if (str < 0 || str_error > ACCEPTED_ERROR)
  {
    Serial.print("STR error: ");
    Serial.println(str_error);
    relay(false, STRRD);
  }

  // read signals from PE3 and turn on related relays
  if (mux(PE3FAN) > ANALOG_LOW)
  {
    relay(false, PE3FANRD);
  }
  else
  {
    relay(true, PE3FANRD);
  }
  if (mux(PE3FP) > ANALOG_LOW)
  {
    relay(false, PE3FPRD);
  }
  else
  {
    relay(true, PE3FPRD);
  }
  // TODO: Switch to push to start
  // if (mux(STRIN) > ANALOG_LOW)
  // {
  //   relay(true, STRRD);
  // }
  // else
  // {
  //   relay(false, STRRD);
  // }

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
  float battery_voltage = mux(BAT123);
  if (battery_voltage < LOW_VOLTAGE)
  {
    Serial.print("Battery voltage too low: ");
    Serial.println(battery_voltage);
    for (int i = 0; i < 8; i++)
    {
      relay(false, i);
    }
  }

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
  digitalWrite(ADC_CS, HIGH);
  SPI.beginTransaction(SPISettings(SPI_SPEED_ADC, MSBFIRST, SPI_MODE3));
  digitalWrite(ADC_CS, LOW);

  uint16_t mesg = 0b00110000; // params: buffer (0b - binary, 0 - unipolar binary, 0 - MSB out first, 11 - 16-bit output length, XXXX - pin command), return size
  mesg |= pin;                // bitwise operator

  uint16_t output = SPI.transfer16(mesg);
  digitalWrite(ADC_CS, HIGH);

  SPI.endTransaction();

  return output * VREF / (float)ADC_RES * MAX_CURRENT; // Linearize
}

// params:
// enable (true if enabling, false if disabling)
// relay (number between 0-7 corresponding to the relay number)
void relay(bool enable, uint8_t relay)
{
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

  // enable/disable relay
  uint16_t mesg = 0b1000000000000000 | command_data; // put command data in message
  SPI.transfer16(mesg);
  digitalWrite(RD_CS, HIGH);
  SPI.endTransaction();
}

// params:
// index: number between 0-15 (DOUBLE CHECK)
float mux(int index)
{
  // set multiplexer pins
  if (index & 0b0001 > 0)
  {
    digitalWrite(MUX_A0, HIGH);
  }
  else
  {
    digitalWrite(MUX_A0, LOW);
  }

  if (index & 0b0010 > 0)
  {
    digitalWrite(MUX_A1, HIGH);
  }
  else
  {
    digitalWrite(MUX_A1, LOW);
  }

  if (index & 0b0100 > 0)
  {
    digitalWrite(MUX_A2, HIGH);
  }
  else
  {
    digitalWrite(MUX_A2, LOW);
  }

  if (index & 0b1000 > 0)
  {
    digitalWrite(MUX_A3, HIGH);
  }
  else
  {
    digitalWrite(MUX_A3, LOW);
  }

  float voltage_measured = analogRead(MUX_OUT_FB); // voltage_measured = voltage_out
  voltage_measured /= 1023;                        // max val is a 7 bit integer
  voltage_measured *= 3.3;                         // 3.3V board

  // voltage divider equation
  float voltage = (R1 + R2) * voltage_measured / R2;
  return voltage;
}