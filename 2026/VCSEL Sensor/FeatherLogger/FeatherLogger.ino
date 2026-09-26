// // Feather M4 Express, Adafruit SAMD core, Arduino USB stack.
// // Binary log: 16-byte header, followed by little-endian <IBB records.
// #include <Wire.h>
// #include <SPI.h>
// #include <Adafruit_VL6180X.h>
// #include <SdFat_Adafruit_Fork.h>
// #include <Adafruit_SPIFlash.h>
// #include "config.h"
// #include <initializer_list>

// Adafruit_FlashTransport_QSPI flashTransport;
// Adafruit_SPIFlash flash(&flashTransport);
// FatVolume fatfs;
// File32 logFile;
// Adafruit_VL6180X vl;
// static_assert(SENSOR_COUNT >= 1 && SENSOR_COUNT <= 8, "Use 1 to 8 channels");

// bool mounted = false, sensorsReady = false, recording = false;
// bool pending[8] = {};
// uint8_t distances[8] = {};
// bool cycleActive = false;
// uint32_t nextTick = 0, cycleTime = 0, originMs = 0;
// uint32_t cycleStartUs = 0, lastPollUs = 0, lastSyncMs = 0;
// uint64_t byteLimit = 0, acceptedBytes = 0;
// uint8_t buffer[504];  // Whole six-byte records only.
// size_t buffered = 0;
// char currentName[13] = "";
// const char *state = "BOOT";

// // Select a channel on the mux at its fixed address, 0x70.
// void tcaselect(uint8_t i) {
//   if (i > 7) return;
//   Wire.beginTransmission(0x70);
//   Wire.write(1 << i);
//   Wire.endTransmission();
// }

// // Checked register operations avoid the driver's unbounded readiness loops.
// bool selectChecked(uint8_t channel) {
//   Wire.beginTransmission(0x70);
//   Wire.write(uint8_t(1 << channel));
//   return Wire.endTransmission() == 0;
// }
// bool writeReg(uint16_t reg, uint8_t value) {
//   Wire.beginTransmission(SENSOR_ADDRESS);
//   Wire.write(uint8_t(reg >> 8)); Wire.write(uint8_t(reg)); Wire.write(value);
//   return Wire.endTransmission() == 0;
// }
// bool readReg(uint16_t reg, uint8_t &value) {
//   Wire.beginTransmission(SENSOR_ADDRESS);
//   Wire.write(uint8_t(reg >> 8)); Wire.write(uint8_t(reg));
//   if (Wire.endTransmission(false)) return false;
//   if (Wire.requestFrom(SENSOR_ADDRESS, uint8_t(1)) != 1) return false;
//   value = Wire.read();
//   return true;
// }
// void put32(uint8_t *p, uint32_t v) {
//   for (uint8_t i = 0; i < 4; ++i) p[i] = uint8_t(v >> (8 * i));
// }
// bool flushBuffer() {
//   if (!logFile) return buffered == 0;
//   if (buffered) {
//     uint32_t before = logFile.curPosition();
//     if (logFile.write(buffer, buffered) != buffered) {
//       // Roll back a short write to the last complete, synced batch.
//       logFile.truncate(before);
//       logFile.sync();
//       buffered = 0;
//       return false;
//     }
//     buffered = 0;
//   }
//   lastSyncMs = millis();
//   return logFile.sync();
// }
// void stopRecording(const char *reason) {
//   recording = false;
//   cycleActive = false;
//   state = reason;
//   if (logFile) {
//     if (!flushBuffer()) state = "WRITE_ERROR";
//     if (!logFile.close()) state = "WRITE_ERROR";
//   }
//   digitalWrite(LED_BUILTIN, LOW);
// }
// bool initSensors() {
//   uint8_t mask = 0;

//   for (uint8_t i = 0; i < SENSOR_COUNT; ++i) {
//     uint8_t ch = SENSOR_CHANNELS[i];

//     Serial.print("Testing channel ");
//     Serial.println(ch);

