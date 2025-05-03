#include "ble_uart_central.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <Arduino.h>

// UUID сервиса и характеристик BLE UART (Nordic)
static BLEUUID UART_SERVICE_UUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID UART_CHAR_RX_UUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E"); // RX ← уведомления от пульта
static BLEUUID UART_CHAR_TX_UUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E"); // TX → команды пульту

static BLEClient* bleClient = nullptr;
static BLERemoteCharacteristic* uartRXChar = nullptr;
static BLERemoteCharacteristic* uartTXChar = nullptr;

static bool bleConnected = false;

// ==== Колбэк подключения/отключения ====
class CentralClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) override {
    Serial.println("[BLE] ✅ Подключено к Peripheral");
  }
  void onDisconnect(BLEClient* pclient) override {
    Serial.println("[BLE] ❌ Отключено от Peripheral");
    bleConnected = false;
  }
};

// ==== Колбэк уведомлений с UART RX ====
class RXNotifyCallback {
public:
  static void onNotify(BLERemoteCharacteristic* pCharacteristic, uint8_t* data, size_t length, bool isNotify) {
    Serial.print("[BLE] 📥 Принято сообщение: ");
    for (size_t i = 0; i < length; i++) {
      Serial.write(data[i]);
    }
    Serial.println();
  }
};

void initBLECentral() {
    BLEDevice::init("CAMERA_CONTROLLER");
  
    BLEScan* scan = BLEDevice::getScan();
    scan->setActiveScan(true);
    BLEScanResults results = scan->start(5);
  
    for (int i = 0; i < results.getCount(); ++i) {
      BLEAdvertisedDevice device = results.getDevice(i);
  
      // === 🔎 Уточнённая проверка: и имя, и UUID ===
      if (device.haveServiceUUID() &&
          device.isAdvertisingService(UART_SERVICE_UUID) &&
          device.haveName() &&
          device.getName() == "PULT_UART") {
  
        Serial.print("[BLE] 🔎 Найден нужный Peripheral: ");
        Serial.println(device.toString().c_str());
  
        bleClient = BLEDevice::createClient();
        bleClient->setClientCallbacks(new CentralClientCallback());
  
        if (!bleClient->connect(&device)) {
          Serial.println("[BLE] ❌ Не удалось подключиться");
          return;
        }
  
        BLERemoteService* uartService = bleClient->getService(UART_SERVICE_UUID);
        if (!uartService) {
          Serial.println("[BLE] ❌ UART-сервис не найден");
          return;
        }
  
        uartRXChar = uartService->getCharacteristic(UART_CHAR_RX_UUID);
        if (uartRXChar && uartRXChar->canNotify()) {
          uartRXChar->registerForNotify(RXNotifyCallback::onNotify);
          Serial.println("[BLE] ✅ RX уведомления активированы");
        }
  
        uartTXChar = uartService->getCharacteristic(UART_CHAR_TX_UUID);
        if (uartTXChar && uartTXChar->canWrite()) {
          Serial.println("[BLE] ✅ TX характеристика готова к отправке");
        }
  
        bleConnected = true;
        break;
      }
    }
  
    if (!bleConnected) {
      Serial.println("[BLE] ❌ UART BLE устройство с именем 'PULT_UART' не найдено");
    }
  }

void processBLECentral() {
  // можно обновлять состояния, контролировать reconnection
}

bool isBLEConnected() {
  return bleConnected;
}

bool sendToPeripheral(const std::string& message) {
  if (bleConnected && uartTXChar) {
    try {
      uartTXChar->writeValue((uint8_t*)message.data(), message.length());
      Serial.print("[BLE] 📤 Отправлено Peripheral: ");
      Serial.println(message.c_str());
      return true;
    } catch (...) {
      Serial.println("[BLE] ❌ Ошибка отправки");
    }
  }
  return false;
}