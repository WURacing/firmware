// Feather M4 (Express or CAN), Adafruit SAMD core, Arduino USB stack.
// VCSEL ride height logger: FeatherLogger + mux/sensor auto-detection and diagnostics.
// Binary log: 16-byte header, followed by little-endian <IBB records
// (uint32 ms timestamp, uint8 channel, uint8 distance mm). Format unchanged.
//
// Serial commands (115200, newline terminated):
//   HELLO, INFO, START, STOP, LIST, GET <name>, DELETE <name>
//   SCAN  - (new) re-detect mux + sensors and print per-channel diagnostics
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_VL6180X.h>
#include <SdFat_Adafruit_Fork.h>
#include <Adafruit_SPIFlash.h>
#include "config.h"

// true  = log every channel where a working VL6180X is found (ignores SENSOR_CHANNELS)
// false = only use the channels listed in config.h SENSOR_CHANNELS
#define AUTO_DETECT_CHANNELS true

// VL6180X registers used directly
#define REG_MODEL_ID          0x0000   // expect 0xB4
#define REG_FRESH_OUT_RESET   0x0016
#define REG_SYSRANGE_START    0x0018
#define REG_MAX_CONVERGENCE   0x001C
#define REG_INT_CLEAR         0x0015
#define REG_RANGE_STATUS      0x004D
#define REG_INT_STATUS        0x004F
#define REG_RANGE_VAL         0x0062
#define VL6180X_MODEL_ID      0xB4

Adafruit_FlashTransport_QSPI flashTransport;
Adafruit_SPIFlash flash(&flashTransport);
FatVolume fatfs;
File32 logFile;
Adafruit_VL6180X vl;
static_assert(SENSOR_COUNT >= 1 && SENSOR_COUNT <= 8, "Use 1 to 8 channels");

bool mounted = false, sensorsReady = false, recording = false;
bool pending[8] = {};
uint8_t distances[8] = {};
bool cycleActive = false;
uint32_t nextTick = 0, cycleTime = 0, originMs = 0;
uint32_t cycleStartUs = 0, lastPollUs = 0, lastSyncMs = 0;
uint64_t byteLimit = 0, acceptedBytes = 0;
uint8_t buffer[504];  // Whole six-byte records only.
size_t buffered = 0;
char currentName[13] = "";
const char *state = "BOOT";

// Detected hardware
uint8_t tcaAddr = 0;             // 0 = mux not found
uint8_t activeChannels[8];       // channels with a working sensor
uint8_t activeCount = 0;

// ------------------------------------------------------------------
// Mux helpers
// ------------------------------------------------------------------

// Find the TCA9548A anywhere in 0x70-0x77 instead of assuming 0x70.
bool findMux() {
  for (uint8_t addr = 0x70; addr <= 0x77; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      tcaAddr = addr;
      Serial.print("Mux found at 0x");
      Serial.println(addr, HEX);
      return true;
    }
  }
  tcaAddr = 0;
  Serial.println("FAIL: mux not found at 0x70-0x77 (check SDA/SCL pullups, power, RESET)");
  return false;
}

bool selectChecked(uint8_t channel) {
  if (tcaAddr == 0 || channel > 7) return false;
  Wire.beginTransmission(tcaAddr);
  Wire.write(uint8_t(1 << channel));
  return Wire.endTransmission() == 0;
}

void deselectAll() {
  if (tcaAddr == 0) return;
  Wire.beginTransmission(tcaAddr);
  Wire.write(uint8_t(0));
  Wire.endTransmission();
}

// ------------------------------------------------------------------
// Checked register operations avoid the driver's unbounded readiness loops.
// ------------------------------------------------------------------
bool writeReg(uint16_t reg, uint8_t value) {
  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(uint8_t(reg >> 8)); Wire.write(uint8_t(reg)); Wire.write(value);
  return Wire.endTransmission() == 0;
}
bool readReg(uint16_t reg, uint8_t &value) {
  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(uint8_t(reg >> 8)); Wire.write(uint8_t(reg));
  if (Wire.endTransmission(false)) return false;
  if (Wire.requestFrom(SENSOR_ADDRESS, uint8_t(1)) != 1) return false;
  value = Wire.read();
  return true;
}

