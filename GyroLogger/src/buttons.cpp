#include "buttons.h"
#include <Arduino.h>
#include <Bounce2.h>

#define PIN_BTN_START 20
#define PIN_BTN_STOP  21

static Bounce startButton = Bounce();
static Bounce stopButton  = Bounce();
static bool isLogging = false;
static bool startRequested = false;
static bool stopRequested  = false;

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
    startRequested = true;
  }

  if (stopButton.fell()) {
    stopRequested = true;
  }
}

bool shouldStartLogging() {
  if (startRequested) {
    startRequested = false;
    return true;
  }
  return false;
}

bool shouldStopLogging() {
  if (stopRequested) {
    stopRequested = false;
    return true;
  }
  return false;
}