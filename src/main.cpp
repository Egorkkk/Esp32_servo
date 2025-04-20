#include <Arduino.h>
//#include "lcdgfx.h"
//#include "lcdgfx_gui.h"
#undef min
#undef max
#include <ArduinoJson.h>
#include <Preferences.h>
#include "FS.h"
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "Servo.h"


#define BUTTON_1_PIN  25
#define BUTTON_2_PIN  26
#define BUTTON_3_PIN  32
#define BUTTON_4_PIN  33

bool lastState1 = HIGH;
bool lastState2 = HIGH;
bool lastState3 = HIGH;
bool lastState4 = HIGH;


#define SERVO_1_PIN  16
#define SERVO_2_PIN  17
#define SERVO_3_PIN  18
#define SERVO_4_PIN  15
#define SERVO_5_PIN  2
#define SERVO_6_PIN  4
#define SERVO_7_PIN  27
#define SERVO_8_PIN  14
#define SERVO_9_PIN  12

bool motorsEnabled = false;

/*
Servo servos[9];
int servoPins[9] = {
  SERVO_1_PIN, SERVO_2_PIN, SERVO_3_PIN, SERVO_4_PIN, SERVO_5_PIN,
  SERVO_6_PIN, SERVO_7_PIN, SERVO_8_PIN, SERVO_9_PIN
};
*/

int currentServo = 0;

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

Preferences prefs;
std::vector<ServoConfig> motors;

//DisplaySH1106_128x64_SPI display(22,{-1, 5, 21, 0,23,19});

static AsyncWebServer server(80);

#define FORMAT_LITTLEFS_IF_FAILED true

const char* ssid = "Robocast2.4G";
const char* password = "1008121982";
IPAddress local_IP(192, 168, 0, 200);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if(!root){
      Serial.println("- failed to open directory");
      return;
  }
  if(!root.isDirectory()){
      Serial.println(" - not a directory");
      return;
  }

  File file = root.openNextFile();
  while(file){
      if(file.isDirectory()){
          Serial.print("  DIR : ");
          Serial.println(file.name());
          if(levels){
              listDir(fs, file.path(), levels -1);
          }
      } else {
          Serial.print("  FILE: ");
          Serial.print(file.name());
          Serial.print("\tSIZE: ");
          Serial.println(file.size());
      }
      file = root.openNextFile();
  }
}

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
  const int SERVO_PWM_FREQ = 200; // рекомендуемая частота для ESP32-S2/S3/C3, допустима и на обычном ESP32

  motors = {
    // Камера Лево
    {"camL1", "Камера Лево (Тангаж A)", SERVO_7_PIN, 90, 75, 105, 0, false},
    {"camL2", "Камера Лево (Тангаж B)", SERVO_9_PIN, 90, 75, 105, 0, true},
    {"camL_YAW", "Камера Лево (Рыскание)", SERVO_1_PIN, 135, 90, 180, 0},

    // Камера Центр
    {"camC1", "Камера Центр (Тангаж A)", SERVO_8_PIN, 90, 75, 105, 0, false},
    {"camC2", "Камера Центр (Тангаж B)", SERVO_5_PIN, 90, 75, 105, 0, true},
    {"camC_YAW", "Камера Центр (Рыскание)", SERVO_2_PIN, 135, 90, 180, 0},

    // Камера Право
    {"camR1", "Камера Право (Тангаж A)", SERVO_4_PIN, 90, 75, 105, 0, false},
    {"camR2", "Камера Право (Тангаж B)", SERVO_6_PIN, 90, 75, 105, 0, true},
    {"camR_YAW", "Камера Право (Рыскание)", SERVO_3_PIN, 135, 90, 180, 0}
  };

  prefs.begin("motors", false);
  for (auto &m : motors) {
    m.minAngle = prefs.getInt((m.id + "_minAngle").c_str(), m.minAngle);
    m.maxAngle = prefs.getInt((m.id + "_maxAngle").c_str(), m.maxAngle);
    m.offset   = prefs.getInt((m.id + "_offset").c_str(), m.offset);

    bool ok = m.servo.attach(
      m.pin,
      Servo::CHANNEL_NOT_ATTACHED,
      Servo::DEFAULT_MIN_ANGLE,
      Servo::DEFAULT_MAX_ANGLE,
      500, 2400, // микросекунды
      SERVO_PWM_FREQ
    );

    Serial.printf("[attach] %s → GPIO %d → %s\n",
      m.id.c_str(),
      m.pin,
      ok ? "успешно" : "ОШИБКА!"
    );
  }
  prefs.end();
}

