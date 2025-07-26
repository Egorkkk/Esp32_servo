#include "display.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define OLED_SDA 42
#define OLED_SCL 41

static TwoWire oledWire(1); // используем Wire1

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &oledWire, OLED_RESET);
static int currentLine = 0;

void setupDisplay() {
  oledWire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[DISPLAY] ❌ SSD1306 init failed");
    return;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("GyroLogger Init...");
  display.display();
  currentLine = 1;
}

void showInitStatus(const char* component, bool success) {
  if (currentLine >= 8) return; // 8 строк по высоте

  display.setCursor(0, currentLine * 8);
  display.print(component);
  display.print(": ");
  display.println(success ? "OK" : "FAIL");
  display.display();
  currentLine++;
}

void showMessage(const char* message) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(message);
  display.display();
  currentLine = 1;
}

void clearDisplay() {
  display.clearDisplay();
  display.display();
  currentLine = 0;
}

void updateStatusScreen(bool gpsValid, double gpsTime, bool isLogging, unsigned long logDurationSec, float batteryVoltage, const char* canStatusStr) {
  
  display.clearDisplay();
  display.setCursor(0, 0);

  if (gpsValid) {
    display.println("GPS: OK");

    int hours = int(gpsTime / 3600) % 24;
    int minutes = int(gpsTime / 60) % 60;
    int seconds = int(gpsTime) % 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "T: %02d:%02d:%02d", hours, minutes, seconds);
    display.println(buf);
  } else {
    // 👇 Изменено здесь
    display.println("GPS: LOST");
    display.println("T: --:--:--");
  }

  if (isLogging) {
    display.println("LOGGING...");
    char buf[16];
    snprintf(buf, sizeof(buf), "REC: %02lu:%02lu", logDurationSec / 60, logDurationSec % 60);
    display.println(buf);
  } else {
    display.println("IDLE");
  }

  if (canStatusStr) {
    display.println(canStatusStr);
  } else {
    display.println("CAN: ---");
  }


  char buf2[16];
  snprintf(buf2, sizeof(buf2), "Battery: %.2f V", batteryVoltage);
  display.setCursor(0, 56); // Нижняя строка
  display.println(buf2);

  display.display();
}