// ------------------------------------------------------------------
// Sensor detection with a specific reason for every failure
// ------------------------------------------------------------------
// Plain uint8_t codes (not an enum type) because the Arduino IDE auto-generates
// function prototypes above any type definitions in the sketch.
#define PROBE_OK          0
#define PROBE_SELECT_FAIL 1   // mux did not ACK the channel-select write
#define PROBE_NO_ACK      2   // nothing at SENSOR_ADDRESS on this channel
#define PROBE_READ_FAIL   3   // address ACKs, but a register read fails
#define PROBE_BAD_ID      4   // register read works, but model ID is not 0xB4

uint8_t probeChannel(uint8_t ch, uint8_t &modelId) {
  modelId = 0;
  if (!selectChecked(ch)) return PROBE_SELECT_FAIL;
  Wire.beginTransmission(SENSOR_ADDRESS);
  if (Wire.endTransmission() != 0) return PROBE_NO_ACK;
  if (!readReg(REG_MODEL_ID, modelId)) return PROBE_READ_FAIL;
  if (modelId != VL6180X_MODEL_ID) return PROBE_BAD_ID;
  return PROBE_OK;
}

void printProbe(uint8_t r, uint8_t modelId) {
  switch (r) {
    case PROBE_OK:          Serial.print("VL6180X OK (model 0xB4)"); break;
    case PROBE_SELECT_FAIL: Serial.print("FAIL: mux did not ACK channel select"); break;
    case PROBE_NO_ACK:      Serial.print("empty (no ACK at sensor address)"); break;
    case PROBE_READ_FAIL:   Serial.print("FAIL: sensor ACKs address but register read failed "
                                         "(signal levels / pullups / wiring)"); break;
    case PROBE_BAD_ID:      Serial.print("FAIL: model ID read as 0x");
                            Serial.print(modelId, HEX);
                            Serial.print(", expected 0xB4 (corrupt reads or wrong part)"); break;
  }
}

// Detects the mux, finds sensors, and initializes each one step by step.
// Sensors that fail are skipped (with the reason printed); at least one must work.
bool initSensors() {
  activeCount = 0;
  Serial.println("---- Sensor scan ----");
  if (!findMux()) return false;

  uint8_t candidates[8];
  uint8_t candidateCount = 0;
#if AUTO_DETECT_CHANNELS
  for (uint8_t ch = 0; ch < 8; ++ch) candidates[candidateCount++] = ch;
#else
  uint8_t mask = 0;
  for (uint8_t i = 0; i < SENSOR_COUNT; ++i) {
    uint8_t ch = SENSOR_CHANNELS[i];
    if (ch > 7 || (mask & (1 << ch))) {
      Serial.print("Config error: invalid or duplicate channel ");
      Serial.println(ch);
      continue;
    }
    mask |= 1 << ch;
    candidates[candidateCount++] = ch;
  }
#endif

  for (uint8_t i = 0; i < candidateCount; ++i) {
    uint8_t ch = candidates[i];
    uint8_t modelId;
    uint8_t r = probeChannel(ch, modelId);

    Serial.print("CH"); Serial.print(ch); Serial.print(": ");
    printProbe(r, modelId);

    if (r != PROBE_OK) {
      Serial.println();
      continue;
    }

    uint8_t fresh = 0xFF;
    readReg(REG_FRESH_OUT_RESET, fresh);

    // Driver begin loads ST's recommended settings if the sensor is fresh out of reset.
    // It re-checks the model ID, so a failure here after a good probe is itself a clue.
    if (!selectChecked(ch) || !vl.begin(&Wire)) {
      Serial.print(" -> FAIL: driver begin() (fresh_out_of_reset=0x");
      Serial.print(fresh, HEX);
      Serial.println(")");
      continue;
    }
    if (!writeReg(REG_MAX_CONVERGENCE, MAX_CONVERGENCE_MS)) {
      Serial.println(" -> FAIL: convergence register write");
      continue;
    }
    if (!writeReg(REG_INT_CLEAR, 0x07)) {
      Serial.println(" -> FAIL: interrupt-clear register write");
      continue;
    }

    activeChannels[activeCount++] = ch;
    Serial.println(" -> initialized, logging");
  }

  deselectAll();

  Serial.print("Active channels: ");
  if (activeCount == 0) Serial.print("none");
  for (uint8_t i = 0; i < activeCount; ++i) {
    Serial.print(activeChannels[i]);
    Serial.print(i + 1 < activeCount ? ", " : "");
  }
  Serial.println();
  Serial.println("---------------------");
  return activeCount > 0;
}

