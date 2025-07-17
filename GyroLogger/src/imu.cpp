#include "imu.h"

#define IMU_CS_PIN   9
#define IMU_RST_PIN  8
#define IMU_INT_PIN  10
#define IMU_WAK_PIN  47

BNO080 imu;

bool initIMU(SPIClass &spi) {
  pinMode(IMU_CS_PIN, OUTPUT);
  digitalWrite(IMU_CS_PIN, HIGH);

  pinMode(IMU_RST_PIN, OUTPUT);
  digitalWrite(IMU_RST_PIN, LOW);
  delay(10);
  digitalWrite(IMU_RST_PIN, HIGH);
  delay(100);

  if (!imu.beginSPI(IMU_CS_PIN, IMU_WAK_PIN, IMU_INT_PIN, IMU_RST_PIN, 1000000, spi)) {
    Serial.println("[IMU] ❌ IMU init failed!");
    return false;
  }

  Serial.println("[IMU] ✅ IMU initialized.");
  imu.enableRotationVector(50);  // 20Hz
  return true;
}

bool readIMU(IMUData& out) {
  if (!imu.dataAvailable()) return false;

  out.qw = imu.getQuatReal();
  out.qx = imu.getQuatI();
  out.qy = imu.getQuatJ();
  out.qz = imu.getQuatK();

  out.gyroX = imu.getGyroX();
  out.gyroY = imu.getGyroY();
  out.gyroZ = imu.getGyroZ();

  out.accelX = imu.getAccelX();
  out.accelY = imu.getAccelY();
  out.accelZ = imu.getAccelZ();

  return true;
}

