#include "imu.h"
#include <Adafruit_BNO08x.h>
#include <Wire.h>

// I2C на свободных пинах ESP32-S3
#define I2C_SDA 8
#define I2C_SCL 9

Adafruit_BNO08x bno08x;

void initIMU() {
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!bno08x.begin_I2C()) {
    Serial.println("BNO080 not detected!");
    while (1) delay(10);
  } else {
    Serial.println("BNO080 initialized.");
  }
}