#include <CANSAME5x.h>
#include <math.h>
#include "FuelStorage.h"

// AiM rebroadcasts the decoded engine channels as integers, confirmed by user.
enum WireType { NOT_SET, UINT16_LE, INT16_LE, INT32_LE, FLOAT32_LE };
const WireType RPM_TYPE=UINT16_LE;
const WireType PULSE_TYPE=INT32_LE; // decoded engine channel, four-byte integer output
const double PULSE_VALUE_TO_TRUE_MS=0.1; // paper: logged pulse value 55 corresponds to 5.5 actual ms
// RPM: 0x6A bytes 6-7. FuelOpenTime: 0x6E bytes 4-7.
// Both little endian, output multiplier 1 and offset 0. GPS floats are separate.
// Apply the paper coefficient directly to the received pulse channel value.
const bool AIM_REBROADCAST_VERIFIED=true;
const uint16_t FUEL_CAN_ID=0x33;       // owned by this Feather, confirmed by user
const uint8_t RESET_PIN=5;
const uint16_t FUEL_RESET_CAN_ID=0x6F; // front board: one byte, released=0 / pressed=1
bool frontReset=false,frontResetArmed=false;
uint32_t frontResetAt=0;
const uint32_t RPM_STALE_MS=500, PULSE_STALE_MS=1500, RADIO_MS=200, SAVE_MS=1000;
const double FUEL_CONSTANT=2.5012e-5; // paper's gain per historical logged PW unit * RPM

CANSAME5x CAN;
Adafruit_FlashTransport_QSPI flashTransport;
FuelStorage storage(&flashTransport);
const uint16_t frameIDs[]={0x69,0x6A,0x6B,0x6C,0x6D,0x6E};
uint8_t canData[48]={0},sequence=0;
double rpm=NAN,pulseValue=NAN,liters=0,rate=0,lastSaved=-1;
uint32_t rpmAt=0,pulseAt=0,lastIntegrate=0,lastRadio=0,lastCAN=0,lastSave=0,lastID=0;
bool haveRPM=false,havePulse=false,totalReady=false;
bool resetting=false;
double resetStartLiters=0;

