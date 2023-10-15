#include <SPI.h>
#include "printf.h"
#include "RF24.h"

#define CE_PIN 7
#define CSN_PIN 8
#define DELAY_TIME 1000

RF24 radio(CE_PIN, CSN_PIN);

//2NODE
uint8_t address[][6] = { "1Node", "2Node" };

bool role = false; //false == RX

bool radioNumber = 0;



void setup() {
  Serial.begin(115200);

  Serial.println(F("RF24/examples/GettingStarted"));

  //POWER LEVEL SET TO LOW TO ATTEMPT TO ELIMINATE NOISE
  radio.setPALevel(RF24_PA_LOW);

  //PAYLOAD SIZE
  radio.setPayloadSize(sizeof(payload));

  //I think this should stay the same for both files

  // set the TX address of the RX node into the TX pipe
  radio.openWritingPipe(address[radioNumber]);  // always uses pipe 0

  // set the RX address of the TX node into a RX pipe
  radio.openReadingPipe(1, address[!radioNumber]);

  radio.startListening(); //RX MODE

//  printf_begin();             // needed only once for printing details
  // radio.printDetails();       // (smaller) function that prints raw register values
//  radio.printPrettyDetails(); // (larger) function that prints human readable data
}

void loop() {
  uint8_t pipe;
  if (radio.available(&pipe)) {              // is there a payload? get the pipe number that recieved it
    uint8_t bytes = radio.getPayloadSize();  // get the size of the payload
    radio.read(&payload, bytes);             // fetch payload from FIFO
    Serial.print(F("Received "));
    Serial.print(bytes);  // print the size of the payload
    Serial.print(F(" bytes on pipe "));
    Serial.print(pipe);  // print the pipe number
    Serial.print(F(": "));
    Serial.println(payload);  // print the payload's value
  }
}

}
