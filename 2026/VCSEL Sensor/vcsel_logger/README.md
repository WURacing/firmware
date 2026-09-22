# Feather M4 VCSEL logger + Windows downloader

## What is included

- `FeatherLogger/FeatherLogger.ino` and `config.h`: battery-powered QSPI logging.
- `FormatFlash/FormatFlash.ino`: separately uploaded, explicitly confirmed one-time formatter.
- `windows/logger.py`: USB downloader, CSV conversion, watch mode, start/stop and explicit deletion.
- `windows/install.bat`, `watch.bat`, `requirements.txt`, `positions.json`: Windows setup.
- `windows/test_logger.py`: desktop format/transfer tests.

This targets the **Adafruit Feather M4 Express** with onboard QSPI data flash,
the Adafruit SAMD Arduino core, and a TCA9548A-compatible mux at `0x70`.
The default is one VL6180/VL6180X-compatible sensor on **SD1/SC1 (channel 1)**,
at I2C address `0x29`. Sensor initialization uses Adafruit_VL6180X; subsequent
checked register accesses avoid the library's unbounded `readRange()` wait.

Data records contain a **one-byte distance**, no range-status field, and no
session field. CSV columns are `time_ms,channel,position,distance_mm`.
Separate files distinguish recordings; time starts at zero for each new file.
The firmware begins recording at power-up without waiting for USB.

## 1. Arduino setup

1. In Arduino IDE, install **Adafruit SAMD Boards** through Boards Manager.
   If not already configured, add Adafruit's board package URL in Preferences:
   `https://adafruit.github.io/arduino-board-index/package_adafruit_index.json`.
2. Select **Adafruit Feather M4 Express**. If a USB stack option is shown,
   select the **Arduino** USB stack for this sketch.
3. Install **Adafruit SPIFlash**, **SdFat - Adafruit Fork**,
   **Adafruit VL6180X**, and **Adafruit BusIO** through Library Manager,
   accepting their dependencies. The include is `SdFat_Adafruit_Fork.h`, not
   a different SdFat library. SPIFlash supplies the formatter's `ff.h`/`diskio.h`.
4. Connect Feather SDA/SCL to the mux's upstream SDA/SCL, plus power and common
   ground. Your current sensor's SDA/SCL connect to SD1/SC1. Use the voltage
   required by your particular breakout; Feather logic is 3.3 V.
5. On a new/unformatted data flash, upload `FormatFlash/FormatFlash.ino`.
   Open Serial Monitor at 115200 with Newline, read the warning and send
   `ERASE` only if you intend to remove ALL existing data-flash files.
   Wait for `Ready. Upload FeatherLogger now.` Skip formatting when a working
   FAT filesystem already exists. The logger never formats automatically.
6. Open and upload `FeatherLogger/FeatherLogger.ino`. Keep `config.h` in the
   same sketch directory. Close Serial Monitor before the downloader uses COM.

The built-in LED is on while recording and off when stopped. No console
printing is performed in the acquisition loop.

## 2. Windows setup and automatic download

1. Install Python **3.10 or newer**, including the `py` launcher.
2. Extract the whole ZIP to a normal writable folder (do not run inside ZIP).
3. Double-click `windows/install.bat` once. It creates a private virtual
   environment and installs pyserial 3.5.
4. Double-click `windows/watch.bat` and leave its window running.
5. Plug in the Feather using a USB data cable. The watcher checks Adafruit USB
   ports, verifies the logger handshake, stops and flushes recording, downloads
   every log, verifies the transfer CRC32, and saves `.bin` and `.csv` files
   under **Desktop/VCSEL_Logs**. Windows' Desktop API handles redirected and
   OneDrive Desktop folders.

The watcher must be running for connection-triggered downloads; the Feather
cannot itself write to a Windows folder. It handles a board once per observed
connection. A subsequent reconnection triggers another download; identical
filename/content pairs reuse the same output paths instead of making duplicates.
An extremely fast unplug/replug within the one-second scan interval may require
restarting the watcher or using `download` manually.

