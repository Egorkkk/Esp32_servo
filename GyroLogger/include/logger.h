#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "imu.h"


void logIMUData(const IMUData& data);
void flushLogger();
bool isLogging(); // возвращает true, если файл открыт и данные пишутся

bool startLogger(SPIClass& spi);
void stopLogger();
bool isLogging();