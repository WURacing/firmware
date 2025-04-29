#include <SPI.h>

#define ADC_CS A5
#define ADC_EOC 0
#define VREF 3.3
#define ADC_RES 4096
#define SPI_SPEED_ADC 1500000


void setup() {
  // put your setup code here, to run once:
  pinMode(ADC_CS, OUTPUT);
  pinMode(ADC_EOC, INPUT);
  Serial.begin(115200);
  while (!Serial) delay(10);
  SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0));


  SPI.begin();
  digitalWrite(ADC_CS, HIGH);  
}

void loop() {
  // put your main code here, to run repeatedly:
  float curr = 0;

  curr = currSense(1);
  Serial.print("Voltage:");
  Serial.println(curr);
  delay(100);
}

float currSense(int pin)
{
  // Wait for conversion to finish
  while (digitalRead(ADC_EOC) == LOW)
  {
    delay(1);
  }
  uint16_t mesg = 0b00001000 << 8;
  mesg |= pin << 12; // bitwise operator

  // SPI.beginTransaction(SPISettings(SPI_SPEED_ADC, MSBFIRST, SPI_MODE0));
  digitalWrite(ADC_CS, LOW);
  uint16_t output = SPI.transfer16(mesg);
  digitalWrite(ADC_CS, HIGH);
  // SPI.endTransaction();

  output = output >> 4;

  float voltage = output * (VREF / (float)ADC_RES);

  float current = (voltage) / 0.60;
  return voltage; //* 30.303 - 50; // Linearize
  // return output >> 2;
}

