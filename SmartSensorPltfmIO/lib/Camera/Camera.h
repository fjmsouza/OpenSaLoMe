#pragma once

#include "esp_camera.h"
#include "Arduino.h"
#include "system.h"

#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 10
#define SIOD_GPIO_NUM 40
#define SIOC_GPIO_NUM 39
#define Y9_GPIO_NUM 48
#define Y8_GPIO_NUM 11
#define Y7_GPIO_NUM 12
#define Y6_GPIO_NUM 14
#define Y5_GPIO_NUM 16
#define Y4_GPIO_NUM 18
#define Y3_GPIO_NUM 17
#define Y2_GPIO_NUM 15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM 47
#define PCLK_GPIO_NUM 13
#define JPG_QUALITY 63 // 4, the minimum without resets, while higher better images

extern camera_fb_t *image;
class CameraHandler
{
public:
    camera_fb_t *fb = NULL;
    camera_fb_t *image = NULL;
    // Defina em um arquivo de configuração (ex: config.h)
    const bool HIGH_RES = true;
    const bool LOW_RES = false;

    void setup(bool resolution);
    void reset();
    camera_fb_t *takeDayPicture();
    void powerOff();
};

extern CameraHandler Camera;