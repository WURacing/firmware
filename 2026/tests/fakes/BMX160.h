/**
 * @file BMX160.h
 * @brief Fake BMX160 IMU library (team fork of DFRobot_BMX160) for tests.
 *
 * Talks to address 0x68 through the fake Wire bus, so it only finds the IMU
 * when the right I2C mux channel is selected. getAllData() returns the
 * readings the test set with SetData(), or zeros if 0x68 isn't reachable.
 */

// NOLINT(build/header_guard)
// No include guard, like the real library header (2024/BMX160/BMX160.h):
// including it twice fails here as it does on the board. Sketches include
// it only from their own guarded header.

#include <Wire.h>
#include <stdint.h>

typedef struct {
  float x;
  float y;
  float z;
  uint32_t sensortime;
} sBmx160SensorData_t;

typedef enum {
  eGyroRange_2000DPS,
  eGyroRange_1000DPS,
  eGyroRange_500DPS,
  eGyroRange_250DPS,
  eGyroRange_125DPS
} eGyroRange_t;

typedef enum {
  eAccelRange_2G,
  eAccelRange_4G,
  eAccelRange_8G,
  eAccelRange_16G
} eAccelRange_t;

class BMX160 {
 public:
  static constexpr int address = 0x68;

  bool begin() { return fake::I2cReachable(address); }
  void setGyroRange(eGyroRange_t range) { gyro_range_ = range; }
  void setAccelRange(eAccelRange_t range) { accel_range_ = range; }
  void getAllData(sBmx160SensorData_t* magn, sBmx160SensorData_t* gyro,
                  sBmx160SensorData_t* accel) {
    ++read_count_;
    const bool reachable = fake::I2cReachable(address);
    if (!reachable) {
      ++unreachable_read_count_;
    }
    const sBmx160SensorData_t zero = {};
    *magn = reachable ? magn_ : zero;
    *gyro = reachable ? gyro_ : zero;
    *accel = reachable ? accel_ : zero;
  }

  // Test controls and records.
  void SetData(const sBmx160SensorData_t& accel,
               const sBmx160SensorData_t& gyro,
               const sBmx160SensorData_t& magn) {
    accel_ = accel;
    gyro_ = gyro;
    magn_ = magn;
  }
  eGyroRange_t gyro_range() const { return gyro_range_; }
  eAccelRange_t accel_range() const { return accel_range_; }
  int read_count() const { return read_count_; }
  // Reads made while 0x68 wasn't reachable (wrong mux channel selected).
  int unreachable_read_count() const { return unreachable_read_count_; }

 private:
  eGyroRange_t gyro_range_ = eGyroRange_2000DPS;
  eAccelRange_t accel_range_ = eAccelRange_2G;
  sBmx160SensorData_t accel_ = {};
  sBmx160SensorData_t gyro_ = {};
  sBmx160SensorData_t magn_ = {};
  int read_count_ = 0;
  int unreachable_read_count_ = 0;
};
