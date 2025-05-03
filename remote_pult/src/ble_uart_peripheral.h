#pragma once
#include <Arduino.h>

void initBLEPeripheral();
void notifyCentral(const std::string& message);  // можно вызывать для отправки