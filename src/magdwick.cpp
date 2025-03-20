#include <Wire.h>
#include <Arduino.h>


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

// Параметры фильтра
float beta = 0.1f; // Коэффициент фильтра
float q[4] = {1.0f, 0.0f, 0.0f, 0.0f}; // Кватернион (w, x, y, z)
float dt = 0.01f; // Шаг времени (100 Гц)

// Переменные для хранения калибровочных смещений магнитометра (нужно определить экспериментально)
float mag_offset_x = 0.0f, mag_offset_y = 0.0f, mag_offset_z = 0.0f;

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

  Serial.println("Sensors initialized");
}

void loop() {
  // Данные с MPU-6050
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

  // Данные с QMC5883L
  int16_t mx_raw, my_raw, mz_raw;
  Wire.beginTransmission(QMC5883L_ADDR);
  Wire.write(QMC5883L_DATA_XOUT_LSB);
  Wire.endTransmission(false);
  Wire.requestFrom(QMC5883L_ADDR, 6, true);
  mx_raw = Wire.read() | Wire.read() << 8; // Младший байт первый
  my_raw = Wire.read() | Wire.read() << 8;
  mz_raw = Wire.read() | Wire.read() << 8;
  Wire.endTransmission(true);

  // Преобразование данных
  float ax = ax_raw / 16384.0f; // ±2g
  float ay = ay_raw / 16384.0f;
  float az = az_raw / 16384.0f;
  float gx = gx_raw * 0.0174533f / 131.0f; // ±250°/s в радианы
  float gy = gy_raw * 0.0174533f / 131.0f;
  float gz = gz_raw * 0.0174533f / 131.0f;
  float mx = (mx_raw - mag_offset_x); // Применение смещений
  float my = (my_raw - mag_offset_y);
  float mz = (mz_raw - mag_offset_z);

  // Нормализация акселерометра и магнитометра
  float norm = sqrt(ax * ax + ay * ay + az * az);
  ax /= norm; ay /= norm; az /= norm;
  norm = sqrt(mx * mx + my * my + mz * mz);
  mx /= norm; my /= norm; mz /= norm;

  // Применение фильтра Маджвика
  madgwickFilter(ax, ay, az, gx, gy, gz, mx, my, mz);

  // Вычисление углов Эйлера из кватерниона
  float roll = atan2(2.0f * (q[0] * q[1] + q[2] * q[3]), 1.0f - 2.0f * (q[1] * q[1] + q[2] * q[2]));
  float pitch = asin(2.0f * (q[0] * q[2] - q[3] * q[1]));
  float yaw = atan2(2.0f * (q[0] * q[3] + q[1] * q[2]), 1.0f - 2.0f * (q[2] * q[2] + q[3] * q[3]));

  // Вывод в Serial
  Serial.print("Roll: "); Serial.print(roll * 57.2958f);
  Serial.print(" | Pitch: "); Serial.print(pitch * 57.2958f);
  Serial.print(" | Yaw: "); Serial.println(yaw * 57.2958f);

  delay(10); // 100 Гц
}

void madgwickFilter(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz) {
  float qDot[4], s[4];

  // Интеграция гироскопа
  qDot[0] = 0.5f * (-q[1] * gx - q[2] * gy - q[3] * gz);
  qDot[1] = 0.5f * (q[0] * gx + q[2] * gz - q[3] * gy);
  qDot[2] = 0.5f * (q[0] * gy - q[1] * gz + q[3] * gx);
  qDot[3] = 0.5f * (q[0] * gz + q[1] * gy - q[2] * gx);

  // Ошибка акселерометра
  float f[6] = {
    2.0f * (q[1] * q[3] - q[0] * q[2]) - ax,
    2.0f * (q[0] * q[1] + q[2] * q[3]) - ay,
    2.0f * (0.5f - q[1] * q[1] - q[2] * q[2]) - az,
    0, 0, 0 // Магнитометр добавим ниже
  };

  // Преобразование магнитометра в глобальную систему координат
  float bx = mx * (0.5f - q[2] * q[2] - q[3] * q[3]) + mz * (q[1] * q[3] - q[0] * q[2]);
  float bz = mx * (q[1] * q[2] - q[0] * q[3]) + my * (q[0] * q[0] + q[1] * q[1] - 0.5f) + mz * (q[0] * q[1] + q[2] * q[3]);

  // Ошибка магнитометра (предполагаем, что магнитное поле направлено только по X и Z)
  f[3] = bx - mx; // Ошибка по X
  f[4] = 0;       // Y игнорируем для простоты
  f[5] = bz - mz; // Ошибка по Z

  // Градиент (объединяем акселерометр и магнитометр)
  s[0] = -2.0f * q[2] * f[0] + 2.0f * q[1] * f[1] + 2.0f * q[3] * f[3] - 2.0f * q[2] * f[5];
  s[1] = 2.0f * q[3] * f[0] + 2.0f * q[0] * f[1] - 4.0f * q[1] * f[2] + 2.0f * q[2] * f[3] + 2.0f * q[1] * f[5];
  s[2] = -2.0f * q[0] * f[0] + 2.0f * q[3] * f[1] - 4.0f * q[2] * f[2] - 2.0f * q[1] * f[3] + 2.0f * q[0] * f[5];
  s[3] = 2.0f * q[1] * f[0] + 2.0f * q[2] * f[1] + 2.0f * q[0] * f[3] - 2.0f * q[3] * f[5];

  // Коррекция
  for (int i = 0; i < 4; i++) {
    qDot[i] -= beta * s[i];
    q[i] += qDot[i] * dt;
  }

  // Нормализация кватерниона
  float norm = sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
  for (int i = 0; i < 4; i++) q[i] /= norm;
}