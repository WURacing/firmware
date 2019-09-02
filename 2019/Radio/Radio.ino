#include <SparkFun_LTE_Shield_Arduino_Library.h>
#include <Canbus.h>
#include <defaults.h>
#include <global.h>
#include <mcp2515.h>
#include <mcp2515_defs.h>
#include <avr/wdt.h>

SoftwareSerial lteSerial(8, 9);
LTE_Shield lte;
const char URL[] = "35.162.212.169";
const unsigned int PORT = 10000;
tCAN message;
unsigned long next_send = 0, tnow;
volatile int mgood = 0;
volatile int updated = 0;
#define TXBUFLEN 30
char txmessage[TXBUFLEN];

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ;
  }
  if (!Canbus.init(CANSPEED_500)) {
    debug("E2");
    reboot();
  }
  if (!lte.begin(lteSerial, 9600)) {
    debug("E1");
    reboot();
  }
  lte.setSocketCloseCallback(socketCloseCallback);
  debug("S");
}

void loop() {
  int r;
  tnow = millis();
  if (mcp2515_check_message()) {
    if (mcp2515_get_message(&message)) {
      mgood = 1;
      debu(".");
    }
  }
  if (next_send < tnow) {
    next_send = tnow + 250;
    debu("-");
    if (mgood) {
      snprintf(txmessage, TXBUFLEN, "WU%08x,%02x%02x%02x%02x%02x%02x%02x%02x", message.id, message.data[0], message.data[1], message.data[2], message.data[3], message.data[4], message.data[5], message.data[6], message.data[7]);
      debug(txmessage);
      r = sendData(txmessage);
      snprintf(txmessage, TXBUFLEN, "%d", r);
      debug(txmessage);
    }
    mgood = 0;
  }
//  debu("#");
  updated = lte.poll();
}

void reboot() {
  debug("R");
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {}
}

void debu(const char *msg) {
  Serial.print(msg);
}
void debug(const char *msg) {
  Serial.println(msg);
}

volatile int socket = -1;
int sendData(const char *msg) {
  int ret = 0;
  if (socket < 0) {
    socket = lte.socketOpen(LTE_SHIELD_UDP);
    if (socket < 0) {
      return -2;
    }
  }
  ret = lte.socketSendTo(socket, URL, PORT, msg);
  if (ret == LTE_SHIELD_SUCCESS) {
    ret = 0;
  } else {
    //ret = 2;
//    if (lte.socketClose(socket) == LTE_SHIELD_SUCCESS)
//    socket = -1;
  }
  return ret;
}

void socketCloseCallback(int s) {
  if (socket == s) {
    socket = -1;
  }
}
