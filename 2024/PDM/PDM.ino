#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <SPI.h>

#define SPI_SPEED 1000000

byte whoami;


//Current Sensors

//feather pins
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

//relay pin indices
#define ENGRD 0
#define AUX1RD 2
#define CANRD 4
#define STRRD 6
#define PE3FPRD 1
#define PE3FANRD 3
#define AUX2RD 5
#define WTPRD 7

//ADC pins
#define AUX1F_PIN 0b0111
#define AUX2F_PIN 0b1010
#define PE3F_PIN 0b0010
#define ETHF_PIN 0b0000
#define ENGF_PIN 0b0101
#define FPF_PIN 0b0001
#define FANF_PIN 0b0100
#define CANF_PIN 0b0110
#define WTPF_PIN 0b1001
#define STRF_PIN 0b1000


void setup() {
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


  Serial.begin(9600); //start Serial monitor to display current read on monitor

  //enable all relays
  for (int i = 0; i < 8; i++){
    relay(true, i);
  }
  
}

int aux1_error = 0;

void loop() {
  //sense current on each pin
  uint16_t aux1 = currSense(AUX1F_PIN);
  if(aux1 > 15){
      aux1_error += aux1-15;
      if(aux1_error > 300){
          //disable relay
      }
  }
  else {
      aux1_error = 0;
  }


  //read signals from PE3 and turn on related relays
  //PE3FAN (S16), PE3FP (S15), Kill Switch, Starter (25) on Multiplexer Circuit
  //if >4.5V, turn on

}

uint16_t currSense(int pin){
  digitalWrite(ADC_CS, HIGH);
  SPI.beginTransaction(SPISettings(1000, MSBFIRST, SPI_MODE3));
  digitalWrite(ADC_CS, LOW);

  uint16_t mesg = 0b00110000; //params: buffer (0b - unipolar binary, 0 - MSB out first, 0 - ?, 11 - 16-bit output length, XXXX - pin command), return size
  mesg |= pin; //bitwise operator

  uint16_t output = SPI.transfer16(mesg);
  digitalWrite(ADC_CS, HIGH);

  SPI.endTransaction();

  return output;

}


//params: 
//enable (true if enabling, false if disabling)
//relay (number between 0-7 corresponding to the relay number)
void relay(bool enable, uint8_t relay){
  //read relay state first
  uint16_t read = 0b0100000000000000;

  digitalWrite(RD_CS, HIGH);
  SPI.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE3));
  digitalWrite(RD_CS, LOW);
  uint16_t read_output = SPI.transfer16(read); //returns a 16 bit, convert it to 8

  uint8_t data = read_output & 0b11111111; //get data

  uint8_t relaybit = 1 << relay; //bit that's being set
  uint8_t disabled = ~relaybit & data; //normal data, but disable relay bit

  uint16_t command_data = disabled; //if it doesn't work, change this to uint8_t
 
  if(enable){
     command_data = disabled | relaybit; //enable relay bit
  }

  //enable/disable relay
  uint16_t mesg = 0b1000000000000000 | command_data; //put command data in message
  SPI.transfer16(mesg);
  digitalWrite(RD_CS, HIGH);
  SPI.endTransaction();

}

//Mux voltage divider resistors
#define R1 3000.0 //ohms
#define R2 840.0 //ohms

float mux(int index){
  //set multiplexer pins
  if (index & 0b0001 > 0){
    digitalWrite(MUX_A0, HIGH);
  }else{
    digitalWrite(MUX_A0, LOW);
  }

  if(index & 0b0010 > 0){
    digitalWrite(MUX_A1, HIGH);
  }else{
    digitalWrite(MUX_A1, LOW);
  }

  if(index & 0b0100 > 0){
    digitalWrite(MUX_A2, HIGH);
  }else{
    digitalWrite(MUX_A2, LOW);
  }

  if(index & 0b1000 > 0){
    digitalWrite(MUX_A3, HIGH);
  }else{
    digitalWrite(MUX_A3, LOW);
  }


  float voltage_measured = analogRead(MUX_OUT_FB); //voltage_measured = voltage_out
  voltage_measured /= 1023;  //max val is a 7 bit integer
  voltage_measured *= 3.3; //3.3V board

  //voltage divider equation
  float voltage = (R1 + R2) * voltage_measured / R2;
  return voltage;
}