//     if (ch > 7 || (mask & (1 << ch))) {
//       Serial.println("FAIL: invalid or duplicate channel");
//       return false;
//     }
//     mask |= 1 << ch;

//     if (!selectChecked(ch)) {
//       Serial.println("FAIL: mux channel selection");
//       return false;
//     }

//     // Check whether the sensor responds on this channel.
//     Wire.beginTransmission(SENSOR_ADDRESS);
//     uint8_t err = Wire.endTransmission();

//     Serial.print("Sensor address 0x");
//     Serial.print(SENSOR_ADDRESS, HEX);
//     Serial.print(": ");
//     Serial.println(err == 0 ? "FOUND" : "NOT RESPONDING");
//     if (err != 0) return false;

//     if (!vl.begin(&Wire)) {
//       Serial.println("FAIL: VL6180X driver initialization");
//       return false;
//     }

//     if (!writeReg(0x001C, MAX_CONVERGENCE_MS)) {
//       Serial.println("FAIL: convergence register write");
//       return false;
//     }

//     if (!writeReg(0x0015, 0x07)) {
//       Serial.println("FAIL: interrupt-clear register write");
//       return false;
//     }

//     Serial.println("Channel initialized successfully");
//   }

//   return true;
// }

// bool startRecording() {
//   if (recording) return true;
//   if (!mounted) { state = "NO_FILESYSTEM"; return false; }
//   sensorsReady = initSensors();
//   if (!sensorsReady) { state = "SENSOR_INIT_ERROR"; return false; }
//   Wire.setClock(400000);
//   int32_t clusters = fatfs.freeClusterCount();
//   if (clusters < 0) { state = "FILESYSTEM_ERROR"; return false; }
//   uint64_t freeBytes = uint64_t(clusters) * fatfs.bytesPerCluster();
//   if (freeBytes <= STORAGE_RESERVE_BYTES + 16 + 6 * SENSOR_COUNT) {
//     state = "FULL"; return false;
//   }
//   byteLimit = freeBytes - STORAGE_RESERVE_BYTES;
//   bool found = false;
//   for (uint32_t n = 1; n <= 999999; ++n) {
//     snprintf(currentName, sizeof(currentName), "%06lu.BIN", (unsigned long)n);
//     if (!fatfs.exists(currentName)) { found = true; break; }
//   }
//   if (!found) { state = "NAME_LIMIT"; return false; }
//   logFile = fatfs.open(currentName, O_WRONLY | O_CREAT | O_EXCL);
//   if (!logFile) { state = "OPEN_ERROR"; return false; }
//   uint8_t header[16] = {'V','C','S','L',1,6,0,50,0,0,0,0,0,0,0,0};
//   for (uint8_t i = 0; i < SENSOR_COUNT; ++i) header[6] |= 1 << SENSOR_CHANNELS[i];
//   if (logFile.write(header, sizeof(header)) != sizeof(header) || !logFile.sync()) {
//     stopRecording("WRITE_ERROR"); return false;
//   }
//   buffered = 0; acceptedBytes = sizeof(header);
//   originMs = millis(); lastSyncMs = originMs;
//   cycleActive = false; nextTick = micros();
//   recording = true; state = "RECORDING";
//   digitalWrite(LED_BUILTIN, HIGH);
//   return true;
// }
// void sampleTask() {
//   if (!recording) return;
//   uint32_t now = micros();
//   if (cycleActive) {
//     if (uint32_t(now - cycleStartUs) >= SAMPLE_PERIOD_US) {
//       stopRecording("SENSOR_TIMEOUT"); return;
//     }
//     if (uint32_t(now - lastPollUs) < 500) return;
//     lastPollUs = now;
//     bool allDone = true;
//     for (uint8_t i = 0; i < SENSOR_COUNT; ++i) {
//       if (!pending[i]) continue;
//       uint8_t ready;
//       if (!selectChecked(SENSOR_CHANNELS[i]) || !readReg(0x004F, ready)) {
//         stopRecording("I2C_ERROR"); return;
//       }
//       if (!(ready & 4)) { allDone = false; continue; }
//       if (!readReg(0x0062, distances[i]) || !writeReg(0x0015, 7)) {
//         stopRecording("I2C_ERROR"); return;
//       }
//       pending[i] = false;
//     }
//     if (!allDone) return;
//     // Space was checked for the entire group before triggering.
//     for (uint8_t i = 0; i < SENSOR_COUNT; ++i) {
//       if (buffered + 6 > sizeof(buffer) && !flushBuffer()) {
//         stopRecording("WRITE_ERROR"); return;
//       }
//       put32(buffer + buffered, cycleTime);
//       buffer[buffered + 4] = SENSOR_CHANNELS[i];
//       buffer[buffered + 5] = distances[i];
//       buffered += 6; acceptedBytes += 6;
//     }
//     cycleActive = false;
//     if (buffered == sizeof(buffer) || millis() - lastSyncMs >= 1000) {
//       if (!flushBuffer()) { stopRecording("WRITE_ERROR"); return; }
//     }
//     now = micros();
//   }
//   if (int32_t(now - nextTick) < 0) return;
//   // A late sample is allowed: retain actual timestamps and skip missed slots.
//   // Flash writes can create gaps; the sampling rate remains a 50 Hz target.
//   if (acceptedBytes + 6 * SENSOR_COUNT > byteLimit) {
//     stopRecording("FULL"); return;
//   }
//   // End before a uint32 millisecond timestamp would wrap.
//   if (millis() - originMs >= 0xFFFFFF00UL) {
//     stopRecording("TIME_LIMIT"); return;
//   }
//   cycleTime = millis() - originMs;
//   cycleStartUs = micros();
//   // Schedule from this actual start so delays never cause catch-up bursts.
//   nextTick = cycleStartUs + SAMPLE_PERIOD_US;
//   for (uint8_t i = 0; i < SENSOR_COUNT; ++i) {
//     uint8_t ready;
//     if (!selectChecked(SENSOR_CHANNELS[i]) || !readReg(0x004D, ready)) {
//       stopRecording("I2C_ERROR"); return;
//     }
//     // Only inspect the device-ready bit; range status is not recorded.
//     if (!(ready & 1)) { stopRecording("SENSOR_NOT_READY"); return; }
//     if (!writeReg(0x0018, 1)) { stopRecording("I2C_ERROR"); return; }
//     pending[i] = true;
//   }
//   cycleActive = true;
// }
// bool validName(const char *s) {
//   if (strlen(s) != 10 || strcmp(s + 6, ".BIN")) return false;
//   for (uint8_t i = 0; i < 6; ++i) if (s[i] < '0' || s[i] > '9') return false;
//   return true;
// }

