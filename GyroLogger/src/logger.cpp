#include "logger.h"
#include <SD.h>

#define MAX_BUFFER_SIZE 200

static File logFile;
static IMUData buffer[MAX_BUFFER_SIZE];
static size_t bufferIndex = 0;
static bool isLoggingFlag = false;

bool startLogger(SPIClass& spi) {
  const uint8_t SD_CS_PIN = 47;  // тот же, что в main и sdcard.cpp

  if (isLoggingFlag) {
    Serial.println("[LOGGER] ⚠ Already logging");
    return true;
  }

  if (!SD.begin(SD_CS_PIN, spi)) {
    Serial.println("[LOGGER] ❌ SD init failed");
    return false;
  }

  uint16_t logIndex = 1;
  char filename[20];
  do {
    snprintf(filename, sizeof(filename), "/log_%03u.csv", logIndex++);
  } while (SD.exists(filename));

  logFile = SD.open(filename, FILE_WRITE);
  if (!logFile) {
    Serial.println("[LOGGER] ❌ Failed to open log file");
    return false;
  }

  logFile.println("timestamp,quat_w,quat_x,quat_y,quat_z,gyro_x,gyro_y,gyro_z,accel_x,accel_y,accel_z,lat,lon,alt");
  logFile.flush();

  Serial.printf("[LOGGER] ✅ Logging to %s\n", filename);
  bufferIndex = 0;
  isLoggingFlag = true;
  return true;
}

void stopLogger() {
  if (logFile) {
    flushLogger();
    logFile.close();
    Serial.println("[LOGGER] 🛑 Logging stopped.");
  }
  isLoggingFlag = false;
}

bool isLogging() {
  return isLoggingFlag;
}

void logIMUData(const IMUData& data) {
  if (bufferIndex < MAX_BUFFER_SIZE) {
    buffer[bufferIndex++] = data;
  } else {
    Serial.println("[LOGGER] ⚠ Buffer overflow — data lost");
  }
}

void flushLogger() {
  unsigned long t0 = millis();

  if (!logFile) {
    Serial.println("[LOGGER] ❌ logFile is null");
    return;
  }

  if (bufferIndex == 0) {
    Serial.println("[LOGGER] Nothing to write");
    return;
  }

  size_t recordsWritten = bufferIndex;

  for (size_t i = 0; i < bufferIndex; i++) {
    const IMUData& d = buffer[i];
    logFile.printf("%.3f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.2f\n",
      d.timestamp, d.qw, d.qx, d.qy, d.qz,
      d.gyroX, d.gyroY, d.gyroZ,
      d.accelX, d.accelY, d.accelZ,
      d.latitude, d.longitude, d.altitude);
  }

  logFile.flush();
  bufferIndex = 0;

  unsigned long dt = millis() - t0;
  Serial.printf("[LOGGER] Wrote %d records in %lu ms\n", recordsWritten, dt);
}