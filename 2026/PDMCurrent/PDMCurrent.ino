#include <SPI.h>
#include <CANSAME5x.h>

CANSAME5x CAN;

#define ADC_CS A5
#define ADC_EOC 0
#define VREF 3.3
#define ADC_RES 4096
#define SPI_SPEED_ADC 1500000

#define NUM_ADC 11
#define BASE 0x50
#define BAUD_RATE 1000000

#define MESSAGE_LENGTH 8
// #define FLOAT_PER_MESSAGE MESSAGE_LENGTH / sizeof(float)
#define FLOAT_PER_MESSAGE 2
void setup() {
  // put your setup code here, to run once:
  pinMode(ADC_CS, OUTPUT);
  pinMode(ADC_EOC, INPUT);
  Serial.begin(115200);
  while (!Serial) delay(10);

  SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0));

  SPI.begin();
  digitalWrite(ADC_CS, HIGH);  


  // CAN SETUP
  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false);
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true);

  // CAN error message
  if (!CAN.begin(BAUD_RATE)) {
    Serial.println("Starting CAN failed!");
  }

}

void loop() {
  // put your main code here, to run repeatedly:
  float curr = 0;

  for (int i = 0; i < NUM_ADC; ++i) {
    curr = currSense(i);
    
    if (i % FLOAT_PER_MESSAGE == 0) {
      CAN.beginPacket(BASE + i);
      // Serial.println("start");
    }
    
    CAN.write((const uint8_t*) &curr, sizeof(float));

    if ((i % FLOAT_PER_MESSAGE) == (FLOAT_PER_MESSAGE - 1)) {
      CAN.endPacket();
      // Serial.print(i);
      // Serial.print(" ");
      // Serial.print(FLOAT_PER_MESSAGE);
      // Serial.print(" ");
      // Serial.print(i % FLOAT_PER_MESSAGE);
      // Serial.println("output");
    }

    Serial.print("Current ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(curr);
    if (i == NUM_ADC - 1)
    {
      Serial.println();
    }
    else
    {
      Serial.print("\t");
    }

  }
  CAN.endPacket();
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
  mesg |= (pin << 12); // bitwise operator

  // SPI.beginTransaction(SPISettings(SPI_SPEED_ADC, MSBFIRST, SPI_MODE0));
  digitalWrite(ADC_CS, LOW);
  uint16_t output = SPI.transfer16(mesg);
  digitalWrite(ADC_CS, HIGH);
  // SPI.endTransaction();

  output = output >> 4;

  float voltage = output * (VREF / (float)ADC_RES);

  float current = (voltage) * 0.06;
  // return voltage; //* 30.303 - 50; // Linearize
  return current;
  // return output >> 2;
}

