#pragma once

#include <Arduino.h>

enum class LEDState {
  INIT_ERROR,
  OK_IDLE,
  GPS_OK,
  RECORDING,
  LOW_BATTERY
};

void setupLED();
void updateLEDState(LEDState state);
void tickLED();  // вызывать каждый loop()