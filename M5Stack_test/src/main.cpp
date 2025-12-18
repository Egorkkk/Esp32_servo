// === main.cpp ===
#include <M5AtomS3.h>
#include <math.h>

extern "C" {
  #include "Fusion.h"  // путь зависит от структуры каталога
}

struct Vec3 { float x, y, z; };

// Глобальные переменные
FusionAhrs ahrs;
Vec3 cubeVertices[8] = {
  {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
  {-1, -1,  1}, {1, -1,  1}, {1, 1,  1}, {-1, 1,  1}
};
uint8_t cubeEdges[12][2] = {
  {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

void drawCube(float roll, float pitch, float yaw) {
  float cr = cos(roll), sr = sin(roll);
  float cp = cos(pitch), sp = sin(pitch);
  float cy = cos(yaw), sy = sin(yaw);

  Vec3 transformed[8];
  for (int i = 0; i < 8; i++) {
    float x = cubeVertices[i].x;
    float y = cubeVertices[i].y;
    float z = cubeVertices[i].z;

    float x1 = cy * x - sy * z;
    float z1 = sy * x + cy * z;
    float y1 = cp * y - sp * z1;
    float z2 = sp * y + cp * z1;
    float x2 = cr * x1 - sr * y1;
    float y2 = sr * x1 + cr * y1;

    transformed[i] = {x2, y2, z2};
  }

  AtomS3.Display.fillRect(0, 48, 128, 80, BLACK);
  for (int i = 0; i < 12; i++) {
    Vec3 p1 = transformed[cubeEdges[i][0]];
    Vec3 p2 = transformed[cubeEdges[i][1]];
    int x0 = 64 + p1.x * 20;
    int y0 = 64 + p1.y * 20;
    int x1 = 64 + p2.x * 20;
    int y1 = 64 + p2.y * 20;
    AtomS3.Display.drawLine(x0, y0, x1, y1, YELLOW);
  }
}

void updateOrientation(float& roll, float& pitch, float& yaw) {
  float ax, ay, az, gx, gy, gz;
  AtomS3.Imu.getAccelData(&ax, &ay, &az);
  AtomS3.Imu.getGyroData(&gx, &gy, &gz);

  FusionVector gyro = {gx * DEG_TO_RAD, gy * DEG_TO_RAD, gz * DEG_TO_RAD};
  FusionVector accel = {ax, ay, az};
  FusionVector mag = {0.0f, 0.0f, 0.0f};

  static uint32_t lastMicros = micros();
  uint32_t now = micros();
  float dt = (now - lastMicros) / 1e6f;
  lastMicros = now;

  FusionAhrsUpdate(&ahrs, gyro, accel, mag, dt);

  FusionEuler eul = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));
  roll  = eul.angle.roll * DEG_TO_RAD;
  pitch = eul.angle.pitch * DEG_TO_RAD;
  yaw   = eul.angle.yaw * DEG_TO_RAD;
}

void setup() {
  AtomS3.begin();
  AtomS3.Display.setRotation(0);
  AtomS3.Display.setFont(&fonts::Font2);
  AtomS3.Display.setTextColor(GREEN, BLACK);

  FusionAhrsInitialise(&ahrs);  // инициализация фильтра
}

void loop() {
  static float roll, pitch, yaw;

  static uint32_t lastUpdate = 0;
  if (millis() - lastUpdate >= 10) {
    lastUpdate = millis();
    updateOrientation(roll, pitch, yaw);
  }

  static uint32_t lastDraw = 0;
  if (millis() - lastDraw >= 33) {
    lastDraw = millis();
    drawCube(roll, pitch, yaw);
    AtomS3.Display.fillRect(0, 0, 128, 48, BLACK);
    AtomS3.Display.setCursor(0, 0);
    AtomS3.Display.printf("Roll:  %.1f\n", roll * RAD_TO_DEG);
    AtomS3.Display.printf("Pitch: %.1f\n", pitch * RAD_TO_DEG);
    AtomS3.Display.printf("Yaw:   %.1f\n", yaw * RAD_TO_DEG);
  }
}