// uint32_t crcUpdate(uint32_t crc, const uint8_t *p, size_t n) {
//   while (n--) {
//     crc ^= *p++;
//     for (uint8_t i = 0; i < 8; ++i) crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320UL : 0);
//   }
//   return crc;
// }

// bool sendBytes(const uint8_t *p, size_t n) {
//   uint32_t lastProgress = millis();
//   while (n) {
//     if (!Serial || millis() - lastProgress > 5000) return false;
//     int room = Serial.availableForWrite();
//     if (room <= 0) { delay(1); continue; }
//     size_t chunk = n < size_t(room) ? n : size_t(room);
//     size_t sent = Serial.write(p, chunk);
//     if (sent) { p += sent; n -= sent; lastProgress = millis(); }
//   }
//   return true;
// }
// void listFiles() {
//   if (!mounted) { Serial.println("ERR NO_FILESYSTEM"); return; }
//   File32 root = fatfs.open("/", O_RDONLY), entry;
//   if (!root) { Serial.println("ERR DIRECTORY"); return; }
//   while (entry.openNext(&root, O_RDONLY)) {
//     char name[32]; entry.getName(name, sizeof(name));
//     if (!entry.isDir() && validName(name)) {
//       Serial.print("FILE "); Serial.print(name); Serial.print(' ');
//       Serial.println(entry.fileSize());
//     }
//     entry.close();
//   }
//   root.close(); Serial.println("END");
// }

