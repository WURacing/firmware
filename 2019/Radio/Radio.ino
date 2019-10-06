#include <SparkFun_LTE_Shield_Arduino_Library.h>
#include <mcp_can.h>
#include <SPI.h>
#include <avr/wdt.h>
//Arduino Ethernet, LGPL2
#ifndef htonl
#define htonl(x) ( ((x)<<24 & 0xFF000000UL) | \
                   ((x)<< 8 & 0x00FF0000UL) | \
                   ((x)>> 8 & 0x0000FF00UL) | \
                   ((x)>>24 & 0x000000FFUL) )
#endif

typedef struct {
  uint32_t id;
  int32_t ts;
  uint8_t data[8];
  uint8_t checksum;
  char pad[3];
} can_packet_t;

#define OUTGOING_LEN 8
can_packet_t outgoing_buf[OUTGOING_LEN];

SoftwareSerial lteSerial(8, 9);
LTE_Shield lte;
const char URL[] = "178.128.235.225";
const unsigned int PORT = 9999;
unsigned long next_send = 0, tnow;
volatile int mgood = 0;
volatile int updated = 0;
#define TXBUFLEN 30
char txmessage[TXBUFLEN];

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
char msgString[128];    
#define CAN0_INT 2
MCP_CAN CAN0(10);  

void setup() {
  for (int i = 0; i < OUTGOING_LEN; ++i) {
    outgoing_buf[i].id = 0;
    outgoing_buf[i].pad[0] = 'W';
    outgoing_buf[i].pad[1] = 'U';
    outgoing_buf[i].pad[2] = '\n';
  }
  Serial.begin(9600);
  while (!Serial) {
    ;
  }
  if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) != CAN_OK) {
    debug("E2");
    reboot();
  }
  CAN0.setMode(MCP_NORMAL);
  if (!lte.begin(lteSerial, 9600)) {
    debug("E1");
    reboot();
  }
  lte.setSocketCloseCallback(socketCloseCallback);
  debug("S");
}

void loop() {
  int r, target;
  tnow = millis();
  if (!digitalRead(CAN0_INT)) {
    CAN0.readMsgBuf(&rxId, &len, rxBuf);
    // find the slot in the buffer
    for (target = 0; target < OUTGOING_LEN; ++target) {
      if (outgoing_buf[target].id == 0 || outgoing_buf[target].id == rxId)
        break;
    }
    // store the message
    if (target < OUTGOING_LEN) {
      outgoing_buf[target].id = rxId;
      outgoing_buf[target].ts = tnow;
      for (int i = 0; i < len; ++i) {
        outgoing_buf[target].data[i] = rxBuf[i];
      }
    }
    debu(".");
  }
  if (next_send < tnow) {
    next_send = tnow + 250;
    debu("-");
    // find number of packets to send, perform endianness conversion
    for (target = 0; target < OUTGOING_LEN; ++target) {
      if (outgoing_buf[target].id == 0)
        break;
      outgoing_buf[target].checksum = 0;
      outgoing_buf[target].ts = outgoing_buf[target].ts - tnow; // offset ms from current time
      for (int i = 0; i < 8; ++i)
        outgoing_buf[target].checksum ^= outgoing_buf[target].data[i];
      outgoing_buf[target].id = htonl(outgoing_buf[target].id); // little to big
      outgoing_buf[target].ts = htonl(outgoing_buf[target].ts); // little to big
    }
    // send packets if there are any
    if (target > 0) {
      r = sendData((uint8_t *)outgoing_buf, target * sizeof(can_packet_t));
      snprintf(txmessage, TXBUFLEN, "%d", r);
      debug(txmessage);
      if (r == 4) {
        reboot();
      }
    }
    // clear old buffer
    for (target = 0; target < OUTGOING_LEN; ++target) {
      outgoing_buf[target].id = 0;
    }
  }
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
int sendData(const char *buf, unsigned int len) {
  int ret = 0;
  if (socket < 0) {
    socket = lte.socketOpen(LTE_SHIELD_UDP);
    if (socket < 0) {
      return -2;
    }
  }
  ret = lte.socketSendTo(socket, URL, PORT, buf, len);
  if (ret == LTE_SHIELD_SUCCESS) {
    ret = 0;
  }
  return ret;
}

void socketCloseCallback(int s) {
  if (socket == s) {
    socket = -1;
  }
}
