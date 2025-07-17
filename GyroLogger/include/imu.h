#ifndef IMU_H
#define IMU_H

#include <Arduino.h>
#include <SparkFun_BNO080_Arduino_Library.h>

struct IMUData {
  double timestamp;

  float qw, qx, qy, qz;
  float gyroX, gyroY, gyroZ;
  float accelX, accelY, accelZ;

  double latitude = 0.0;
  double longitude = 0.0;
  double altitude = 0.0;
};

extern BNO080 imu;

bool initIMU(SPIClass &spi);
bool readIMU(IMUData &out);


#endif