uint32_t read32(const uint8_t* p) {
  return uint32_t(p[0]) | uint32_t(p[1])<<8 | uint32_t(p[2])<<16 | uint32_t(p[3])<<24;
}
void put32(uint8_t* p,uint32_t v) { for(int i=0;i<4;i++) p[i]=uint8_t(v>>(8*i)); }
double decode(const uint8_t* p,WireType type) {
  if(type==UINT16_LE) return uint16_t(p[0]) | uint16_t(p[1])<<8;
  if(type==INT16_LE) { uint16_t n=uint16_t(p[0]) | uint16_t(p[1])<<8; return n&0x8000 ? double(n)-65536 : n; }
  if(type==INT32_LE) { uint32_t n=read32(p); return n&0x80000000u ? double(n)-4294967296.0 : n; }
  if(type==FLOAT32_LE) { uint32_t n=read32(p); float f; memcpy(&f,&n,4); return f; }
  return NAN;
}
uint8_t crc8(const uint8_t* p,unsigned n) {
  uint8_t c=0;
  while(n--) { c^=*p++; for(int i=0;i<8;i++) c=(c&128)?uint8_t((c<<1)^7):uint8_t(c<<1); }
  return c;
}
bool recent(uint32_t now) {
  return haveRPM && havePulse && uint32_t(now-rpmAt)<RPM_STALE_MS && uint32_t(now-pulseAt)<PULSE_STALE_MS;
}
bool valid(uint32_t now) {
  return AIM_REBROADCAST_VERIFIED && recent(now) && isfinite(rpm) && rpm>=0 && rpm<=30000 &&
    isfinite(pulseValue) && pulseValue>=0 && pulseValue*PULSE_VALUE_TO_TRUE_MS<=30 &&
    rpm*pulseValue*PULSE_VALUE_TO_TRUE_MS<=120000;
}
void integrate(uint32_t now) {
  uint32_t dt=now-lastIntegrate;
  // Count the held rate only until the older input expires.
  if(totalReady && valid(lastIntegrate)) {
    uint32_t remain=RPM_STALE_MS-uint32_t(lastIntegrate-rpmAt);
    uint32_t other=PULSE_STALE_MS-uint32_t(lastIntegrate-pulseAt);
    if(remain>other) remain=other;
    if(dt>remain) dt=remain;
    liters += FUEL_CONSTANT*rpm*pulseValue*double(dt)/3600000.0;
  }
  lastIntegrate=now;
  rate=valid(now) ? FUEL_CONSTANT*rpm*pulseValue : 0;
}
void receiveCAN() {
  for(int count=0;count<16;count++) {
    int size=CAN.parsePacket(); if(!size) break;
    int frame=-1;
    for(int i=0;i<6;i++) if(CAN.packetId()==frameIDs[i]) frame=i;
    uint8_t bytes[8]; int n=0;
    bool good=frame>=0 && size==8 && !CAN.packetExtended() && !CAN.packetRtr();
    while(CAN.available()) { int b=CAN.read(); if(n<8) bytes[n++]=uint8_t(b); }
    if(CAN.packetId()==FUEL_RESET_CAN_ID && !CAN.packetExtended() &&
       !CAN.packetRtr() && size==1 && n==1 && bytes[0]<=1) {
      if(bytes[0]==0) frontResetArmed=true; // require a received release before a hold
      frontReset=frontResetArmed && bytes[0]==1;
      frontResetAt=millis();
      continue;
    }
    if(!good || n!=8) {
      if(frame==1) haveRPM=false;
      if(frame==5) havePulse=false;
      continue;
    }
    uint32_t now=millis(); integrate(now);
    memcpy(canData+frame*8,bytes,8);
    if(frame==1) { rpm=decode(bytes+6,RPM_TYPE); rpmAt=now; haveRPM=true; }
    if(frame==5) { pulseValue=decode(bytes+4,PULSE_TYPE); pulseAt=now; havePulse=true; }
  }
}
uint32_t scaled(double value,double factor) {
  if(!isfinite(value) || value<0 || value*factor>=4294967294.5) return UINT32_MAX;
  return uint32_t(value*factor+0.5);
}
void fuelBytes(uint8_t* p) {
  put32(p,scaled(rate,1000));
  put32(p+4,totalReady ? scaled(liters,1000000) : UINT32_MAX);
}
void canFuelBytes(uint8_t* p) {
  // Waiting/stale rate is zero. Clamp before conversion; never wrap.
  uint32_t r=uint32_t(fmin(65535.0,fmax(0.0,round(rate*100.0))));
  uint32_t t=uint32_t(fmin(16777215.0,fmax(0.0,round(liters*1000.0))));
  p[0]=uint8_t(r); p[1]=uint8_t(r>>8);
  p[2]=uint8_t(t); p[3]=uint8_t(t>>8); p[4]=uint8_t(t>>16);
  p[5]=p[6]=p[7]=0;
}
void sendRadio(uint32_t now) {
  // Original packet unchanged; the added packet contains just two values + missing flag.
  uint8_t packet[67];
  packet[0]=0x55; packet[1]=0xAA; packet[2]=sequence;
  memcpy(packet+3,canData,48); packet[51]=crc8(canData,48);
  uint8_t* f=packet+52;
  f[0]=0x55; f[1]=0xAB; f[2]=3; f[3]=10; f[4]=sequence++;
  fuelBytes(f+5); f[13]=valid(now)?0:1; f[14]=crc8(f+2,12);
  Serial1.write(packet,sizeof(packet));
}
void checkReset(uint32_t now) {
  static bool released=false,holding=false,allowed=false;
  static uint32_t holdAt=0,flashAt=0;
  static uint8_t flashes=0;
  static uint16_t period=400;
  if(resetting && !storage.busy()) {
    bool saved=storage.ok;
    if(saved) { liters-=resetStartLiters; lastSaved=0; }
    resetting=false;
    flashAt=now; flashes=saved?3:6; period=saved?400:160;
  }
  static bool first=true,raw=false,stable=false;
  static uint32_t changedAt=0;
  if(uint32_t(now-frontResetAt)>=350) { frontReset=false; frontResetArmed=false; }
  bool input=digitalRead(RESET_PIN)==LOW || frontReset;
  if(first) { raw=stable=input; changedAt=now; first=false; }
  if(input!=raw) { raw=input; changedAt=now; }
  if(uint32_t(now-changedAt)>=50) stable=raw;
  bool low=stable;
  bool stopped=AIM_REBROADCAST_VERIFIED && haveRPM && uint32_t(now-rpmAt)<RPM_STALE_MS && rpm==0;
  if(!low) { if(!raw && uint32_t(now-changedAt)>=50) released=true; holding=false; }
  else if(released) {
    if(!holding) { holding=true; holdAt=now; allowed=true; }
    allowed &= stopped && totalReady && storage.ok && !resetting;
    if(uint32_t(now-holdAt)>=3000) {
      released=false; holding=false;
      resetting=allowed && storage.save(0,now);
      if(resetting) resetStartLiters=liters;
      else { flashAt=now; flashes=6; period=160; }
    }
  }
  now=millis();
  bool lit=false;
  if(flashes && uint32_t(now-flashAt)<uint32_t(flashes)*period) lit=(now-flashAt)%period<period/2;
  else { flashes=0; lit=holding && (now/250)%2==0; }
  digitalWrite(LED_BUILTIN,lit);
}
void setup() {
  Serial1.begin(19200);
  pinMode(RESET_PIN,INPUT_PULLUP); pinMode(LED_BUILTIN,OUTPUT);
  totalReady=storage.begin(liters);
  if(totalReady) lastSaved=storage.savedLiters; // no unchanged checkpoint on every boot
  else { liters=0; totalReady=true; } // allow visible RAM-only counting; never erase foreign flash
  pinMode(PIN_CAN_STANDBY,OUTPUT); digitalWrite(PIN_CAN_STANDBY,LOW);
  pinMode(PIN_CAN_BOOSTEN,OUTPUT); digitalWrite(PIN_CAN_BOOSTEN,HIGH);
  if(!CAN.begin(1000000)) { while(true) { digitalWrite(LED_BUILTIN,HIGH); delay(100); } }
  CAN.filter(0x68,0x7F8);
  Serial1.print("\r\nDE KQ9L\r\n");
  lastIntegrate=lastRadio=lastCAN=lastSave=lastID=millis();
}
void loop() {
  receiveCAN();
  storage.poll(millis()); // one short flash operation; CAN is serviced again next loop
  uint32_t now=millis(); integrate(now); checkReset(now);
  now=millis(); integrate(now);
  if(uint32_t(now-lastCAN)>=100) {
    lastCAN=now;
    uint8_t b[8];
    canFuelBytes(b);
    CAN.beginPacket(FUEL_CAN_ID,8);
    CAN.write(b,8);
    CAN.endPacket();
  }
  if(uint32_t(now-lastRadio)>=RADIO_MS) { lastRadio=now; sendRadio(now); }
  if(uint32_t(now-lastSave)>=SAVE_MS) {
    lastSave=now;
    lastSaved=storage.savedLiters;
    if(totalReady && !resetting && !storage.busy() && liters!=lastSaved) storage.save(liters,now);
  }
  if(uint32_t(now-lastID)>=540000) { lastID=now; Serial1.print("\r\nDE KQ9L\r\n"); }
}
