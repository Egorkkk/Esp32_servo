#pragma once
#include <BLEClient.h>
#include <BLEUtils.h>
#include <BLEAddress.h>

void initBLECentral();
void processBLECentral();
bool isBLEConnected();

bool sendToPeripheral(const std::string& message);