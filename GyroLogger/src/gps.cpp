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
  if (!gps.time.isValid() || !gps.date.isValid()) return false;

  tm timeinfo;
  timeinfo.tm_year = gps.date.year() - 1900;
  timeinfo.tm_mon  = gps.date.month() - 1;
  timeinfo.tm_mday = gps.date.day();
  timeinfo.tm_hour = gps.time.hour();
  timeinfo.tm_min  = gps.time.minute();
  timeinfo.tm_sec  = gps.time.second();
  time_t unix_time = mktime(&timeinfo);

  double millis = gps.time.centisecond() * 10.0;
  timestamp_out = (double)unix_time + millis / 1000.0;

  return true;
}