// ------------------------------------------------------------------
// Logging (FeatherLogger, using the detected channel list)
// ------------------------------------------------------------------
void put32(uint8_t *p, uint32_t v) {
  for (uint8_t i = 0; i < 4; ++i) p[i] = uint8_t(v >> (8 * i));
}
bool flushBuffer() {
  if (!logFile) return buffered == 0;
  if (buffered) {
    uint32_t before = logFile.curPosition();
    if (logFile.write(buffer, buffered) != buffered) {
      // Roll back a short write to the last complete, synced batch.
      logFile.truncate(before);
      logFile.sync();
      buffered = 0;
      return false;
    }
    buffered = 0;
  }
  lastSyncMs = millis();
  return logFile.sync();
}
void stopRecording(const char *reason) {
  recording = false;
  cycleActive = false;
  state = reason;
  if (logFile) {
    if (!flushBuffer()) state = "WRITE_ERROR";
    if (!logFile.close()) state = "WRITE_ERROR";
  }
  digitalWrite(LED_BUILTIN, LOW);
}

bool startRecording() {
  if (recording) return true;
  if (!mounted) { state = "NO_FILESYSTEM"; return false; }
  sensorsReady = initSensors();
  if (!sensorsReady) { state = "SENSOR_INIT_ERROR"; return false; }
  Wire.setClock(400000);
  int32_t clusters = fatfs.freeClusterCount();
  if (clusters < 0) { state = "FILESYSTEM_ERROR"; return false; }
  uint64_t freeBytes = uint64_t(clusters) * fatfs.bytesPerCluster();
  if (freeBytes <= STORAGE_RESERVE_BYTES + 16 + 6 * activeCount) {
    state = "FULL"; return false;
  }
  byteLimit = freeBytes - STORAGE_RESERVE_BYTES;
  bool found = false;
  for (uint32_t n = 1; n <= 999999; ++n) {
    snprintf(currentName, sizeof(currentName), "%06lu.BIN", (unsigned long)n);
    if (!fatfs.exists(currentName)) { found = true; break; }
  }
  if (!found) { state = "NAME_LIMIT"; return false; }
  logFile = fatfs.open(currentName, O_WRONLY | O_CREAT | O_EXCL);
  if (!logFile) { state = "OPEN_ERROR"; return false; }
  uint8_t header[16] = {'V','C','S','L',1,6,0,50,0,0,0,0,0,0,0,0};
  for (uint8_t i = 0; i < activeCount; ++i) header[6] |= 1 << activeChannels[i];
  if (logFile.write(header, sizeof(header)) != sizeof(header) || !logFile.sync()) {
    stopRecording("WRITE_ERROR"); return false;
  }
  buffered = 0; acceptedBytes = sizeof(header);
  originMs = millis(); lastSyncMs = originMs;
  cycleActive = false; nextTick = micros();
  recording = true; state = "RECORDING";
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.print("Recording to "); Serial.println(currentName);
  return true;
}

