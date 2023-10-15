#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>

RF24 radio(7,8);

const byte address[6] = {000001};

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
}

void loop() {
  if (radio.available()) {
    char text[100] = "";
    radio.read(&text, sizeof(text));
    Serial.println(text);
  }
}
