#include "gps.h"
#include <TinyGPSPlus.h>

HardwareSerial gpsSerial(1); // UART1
TinyGPSPlus gps;

void initGPS() {
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17); // RX, TX
  Serial.println("GPS initialized.");
}