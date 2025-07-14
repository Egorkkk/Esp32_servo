#include "sdcard.h"
#include <SPI.h>
#include <SD.h>

// Новые пины SPI под ESP32-S3
#define SD_CS_PIN    10
#define SD_MOSI_PIN  11
#define SD_MISO_PIN  13
#define SD_SCK_PIN   12

void initSDCard() {
  // Инициализация SPI на пользовательских пинах
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card init failed!");
    while (1) delay(10);
  } else {
    Serial.println("SD card initialized.");
  }
}