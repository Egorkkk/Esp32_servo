#include "can.h"
#include "driver/twai.h"
#include "imu.h"
#include "esp_err.h"

#define CAN_TX_PIN 15
#define CAN_RX_PIN 18

void initCAN() {
  // Останавливаем и удаляем драйвер, если он уже был установлен
  twai_stop();
  twai_driver_uninstall();

  // Приводим типы аргументов к gpio_num_t явно
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
    (gpio_num_t)CAN_TX_PIN,
    (gpio_num_t)CAN_RX_PIN,
    TWAI_MODE_NORMAL
  );

  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
  if (err != ESP_OK) {
    Serial.printf("[CAN] ❌ Ошибка установки драйвера: 0x%04X (%s)\n", err, esp_err_to_name(err));
    return;
  }

  err = twai_start();
  if (err != ESP_OK) {
    Serial.printf("[CAN] ❌ Ошибка запуска CAN: 0x%04X (%s)\n", err, esp_err_to_name(err));
    return;
  }

  Serial.println("[CAN] ✅ Инициализация успешна (500 кбит/с)");
}



bool sendIMUOverCAN(const IMUData& s, uint8_t seqID) {
  twai_message_t msg;

  // --- Quaternion: qw, qx, qy, qz — 4×int16 = 8 байт (сдвигаем на 1 для seqID)
  msg.identifier = 0x100;
  msg.flags = TWAI_MSG_FLAG_NONE;
  msg.data_length_code = 8;
  msg.data[0] = seqID;
  msg.data[1] = (int16_t)(s.qw * 10000) >> 8;
  msg.data[2] = (int16_t)(s.qw * 10000) & 0xFF;
  msg.data[3] = (int16_t)(s.qx * 10000) >> 8;
  msg.data[4] = (int16_t)(s.qx * 10000) & 0xFF;
  msg.data[5] = (int16_t)(s.qy * 10000) >> 8;
  msg.data[6] = (int16_t)(s.qy * 10000) & 0xFF;
  msg.data[7] = (int16_t)(s.qz * 10000) >> 8;
  if (twai_transmit(&msg, pdMS_TO_TICKS(5)) != ESP_OK) return false;

  // --- Gyroscope
  msg.identifier = 0x101;
  msg.data_length_code = 7;
  msg.data[0] = seqID;
  msg.data[1] = (int16_t)(s.gyroX * 1000) >> 8;
  msg.data[2] = (int16_t)(s.gyroX * 1000) & 0xFF;
  msg.data[3] = (int16_t)(s.gyroY * 1000) >> 8;
  msg.data[4] = (int16_t)(s.gyroY * 1000) & 0xFF;
  msg.data[5] = (int16_t)(s.gyroZ * 1000) >> 8;
  msg.data[6] = (int16_t)(s.gyroZ * 1000) & 0xFF;
  if (twai_transmit(&msg, pdMS_TO_TICKS(5)) != ESP_OK) return false;

  // --- Accelerometer
  msg.identifier = 0x102;
  msg.data_length_code = 7;
  msg.data[0] = seqID;
  msg.data[1] = (int16_t)(s.accelX * 1000) >> 8;
  msg.data[2] = (int16_t)(s.accelX * 1000) & 0xFF;
  msg.data[3] = (int16_t)(s.accelY * 1000) >> 8;
  msg.data[4] = (int16_t)(s.accelY * 1000) & 0xFF;
  msg.data[5] = (int16_t)(s.accelZ * 1000) >> 8;
  msg.data[6] = (int16_t)(s.accelZ * 1000) & 0xFF;
  if (twai_transmit(&msg, pdMS_TO_TICKS(5)) != ESP_OK) return false;

  return true;
}
void checkCANRecovery() {
  twai_status_info_t status;
  twai_get_status_info(&status);

  if (status.state == TWAI_STATE_BUS_OFF) {
    Serial.println("[CAN] ⚠ BUS OFF — перезапуск CAN");
    twai_stop();
    twai_driver_uninstall();
    delay(100);  // пауза для шины

    initCAN();  // переинициализация
  }
}