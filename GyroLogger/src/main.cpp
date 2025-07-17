#include <Arduino.h>
#include <SPI.h>
#include "imu.h"
#include "sdcard.h"
#include "gps.h"
#include "logger.h"
#include "buttons.h"

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

  setupButtons();


  Serial.println("[MAIN] ✅ System initialized.");
}

void loop() {
  static unsigned long lastSample = 0;
  static unsigned long lastFlush = 0;

  handleButtons();
  handleGPS(); 

  if (millis() - lastSample >= 10) {
    lastSample = millis();

    IMUData sample;

    if (!readIMU(sample)) {
      Serial.println("[IMU] No new IMU data");
      return;
    }

    // Время по GPS или fallback
    if (!getGPSTimestamp(sample.timestamp)) {
      sample.timestamp = millis() / 1000.0;
    }

    if (gps.location.isValid()) {
      sample.latitude = gps.location.lat();
      sample.longitude = gps.location.lng();
    }

    if (gps.altitude.isValid()) {
      sample.altitude = gps.altitude.meters();
    }

    // 👉 Отладочный вывод
    Serial.printf("[IMU] t=%.3f  q=(%.2f, %.2f, %.2f, %.2f)  g=(%.2f, %.2f, %.2f)  a=(%.2f, %.2f, %.2f)\n",
                  sample.timestamp,
                  sample.qw, sample.qx, sample.qy, sample.qz,
                  sample.gyroX, sample.gyroY, sample.gyroZ,
                  sample.accelX, sample.accelY, sample.accelZ);

    logIMUData(sample);
  }

  if (millis() - lastFlush >= 1000) {
    lastFlush = millis();
    flushLogger();
  }

}