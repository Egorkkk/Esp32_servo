#include <Arduino.h>
#include "driver/twai.h"

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
static uint8_t lastSeqID_100 = 0xFF;
static uint8_t lastSeqID_101 = 0xFF;
static uint8_t lastSeqID_102 = 0xFF;
static uint32_t lost_100 = 0, lost_101 = 0, lost_102 = 0;
static unsigned long lastPrint = 0;

void IRAM_ATTR handleEncoder() {
  bool A = digitalRead(ENC_A);
  bool B = digitalRead(ENC_B);
  if (A == B) {
    encoderCount++;
  } else {
    encoderCount--;
  }
}

void setMotorPWM(int value) {
  value = constrain(value, -255, 255);
  pwmValue = value;
  ledcWrite(PWM_CH_RPWM, 0);
  ledcWrite(PWM_CH_LPWM, 0);
  delayMicroseconds(100);

  if (value > 0) {
    ledcWrite(PWM_CH_RPWM, value);
  } else if (value < 0) {
    ledcWrite(PWM_CH_LPWM, -value);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(ENC_A, INPUT);
  pinMode(ENC_B, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENC_A), handleEncoder, CHANGE);

  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(REN, OUTPUT);
  pinMode(LEN, OUTPUT);
  digitalWrite(REN, HIGH);
  digitalWrite(LEN, HIGH);

  ledcSetup(PWM_CH_RPWM, 20000, 8);
  ledcSetup(PWM_CH_LPWM, 20000, 8);
  ledcAttachPin(RPWM, PWM_CH_RPWM);
  ledcAttachPin(LPWM, PWM_CH_LPWM);

  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)15, (gpio_num_t)18, TWAI_MODE_NORMAL);
  g_config.rx_queue_len = 20;
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    Serial.println("[CAN] ❌ Ошибка установки драйвера");
  } else if (twai_start() != ESP_OK) {
    Serial.println("[CAN] ❌ Ошибка запуска CAN");
  } else {
    Serial.println("[CAN] ✅ CAN-шина инициализирована");
  }
}

void loop() {
  twai_message_t message;
  int currentCount = encoderCount;
  int delta = currentCount - lastEncoderCount;
  lastEncoderCount = currentCount;
  int pwm = delta * 20;
  if (abs(pwm) < 5) pwm = 0;
  pwm = constrain(pwm, -255, 255);
  setMotorPWM(pwm);

  while (twai_receive(&message, 0) == ESP_OK) {
    uint8_t seqID = message.data[0];
    uint32_t id = message.identifier;

    uint8_t* lastSeq = nullptr;
    if (id == 0x100) lastSeq = &lastSeqID_100;
    else if (id == 0x101) lastSeq = &lastSeqID_101;
    else if (id == 0x102) lastSeq = &lastSeqID_102;

    if (lastSeq) {
      if (*lastSeq != 0xFF && (uint8_t)(seqID - *lastSeq) != 1) {
        uint8_t lost = (uint8_t)(seqID - *lastSeq) - 1;
        if (id == 0x100) lost_100 += lost;
        else if (id == 0x101) lost_101 += lost;
        else if (id == 0x102) lost_102 += lost;
      }
      *lastSeq = seqID;
    }
  }

  if (millis() - lastPrint > 1000) {
    Serial.printf("[CAN] Lost frames/s → 0x100: %lu, 0x101: %lu, 0x102: %lu\n", lost_100, lost_101, lost_102);
    lost_100 = lost_101 = lost_102 = 0;
    lastPrint = millis();
  }

  delay(20);
}
