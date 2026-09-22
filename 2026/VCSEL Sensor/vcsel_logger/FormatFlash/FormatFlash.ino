// One-time FAT12 formatter for the Feather M4 Express QSPI data flash.
// Formatting requires the exact confirmation line ERASE. Do not use on logs
// you want to preserve. Based on Adafruit SPIFlash's SdFat_format example.
#include <SPI.h>
#include <SdFat_Adafruit_Fork.h>
#include <Adafruit_SPIFlash.h>
// #include "ff.h"
// #include "diskio.h"

Adafruit_FlashTransport_QSPI transport;
Adafruit_SPIFlash flash(&transport);
FatVolume volume;

extern "C" {
DSTATUS disk_status(BYTE drive) { (void)drive; return 0; }
DSTATUS disk_initialize(BYTE drive) { (void)drive; return 0; }
DRESULT disk_read(BYTE drive, BYTE *data, DWORD sector, UINT count) {
  (void)drive; return flash.readBlocks(sector, data, count) ? RES_OK : RES_ERROR;
}
DRESULT disk_write(BYTE drive, const BYTE *data, DWORD sector, UINT count) {
  (void)drive; return flash.writeBlocks(sector, data, count) ? RES_OK : RES_ERROR;
}
DRESULT disk_ioctl(BYTE drive, BYTE cmd, void *data) {
  (void)drive;
  switch (cmd) {
    case CTRL_SYNC: return flash.syncBlocks() ? RES_OK : RES_ERROR;
    case GET_SECTOR_COUNT: *static_cast<DWORD *>(data) = flash.size() / 512; return RES_OK;
    case GET_SECTOR_SIZE: *static_cast<WORD *>(data) = 512; return RES_OK;
    case GET_BLOCK_SIZE: *static_cast<DWORD *>(data) = 8; return RES_OK;
    default: return RES_PARERR;
  }
}
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(100);
  if (!flash.begin()) { Serial.println("Flash initialization failed."); return; }
  Serial.println("This erases ALL QSPI data flash files, including existing logs.");
  Serial.println("Send exactly ERASE followed by newline to format. Otherwise unplug.");
  Serial.setTimeout(1000);
  while (true) {
    if (!Serial.available()) { delay(10); continue; }
    String line = Serial.readStringUntil('\n'); line.trim();
    if (line == "ERASE") break;
    Serial.println("Not confirmed. Send ERASE to format.");
  }
  uint8_t work[4096]; FATFS fs;
  Serial.println("Formatting...");
  FRESULT result = f_mkfs("", FM_FAT, 0, work, sizeof(work));
  if (result != FR_OK) { Serial.print("Format failed: "); Serial.println(int(result)); return; }
  if (f_mount(&fs, "0:", 1) == FR_OK) {
    f_setlabel("VCSEL_LOGS");
    f_unmount("0:");
  }
  flash.syncBlocks();
  Serial.println(volume.begin(&flash) ? "Ready. Upload FeatherLogger now." : "Mount check failed.");
}
void loop() {}
