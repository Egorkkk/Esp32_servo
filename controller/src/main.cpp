#include <Arduino.h>
#include "lcdgfx.h"
#include "lcdgfx_gui.h"
#undef min
#undef max
#include <ArduinoJson.h>
#include "FS.h"
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "motors.h"
#include <Preferences.h>
#include "webserver.h"

#define BUTTON_1_PIN  25
#define BUTTON_2_PIN  26
#define BUTTON_3_PIN  32
#define BUTTON_4_PIN  33

bool lastState1 = HIGH;
bool lastState2 = HIGH;
bool lastState3 = HIGH;
bool lastState4 = HIGH;

//Preferences prefs;

DisplaySH1106_128x64_SPI display(22,{-1, 5, 21, 0,23,19});

static AsyncWebServer server(80);

#define FORMAT_LITTLEFS_IF_FAILED true

const char* ssid = "Motobacks";
const char* password = "1008121982";
IPAddress local_IP(192, 168, 0, 200);
IPAddress gateway(192, 168, 0, 3);
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

  listDir(LittleFS, "/", 1);
  Serial.println( "Test complete" ); 
  
  setupMotors();
  setupServer();
  //////////////////////////
  //display.begin();
  //display.setFixedFont(ssd1306xled_font6x8);
  //display.fill( 0x00 );
  //display.clear();
  //////////////////////////

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







