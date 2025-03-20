#include <Arduino.h>
#include "FS.h"
#include <LittleFS.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "lcdgfx.h"
#include "lcdgfx_gui.h"
#include "owl.h"

Adafruit_MPU6050 mpu;

// Адреса I2C устройств
#define MPU6050_ADDR 0x68  // Адрес MPU-6050 по умолчанию
#define QMC5883L_ADDR 0x0D // Адрес QMC5883L по умолчанию


// Регистры MPU-6050
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_GYRO_XOUT_H 0x43

// Регистры QMC5883L
#define QMC5883L_CONFIG_REG_A 0x09
#define QMC5883L_DATA_XOUT_LSB 0x00

DisplaySH1106_128x64_SPI display(22,{-1, 5, 21, 0,23,19});

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

////////////////////
const PROGMEM uint8_t heartImage[8] =
{
    0B00001110,
    0B00011111,
    0B00111111,
    0B01111110,
    0B01111110,
    0B00111101,
    0B00011001,
    0B00001110
};

/*
 * Define sprite width. The width can be of any size.
 * But sprite height is always assumed to be 8 pixels
 * (number of bits in single byte).
 */
const int spriteWidth = sizeof(heartImage);

const char *menuItems[] =
{
    "draw bitmap",
    "sprites",
    "fonts",
    "canvas gfx",
    "draw lines",
};

LcdGfxMenu menu( menuItems, sizeof(menuItems) / sizeof(char *) );

static void bitmapDemo()
{
    display.drawBitmap1(0, 0, 128, 64, Owl);
    lcd_delay(1000);
    display.getInterface().invertMode();
    lcd_delay(2000);
    display.getInterface().normalMode();
}

static void spriteDemo()
{
    display.clear();
    /* Declare variable that represents our sprite */
    NanoPoint sprite = {0, 0};
    for (int i=0; i<250; i++)
    {
        lcd_delay(15);
        /* Erase sprite on old place. The library knows old position of the sprite. */
        display.setColor( 0 );
        display.drawBitmap1( sprite.x, sprite.y, spriteWidth, 8, heartImage );
        sprite.x++;
        if (sprite.x >= display.width())
        {
            sprite.x = 0;
        }
        sprite.y++;
        if (sprite.y >= display.height())
        {
            sprite.y = 0;
        }
        /* Draw sprite on new place */
        display.setColor( 0xFFFF );
        display.drawBitmap1( sprite.x, sprite.y, spriteWidth, 8, heartImage );
    }
}


static void textDemo()
{
    display.setFixedFont(ssd1306xled_font6x8);
    display.clear();
    display.printFixed(0,  8, "Normal text", STYLE_NORMAL);
    display.printFixed(0, 16, "Bold text", STYLE_BOLD);
    display.printFixed(0, 24, "Italic text", STYLE_ITALIC);
    display.invertColors();
    display.printFixed(0, 32, "Inverted bold", STYLE_BOLD);
    display.invertColors();
    lcd_delay(3000);
    display.clear();
}

static void LCDText()
{
    display.setFixedFont(ssd1306xled_font6x8);
    display.clear();
    display.printFixed(0,  8, "Normal text", STYLE_NORMAL);
    display.printFixed(0, 16, "Bold text", STYLE_BOLD);
    display.printFixed(0, 24, "Italic text", STYLE_ITALIC);
    display.invertColors();
    display.printFixed(0, 32, "Inverted bold", STYLE_BOLD);
    display.invertColors();
    lcd_delay(3000);
    display.clear();
}

static void canvasDemo()
{
    NanoCanvas<64,16,1> canvas;
    display.clear();
    canvas.clear();
    canvas.setColor( 0xFF );
    canvas.fillRect(10, 3, 80, 5);
    display.drawCanvas((display.width()-64)/2, 1, canvas);
    lcd_delay(500);
    canvas.setColor( 0xFF );
    canvas.fillRect(50, 1, 60, 15);
    display.drawCanvas((display.width()-64)/2, 1, canvas);
    lcd_delay(1500);
    canvas.setFixedFont(ssd1306xled_font6x8);
    canvas.printFixed(20, 1, " DEMO ", STYLE_BOLD );
    display.drawCanvas((display.width()-64)/2, 1, canvas);
    lcd_delay(3000);
}

static void drawLinesDemo()
{
    display.clear();
    for (uint8_t y = 0; y < display.height(); y += 8)
    {
        display.drawLine(0,0, display.width() -1, y);
    }
    for (uint8_t x = display.width() - 1; x > 7; x -= 8)
    {
        display.drawLine(0,0, x, display.height() - 1);
    }
    lcd_delay(3000);
}
////////////////////

void setup() {
  Serial.begin(115200);
  Wire.begin(17, 18);


  //WiFi.mode(WIFI_AP);
  //WiFi.softAP("esp-captive");
  //if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
  //  Serial.println("STA Failed to configure");
  //}

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

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/index.html");
  });

  // curl -v http://192.168.4.1/index.html
  server.serveStatic("/index.html", LittleFS, "/index.html");
  server.serveStatic("/style.css", LittleFS, "/style.css");
  server.serveStatic("/BMDevice.js", LittleFS, "/BMDevice.js");
  server.serveStatic("/web-ui.js", LittleFS, "/web-ui.js");
  server.begin();

  //////////////////////////
  display.begin();
  display.setFixedFont(ssd1306xled_font6x8);
  display.fill( 0x00 );
  menu.show( display );
  //////////////////////////

  // Инициализация MPU-6050
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_PWR_MGMT_1); // Регистр управления питанием
  Wire.write(0x00);               // Вывод из спящего режима
  Wire.endTransmission(true);


}



void loop() {
  lcd_delay(100);
  /*
  switch (menu.selection())
  {
      case 0:
          bitmapDemo();
          break;

      case 1:
          spriteDemo();
          break;

      case 2:
          textDemo();
          break;

      case 3:
          canvasDemo();
          break;

      case 4:
          drawLinesDemo();
          break;

      default:
          break;
  }
  display.fill( 0x00 );
  menu.show( display );
  lcd_delay(500);
  menu.down();
  menu.show( display );
 */

  // Считывание данных с MPU-6050
  int16_t accelX, accelY, accelZ, gyroX, gyroY, gyroZ;
  String accelStr;

  // Чтение акселерометра
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_ACCEL_XOUT_H); // Начальный регистр данных акселерометра
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 6, true); // Запрос 6 байт (X, Y, Z)
  accelX = Wire.read() << 8 | Wire.read();
  accelY = Wire.read() << 8 | Wire.read();
  accelZ = Wire.read() << 8 | Wire.read();
  Wire.endTransmission(true);

  // Чтение гироскопа
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_GYRO_XOUT_H); // Начальный регистр данных гироскопа
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 6, true); // Запрос 6 байт (X, Y, Z)
  gyroX = Wire.read() << 8 | Wire.read();
  gyroY = Wire.read() << 8 | Wire.read();
  gyroZ = Wire.read() << 8 | Wire.read();
  Wire.endTransmission(true);

  accelStr = String(accelX) + " " + String(accelY) + " " + String(accelZ);
  display.printFixed(0,  8, accelStr.c_str(), STYLE_NORMAL);
}