void setupServer() {
  // Главная страница с интерфейсом
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/style.css", LittleFS, "/style.css");
  server.serveStatic("/BMDevice.js", LittleFS, "/BMDevice.js");
  server.serveStatic("/web-ui.js", LittleFS, "/web-ui.js");

   // API: получить параметры мотора
   server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!request->hasParam("id")) {
      request->send(400, "text/plain", "Missing motor ID");
      return;
    }
    String id = request->getParam("id")->value();
    for (auto &m : motors) {
      if (m.id == id) {
        DynamicJsonDocument doc(256);
        doc["angle"] = m.angle;
        doc["min"] = m.minAngle;
        doc["max"] = m.maxAngle;
        doc["offset"] = m.offset;

        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
        return;
      }
    }
    request->send(404, "text/plain", "Motor not found");
  });

  // API: установить параметры мотора
  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!request->hasParam("id")) {
      request->send(400, "text/plain", "Missing motor ID");
      return;
    }
  
    String id = request->getParam("id")->value();
    bool save = request->hasParam("save") && request->getParam("save")->value() == "true";
  
    for (auto &m : motors) {
      if (m.id == id) {
        if (request->hasParam("angle")) {
          m.angle = request->getParam("angle")->value().toInt();
          applyServo(m);  // угол применим всегда
        }
  
        if (request->hasParam("min")) m.minAngle = request->getParam("min")->value().toInt();
        if (request->hasParam("max")) m.maxAngle = request->getParam("max")->value().toInt();
        if (request->hasParam("offset")) m.offset = request->getParam("offset")->value().toInt();
  
        if (save) {
          prefs.begin("motors", false);
          prefs.putInt((id + "_minAngle").c_str(), m.minAngle);
          prefs.putInt((id + "_maxAngle").c_str(), m.maxAngle);
          prefs.putInt((id + "_offset").c_str(), m.offset);
          prefs.end();
          Serial.printf("Сохранены параметры для %s\n", id.c_str());
        }
  
        request->send(200, "text/plain", "OK");
        return;
      }
    }
  
    request->send(404, "text/plain", "Motor not found");
  });

  server.on("/enable", HTTP_GET, [](AsyncWebServerRequest *request){
    motorsEnabled = true;
    for (auto &m : motors) applyServo(m);  // применим сохранённые значения
    Serial.println("Моторы активированы вручную.");
    request->send(200, "text/plain", "Motors enabled");
  });

  server.begin();
}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  pinMode(BUTTON_3_PIN, INPUT_PULLUP);
  pinMode(BUTTON_4_PIN, INPUT_PULLUP);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


  if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
      Serial.println("LittleFS Mount Failed");
      return;
  }

  //createDir(LittleFS, "/www"); // Create a mydir folder
  listDir(LittleFS, "/", 1);
  Serial.println( "Test complete" ); 

  /*
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/index.html");
  });

  // curl -v http://192.168.4.1/index.html
  server.serveStatic("/index.html", LittleFS, "/index.html");
  server.serveStatic("/style.css", LittleFS, "/style.css");
  server.serveStatic("/BMDevice.js", LittleFS, "/BMDevice.js");
  server.serveStatic("/web-ui.js", LittleFS, "/web-ui.js");
  server.begin();
*/
  
  setupMotors();
  setupServer();
  //////////////////////////
  //display.begin();
  //display.setFixedFont(ssd1306xled_font6x8);
  //display.fill( 0x00 );
  //display.clear();
  //////////////////////////

  // Привязываем все сервы
  /*
  for (int i = 0; i < 9; i++) {
    servos[i].attach(servoPins[i]);
  }
  */

  //Serial.print("Выбран сервопривод №");
  //Serial.println(currentServo + 1);

  // Дисплей
  //display.begin();
  //display.setFixedFont(ssd1306xled_font6x8);
  //display.clear();
}


void loop() {
  /*
  // lcd_delay(100);
 String Col1;
 String Col2;
 String Col3;
 String Col4;
 String Col5;


  static unsigned long lastDebounce = 0;
  if (millis() - lastDebounce < 200) return;

  if (digitalRead(BUTTON_1_PIN) == LOW) {
   currentServo = (currentServo + 1) % 9;
   Serial.print("Выбран сервопривод №");
   Serial.println(currentServo + 1);
   Col1 = "Servo=" + String(currentServo + 1);
   display.printFixed(0,  8, Col1.c_str(), STYLE_NORMAL);
   lastDebounce = millis();
 }
 if (digitalRead(BUTTON_2_PIN) == LOW) {
   currentServo = (currentServo + 8) % 9;  // эквивалент (currentServo - 1 + 9) % 9
   Serial.print("Выбран сервопривод №");
   Serial.println(currentServo + 1);
   Col1 = "Servo=" + String(currentServo + 1);
   display.printFixed(0,  8, Col1.c_str(), STYLE_NORMAL);
   lastDebounce = millis();
 }

 if (digitalRead(BUTTON_3_PIN) == LOW) {
   servos[currentServo].write(50);
   Serial.print("Сервопривод №");
   Serial.print(currentServo + 1);
   Serial.println(" → 0°");
   Col2 = "Servo = " + String(currentServo + 1) + "   Pos → 0°";
   lastDebounce = millis();
 }

 if (digitalRead(BUTTON_4_PIN) == LOW) {
   servos[currentServo].write(100);
   Serial.print("Сервопривод №");
   Serial.print(currentServo + 1);
   Serial.println(" → 180°");
   Col2 = "Servo = " + String(currentServo + 1) + "  Pos → 180°";
   lastDebounce = millis();
 }
  display.printFixed(0,  16, Col2.c_str(), STYLE_NORMAL);
  */
}