Recording remains stopped after download. If USB was the only power source,
the next power-up starts a new file. If a battery keeps the board powered,
close the watcher and issue `start` manually or press Reset after unplugging.
Do not leave the watcher active while using other programs on the same COM port.

Output basenames include the onboard filename and a content hash, not a CSV
session column. Onboard copies are **never automatically deleted**.

## 3. Switch to four sensors

In `FeatherLogger/config.h`, change:

```cpp
constexpr uint8_t SENSOR_CHANNELS[] = {1};
```

to, for example:

```cpp
constexpr uint8_t SENSOR_CHANNELS[] = {1, 2, 3, 4};
```

Connect each sensor to its own mux channel. Then edit `windows/positions.json`
to match your actual wiring, for example:

```json
{"1":"front_left","2":"front_right","3":"rear_left","4":"rear_right"}
```

These labels describe wiring positions, not factory sensor serial numbers.
The default neutral names avoid assuming your production locations. All
configured sensors must be connected; an initialization failure stops logging
and is reported over the command interface.

## 4. Sampling and parallel behavior

The target is **50 Hz per sensor**, with one group of measurements triggered
every 20,000 microseconds. The mux briefly selects each sensor and starts a
single-shot measurement, then polls each sensor for completion. Optical
measurements overlap; I2C commands and reads are serialized, so the sensors
are not perfectly synchronized. All records in a group use the timestamp
immediately before that group's first trigger, not individual completion times.

Maximum convergence is set to 14 ms to leave room in the 20 ms interval.
This can cause invalid distance values under difficult conditions; per your
request, range-error status is neither exported nor used to filter distances.
Every completed measurement's raw 0-255 mm byte is recorded. A value such as
255 is not a reserved missing-data marker and does not guarantee valid range.

The implementation never calls blocking `readRange()` for four sensors in
sequence. It still relies on the board's Wire implementation for individual
I2C transactions; it does not add hardware recovery for a physically stuck bus.

50 Hz is a configured target, **not a bench-verified guarantee**. If measurement
completion takes a full 20 ms, recording stops with `SENSOR_TIMEOUT`. If the
main loop loses an entire sample interval (for example during a long flash
write), it stops with `TIMING_OVERRUN` instead of silently claiming 50 Hz or
duplicating readings. Some sub-period timing jitter is possible. Verify CSV
timestamp differences and stop reasons on the actual hardware before a run.
Flash erase/sync latency may require a more advanced storage strategy for
strict uninterrupted 50 Hz operation; this package does not claim hard-real-time
flash writing. Sensor optical cross-talk also depends on physical placement.

## 5. Storage and graceful full handling

Each record is **6 bytes**: four-byte timestamp + one-byte channel + one-byte
distance. There is one 16-byte header per file.

| Configuration | Payload rate | Payload per minute |
|---|---:|---:|
| One sensor at 50 Hz | 300 B/s | 18,000 B |
| Four sensors at 50 Hz | 1,200 B/s | 72,000 B |

For illustration, 2 MiB divided by those rates is about 116.5 minutes or
29.1 minutes respectively, **before** filesystem overhead, reserved space,
other files, and headers. The firmware measures actual free clusters.

At the start of a file, it establishes a capacity budget from free space and
reserves 8 KiB for allocation/metadata headroom. Before triggering a group it
checks that the entire group's records fit. At capacity it flushes buffered
records, synchronizes/closes the file, sets `FULL`, and keeps USB commands
available. It never wraps around, overwrites old logs, or reformats to make room.
The 8 KiB reserve is intentional; `FULL` can occur before the disk reaches zero.

Writes use a 504-byte buffer and synchronize at least once per second while
data is flowing. A short write attempts to truncate back to the last complete
batch and stops with `WRITE_ERROR`. Files are newly created with exclusive
names `000001.BIN`, `000002.BIN`, etc. Restarting does not append new zero-based
timestamps to an older log.

