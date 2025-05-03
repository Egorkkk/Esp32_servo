#include <Wire.h>
#include <Arduino.h>
/*
float ax = (ax_raw - accel_x_offset) / 16384.0f; // Применение смещения
float gy = (gy_raw - gyro_x_offset) * 0.0174533f / 131.0f;

float mx = (mx_raw - mag_offset_x) / ((mag_max_x - mag_min_x) / 2.0f); // Смещение и масштабирование
float my = (my_raw - mag_offset_y) / ((mag_max_y - mag_min_y) / 2.0f);
float mz = (mz_raw - mag_offset_z) / ((mag_max_z - mag_min_z) / 2.0f);
*/
/*

// Адреса I2C устройств
#define MPU6050_ADDR 0x68
#define QMC5883L_ADDR 0x0D

// Регистры MPU-6050
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_GYRO_XOUT_H 0x43

// Регистры QMC5883L
#define QMC5883L_CONFIG_REG_A 0x09
#define QMC5883L_DATA_XOUT_LSB 0x00

// Переменные для хранения минимальных и максимальных значений (магнитометр)
int16_t mag_min_x, mag_min_y, mag_min_z;
int16_t mag_max_x, mag_max_y, mag_max_z;

// Переменные для аккумуляции данных (MPU-6050)
int32_t accel_x_sum, accel_y_sum, accel_z_sum;
int32_t gyro_x_sum, gyro_y_sum, gyro_z_sum;
int32_t sample_count = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Инициализация MPU-6050
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_PWR_MGMT_1);
  Wire.write(0x00); // Вывод из спящего режима
  Wire.endTransmission(true);

  // Инициализация QMC5883L
  Wire.beginTransmission(QMC5883L_ADDR);
  Wire.write(QMC5883L_CONFIG_REG_A);
  Wire.write(0x01); // Непрерывное измерение, 200 Гц
  Wire.endTransmission(true);

  // Инициализация переменных для магнитометра
  mag_min_x = mag_min_y = mag_min_z = 32767; // Максимальное значение int16_t
  mag_max_x = mag_max_y = mag_max_z = -32768; // Минимальное значение int16_t

  // Инициализация переменных для MPU-6050
  accel_x_sum = accel_y_sum = accel_z_sum = 0;
  gyro_x_sum = gyro_y_sum = gyro_z_sum = 0;
  sample_count = 0;

  Serial.println("Calibration started...");
  Serial.println("1. Keep MPU-6050 still on a flat surface for 10 seconds.");
  Serial.println("2. Then rotate QMC5883L in all directions for 20 seconds.");
  Serial.println("Starting MPU-6050 calibration...");
  delay(2000); // Дать время на подготовку
}

void loop() {
  static uint32_t start_time = millis();
  uint32_t current_time = millis();

  // Калибровка MPU-6050 ( первые 10 секунд, устройство неподвижно)
  if (current_time - start_time < 10000) {
    int16_t ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw;
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(MPU6050_ACCEL_XOUT_H);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6050_ADDR, 14, true);
    ax_raw = Wire.read() << 8 | Wire.read();
    ay_raw = Wire.read() << 8 | Wire.read();
    az_raw = Wire.read() << 8 | Wire.read();
    Wire.read(); Wire.read(); // Пропуск температуры
    gx_raw = Wire.read() << 8 | Wire.read();
    gy_raw = Wire.read() << 8 | Wire.read();
    gz_raw = Wire.read() << 8 | Wire.read();
    Wire.endTransmission(true);

    // Аккумулируем данные
    accel_x_sum += ax_raw;
    accel_y_sum += ay_raw;
    accel_z_sum += az_raw;
    gyro_x_sum += gx_raw;
    gyro_y_sum += gy_raw;
    gyro_z_sum += gz_raw;
    sample_count++;

    Serial.print("Calibrating MPU-6050... Sample: ");
    Serial.println(sample_count);
    delay(10); // Частота ~100 Гц
  }
  // Переход к калибровке магнитометра (следующие 20 секунд)
  else if (current_time - start_time < 30000) {
    if (current_time - start_time == 10000) {
      // Вывод результатов калибровки MPU-6050
      Serial.println("\nMPU-6050 Calibration Complete:");
      float accel_x_offset = accel_x_sum / (float)sample_count;
      float accel_y_offset = accel_y_sum / (float)sample_count;
      float accel_z_offset = (accel_z_sum / (float)sample_count) - 16384.0f; // Учитываем 1g по Z
      float gyro_x_offset = gyro_x_sum / (float)sample_count;
      float gyro_y_offset = gyro_y_sum / (float)sample_count;
      float gyro_z_offset = gyro_z_sum / (float)sample_count;

      Serial.print("Accel X Offset: "); Serial.println(accel_x_offset);
      Serial.print("Accel Y Offset: "); Serial.println(accel_y_offset);
      Serial.print("Accel Z Offset: "); Serial.println(accel_z_offset);
      Serial.print("Gyro X Offset: "); Serial.println(gyro_x_offset);
      Serial.print("Gyro Y Offset: "); Serial.println(gyro_y_offset);
      Serial.print("Gyro Z Offset: "); Serial.println(gyro_z_offset);
      Serial.println("\nNow rotate QMC5883L in all directions...");
    }

    // Калибровка QMC5883L
    int16_t mx_raw, my_raw, mz_raw;
    Wire.beginTransmission(QMC5883L_ADDR);
    Wire.write(QMC5883L_DATA_XOUT_LSB);
    Wire.endTransmission(false);
    Wire.requestFrom(QMC5883L_ADDR, 6, true);
    mx_raw = Wire.read() | Wire.read() << 8;
    my_raw = Wire.read() | Wire.read() << 8;
    mz_raw = Wire.read() | Wire.read() << 8;
    Wire.endTransmission(true);

    // Обновление минимальных и максимальных значений
    mag_min_x = min(mag_min_x, mx_raw);
    mag_min_y = min(mag_min_y, my_raw);
    mag_min_z = min(mag_min_z, mz_raw);
    mag_max_x = max(mag_max_x, mx_raw);
    mag_max_y = max(mag_max_y, my_raw);
    mag_max_z = max(mag_max_z, mz_raw);

    Serial.print("Mag X: "); Serial.print(mx_raw);
    Serial.print(" | Y: "); Serial.print(my_raw);
    Serial.print(" | Z: "); Serial.println(mz_raw);
    delay(50); // Частота ~20 Гц для удобства
  }
  // Завершение калибровки
  else if (current_time - start_time >= 30000) {
    // Вывод результатов калибровки QMC5883L
    Serial.println("\nQMC5883L Calibration Complete:");
    float mag_offset_x = (mag_max_x + mag_min_x) / 2.0f;
    float mag_offset_y = (mag_max_y + mag_min_y) / 2.0f;
    float mag_offset_z = (mag_max_z + mag_min_z) / 2.0f;

    Serial.print("Mag X Offset: "); Serial.println(mag_offset_x);
    Serial.print("Mag Y Offset: "); Serial.println(mag_offset_y);
    Serial.print("Mag Z Offset: "); Serial.println(mag_offset_z);
    Serial.print("Mag X Scale: "); Serial.println((mag_max_x - mag_min_x) / 2.0f);
    Serial.print("Mag Y Scale: "); Serial.println((mag_max_y - mag_min_y) / 2.0f);
    Serial.print("Mag Z Scale: "); Serial.println((mag_max_z - mag_min_z) / 2.0f);

    Serial.println("\nCalibration finished. Use these offsets in your code.");
    while (1); // Остановка после завершения
  }
}
  */