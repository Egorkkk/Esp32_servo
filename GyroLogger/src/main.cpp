
#include <Arduino.h>
#include "imu.h"
#include "gps.h"
#include "sdcard.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  initIMU();
  initGPS();
  initSDCard();
}

void loop() {
  // пока ничего
}