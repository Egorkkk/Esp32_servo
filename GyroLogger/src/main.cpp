#include <Arduino.h>
#include <SPI.h>
#include "imu.h"
#include "sdcard.h"
#include "gps.h"
#include "logger.h"
#include "buttons.h"
#include "display.h"
#include "battery.h"
#include "led.h"

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

  setupDisplay();        // 🟢 Обязательно первым, чтобы дисплей был готов
  showMessage("Boot..."); // Пишем на экран

  setupLED();

  sharedSPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  Serial.printf("SD pins: CLK=%d MISO=%d MOSI=%d CS=%d\n", SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  bool sdOk = initSDCard(sdSPI);
  showInitStatus("SD Card", sdOk);
  if (!sdOk) {
    Serial.println("[MAIN] ❌ SD init failed.");
    //while (1);
  }

  bool imuOk = initIMU(sharedSPI);
  showInitStatus("IMU", imuOk);
  if (!imuOk) {
    Serial.println("[MAIN] ❌ IMU init failed.");
    while (1);
  }

  bool gpsOk = initGPS();
  showInitStatus("GPS", gpsOk);
  if (!gpsOk) {
    Serial.println("[MAIN] ❌ GPS init failed.");
  }

  setupButtons();
  showInitStatus("Buttons", true);

  setupBatteryMonitor();  

  Serial.println("[MAIN] ✅ System initialized.");
  showInitStatus("System", true);
}

void loop() {
  static unsigned long lastSample = 0;
  static unsigned long lastFlush = 0;
  static unsigned long lastDisplayUpdate = 0;
  static unsigned long logStartMillis = 0;

  handleButtons();
  handleGPS();

  // Обработка команд от кнопок
  if (shouldStartLogging()) {
  if (startLogger(sdSPI)) {
    logStartMillis = millis(); // начинаем отсчёт времени записи
  }
}

  if (shouldStopLogging()) {
    stopLogger();
  }

  // 👉 Обновление дисплея раз в секунду
  if (millis() - lastDisplayUpdate >= 1000) {
    lastDisplayUpdate = millis();

    bool gpsHasTime = gps.date.isValid() &&
                      gps.time.isValid() &&
                      (gps.time.hour() > 0 || gps.time.minute() > 0 || gps.time.second() > 0);

    double gpsTime = 0.0;
    if (gpsHasTime) {
      gpsTime = gps.time.hour() * 3600 + gps.time.minute() * 60 + gps.time.second();
    }

    unsigned long logDurationSec = 0;
    if (isLogging()) {
      logDurationSec = (millis() - logStartMillis) / 1000;
    }


    float voltage = getBatteryVoltage();
    updateStatusScreen(gpsHasTime, gpsTime, isLogging(), logDurationSec, voltage);


    
  }

  // 👉 Логирование данных только если логгер активен
  if (isLogging() && millis() - lastSample >= 10) {
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

  if (isLogging() && millis() - lastFlush >= 1000) {
    lastFlush = millis();
    flushLogger();
  }
}