Graceful full handling is distinct from unexpected power removal. Cutting
power during a flash/filesystem update can lose recent readings or damage the
filesystem. Issue STOP and wait for its response before planned power-off.
The last in-flight, incomplete measurement group is discarded on STOP.

## 6. Commands

Run from the extracted `windows` folder; replace COM5 with the Arduino IDE port.
Close `watch.bat` and Serial Monitor first.

```bat
.venv\Scripts\python.exe logger.py status --port COM5
.venv\Scripts\python.exe logger.py stop --port COM5
.venv\Scripts\python.exe logger.py download --port COM5
.venv\Scripts\python.exe logger.py start --port COM5
.venv\Scripts\python.exe logger.py watch --port COM5 --output "D:\RideHeight"
.venv\Scripts\python.exe logger.py convert --input "C:\path\recording.bin"
```

After confirming your saved files, explicitly remove a named onboard file:

```bat
.venv\Scripts\python.exe logger.py delete --port COM5 --name 000001.BIN
```

This deletion is irreversible on the board; the command does not automatically
check for your desktop backup. It stops recording first and removes only the
exact named file. Freeing space does not automatically restart recording.

`status` returns the stop reason and current/last onboard filename. Typical
states: `RECORDING`, `STOPPED`, `DOWNLOAD`, `FULL`, `NO_FLASH`, `NO_FILESYSTEM`,
`SENSOR_INIT_ERROR`, `SENSOR_TIMEOUT`, `SENSOR_NOT_READY`, `I2C_ERROR`,
`TIMING_OVERRUN`, `WRITE_ERROR`. A read failure during transfer times out on
the PC, which keeps onboard data and retries on the next attempt.

## 7. Binary layout and protocol

Header: `VCSL` (4 ASCII bytes), version 1, record size 6, channel bitmask,
sample frequency 50, eight reserved zero bytes. Records are explicitly
serialized little-endian `<IBB`; no C struct padding is written.

USB commands are ASCII lines: HELLO, INFO, STOP, START, LIST, GET filename,
DELETE filename. GET returns `DATA <byte_count>\n`, exactly that many bytes,
then `\nCRC <hex_crc32>\n`. Checksum is CRC32/IEEE over the entire transferred
file. This detects transfer corruption; there is no stored per-record CRC to
detect older on-flash corruption. LIST returns `FILE <name> <size>` lines
followed by `END`. Serial is 115200 baud (USB CDC transport).

The converter rejects incomplete records and invalid headers rather than
silently emitting a plausible-looking CSV. Files must contain nondecreasing
timestamps and channels present in the header mask.

## Validation and first bench run

Desktop tests: `python -m unittest -v test_logger.py` from `windows`.
Six tests cover one-byte distance boundaries, exact CSV columns, four-channel
records, malformed/truncated files, transfer checksum failures, interrupted
downloads, and repeatable saved outputs. These passed in the authoring
environment. The Feather sketch was reviewed against upstream APIs, but
**was not compiled with the Adafruit board toolchain or run on hardware here**.
The Windows Known Folder/COM detection paths also need a Windows machine.

First run: log for about 10 seconds, download, check roughly 500 readings per
sensor and about 20 ms between consecutive same-channel timestamps. Move the
target to verify distances change. Check status for any stop reason. Then
perform a capacity run and confirm FULL leaves prior logs readable.

## Sources

- [Adafruit SPIFlash datalogging](https://github.com/adafruit/Adafruit_SPIFlash/blob/master/examples/SdFat_datalogging/SdFat_datalogging.ino)
- [Adafruit SPIFlash format example](https://github.com/adafruit/Adafruit_SPIFlash/blob/master/examples/SdFat_format/SdFat_format.ino)
- [Adafruit VL6180X driver](https://github.com/adafruit/Adafruit_VL6180X/blob/master/Adafruit_VL6180X.cpp)
- [Adafruit SdFat FAT volume API](https://github.com/adafruit/SdFat/blob/master/src/FatLib/FatVolume.h)

The supplied code is a purpose-built implementation using those libraries;
install them through Arduino Library Manager rather than copying their sources.
