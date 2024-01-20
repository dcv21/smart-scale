#include "main.h"

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    pinMode(FLASH_GPIO_NUM, OUTPUT);
    digitalWrite(FLASH_GPIO_NUM, HIGH);

    Serial.begin(115200);
    wifiSetup();
    TCPserver.begin();
    cameraSetup();
    delay(1000);
}

void loop()
{
    WiFiClient client = TCPserver.available();
    String res = "";
    if (client) {
        // Read the command from the TCP client:
        String rcv = client.readString();
        Serial.print("Received command: ");
        Serial.println(rcv);

        int modeIndex = rcv.indexOf("mode");
        const int mode = rcv[modeIndex + 6] - '0';

        switch (mode)
        {
          case 0:
          case 1:
            res = sendPhoto();
            if (!res) 
            {
                Serial.println("Failed to send image");
                client.stop();
                return;
            }
            Serial.println("Sending data");
            res = sendData(client, rcv, "/predict");
            break;
          case 2:
            sendData(client, rcv, "/remove");
            break;
          case 3:
            res = sendData(client, rcv, "/receipt");
            printReceipt(res);
            break;
        }
        client.stop();
    }

    if (res != "") {
        printReceipt(res);
    }

    // if (Serial.available() > 0) {
    //     String rcv = Serial.readString();
    //     sendPhoto(rcv);
    // }
}

void wifiSetup()
{
    IPAddress local_IP(192, 168, 4, 10);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);

    if (!WiFi.config(local_IP, gateway, subnet)) {
        Serial.println("STA Failed to configure");
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print('.');
        delay(500);
    }
    Serial.println(WiFi.localIP());
}

void cameraSetup()
{
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    // init with high specs to pre-allocate larger buffers
    if (psramFound())
    {
        config.frame_size = FRAMESIZE_HD;
        config.jpeg_quality = 10; // 0-63 lower number means higher quality
        config.fb_count = 2;
        config.grab_mode = CAMERA_GRAB_LATEST;
    }
    else
    {
        config.frame_size = FRAMESIZE_CIF;
        config.jpeg_quality = 12; // 0-63 lower number means higher quality
        config.fb_count = 1;
    }

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        delay(1000);
        ESP.restart();
    }

    Serial.println("CAM_OK");
}

String sendPhoto() {
  WiFiClient client;

  String getAll;
  String getBody;

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }
  
  Serial.println("Connecting to server: " + serverName);

  if (client.connect(serverName.c_str(), serverPort)) {
    Serial.println("Connection successful!");    
    String head = "--ESP32CAM\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"1.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--ESP32CAM--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;
  
    client.println("POST " + serverPath + " HTTP/1.1");
    client.println("Host: " + serverName);
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=ESP32CAM");
    client.println();
    client.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0; n<fbLen; n=n+1024) {
      if (n+1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        client.write(fbBuf, remainder);
      }
    }   
    client.print(tail);
    
    esp_camera_fb_return(fb);
    
    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + timoutTimer) > millis()) {
      Serial.print(".");
      delay(100);      
      while (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (getAll.length()==0) { state=true; }
          getAll = "";
        }
        else if (c != '\r') { getAll += String(c); }
        if (state==true) { getBody += String(c); }
        startTimer = millis();
      }
      if (getBody.length()>0) { break; }
    }
    client.flush();
    Serial.println();
    client.stop();
    Serial.println(getBody);
  }
  else {
    getBody = "Connection to " + serverName +  " failed.";
    Serial.println(getBody);
  }
  return getBody;
}

String sendData(WiFiClient &client, String &rcv, String path)
{
    http.begin(url + path);

    http.addHeader("Content-Type", "application/json");

    String res;

    int httpResponseCode = http.POST(rcv);

    if (httpResponseCode == 200) {
        res = http.getString();
        client.print(res);
        client.flush();
    }

    http.end();

    return res;
}

void printReceipt(String &res)
{
    DynamicJsonDocument doc(1024);

    deserializeJson(doc, res);

    Serial.println(res);

    if (!doc.containsKey("receipt")) return;

    Serial.println("Scanning");

    String data = doc["receipt"].as<String>();

    if (tpScan())
    {
        Serial.println("Found a printer!, connecting...");
        if (tpConnect())
        {
            Serial.println("Connected!, printing...");
            tpSetFont(0, 0, 0, 0, 0);
            tpPrint((char *)data.c_str());
            tpDisconnect();
            delay(500);
            ESP.restart();
        }
    }
    else
    {
        Serial.println("Didn't find a printer");
    }
}