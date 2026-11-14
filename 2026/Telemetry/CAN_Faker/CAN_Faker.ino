#include <CAN.h>

#define BLINK_INTERVAL 1000
#define LED 13
#define starter_val 50

//Delta Time Loop Setup
unsigned long blinkCurrentMillis = millis();
unsigned long blinkPreviousMillis = 0;
bool LEDState = LOW;
int test = 0;
unsigned long current_millis = millis();
int starter = starter_val;
int value = 0;


//short SBCANFrame[29];
//short CBCANFrame[2];
byte CANFrame[8];

void setup() {
  Serial.begin(115200);

  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, false);
  pinMode(PIN_CAN_BOOSTEN, OUTPUT);
  digitalWrite(PIN_CAN_BOOSTEN, true);

  if(!CAN.begin(1000000)){
    Serial.println("Starting CAN failed!");
    while(1);
  }

}

void loop() {
  blink();
  //This is off an old DBC model, but we should be able to switch it to the 17 CAN frame version

  //Generate 4 random shorts
  //Pick a frame from the list
  //Send CAN over

  short short_one = random(0,1000);
  short short_two = random(0,1000);
  short short_three = random(0,1000);
  short short_four = random(0,1000);

  CANFrame[0] = short_one & 0xFF;
  CANFrame[1] = short_one >> 8;
  CANFrame[2] = short_two & 0xFF;
  CANFrame[3] = short_two >> 8;
  CANFrame[4] = short_three & 0xFF;
  CANFrame[5] = short_three >> 8;
  CANFrame[6] = short_four & 0xFF;
  CANFrame[7] = short_four >> 8;

  CAN.beginPacket(0x16);
  for(int i=0; i<8; i++){
    CAN.write(CANFrame[i]);
  }
  CAN.endPacket();
  
  
  
  
  
  }
  
  
  
 





void blink()
{
  blinkCurrentMillis = millis();
  if (blinkCurrentMillis - blinkPreviousMillis >= BLINK_INTERVAL)
  {
    blinkPreviousMillis = blinkCurrentMillis;
    if (LEDState == LOW)
    {
      LEDState = HIGH;
    }
    else
    {
      LEDState = LOW;
    }
    digitalWrite(LED, LEDState);
  }
}

void canWriteShort(short data)
{
  CAN.write(data & 0xFF);
  CAN.write(data >> 8);
}

void canShortFrame(short *send, int i, int Hex)
{
  CAN.beginPacket(Hex);
  for (int j = i; j < i + 4; j++)
  {
    canWriteShort(send[j]);
  }
  CAN.endPacket();
}
