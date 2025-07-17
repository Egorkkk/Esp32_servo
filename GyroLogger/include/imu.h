#ifndef IMU_H
#define IMU_H

#include <Arduino.h>
#include <SparkFun_BNO080_Arduino_Library.h>

struct IMUData {
  double timestamp;
  float qx, qy, qz, qw;
  float gyroX, gyroY, gyroZ;
  float accelX, accelY, accelZ;
};

extern BNO080 imu;

bool initIMU(SPIClass &spi);
bool readIMU(IMUData &out);

#endif