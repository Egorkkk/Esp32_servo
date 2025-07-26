#pragma once
#include <Arduino.h>

struct IMUData;  // Объявление структуры из imu.h

void initCAN();
bool sendIMUOverCAN(const IMUData& sample, uint8_t seqID);
void checkCANRecovery();  // 🔧 новое