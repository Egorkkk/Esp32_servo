#include "gps.h"
#include <TinyGPSPlus.h>
#include <Arduino.h>
#include <time.h>

HardwareSerial gpsSerial(1);
TinyGPSPlus gps;

bool initGPS() {
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("[GPS] Initialized.");
  return true;
}

void handleGPS() {
  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    gps.encode(c);
  }

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    lastPrint = millis();

    Serial.println("\n--- GPS STATUS ---");

    if (gps.location.isValid()) {
      Serial.print("Location: ");
      Serial.print(gps.location.lat(), 6);
      Serial.print(", ");
      Serial.println(gps.location.lng(), 6);
    } else {
      Serial.println("Location: not valid");
    }

    if (gps.time.isValid()) {
      Serial.print("Time: ");
      Serial.printf("%02d:%02d:%02d.%02d\n", gps.time.hour(), gps.time.minute(), gps.time.second(), gps.time.centisecond());
    } else {
      Serial.println("Time: not valid");
    }

    if (gps.date.isValid()) {
      Serial.print("Date: ");
      Serial.printf("%02d.%02d.%04d\n", gps.date.day(), gps.date.month(), gps.date.year());
    } else {
      Serial.println("Date: not valid");
    }

    Serial.println("--- END ---\n");
  }
}

bool getGPSTimestamp(double& timestamp_out) {
  Serial.println("[GPS] Checking GPS timestamp validity...");
  if (!gps.time.isValid() || !gps.date.isValid()) {
    static bool warned = false;
    if (!warned) {
      Serial.println("[GPS] ⚠ GPS time not yet valid, using millis()");
      warned = true;
    }
    return false;
  }

  // Переводим время в секунды с начала суток (UTC)
  timestamp_out =
      gps.time.hour() * 3600.0 +
      gps.time.minute() * 60.0 +
      gps.time.second() +
      gps.time.centisecond() / 100.0;

  static bool confirmed = false;
  if (!confirmed) {
    Serial.println("[GPS] ✅ GPS time is now valid and used for timestamps");
    confirmed = true;
  }

  return true;
}