#pragma once
#include <TinyGPSPlus.h>

extern TinyGPSPlus gps; 

bool initGPS();
void handleGPS();
bool getGPSTimestamp(double& timestamp_out);
bool isGPSLost(unsigned long timeoutMs = 5000);