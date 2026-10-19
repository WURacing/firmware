#pragma once
#include <math.h>
#include <string.h>
#include <Adafruit_SPIFlashBase.h>

// Append-only flash records: keep the last good save until a new one is verified.
// Same record format/region as the earlier package; no filesystem or extra wiring.
class FuelStorage {
  static_assert(sizeof(double)==8,"Storage requires an eight-byte double");
  Adafruit_SPIFlashBase flash;
  uint32_t base=0, sequence=0, latest=0;
  bool found=false;
  uint8_t stage=0;
  uint32_t target=0,eraseOffset=0,startedAt=0;
  uint8_t record[64];
  static const uint32_t COUNT=2048, RECORD=64, SECTOR_RECORDS=64;
  static uint32_t number(const uint8_t* p) {
    return uint32_t(p[0]) | uint32_t(p[1])<<8 | uint32_t(p[2])<<16 | uint32_t(p[3])<<24;
  }
  static void put(uint8_t* p,uint32_t n) { for(int i=0;i<4;i++) p[i]=uint8_t(n>>(8*i)); }
  static uint32_t crc(const uint8_t* p,unsigned n) {
    uint32_t c=0xffffffff;
    while(n--) { c^=*p++; for(int i=0;i<8;i++) c=(c>>1)^((c&1)?0xedb88320u:0); }
    return ~c;
  }
  static bool blank(const uint8_t* p,unsigned n) { while(n--) if(*p++!=255) return false; return true; }
  bool read(uint32_t slot,uint8_t* p) { return flash.readBuffer(base+slot*RECORD,p,RECORD)==RECORD; }
public:
  bool ok=false;
  double savedLiters=0;
  bool busy() const { return stage!=0; }
  explicit FuelStorage(Adafruit_FlashTransport* transport):flash(transport) {}
  bool begin(double& liters) {
    if(!flash.begin() || flash.size()<COUNT*RECORD || flash.size()%4096) return false;
    base=flash.size()-COUNT*RECORD;
    // Refuse recognizable FAT/exFAT/MBR filesystems even if an old journal also
    // happens to survive in their tail. Never format or erase foreign storage.
    uint8_t boot[512];
    if(flash.readBuffer(0,boot,sizeof(boot))!=sizeof(boot)) return false;
    if((boot[510]==0x55 && boot[511]==0xAA) ||
       memcmp(boot+3,"EXFAT   ",8)==0 || memcmp(boot+54,"FAT",3)==0 ||
       memcmp(boot+82,"FAT",3)==0) return false;
    bool occupied=false;
    uint8_t b[RECORD];
    for(uint32_t i=0;i<COUNT;i++) {
      if(!read(i,b)) return false;
      occupied |= !blank(b,RECORD);
      if(number(b)!=0x4c554654 || number(b+4)!=0x00400001 ||
         number(b+60)!=0x51afc03d || number(b+56)!=crc(b,56)) continue;
      double value; memcpy(&value,b+24,8);
      if(!isfinite(value) || value<0) continue;
      if(!found || int32_t(number(b+8)-sequence)>0) {
        found=true; latest=i; sequence=number(b+8); liters=value;
      }
    }
    if(!found) {
      if(occupied) return false; // never silently zero a damaged or foreign journal
      uint8_t page[256];
      for(uint32_t a=0;a<base;a+=256)
        if(flash.readBuffer(a,page,256)!=256 || !blank(page,256)) return false;
      liters=0;
    }
    savedLiters=liters; ok=true; return true;
  }
  // Request a save; poll() advances it without waiting for flash busy periods.
  bool save(double liters,uint32_t now) {
    if(!ok || busy() || !isfinite(liters) || liters<0) return false;
    target=found ? (latest+1)%COUNT : 0;
    memset(record,255,RECORD);
    put(record,0x4c554654); put(record+4,0x00400001); put(record+8,sequence+1);
    memset(record+12,0,12); memcpy(record+24,&liters,8);
    put(record+56,crc(record,56)); put(record+60,0x51afc03d);
    startedAt=now; stage=1; return true;
  }
  void poll(uint32_t now) {
    if(!ok || !busy()) return;
    if(uint32_t(now-startedAt)>2000) { ok=false; stage=0; return; }
    if(flash.readStatus()&1) return;
    uint8_t check[256];
    bool good=true;
    switch(stage) {
      case 1: // locate a free slot, skipping interrupted writes
        good=read(target,check);
        if(!good) break;
        if(blank(check,RECORD)) stage=3;
        else if(found && target/SECTOR_RECORDS==latest/SECTOR_RECORDS) target=(target+1)%COUNT;
        else {
          target=(target/SECTOR_RECORDS)*SECTOR_RECORDS;
          good=flash.eraseSector((base+target*RECORD)/4096);
          eraseOffset=0; stage=2;
        }
        break;
      case 2: // verify one small chunk per loop after erase completes
        good=flash.readBuffer(base+target*RECORD+eraseOffset,check,256)==256 && blank(check,256);
        eraseOffset+=256;
        if(eraseOffset==4096) stage=3;
        break;
      case 3:
        good=flash.writeBuffer(base+target*RECORD,record,60)==60;
        stage=4; break;
      case 4:
        good=flash.writeBuffer(base+target*RECORD+60,record+60,4)==4;
        stage=5; break;
      case 5:
        good=read(target,check) && memcmp(record,check,RECORD)==0;
        if(good) { latest=target; sequence++; found=true; memcpy(&savedLiters,record+24,8); }
        stage=0; break;
    }
    if(!good) { ok=false; stage=0; }
  }
};
 
