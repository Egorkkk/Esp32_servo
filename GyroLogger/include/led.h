#pragma once

#include <Arduino.h>

enum class LEDState {
  INIT_ERROR,
  LOW_BATTERY,
  RECORDING,
  GPS_OK,
  GPS_LOST,
  OK_IDLE
};

void setupLED();
void updateLEDState(LEDState state);
void tickLED();  // вызывать каждый loop()