// void getFile(const char *name) {
//   if (!mounted || !validName(name)) { Serial.println("ERR FILE"); return; }
//   stopRecording(recording ? "DOWNLOAD" : state);
//   File32 f = fatfs.open(name, O_RDONLY);
//   if (!f) { Serial.println("ERR OPEN"); return; }
//   uint32_t remaining = f.fileSize(), crc = 0xFFFFFFFFUL;
//   Serial.print("DATA "); Serial.println(remaining);
//   uint8_t block[256];
//   while (remaining) {
//     size_t n = remaining < sizeof(block) ? remaining : sizeof(block);
//     if (f.read(block, n) != int(n) || !sendBytes(block, n)) { f.close(); return; }
//     crc = crcUpdate(crc, block, n); remaining -= n;
//   }
//   f.close(); Serial.print("\nCRC "); Serial.println(crc ^ 0xFFFFFFFFUL, HEX);
// }
// void commandTask() {
//   static char command[64]; static size_t used = 0; static bool overflow = false;
//   while (Serial.available()) {
//     char c = Serial.read();
//     if (c == '\r') continue;
//     if (c != '\n') {
//       if (used + 1 < sizeof(command)) command[used++] = c;
//       else overflow = true;
//       continue;
//     }
//     command[used] = 0; used = 0;
//     if (overflow) { overflow = false; Serial.println("ERR COMMAND"); continue; }
//     if (!strcmp(command, "HELLO")) Serial.println("VCSEL_LOGGER 1");
//     else if (!strcmp(command, "INFO")) {
//       Serial.print("STATE "); Serial.print(state); Serial.print(' ');
//       Serial.println(currentName);
//     } else if (!strcmp(command, "STOP")) {
//       stopRecording(recording ? "STOPPED" : state);
//       Serial.print("OK "); Serial.println(state);
//     } else if (!strcmp(command, "START")) {
//       bool ok = startRecording(); Serial.print(ok ? "OK " : "ERR "); Serial.println(state);
//     } else if (!strcmp(command, "LIST")) {
//       if (recording) Serial.println("ERR STOP_FIRST"); else listFiles();
//     } else if (!strncmp(command, "GET ", 4)) getFile(command + 4);
//     else if (!strncmp(command, "DELETE ", 7)) {
//       if (recording) Serial.println("ERR STOP_FIRST");
//       else if (mounted && validName(command + 7) && fatfs.remove(command + 7)) Serial.println("OK DELETED");
//       else Serial.println("ERR DELETE");
//     } else Serial.println("ERR COMMAND");
//   }
// }
// void setup() {
//   // Set mux address inputs to 000 → I2C address 0x70.
//   digitalWrite(4, LOW);
//   digitalWrite(5, LOW);
//   digitalWrite(6, LOW);

//   pinMode(4, OUTPUT);
//   pinMode(5, OUTPUT);
//   pinMode(6, OUTPUT);
//   // Hold mux address inputs LOW, selecting address 0x70.
//   // for (uint8_t pin : {4, 5, 6}) {
//   //   digitalWrite(pin, LOW);
//   //   pinMode(pin, OUTPUT);
//   // }

//   // pinMode(LED_BUILTIN, OUTPUT);
//   // digitalWrite(LED_BUILTIN, LOW);

//   Serial.begin(115200);
//   Wire.begin();
//   Wire.setClock(400000);
//   delay(100);

//   if (!flash.begin()) {
//     state = "NO_FLASH";
//     return;
//   }

//   mounted = fatfs.begin(&flash);
//   if (!mounted) {
//     state = "NO_FILESYSTEM";
//     return;
//   }

//   startRecording();
// }
// void loop() {
//   commandTask();
//   sampleTask();
// }