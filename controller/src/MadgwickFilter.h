// MadgwickFilter.h
#ifndef MADGWICK_FILTER_H
#define MADGWICK_FILTER_H

// Структура для хранения кватерниона и параметров фильтра
typedef struct {
    float q[4];  // Кватернион (w, x, y, z)
    float beta;  // Коэффициент фильтра
    float dt;    // Шаг времени в секундах
} MadgwickFilter;

// Инициализация фильтра
void madgwickInit(MadgwickFilter* filter, float beta, float dt);

// Обновление кватерниона с использованием данных акселерометра, гироскопа и магнитометра
void madgwickUpdate(MadgwickFilter* filter, float ax, float ay, float az, 
                    float gx, float gy, float gz, float mx, float my, float mz);

// Получение углов Эйлера (roll, pitch, yaw) в радианах
void madgwickGetEulerAngles(MadgwickFilter* filter, float* roll, float* pitch, float* yaw);

#endif // MADGWICK_FILTER_H