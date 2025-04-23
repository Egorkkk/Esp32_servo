// webserver.cpp — логика веб-интерфейса и API

#include "webserver.h"
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "motors.h"

AsyncWebServer server(80);

void setupServer() {
  // Главная страница и вспомогательные файлы
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/style.css", LittleFS, "/style.css");
  server.serveStatic("/wheel-control.css", LittleFS, "/wheel-control.css");
  server.serveStatic("/BMDevice.js", LittleFS, "/BMDevice.js");
  server.serveStatic("/web-ui.js", LittleFS, "/web-ui.js");
  server.serveStatic("/load-motor-state.js", LittleFS, "/load-motor-state.js");
  server.serveStatic("/web-position.js", LittleFS, "/web-position.js");
  server.serveStatic("/motor-control-sync.js", LittleFS, "/motor-control-sync.js");
  server.serveStatic("/debounced-servo-control.js", LittleFS, "/debounced-servo-control.js");
  server.serveStatic("/wheel-control.js", LittleFS, "/wheel-control.js");

  // Получить параметры конкретного мотора
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

  // Установить параметры мотора
  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("id")) {
      request->send(400, "text/plain", "Missing motor ID");
      return;
    }
    String id = request->getParam("id")->value();
    bool save = request->hasParam("save") && request->getParam("save")->value() == "true";

    for (auto &m : motors) {
      if (m.id == id) {
        if (request->hasParam("angle"))  m.angle     = request->getParam("angle")->value().toInt();
        if (request->hasParam("offset")) m.offset    = request->getParam("offset")->value().toInt();
        if (request->hasParam("min"))    m.minAngle  = request->getParam("min")->value().toInt();
        if (request->hasParam("max"))    m.maxAngle  = request->getParam("max")->value().toInt();

        applyServo(m);

        if (save) {
          Preferences prefs;
          prefs.begin("motors", false);
          prefs.putInt((m.id + "_min").c_str(), m.minAngle);
          prefs.putInt((m.id + "_max").c_str(), m.maxAngle);
          prefs.putInt((m.id + "_offset").c_str(), m.offset);
          prefs.end();
          Serial.printf("[%s] Настройки сохранены в память\n", m.id.c_str());
        }

        request->send(200, "text/plain", "OK");
        return;
      }
    }

    request->send(404, "text/plain", "Unknown motor id");
  });

  // Активация моторов
  server.on("/enable", HTTP_GET, [](AsyncWebServerRequest *request) {
    enableMotors();
    request->send(200, "text/plain", "Servos enabled");
  });
  
  server.on("/disable", HTTP_GET, [](AsyncWebServerRequest *request){
    motorsEnabled = false;
    Serial.println("Моторы отключены вручную.");
    request->send(200, "text/plain", "Motors disabled");
  });
  // Отладочная информация
  server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
    String out;
    for (auto &m : motors) {
      out += m.id + ": angle=" + String(m.angle)
           + ", min=" + String(m.minAngle)
           + ", max=" + String(m.maxAngle)
           + ", offset=" + String(m.offset)
           + ", mirrored=" + (m.isMirrored ? "true" : "false") + "\n";
    }
    request->send(200, "text/plain", out);
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", motorsEnabled ? "true" : "false");
  });

  server.begin();
}