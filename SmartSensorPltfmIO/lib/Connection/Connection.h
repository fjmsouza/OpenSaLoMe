#pragma once

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "esp_camera.h"
#include "Arduino.h"

extern RTC_DATA_ATTR int connection_fail_counter;

class ConnectionHandler
{
public:
    
    // Dados para conexão com o Google Drive
    const char *host = "script.google.com";
    const int httpsPort = 443;

    // Dados para o arquivo de imagem
    String file_name = "filename=ESP32-CAM.jpg"; //"filename=ESP32-CAM.jpg";
    String mime_type = "&mimetype=image/jpeg";   //"&mimetype=image/jpeg";
    String image2string = "&data=";
    int begin;
    const int timeout = 10000; // 10"
    bool connection_status = false;
    // camera_fb_t *fb = NULL;
    // camera_fb_t *image = NULL;
    bool setup();
    void sendData(float moisture1, bool valve_state, float moisture2);
    String urlencode(String str);
    void sendImage(float moisture, camera_fb_t *fb);
    String receiveData();
    void close();
};
extern ConnectionHandler Connection;