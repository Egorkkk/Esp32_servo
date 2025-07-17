#include "buttons.h"
#include <Arduino.h>
#include <Bounce2.h>

#define PIN_BTN_START 0
#define PIN_BTN_STOP  1

static Bounce startButton = Bounce();
static Bounce stopButton  = Bounce();
static bool isLogging = false;

void setupButtons() {
  pinMode(PIN_BTN_START, INPUT_PULLUP);
  pinMode(PIN_BTN_STOP, INPUT_PULLUP);

  startButton.attach(PIN_BTN_START);
  startButton.interval(25);

  stopButton.attach(PIN_BTN_STOP);
  stopButton.interval(25);
}

void handleButtons() {
  startButton.update();
  stopButton.update();

  if (startButton.fell()) {
    isLogging = true;
  }

  if (stopButton.fell()) {
    isLogging = false;
  }
}

bool isLoggingActive() {
  return isLogging;
}

void resetLoggingState() {
  isLogging = false;
}