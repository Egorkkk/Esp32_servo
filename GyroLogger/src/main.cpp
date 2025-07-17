#include <Arduino.h>
#include <SPI.h>
#include "imu.h"
#include "sdcard.h"
#include "gps.h"
#include "logger.h"

#define SPI_MOSI 11
#define SPI_MISO 13
#define SPI_SCK  12

SPIClass sharedSPI(HSPI);

#define SD_MOSI 35
#define SD_MISO 37
#define SD_SCK  36
#define SD_CS   39

SPIClass sdSPI(FSPI);  // Новая шина для SD-карты

unsigned long lastFlush = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("[MAIN] Boot...");

  sharedSPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  Serial.printf("SD pins: CLK=%d MISO=%d MOSI=%d CS=%d\n", SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!initSDCard(sdSPI)) {
    Serial.println("[MAIN] ❌ SD init failed.");
    //while (1);
  }

  if (!initIMU(sharedSPI)) {
    Serial.println("[MAIN] ❌ IMU init failed.");
    while (1);
  }

  if (!initGPS()) {
    Serial.println("[MAIN] ❌ GPS init failed.");
  }

  if (!initLogger(sdSPI)) {
    Serial.println("[MAIN] ❌ Logger init failed.");
    while (1);
  }

  Serial.println("[MAIN] ✅ System initialized.");
}

void loop() {
  static unsigned long lastSample = 0;
  static unsigned long lastFlush = 0;

  if (millis() - lastSample >= 100) {
    lastSample = millis();

    double timestamp;
    if (!getGPSTimestamp(timestamp)) {
      timestamp = millis() / 1000.0; // fallback
    }

    IMUData sample;
    sample.timestamp = timestamp;
    sample.qw = imu.getQuatReal();
    sample.qx = imu.getQuatI();
    sample.qy = imu.getQuatJ();
    sample.qz = imu.getQuatK();
    sample.gyroX = imu.getGyroX();
    sample.gyroY = imu.getGyroY();
    sample.gyroZ = imu.getGyroZ();
    sample.accelX = imu.getAccelX();
    sample.accelY = imu.getAccelY();
    sample.accelZ = imu.getAccelZ();

   // 👉 Отладочный вывод
    Serial.printf("[IMU] t=%.3f  q=(%.2f, %.2f, %.2f, %.2f)  g=(%.2f, %.2f, %.2f)  a=(%.2f, %.2f, %.2f)\n",
                  sample.timestamp,
                  sample.qw, sample.qx, sample.qy, sample.qz,
                  sample.gyroX, sample.gyroY, sample.gyroZ,
                  sample.accelX, sample.accelY, sample.accelZ);

    logIMUData(sample);
  }

  if (millis() - lastFlush >= 100) {
    Serial.println("[LOGGER] Flushing...");
    lastFlush = millis();
    flushLogger();
  }

//  handleGPS();
}