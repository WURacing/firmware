#include <SPI.h>
#include "printf.h"
#include "RF24.h"

#define CE_PIN 9
#define CSN_PIN 10
#define DELAY_TIME 1000

// instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN);

//this is 1Node
uint8_t address[][6] = { "1Node", "2Node" };

bool role = true; //true == TX

bool radioNumber = 0; //address selector

// For this example, we'll be using a payload containing
// a single float number that will be incremented
// on every successful transmission
float payload = 0.0;

void setup() {
  Serial.begin(115200);

  //radio check
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {}  // hold in infinite loop
  }

  // print example's introductory prompt
  Serial.println(F("RF24/examples/GettingStarted"));

  //POWER LEVEL SET TO LOW TO ATTEMPT TO ELIMINATE NOISE
  radio.setPALevel(RF24_PA_LOW);

  //PAYLOAD SIZE
  radio.setPayloadSize(sizeof(payload));

  // set the TX address of the RX node into the TX pipe
  radio.openWritingPipe(address[radioNumber]);  // always uses pipe 0

  // set the RX address of the TX node into a RX pipe
  radio.openReadingPipe(1, address[!radioNumber]);

  radio.stopListening(); //TX MODE
//  radio.startListening(); //RX MODE

  //DEBUGGING INFO
  printf_begin();             // needed only once for printing details
  // radio.printDetails();       // (smaller) function that prints raw register values
  radio.printPrettyDetails(); // (larger) function that prints human readable data
}

void loop() {
  unsigned long start_timer = micros();                // start the timer
  bool report = radio.write(&payload, sizeof(float));  // transmit & save the report
  unsigned long end_timer = micros();                  // end the timer

  if (report) {
    Serial.print(F("Transmission successful! "));  // payload was delivered
    Serial.print(F("Time to transmit = "));
    Serial.print(end_timer - start_timer);  // print the timer result
    Serial.print(F(" us. Sent: "));
    Serial.println(payload);  // print payload sent
    payload += 0.01;          // increment float payload
  } 
  else {
    Serial.println(F("Transmission failed or timed out"));  // payload was not delivered
  }

  delay(DELAY_TIME);
  
}
