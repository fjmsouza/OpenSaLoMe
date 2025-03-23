#pragma once

#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include "Arduino.h"

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

// 0-63 lower number means higher quality....I disagree!!!
// =0 ou 1 fica resetando!2 envia imagem sem a manipulação
// se não declarar fica resetando tb
#define JPG_QUALITY 63 // 4, the minimum without resets, while higher better images
class CameraHandler
{
public:
    camera_fb_t *fb = NULL;
    camera_fb_t *image = NULL;

    void setup(bool high_resolution);
    void reset();
    camera_fb_t *takeDayPicture();
    void powerOff();
};

extern CameraHandler Camera;