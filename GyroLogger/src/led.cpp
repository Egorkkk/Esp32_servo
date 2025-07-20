#include "led.h"
#include <Adafruit_NeoPixel.h>

#define LED_PIN 38
#define NUM_LEDS 1

static Adafruit_NeoPixel led(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
static LEDState currentState = LEDState::OK_IDLE;

static unsigned long lastUpdate = 0;
static bool ledOn = false;
static float pulsePhase = 0.0;

void setupLED() {
  led.begin();
  led.show();  // все выключены
  led.setBrightness(20);  // общая яркость (0–255)
}

void updateLEDState(LEDState state) {
  currentState = state;
}

void setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
  led.setPixelColor(0, led.Color(r, g, b));
  led.show();
}

void tickLED() {
  unsigned long now = millis();

  switch (currentState) {

    case LEDState::INIT_ERROR:
      if (now - lastUpdate >= 150) {
        lastUpdate = now;
        ledOn = !ledOn;
        setLEDColor(ledOn ? 255 : 0, 0, 0);  // мигаем красным
      }
      break;

    case LEDState::LOW_BATTERY:
      if (now - lastUpdate >= 250) {
        lastUpdate = now;
        static bool yellowPhase = true;
        yellowPhase = !yellowPhase;
        setLEDColor(yellowPhase ? 255 : 255, yellowPhase ? 255 : 0, 0);  // жёлтый ↔ красный
      }
      break;

    case LEDState::RECORDING:
      // плавная пульсация красным (sinusoidal)
      pulsePhase += 0.05;
      if (pulsePhase > 2 * PI) pulsePhase -= 2 * PI;
      {
        float brightness = (sin(pulsePhase) + 1.0) / 2.0;  // 0.0 – 1.0
        uint8_t value = (uint8_t)(brightness * 255);
        setLEDColor(value, 0, 0);
      }
      break;

    case LEDState::GPS_OK:
      if (now - lastUpdate >= 1000) {
        lastUpdate = now;
        ledOn = !ledOn;
        setLEDColor(0, 0, ledOn ? 255 : 0);  // мигаем синим
      }
      break;

    case LEDState::OK_IDLE:
    default:
      if (now - lastUpdate >= 1000) {
        lastUpdate = now;
        ledOn = !ledOn;
        setLEDColor(0, ledOn ? 255 : 0, 0);  // мигаем зелёным
      }
      break;
  }
}