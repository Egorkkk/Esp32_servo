// bmp180.h

#ifndef _BMP180_H_
#define _BMP180_H_

#define BMP180_ADDR 0x77

// Регистры BMP180
#define BMP180_CAL_AC1 0xAA  // Калибровочные данные
#define BMP180_CONTROL 0xF4
#define BMP180_DATA 0xF6
#define BMP180_READ_TEMP_CMD 0x2E
#define BMP180_READ_PRESS_CMD 0x34
#define I2C_SDA_PIN 18  // Новый пин для SDA
#define I2C_SCL_PIN 19  // Новый пин для SCL

//int16_t bmp180ReadInt(uint8_t address);
bool bmp180Init();
float bmp180GetTemperature();
float bmp180GetPressure();
float bmp180GetAltitude(float pressure);



#endif