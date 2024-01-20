#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <base64.h>
#include <Thermal_Printer.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define FLASH_GPIO_NUM    4

const char* ssid = "ESP32AP";
const char* password = "12345678";

// Send data
String url = "http://192.168.4.20:8000";
// String url = "http://192.168.1.6:8000/predict";

// Send pic
String serverName = "192.168.4.20";    
String serverPath = "/image";  // Flask upload route
const int serverPort = 8000;

HTTPClient http;
WiFiServer TCPserver(4080);

void wifiSetup();
void cameraSetup();
String sendPhoto();
String sendData(WiFiClient&, String&, String);
void printReceipt(String&);