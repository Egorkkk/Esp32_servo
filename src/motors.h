// motors.h — интерфейс управления сервомоторами

#pragma once

#include <vector>
#include <servo.h>
#include <Preferences.h>

// Определения пинов (можно вынести в pins.h позже)
#define SERVO_1_PIN  16
#define SERVO_2_PIN  17
#define SERVO_3_PIN  18
#define SERVO_4_PIN  15
#define SERVO_5_PIN  2
#define SERVO_6_PIN  4
#define SERVO_7_PIN  27
#define SERVO_8_PIN  14
#define SERVO_9_PIN  12

// Структура конфигурации мотора
struct ServoConfig {
    String id;
    String cameraName;
    int pin;
    int angle;
    int minAngle;
    int maxAngle;
    int offset;
    bool isMirrored;
    Servo servo;
  
    ServoConfig(
      const String& _id,
      const String& _name,
      int _pin,
      int _angle = 90,
      int _minAngle = 0,
      int _maxAngle = 180,
      int _offset = 0,
      bool _isMirrored = false
    )
      : id(_id),
        cameraName(_name),
        pin(_pin),
        angle(_angle),
        minAngle(_minAngle),
        maxAngle(_maxAngle),
        offset(_offset),
        isMirrored(_isMirrored)
    {}
  };
extern std::vector<ServoConfig> motors;
extern bool motorsEnabled;

void applyServo(ServoConfig &m);
void setupMotors();
void enableMotors();