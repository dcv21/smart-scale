#include "main.h"

void setup()
{
  Serial.begin(115200);

  wifiSetup();

  if (TCPclient.connect(tcpUrl, tcpPort))
  {
    Serial.println("Connected to TCP server");
  }
  else
  {
    Serial.println("Failed to connect to TCP server");
  }

  tft.begin();

  tft.fillScreen(TFT_BLACK);

  pinMode(25, INPUT_PULLUP);
  pinMode(26, INPUT_PULLUP);
  pinMode(27, INPUT_PULLUP);

  attachInterrupt(25, tare, FALLING);
  attachInterrupt(26, capture, FALLING);
  attachInterrupt(27, changeMode, FALLING);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(-478.507);

  memset(lastValues, 0, sizeof lastValues);
}

void loop() 
{
  const unsigned long currentMillis = millis();

  if (tareFlag)
  {
    printScreen("Tare...");
    scale.tare();
    tareFlag = false;
  }

  if (modeFlag)
  {
    mode == 3 ? mode = 0 : mode++;
    printScreen(String(weight) + " Kg");
    modeFlag = false;
  }

  if (currentMillis - lastMillis >= 200)
  {
    if (scale.is_ready())
    {
      lastMillis = currentMillis;
      weight = scale.get_units(10);
      weight = weight / 1000.0;
      printScreen(String(weight) + " Kg");
      if (mode == 1)
      {
        for (int i = 0; i < lastValuesSize - 1; i++)
        {
          lastValues[i] = lastValues[i+1];
        }
        lastValues[lastValuesSize - 1] = weight;
      }
    }
  }

  if (mode == 1)
  {
    float avg = 0.0;
    for (int i = 0; i < lastValuesSize; i++)
    {
      avg += lastValues[i];
    }
    avg = avg / lastValuesSize;

    if (!is_predicted && avg - lastValues[0] < 0.1 && avg > 0.1)
    {
      connectAndSend();

      is_predicted = true;
    }
    else
    {
      if (avg <= 0.1)
      {
        is_predicted = false;
      }
    }
  }

  if (captureFlag)
  {
    if (mode == 1)
    {
      captureFlag = false;
      return;
    }

    connectAndSend();

    captureFlag = false;
  }
}

void IRAM_ATTR tare()
{
  tareFlag = true;
}

void IRAM_ATTR capture()
{
  captureFlag = true;
}

void IRAM_ATTR changeMode()
{
  modeFlag = true;
}

void printScreen(String str, uint8_t y_value, boolean clear_screen)
{
  if (clear_screen)
    tft.fillScreen(TFT_BLACK);

  tft.setTextFont(4);

  if (tft.textWidth(str) > 128)
    tft.setTextFont(2);

  tft.setCursor(ceil((128 - tft.textWidth(str)) / 2), ceil(y_value - ((5 * tft.textfont + 6) / 2)));
  tft.println(str);

  if (mode > 0)
  {
    tft.setTextFont(4);
    tft.setCursor(128 - tft.textWidth("$"), tft.textsize);
    switch (mode)
    {
      case 1:
        tft.print("+");
        break;
      case 2:
        tft.print("-");
        break;
      case 3:
        tft.print("$");
        break;
    }
  }
}

void wifiSetup()
{
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

void connectAndSend()
{
  if (!TCPclient.connected())
  {
    Serial.println("Connection is disconnected");
    printScreen("Connecting...");
    TCPclient.stop();
    if (!TCPclient.connect(tcpUrl, tcpPort))
    {
      printScreen("Failed (1)");
      return;
    }
  }

  printScreen("Processing...");

  String json = "{\"id\":1,\"mode\":" + String(mode);

  switch (mode)
  {
    case 0:
    case 1:
      appendData(json);
      break;
    case 2:
      popData(json);
      break;
    case 3:
      printReceipt(json);
      break;
  }
}

String sendData(String &json)
{
  TCPclient.print(json);
  TCPclient.flush();

  String rcv = "";

  unsigned long currentMillis = millis();

  while (millis() - currentMillis <= 10000)
  {
    if (TCPclient.available() > 0)
    {
      rcv = TCPclient.readString();
      break;
    }
  }

  return rcv;
}

void appendData(String &json)
{
  if (weight != 0.0)
  {
    json += ",\"weight\":" + String(weight) + '}';

    String rcv = sendData(json);

    DynamicJsonDocument doc(512);

    deserializeJson(doc, rcv);

    if (doc.isNull())
    {
      printScreen("Failed (2)");
      delay(1000);
      return;
    }

    const String type = doc["type"].as<String>();

    printScreen(type, 32, true);

    if (doc.containsKey("weight"))
    {
      printScreen(String(weight) + " KG", 64, false);
    }

    if (doc.containsKey("price"))
    {
      const String price = doc["price"].as<String>();
      printScreen(price + " VND", 96, false);
    }

    delay(2000);
  }
  else
  {
    printScreen("Failed (3)");
    delay(1000);
  }
}

void popData(String &json)
{
  json += '}';
  
  String rcv = sendData(json);

  if (rcv.indexOf("success") != -1)
  {
    printScreen("Removed");
  }
  else
  {
    printScreen("Failed");
  }
}

void printReceipt(String &json)
{
  json += '}';
  
  String rcv = sendData(json);

  if (rcv)
    printScreen("Printing...");
  else
    printScreen("Failed");

  delay(1000);
}