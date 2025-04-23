#include "motors.h"
#include <Arduino.h>
#undef min
#undef max
#include <servo.h>

Preferences prefs;
std::vector<ServoConfig> motors;
bool motorsEnabled = false;

void applyServo(ServoConfig &m) {
    if (!motorsEnabled) return;  // ← Блокировка!
    int baseAngle = m.angle + m.offset;
    if (m.isMirrored) baseAngle = 180 - baseAngle;
    int corrected = constrain(baseAngle, m.minAngle, m.maxAngle);
    m.servo.write(corrected);
  
    Serial.printf("[%s] Угол: %d → %d (мин: %d, макс: %d, зеркально: %s)\n",
      m.id.c_str(), m.angle, corrected, m.minAngle, m.maxAngle, m.isMirrored ? "да" : "нет");
  }

  void setupMotors() {
    motors = {
      {"camL1", "Камера Лево (Тангаж A)", SERVO_7_PIN, 90, 75, 105, 0, false, 14.0/24.0, 90},
      {"camL2", "Камера Лево (Тангаж B)", SERVO_9_PIN, 90, 75, 105, 0, true, 14.0/24.0, 90},
      {"camL_YAW", "Камера Лево (Рыскание)", SERVO_2_PIN, 135, 90, 180, 0, false, 14.0/24.0, 90},
  
      {"camC1", "Камера Центр (Тангаж A)", SERVO_8_PIN, 90, 75, 105, 0, false, 14.0/24.0, 0},
      {"camC2", "Камера Центр (Тангаж B)", SERVO_5_PIN, 90, 75, 105, 0, true, 14.0/24.0, 0},
      {"camC_YAW", "Камера Центр (Рыскание)", SERVO_3_PIN, 135, 90, 180, 0, false, 14.0/24.0, 0},
  
      {"camR1", "Камера Право (Тангаж A)", SERVO_4_PIN, 90, 75, 105, 0, false, 14.0/24.0, -90},
      {"camR2", "Камера Право (Тангаж B)", SERVO_6_PIN, 90, 75, 105, 0, true, 14.0/24.0, -90},
      {"camR_YAW", "Камера Право (Рыскание)", SERVO_1_PIN, 135, 90, 180, 0, false, 14.0/24.0, -90}
    };
  
    prefs.begin("motors", false);
    for (auto &m : motors) {
      m.minAngle = prefs.getInt((m.id + "_min").c_str(), m.minAngle);
      m.maxAngle = prefs.getInt((m.id + "_max").c_str(), m.maxAngle);
      m.offset   = prefs.getInt((m.id + "_offset").c_str(), m.offset);
      // ВНИМАНИЕ: attach НЕ вызывается здесь
    }
    prefs.end();
  }
  
  void enableMotors() {
    if (motorsEnabled) return;
  
    const int freq = 200;
    for (auto &m : motors) {
      m.servo.attach(
        m.pin,
        Servo::CHANNEL_NOT_ATTACHED,
        Servo::DEFAULT_MIN_ANGLE,
        Servo::DEFAULT_MAX_ANGLE,
        500, 2400,
        freq
      );
      applyServo(m);
    }
  
    motorsEnabled = true;
    Serial.println("[enable] Все сервы активированы.");
  }
  // Преобразование из угла сервы (0–180) в угол камеры (в градусах)
  int servoToCameraAngle(const ServoConfig& m) {
    return round((m.angle - 90 + m.offset) * m.gearRatio + m.cameraZero);
  }

  // Обратное преобразование: из угла камеры в угол сервы
  int cameraToServoAngle(const ServoConfig& m, int cameraAngle) {
    return round((cameraAngle - m.cameraZero) / m.gearRatio - m.offset + 90);
  }