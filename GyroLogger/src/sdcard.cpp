#include "sdcard.h"
#include <SD.h>

// CS пин больше не фиксируем здесь, он передаётся через аргумент
// #define SD_CS_PIN 7 — убрано

bool initSDCard(SPIClass &spi) {
  const uint8_t SD_CS_PIN = 39;  // явно задали тот, что указан в main.cpp

  Serial.println("[SD] Starting SD card init...");
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  delay(100);

  // spi.begin() должен быть вызван в main, повторно здесь не нужно

  if (!SD.begin(SD_CS_PIN, spi, 3000000)) {
    Serial.println("[SD] ❌ SD card init failed!");
    return false;
  }

  Serial.println("[SD] ✅ SD card initialized.");

  File testFile = SD.open("/test.txt", FILE_WRITE);
  if (testFile) {
    testFile.println("SD test OK");
    testFile.close();
    Serial.println("[SD] Test file written.");
  } else {
    Serial.println("[SD] ❌ Failed to write test file.");
  }

  return true;
}