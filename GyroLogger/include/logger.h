#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "imu.h"


bool initLogger(SPIClass& spi);
void logIMUData(const IMUData& data);
void flushLogger();