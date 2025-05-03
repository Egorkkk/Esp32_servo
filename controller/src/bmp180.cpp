
#include <Wire.h>
#include <Arduino.h>
#include "bmp180.h"

// Калибровочные данные BMP180
int16_t AC1, AC2, AC3, B1_cal, B2, MB, MC, MD;
uint16_t AC4, AC5, AC6;


int16_t bmp180ReadInt(uint8_t address) {
  Wire.beginTransmission(BMP180_ADDR);
  Wire.write(address);
  Wire.endTransmission(false);
  Wire.requestFrom(BMP180_ADDR, 2, true);
  return (Wire.read() << 8) | Wire.read();
}

// Функции для BMP180
bool bmp180Init() {
  Wire.beginTransmission(BMP180_ADDR);
  if (Wire.endTransmission() != 0) return false;

  // Чтение калибровочных данных
  AC1 = bmp180ReadInt(BMP180_CAL_AC1);
  AC2 = bmp180ReadInt(BMP180_CAL_AC1 + 2);
  AC3 = bmp180ReadInt(BMP180_CAL_AC1 + 4);
  AC4 = bmp180ReadInt(BMP180_CAL_AC1 + 6);
  AC5 = bmp180ReadInt(BMP180_CAL_AC1 + 8);
  AC6 = bmp180ReadInt(BMP180_CAL_AC1 + 10);
  B1_cal = bmp180ReadInt(BMP180_CAL_AC1 + 12);
  B2 = bmp180ReadInt(BMP180_CAL_AC1 + 14);
  MB = bmp180ReadInt(BMP180_CAL_AC1 + 16);
  MC = bmp180ReadInt(BMP180_CAL_AC1 + 18);
  MD = bmp180ReadInt(BMP180_CAL_AC1 + 20);

  return true;
}

float bmp180GetTemperature() {
  // Запуск измерения температуры
  Wire.beginTransmission(BMP180_ADDR);
  Wire.write(BMP180_CONTROL);
  Wire.write(BMP180_READ_TEMP_CMD);
  Wire.endTransmission(true);
  delay(5); // Ждем 4.5 мс

  // Чтение сырых данных
  Wire.beginTransmission(BMP180_ADDR);
  Wire.write(BMP180_DATA);
  Wire.endTransmission(false);
  Wire.requestFrom(BMP180_ADDR, 2, true);
  int32_t ut = (Wire.read() << 8) | Wire.read();

  // Вычисление температуры
  int32_t x1 = ((ut - AC6) * AC5) >> 15;
  int32_t x2 = (MC << 11) / (x1 + MD);
  int32_t b5 = x1 + x2;
  float temp = (b5 + 8) >> 4;
  return temp / 10.0f;
}

float bmp180GetPressure() {
  // Запуск измерения давления (максимальная точность)
  Wire.beginTransmission(BMP180_ADDR);
  Wire.write(BMP180_CONTROL);
  Wire.write(BMP180_READ_PRESS_CMD + (3 << 6)); // OSS = 3 (ultra high resolution)
  Wire.endTransmission(true);
  delay(26); // Ждем 25.5 мс

  // Чтение сырых данных
  Wire.beginTransmission(BMP180_ADDR);
  Wire.write(BMP180_DATA);
  Wire.endTransmission(false);
  Wire.requestFrom(BMP180_ADDR, 3, true);
  int32_t up = ((Wire.read() << 16) | (Wire.read() << 8) | Wire.read()) >> (8 - 3);

  // Вычисление давления
  int32_t b6 = (((int32_t)(bmp180GetTemperature() * 10) << 4) - 8) - AC6;
  int32_t x1 = (b6 * b6) >> 12;
  int32_t x2 = (AC2 * b6) >> 11;
  int32_t x3 = x1 + x2;
  int32_t b3 = (((AC1 * 4 + x3) << 3) + 2) / 4;
  x1 = (AC3 * b6) >> 13;
  x2 = (B1_cal * ((b6 * b6) >> 12)) >> 16;
  x3 = (x1 + x2 + 2) >> 2;
  uint32_t b4 = (AC4 * (uint32_t)(x3 + 32768)) >> 15;
  uint32_t b7 = ((uint32_t)up - b3) * (50000 >> 3);
  int32_t p = (b7 < 0x80000000) ? (b7 * 2) / b4 : (b7 / b4) * 2;
  x1 = (p >> 8) * (p >> 8);
  x1 = (x1 * 3038) >> 16;
  x2 = (-7357 * p) >> 16;
  p = p + ((x1 + x2 + 3791) >> 4);
  return p / 100.0f; // Давление в гПа
}

float bmp180GetAltitude(float pressure) {
  // Вычисление высоты относительно давления на уровне моря (1013.25 гПа)
  float seaLevelPressure = 1013.25f;
  return 44330.0f * (1.0f - pow(pressure / seaLevelPressure, 0.1903f));
}



/*
void setup() {


  // Инициализация QMC5883L
  Wire.beginTransmission(QMC5883L_ADDR);
  Wire.write(QMC5883L_CONFIG_REG_A);
  Wire.write(0x01); // Режим работы: непрерывное измерение, 200 Гц, 512 отсчетов
  Wire.endTransmission(true);

  // Инициализация BMP180
  if (!bmp180Init()) {
    Serial.println("BMP180 initialization failed!");
    while (1);
  }


  Serial.println("Initialization complete");
}

void loop() {


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

  // BMP180
  float temperature = bmp180GetTemperature();
  float pressure = bmp180GetPressure();
  float altitude = bmp180GetAltitude(pressure);

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
*/
