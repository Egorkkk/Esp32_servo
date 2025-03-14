#include <Wire.h>
#include <Arduino.h>

// Адреса I2C устройств
#define MPU6050_ADDR 0x68  // Адрес MPU-6050 по умолчанию
#define QMC5883L_ADDR 0x0D // Адрес QMC5883L по умолчанию

// Регистры MPU-6050
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_GYRO_XOUT_H 0x43

// Регистры QMC5883L
#define QMC5883L_CONFIG_REG_A 0x09
#define QMC5883L_DATA_XOUT_LSB 0x00

void setup() {
  Serial.begin(115200); // Инициализация Serial для вывода данных
  Wire.begin();         // Инициализация I2C (SDA и SCL по умолчанию на ESP32: 21 и 22)

  // Инициализация MPU-6050
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_PWR_MGMT_1); // Регистр управления питанием
  Wire.write(0x00);               // Вывод из спящего режима
  Wire.endTransmission(true);

  // Инициализация QMC5883L
  Wire.beginTransmission(QMC5883L_ADDR);
  Wire.write(QMC5883L_CONFIG_REG_A);
  Wire.write(0x01); // Режим работы: непрерывное измерение, 200 Гц, 512 отсчетов
  Wire.endTransmission(true);

  Serial.println("Initialization complete");
}

void loop() {
  // Считывание данных с MPU-6050
  int16_t accelX, accelY, accelZ, gyroX, gyroY, gyroZ;

  // Чтение акселерометра
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_ACCEL_XOUT_H); // Начальный регистр данных акселерометра
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 6, true); // Запрос 6 байт (X, Y, Z)
  accelX = Wire.read() << 8 | Wire.read();
  accelY = Wire.read() << 8 | Wire.read();
  accelZ = Wire.read() << 8 | Wire.read();
  Wire.endTransmission(true);

  // Чтение гироскопа
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_GYRO_XOUT_H); // Начальный регистр данных гироскопа
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 6, true); // Запрос 6 байт (X, Y, Z)
  gyroX = Wire.read() << 8 | Wire.read();
  gyroY = Wire.read() << 8 | Wire.read();
  gyroZ = Wire.read() << 8 | Wire.read();
  Wire.endTransmission(true);

  // Считывание данных с QMC5883L
  int16_t magX, magY, magZ;
  Wire.beginTransmission(QMC5883L_ADDR);
  Wire.write(QMC5883L_DATA_XOUT_LSB); // Начальный регистр данных магнитометра
  Wire.endTransmission(false);
  Wire.requestFrom(QMC5883L_ADDR, 6, true); // Запрос 6 байт (X, Y, Z)
  magX = Wire.read() | Wire.read() << 8; // QMC5883L: младший байт первый
  magY = Wire.read() | Wire.read() << 8;
  magZ = Wire.read() | Wire.read() << 8;
  Wire.endTransmission(true);

  // Вывод данных в Serial Monitor
  Serial.print("Accel X: "); Serial.print(accelX);
  Serial.print(" | Y: "); Serial.print(accelY);
  Serial.print(" | Z: "); Serial.print(accelZ);
  Serial.print(" | Gyro X: "); Serial.print(gyroX);
  Serial.print(" | Y: "); Serial.print(gyroY);
  Serial.print(" | Z: "); Serial.print(gyroZ);
  Serial.print(" | Mag X: "); Serial.print(magX);
  Serial.print(" | Y: "); Serial.print(magY);
  Serial.print(" | Z: "); Serial.println(magZ);

  delay(500); // Задержка для читаемости
}