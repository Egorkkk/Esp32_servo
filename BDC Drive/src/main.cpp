#include <Arduino.h>

// Пины энкодера
#define ENC_A 35
#define ENC_B 36

// Пины драйвера BTS7960
#define RPWM 9
#define LPWM 10
#define REN  11
#define LEN  8

// ШИМ-каналы
#define PWM_CH_RPWM 0
#define PWM_CH_LPWM 1

// Глобальные переменные
volatile int encoderCount = 0;
int lastEncoderCount = 0;
int pwmValue = 0;

// Прерывание от канала A энкодера
void IRAM_ATTR handleEncoder() {
  bool A = digitalRead(ENC_A);
  bool B = digitalRead(ENC_B);
  if (A == B) {
    encoderCount++;
  } else {
    encoderCount--;
  }
}

// Безопасная установка PWM
void setMotorPWM(int value) {
  value = constrain(value, -255, 255);
  pwmValue = value;

  // Остановить оба плеча
  ledcWrite(PWM_CH_RPWM, 0);
  ledcWrite(PWM_CH_LPWM, 0);
  delayMicroseconds(100);  // Dead-time

  if (value > 0) {
    ledcWrite(PWM_CH_RPWM, value);
  } else if (value < 0) {
    ledcWrite(PWM_CH_LPWM, -value);
  }
}

void setup() {
  Serial.begin(115200);

  // Пины энкодера
  pinMode(ENC_A, INPUT);
  pinMode(ENC_B, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENC_A), handleEncoder, CHANGE);

  // Пины BTS7960
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(REN, OUTPUT);
  pinMode(LEN, OUTPUT);
  digitalWrite(REN, HIGH);
  digitalWrite(LEN, HIGH);

  // ШИМ
  ledcSetup(PWM_CH_RPWM, 20000, 8);
  ledcSetup(PWM_CH_LPWM, 20000, 8);
  ledcAttachPin(RPWM, PWM_CH_RPWM);
  ledcAttachPin(LPWM, PWM_CH_LPWM);
}

void loop() {
  int currentCount = encoderCount;
  int delta = currentCount - lastEncoderCount;
  lastEncoderCount = currentCount;

  // Преобразование движения энкодера в PWM
  int pwm = delta * 20;
  if (abs(pwm) < 5) pwm = 0;
  pwm = constrain(pwm, -255, 255);

  setMotorPWM(pwm);

  // Вывод в консоль
  Serial.print("Encoder: ");
  Serial.print(currentCount);
  Serial.print(" | Delta: ");
  Serial.print(delta);
  Serial.print(" | PWM: ");
  Serial.println(pwmValue);

  delay(20);
}