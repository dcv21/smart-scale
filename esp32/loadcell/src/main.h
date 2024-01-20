// Calibrating the load cell
#include <Arduino.h>
#include <ArduinoJson.h>
#include "HX711.h"
#include <TFT_eSPI.h>
#include <WiFi.h>

// Define Thermal Printer paper size
#define WIDTH 384
#define HEIGHT 240

const char* ssid = "ESP32AP";
const char* password = "12345678";
WiFiClient TCPclient;
const char *tcpUrl = "192.168.4.10";
const int tcpPort = 4080;

TFT_eSPI tft = TFT_eSPI();

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 33;
const int LOADCELL_SCK_PIN = 32;

// Define HX711
HX711 scale;

// Define variables
bool tareFlag = false;
bool captureFlag = false;
bool modeFlag = false;
unsigned int mode = 0;
float weight = 0.0;
unsigned long lastMillis = 0;

const int lastValuesSize = 3;
float lastValues[lastValuesSize] = {};
bool is_predicted = false;

// Prototypes
void IRAM_ATTR tare();
void IRAM_ATTR capture();
void IRAM_ATTR changeMode();
void printScreen(String, uint8_t = 64, boolean = true);
void wifiSetup();
void connectAndSend();
String sendData(String&);
void appendData(String&);
void popData(String&);
void printReceipt(String&);