#pragma once

void setupDisplay();
void showInitStatus(const char* component, bool success);
void showMessage(const char* message);
void clearDisplay();

void updateStatusScreen(bool gpsHasTime, double gpsTime, bool isLogging, unsigned long logDurationSec, float batteryVoltage, const char* canStatusStr);
