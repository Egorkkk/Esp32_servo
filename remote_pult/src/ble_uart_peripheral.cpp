#include "ble_uart_peripheral.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

static BLECharacteristic* pTXCharacteristic = nullptr;

// UUID-ы UART-сервиса (Nordic UART Service)
static BLEUUID serviceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID charUUID_TX("6E400003-B5A3-F393-E0A9-E50E24DCCA9E"); // Notify
static BLEUUID charUUID_RX("6E400002-B5A3-F393-E0A9-E50E24DCCA9E"); // Write

class RXCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    std::string value = characteristic->getValue();
    Serial.print("Команда от центрального устройства: ");
    Serial.println(value.c_str());

    // Здесь можешь реагировать на команды, например:
    if (value == "GAIN_UP") {
      // Реакция на GAIN_UP
    }
  }
};

void initBLEPeripheral() {
  BLEDevice::init("PULT_UART");  // имя пульта

  BLEServer* server = BLEDevice::createServer();
  BLEService* uartService = server->createService(serviceUUID);

  // TX characteristic (notify)
  pTXCharacteristic = uartService->createCharacteristic(
      charUUID_TX,
      BLECharacteristic::PROPERTY_NOTIFY
  );
  pTXCharacteristic->addDescriptor(new BLE2902());  // разрешает notify

  // RX characteristic (write)
  BLECharacteristic* pRX = uartService->createCharacteristic(
      charUUID_RX,
      BLECharacteristic::PROPERTY_WRITE
  );
  pRX->setCallbacks(new RXCallback());

  uartService->start();
  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(serviceUUID);
  advertising->start();

  Serial.println("BLE UART Peripheral запущен и ожидает подключения...");
}

void notifyCentral(const std::string& message) {
  if (pTXCharacteristic) {
    pTXCharacteristic->setValue(message);
    pTXCharacteristic->notify();
    Serial.print("Отправлено Central: ");
    Serial.println(message.c_str());
  }
}

/* ===========================MAIN.CPP===================================
#include <Arduino.h>
#include "ble_uart_peripheral.h"

void setup() {
  Serial.begin(115200);
  initBLEPeripheral();
}

void loop() {
  // Пример отправки команды Central через 5 секунд
  static unsigned long t0 = millis();
  if (millis() - t0 > 5000) {
    notifyCentral("HELLO\n");
    t0 = millis();
  }
}
  */