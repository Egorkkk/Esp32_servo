#pragma once
#include <Arduino.h>
#include <SPI.h>

struct IMUData {
  double timestamp; // теперь точное время в секундах
  float qw, qx, qy, qz;
  float gyroX, gyroY, gyroZ;
  float accelX, accelY, accelZ;
};

bool initLogger(SPIClass& spi);
void logIMUData(const IMUData& data);
void flushLogger();