void sampleTask() {
  if (!recording) return;
  uint32_t now = micros();
  if (cycleActive) {
    if (uint32_t(now - cycleStartUs) >= SAMPLE_PERIOD_US) {
      stopRecording("SENSOR_TIMEOUT"); return;
    }
    if (uint32_t(now - lastPollUs) < 500) return;
    lastPollUs = now;
    bool allDone = true;
    for (uint8_t i = 0; i < activeCount; ++i) {
      if (!pending[i]) continue;
      uint8_t ready;
      if (!selectChecked(activeChannels[i]) || !readReg(REG_INT_STATUS, ready)) {
        stopRecording("I2C_ERROR"); return;
      }
      if (!(ready & 4)) { allDone = false; continue; }
      if (!readReg(REG_RANGE_VAL, distances[i]) || !writeReg(REG_INT_CLEAR, 7)) {
        stopRecording("I2C_ERROR"); return;
      }
      pending[i] = false;
    }
    if (!allDone) return;
    // Space was checked for the entire group before triggering.
    for (uint8_t i = 0; i < activeCount; ++i) {
      if (buffered + 6 > sizeof(buffer) && !flushBuffer()) {
        stopRecording("WRITE_ERROR"); return;
      }
      put32(buffer + buffered, cycleTime);
      buffer[buffered + 4] = activeChannels[i];
      buffer[buffered + 5] = distances[i];
      buffered += 6; acceptedBytes += 6;
    }
    cycleActive = false;
    if (buffered == sizeof(buffer) || millis() - lastSyncMs >= 1000) {
      if (!flushBuffer()) { stopRecording("WRITE_ERROR"); return; }
    }
    now = micros();
  }
  if (int32_t(now - nextTick) < 0) return;
  // A late sample is allowed: retain actual timestamps and skip missed slots.
  // Flash writes can create gaps; the sampling rate remains a 50 Hz target.
  if (acceptedBytes + 6 * activeCount > byteLimit) {
    stopRecording("FULL"); return;
  }
  // End before a uint32 millisecond timestamp would wrap.
  if (millis() - originMs >= 0xFFFFFF00UL) {
    stopRecording("TIME_LIMIT"); return;
  }
  cycleTime = millis() - originMs;
  cycleStartUs = micros();
  // Schedule from this actual start so delays never cause catch-up bursts.
  nextTick = cycleStartUs + SAMPLE_PERIOD_US;
  for (uint8_t i = 0; i < activeCount; ++i) {
    uint8_t ready;
    if (!selectChecked(activeChannels[i]) || !readReg(REG_RANGE_STATUS, ready)) {
      stopRecording("I2C_ERROR"); return;
    }
    // Only inspect the device-ready bit; range status is not recorded.
    if (!(ready & 1)) { stopRecording("SENSOR_NOT_READY"); return; }
    if (!writeReg(REG_SYSRANGE_START, 1)) { stopRecording("I2C_ERROR"); return; }
    pending[i] = true;
  }
  cycleActive = true;
}

// ------------------------------------------------------------------
// File transfer / commands (unchanged except SCAN and INFO)
// ------------------------------------------------------------------
bool validName(const char *s) {
  if (strlen(s) != 10 || strcmp(s + 6, ".BIN")) return false;
  for (uint8_t i = 0; i < 6; ++i) if (s[i] < '0' || s[i] > '9') return false;
  return true;
}

uint32_t crcUpdate(uint32_t crc, const uint8_t *p, size_t n) {
  while (n--) {
    crc ^= *p++;
    for (uint8_t i = 0; i < 8; ++i) crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320UL : 0);
  }
  return crc;
}

bool sendBytes(const uint8_t *p, size_t n) {
  uint32_t lastProgress = millis();
  while (n) {
    if (!Serial || millis() - lastProgress > 5000) return false;
    int room = Serial.availableForWrite();
    if (room <= 0) { delay(1); continue; }
    size_t chunk = n < size_t(room) ? n : size_t(room);
    size_t sent = Serial.write(p, chunk);
    if (sent) { p += sent; n -= sent; lastProgress = millis(); }
  }
  return true;
}
void listFiles() {
  if (!mounted) { Serial.println("ERR NO_FILESYSTEM"); return; }
  File32 root = fatfs.open("/", O_RDONLY), entry;
  if (!root) { Serial.println("ERR DIRECTORY"); return; }
  while (entry.openNext(&root, O_RDONLY)) {
    char name[32]; entry.getName(name, sizeof(name));
    if (!entry.isDir() && validName(name)) {
      Serial.print("FILE "); Serial.print(name); Serial.print(' ');
      Serial.println(entry.fileSize());
    }
    entry.close();
  }
  root.close(); Serial.println("END");
}

