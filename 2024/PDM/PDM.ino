#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <SPI.h>

#define SPI_SPEED 1000000

byte whoami;


//Current Sensors

//analog pins
const int AUX1F_PIN = 0111;
const int AUX2F_PIN = 1010;
const int PE3F_PIN = 0010;
const int ETHF_PIN = 0000;
const int ENGF_PIN = 0101;
const int FPF_PIN = 0001;
const int FANF_PIN = 0100;
const int CANF_PIN = 0110;
const int WTPF_PIN = 1001;
const int STRF_PIN = 1000;
const int ADC_CS = A5;

void setup() {
  Serial.begin(9600); //start Serial monitor to display current read on monitor
}

void currSense(int pin){
  //pinMode(ADC_CS, OUTPUT);
  digitalWrite(ADC_CS, HIGH);
  SPI.begin();
  SPI.beginTransaction(SPISettings(1000, MSBFIRST, SPI_MODE3));
  digitalWrite(ADC_CS, LOW); 

  mesg = 0b00110000; //params: buffer (0b - unipolar binary, 0 - MSB out first, 0 - ?, 11 - 16-bit output length, XXXX - pin command), return size
  mesg |= pin; //bitwise operator

  uint16_t output = SPI.transfer(mesg, 16);
  digitalWrite(ADC_CS, HIGH);

}




//Relay Controller

const int RC_CS = 1;

void setRelayState(int fd, unint8_t relayState){
  uint8_t txBuffer[2] = {0x00,relayState};

  struct spi_ioc_transfer transfer = {
    .txt_buf = (unsigned long)txBuffer,
    .len = sizeOf(txBuffer),
    .speed_hz = SPI_SPEED,
    .bits_per_word = 8,
  };

  if(ioctl(fd, SPI_IOC_MESSAGE(1), &transfer) < 0){
    perror("SPI transfer failed");
  }

}


int relay() {
    int spi_fd;

    // Open SPI device
    spi_fd = open(SPI_DEVICE, O_RDWR);
    if (spi_fd < 0) {
        perror("Error opening SPI device");
        return -1;
    }

    // Set SPI mode and bits per word
    uint8_t mode = SPI_MODE_0;
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("Error setting SPI mode");
        close(spi_fd);
        return -1;
    }

    uint8_t bits = 8;
    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        perror("Error setting SPI bits per word");
        close(spi_fd);
        return -1;
    }

    // Set SPI speed
    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &SPI_SPEED) < 0) {
        perror("Error setting SPI speed");
        close(spi_fd);
        return -1;
    }

    // Enable or disable the relay (Change 0x01 to 0x00 to disable)
    setRelayState(spi_fd, 0x01);

    // Close SPI device
    close(spi_fd);

    return 0;
}



//Temperature Sensor
#include <> //get microchip header file
#device adc=8 
#use delay(clock=4000000)
#fuses NOWDT,XT, PUT, NOPROTECT, BROWNOUT, LVP
#BIT CKP = 0x14.4
#define CS PIN_A1

void temperature(){
  int MSByte,LSByte;
  long int ADC_Temp_Code;
  float TempVal,ADC_Temp_Code_dec;

  setup_spi(spi_master); //this assume the microchip is set up as the Master device
  CKP = 1; //clock is high.
  do{
    delay_ms(10); //Allow time for conversions.
    Output_low(CS); //Pull CS low.
    delay_us(10); //CS to SCLK setup time.
    MSByte = SPI_Read(0); //The first byte is clocked in.
    LSByte = SPI_Read(0); //The second byte is clocked in.
    delay_us(10); //SCLK to CS setup time.
    Output_High(CS); //Bring CS high.
    ADC_Temp_Code = make16(MSByte,LSByte); //16bit ADC code is stored ADC_Temp_Code.
    ADC_Temp_Code_dec = (float)ADC_Temp_Code; //Convert to float for division.
    if ((0x2000 & ADC_Temp_Code) == 0x2000) //Check sign bit for negative value.
    {
    TempVal = (ADC_Temp_Code_dec - 16384)/32; //Conversion formula if negative temperature.
    }
    else
    {
    TempVal = (ADC_Temp_Code_dec/32); //Conversion formula if positive temperature.
    }
  }while(True);
  //Temperature value stored in TempVal.
}


//Voltage Sensing (Multiplexer)

int MUX-A0 = 11;
int MUX-A1 = 12;
int MUX-A2 = 13;
int MUX-A3 = 14;
#include "Mux.h"
using namespace admux; //initialize


/*
 * Creates a Mux instance.
 *
 * 1st argument is the SIG (signal) pin (Arduino analog input pin A0).
 * 2nd argument is the S0-S3 (channel control) pins (Arduino pins 8, 9, 10, 11).
 */
Mux mux(Pin(MUX-A0, INPUT, PinType::Analog),Pinset(8,9,10,11));


void loop() {
  int data;
  for (byte i=0; i<mux.channelCount();i++){
    int16_t data=mux.read(i);
    Serial.print("Potentiometer at channel");
    Serial.print(i);
    Serial.print(" is at ");
    Serial.print((double) (data) * 100/1023);
    Serial.println("%%");
  }
  Serial.println();
  delay(1500);

}
