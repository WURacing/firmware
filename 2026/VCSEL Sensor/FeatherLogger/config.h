#pragma once

// SD1/SC1 = channel 1. Production example: change to {1, 2, 3, 4}.
constexpr uint8_t SENSOR_CHANNELS[] = {0,1};
constexpr uint8_t SENSOR_COUNT = sizeof(SENSOR_CHANNELS) / sizeof(SENSOR_CHANNELS[0]);
constexpr uint8_t TCA_ADDRESS = 0x70;
constexpr uint8_t SENSOR_ADDRESS = 0x29;
constexpr uint32_t SAMPLE_PERIOD_US = 100000;  // 10 Hz per sensor
constexpr uint8_t MAX_CONVERGENCE_MS = 14;
constexpr uint32_t STORAGE_RESERVE_BYTES = 8192;
