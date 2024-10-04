#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <SPI.h>
// #include <CAN.h>

#define SPI_SPEED_FEATHER 3000000
#define SPI_SPEED_ADC 10000000
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
#define PE3FPRD 1 // fuel pump
#define AUX1RD 2
#define PE3FANRD 3 // fan
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
    analogReadResolution(12); // ADC turns voltage to 12bits

    // set CS pins high. Low means selected "Slave"
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
    relay(true, ENGRD);    // good
    relay(false, AUX1RD);   // good
    relay(true, CANRD);    // good
    relay(false, AUX2RD);   // good
    relay(false, WTPRD);    // good
    relay(false, PE3FPRD);  // good
    relay(true, STRRD);    // good
    relay(false, PE3FANRD); // good

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
    // float current = currSense2(ENGF_PIN);
    // Serial.println(current);
    Serial.println(currSense(CANF_PIN));

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
    // printDebug("Relay: ");
    // printDebug(relay);
    // printDebug("\tEnable: ");
    // printDebug(enable);
    // printDebug("\n");
    // read relay state first
    uint16_t read = 0b0100000000000000;

    digitalWrite(RD_CS, HIGH);
    SPI.beginTransaction(SPISettings(SPI_SPEED_FEATHER, MSBFIRST, SPI_MODE3));
    digitalWrite(RD_CS, LOW);
    uint16_t read_output = SPI.transfer16(read); // returns a 16 bit, convert it to 8

    uint8_t data = read_output & 0b11111111; // get data
    Serial.println(data);
    uint8_t relaybit = 1 << relay;       // Bitshift. bit that's being set
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

uint8_t testAdc()
{
    while (digitalRead(ADC_EOC) == LOW)
    {
        Serial.println("Waiting for conversion to finish");
        delay(1);
    }
    digitalWrite(ADC_CS, HIGH);
    SPI.beginTransaction(SPISettings(SPI_SPEED_ADC, MSBFIRST, SPI_MODE0));
    digitalWrite(ADC_CS, LOW);
    //uint16_t output = SPI.transfer(0b10110100); // 8 bit test command, should return (vref + gnd) / 2     (128)
    uint16_t output = SPI.transfer(0b11010000); // 8 bit test command, should return vref                 (255)
    // uint8_t output = SPI.transfer(0b11000100); // 8 bit test command, should return gnd                  (0)
    digitalWrite(ADC_CS, HIGH);
    SPI.endTransaction();
    return output;
}

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
  uint8_t output1 = SPI.transfer(0b01101000);
  uint8_t output2 = SPI.transfer(0b00000000);
  digitalWrite(ADC_CS, HIGH);

  SPI.endTransaction();
  uint16_t output = ((output1 << 8) | output2) >> 4;
  return output * 0.0201416015625 - 41.47;
  // return output >> 2;
}



