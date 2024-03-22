#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <SPI.h>
#include <CAN.h>

#define SPI_SPEED 1000000
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
#define AUX1RD 2
#define CANRD 4
#define STRRD 6
#define PE3FPRD 1
#define PE3FANRD 3
#define AUX2RD 5
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

uint16_t coolant_temp;

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

  Serial.begin(9600); // start Serial monitor to display current read on monitor
  if (CAN.begin(BAUD_RATE))
  {
    Serial.println("Starting CAN failed");
  }

  // delay(4000);

  // enable all relays
  for (int i = 0; i < 8; i++)
  {
    relay(true, i);
  }
}

void loop()
{
  // sense current on each pin
  uint16_t aux1 = currSense(AUX1F_PIN);
  uint16_t aux2 = currSense(AUX2F_PIN);
  uint16_t pe3 = currSense(PE3F_PIN);
  uint16_t eth = currSense(ETHF_PIN);
  uint16_t eng = currSense(ENGF_PIN);
  uint16_t fp = currSense(FPF_PIN);
  uint16_t fan = currSense(FANF_PIN);
  uint16_t can = currSense(CANF_PIN);
  uint16_t wtp = currSense(WTPF_PIN);
  uint16_t str = currSense(STRF_PIN);
  if (aux1 < 0)
  {
    relay(false, AUX1RD); // disable relay
  }
  // do some digital circuit breaking shit

  // read signals from PE3 and turn on related relays
  if (mux(16) > 0.1)
  {
    relay(false, PE3FANRD);
  }
  if (mux(15) > 0.1)
  {
    relay(false, PE3FPRD);
  }

  if (mux(16) < 0.1)
  {
    relay(true, PE3FANRD);
  }
  if (mux(15) < 0.1)
  {
    relay(true, PE3FPRD)
  }

  // water pump: start whenever engine is turned on, stop when coolant temp gets low enough
  // get coolant temp from CAN
  CAN.filter(2365584712); // coolant temp ID from DBC file
  CAN.onReceive(onReceive);

  // push to start
  // timeout after 2s
  // enable starter relay, stop when RPM >1000
  // get RPM from CAN

  // ethrottle (ETHF)

  // read voltage of battery
  // datasheet says minimum preferred is 8V
  // added 20% factor of safety
  if (mux(BAT121) < 9.6)
  {
    for (int i = 0; i < 8; i++)
    {
      relay(false, i);
    }
  }
}

void onReceive()
{
  uint16_t msg;
  for (int i = 0; i < 6; i++)
  {
    int b = CAN.read() if (i == 4)
    {
      msg = b << 8; // left shift first byte
    }
    if (i == 5)
    {
      msg |= b; // or second byte
    }
  }
  coolant_temp = msg;
}

uint16_t currSense(int pin)
{
  digitalWrite(ADC_CS, HIGH);
  SPI.beginTransaction(SPISettings(1000, MSBFIRST, SPI_MODE3));
  digitalWrite(ADC_CS, LOW);

  uint16_t mesg = 0b00110000; // params: buffer (0b - binary, 0 - unipolar binary, 0 - MSB out first, 11 - 16-bit output length, XXXX - pin command), return size
  mesg |= pin;                // bitwise operator

  uint16_t output = SPI.transfer16(mesg);
  digitalWrite(ADC_CS, HIGH);

  SPI.endTransaction();

  return output;
}

// params:
// enable (true if enabling, false if disabling)
// relay (number between 0-7 corresponding to the relay number)
void relay(bool enable, uint8_t relay)
{
  // read relay state first
  uint16_t read = 0b0100000000000000;

  digitalWrite(RD_CS, HIGH);
  SPI.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE3));
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

// Mux voltage divider resistors
#define R1 3000.0 // ohms
#define R2 840.0  // ohms

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
