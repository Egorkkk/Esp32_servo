// MadgwickFilter.c
#include "MadgwickFilter.h"
#include <math.h>

// Инициализация фильтра
void madgwickInit(MadgwickFilter* filter, float beta, float dt) {
    filter->q[0] = 1.0f; // w
    filter->q[1] = 0.0f; // x
    filter->q[2] = 0.0f; // y
    filter->q[3] = 0.0f; // z
    filter->beta = beta;
    filter->dt = dt;
}

// Обновление кватерниона
void madgwickUpdate(MadgwickFilter* filter, float ax, float ay, float az, 
                    float gx, float gy, float gz, float mx, float my, float mz) {
    float qDot[4], s[4];
    float q[4] = {filter->q[0], filter->q[1], filter->q[2], filter->q[3]};

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

    // Ошибка магнитометра
    f[3] = bx - mx;
    f[4] = 0; // Y игнорируем для простоты
    f[5] = bz - mz;

    // Градиент
    s[0] = -2.0f * q[2] * f[0] + 2.0f * q[1] * f[1] + 2.0f * q[3] * f[3] - 2.0f * q[2] * f[5];
    s[1] = 2.0f * q[3] * f[0] + 2.0f * q[0] * f[1] - 4.0f * q[1] * f[2] + 2.0f * q[2] * f[3] + 2.0f * q[1] * f[5];
    s[2] = -2.0f * q[0] * f[0] + 2.0f * q[3] * f[1] - 4.0f * q[2] * f[2] - 2.0f * q[1] * f[3] + 2.0f * q[0] * f[5];
    s[3] = 2.0f * q[1] * f[0] + 2.0f * q[2] * f[1] + 2.0f * q[0] * f[3] - 2.0f * q[3] * f[5];

    // Коррекция
    for (int i = 0; i < 4; i++) {
        qDot[i] -= filter->beta * s[i];
        q[i] += qDot[i] * filter->dt;
    }

    // Нормализация кватерниона
    float norm = sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (norm > 0.0001f) { // Защита от деления на ноль
        for (int i = 0; i < 4; i++) q[i] /= norm;
    }

    // Обновление кватерниона в структуре
    for (int i = 0; i < 4; i++) filter->q[i] = q[i];
}

// Получение углов Эйлера
void madgwickGetEulerAngles(MadgwickFilter* filter, float* roll, float* pitch, float* yaw) {
    *roll = atan2(2.0f * (filter->q[0] * filter->q[1] + filter->q[2] * filter->q[3]), 
                  1.0f - 2.0f * (filter->q[1] * filter->q[1] + filter->q[2] * filter->q[2]));
    *pitch = asin(2.0f * (filter->q[0] * filter->q[2] - filter->q[3] * filter->q[1]));
    *yaw = atan2(2.0f * (filter->q[0] * filter->q[3] + filter->q[1] * filter->q[2]), 
                 1.0f - 2.0f * (filter->q[2] * filter->q[2] + filter->q[3] * filter->q[3]));
}