void getFile(const char *name) {
  if (!mounted || !validName(name)) { Serial.println("ERR FILE"); return; }
  stopRecording(recording ? "DOWNLOAD" : state);
  File32 f = fatfs.open(name, O_RDONLY);
  if (!f) { Serial.println("ERR OPEN"); return; }
  uint32_t remaining = f.fileSize(), crc = 0xFFFFFFFFUL;
  Serial.print("DATA "); Serial.println(remaining);
  uint8_t block[256];
  while (remaining) {
    size_t n = remaining < sizeof(block) ? remaining : sizeof(block);
    if (f.read(block, n) != int(n) || !sendBytes(block, n)) { f.close(); return; }
    crc = crcUpdate(crc, block, n); remaining -= n;
  }
  f.close(); Serial.print("\nCRC "); Serial.println(crc ^ 0xFFFFFFFFUL, HEX);
}

void commandTask() {
  static char command[64]; static size_t used = 0; static bool overflow = false;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (used + 1 < sizeof(command)) command[used++] = c;
      else overflow = true;
      continue;
    }
    command[used] = 0; used = 0;
    if (overflow) { overflow = false; Serial.println("ERR COMMAND"); continue; }
    if (!strcmp(command, "HELLO")) Serial.println("VCSEL_LOGGER 1");
    else if (!strcmp(command, "INFO")) {
      Serial.print("STATE "); Serial.print(state); Serial.print(' ');
      Serial.print(currentName);
      Serial.print(" MUX 0x"); Serial.print(tcaAddr, HEX);
      Serial.print(" CH");
      for (uint8_t i = 0; i < activeCount; ++i) { Serial.print(' '); Serial.print(activeChannels[i]); }
      Serial.println();
    } else if (!strcmp(command, "SCAN")) {
      if (recording) Serial.println("ERR STOP_FIRST");
      else {
        sensorsReady = initSensors();
        Serial.print(sensorsReady ? "OK " : "ERR "); Serial.println(activeCount);
      }
    } else if (!strcmp(command, "STOP")) {
      stopRecording(recording ? "STOPPED" : state);
      Serial.print("OK "); Serial.println(state);
    } else if (!strcmp(command, "START")) {
      bool ok = startRecording(); Serial.print(ok ? "OK " : "ERR "); Serial.println(state);
    } else if (!strcmp(command, "LIST")) {
      if (recording) Serial.println("ERR STOP_FIRST"); else listFiles();
    } else if (!strncmp(command, "GET ", 4)) getFile(command + 4);
    else if (!strncmp(command, "DELETE ", 7)) {
      if (recording) Serial.println("ERR STOP_FIRST");
      else if (mounted && validName(command + 7) && fatfs.remove(command + 7)) Serial.println("OK DELETED");
      else Serial.println("ERR DELETE");
    } else Serial.println("ERR COMMAND");
  }
}

void setup() {
  // Hold mux address inputs LOW (A2 A1 A0 = 000 -> 0x70).
  // NOTE: on the Feather M4 CAN, D4 is also the CAN boost enable. Driving it LOW is
  // fine here because this sketch doesn't use CAN; it will conflict if CAN is added later.
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

  // Needed for the recording LED to actually drive the pin.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  // Give a connected serial monitor up to 3 s to attach so boot diagnostics are visible.
  // On the car (no USB host) this just times out and continues.
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}

  Wire.begin();
  Wire.setClock(400000);
  delay(100);

  if (!flash.begin()) {
    state = "NO_FLASH";
    Serial.println("FAIL: QSPI flash not found");
    return;
  }

  mounted = fatfs.begin(&flash);
  if (!mounted) {
    state = "NO_FILESYSTEM";
    Serial.println("FAIL: no FAT filesystem on flash");
    return;
  }

  if (!startRecording()) {
    Serial.print("Not recording: "); Serial.println(state);
  }
}

void loop() {
  commandTask();
  sampleTask();
}
