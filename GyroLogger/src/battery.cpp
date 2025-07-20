#include "battery.h"
#include <Arduino.h>

#define BATTERY_PIN 4       // Подключён к делителю
#define ADC_REF_VOLTAGE 3.3  // Vref ESP32
#define ADC_RESOLUTION 4095.0

// Делитель: R1 = R2 = 100k => коэффициент умножения = 2.0
#define VOLTAGE_DIVIDER_RATIO 2.0

void setupBatteryMonitor() {
  analogReadResolution(12);  // 0–4095
}

float getBatteryVoltage() {
  int adcValue = analogRead(BATTERY_PIN);
  float voltageADC = (adcValue / ADC_RESOLUTION) * ADC_REF_VOLTAGE;
  float voltageBattery = voltageADC * VOLTAGE_DIVIDER_RATIO;
  return voltageBattery;
}