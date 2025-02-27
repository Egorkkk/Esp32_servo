#include <Arduino.h>
#include "FS.h"
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

static AsyncWebServer server(80);

#define FORMAT_LITTLEFS_IF_FAILED true



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

void createDir(fs::FS &fs, const char * path){
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path)){
      Serial.println("Dir created");
  } else {
      Serial.println("mkdir failed");
  }
}


void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");

  if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
      Serial.println("LittleFS Mount Failed");
      return;
  }

  //createDir(LittleFS, "/www"); // Create a mydir folder
  listDir(LittleFS, "/", 1);
  Serial.println( "Test complete" ); 

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/index.html");
  });

  // curl -v http://192.168.4.1/index.html
  server.serveStatic("/index.html", LittleFS, "/index.html");
  server.serveStatic("/style.css", LittleFS, "/style.css");
  server.serveStatic("/BMDevice.js", LittleFS, "/BMDevice.js");
  server.serveStatic("/web-ui.js", LittleFS, "/web-ui.js");
  server.begin();

}

void loop() {

  // put your main code here, to run repeatedly:
}

