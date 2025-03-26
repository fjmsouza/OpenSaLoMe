#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include "Arduino.h"
#include "Common.h"

#define PWDN_GPIO_NUM     -1 
#define RESET_GPIO_NUM    -1 
#define XCLK_GPIO_NUM     10 
#define SIOD_GPIO_NUM     40 
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48 
#define Y8_GPIO_NUM       11 
#define Y7_GPIO_NUM       12 
#define Y6_GPIO_NUM       14 
#define Y5_GPIO_NUM       16 
#define Y4_GPIO_NUM       18 
#define Y3_GPIO_NUM       17 
#define Y2_GPIO_NUM       15 
#define VSYNC_GPIO_NUM    38 
#define HREF_GPIO_NUM     47 
#define PCLK_GPIO_NUM     13
#define FLASH 43

// 0-63 lower number means higher quality....I disagree!!!
// =0 ou 1 fica resetando!2 envia imagem sem a manipulação
// se não declarar fica resetando tb
#define JPG_QUALITY 63 // 4, the minimum without resets, while higher better images

camera_fb_t * fb = NULL; 

void cameraSetup(){

  pinMode(FLASH, OUTPUT);

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  //Configuração dos pinos da câmera
  static camera_config_t config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,

    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 10000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
//    FRAMESIZE_QVGA, FRAMESIZE_HVGA, FRAMESIZE_VGA, FRAMESIZE_SVGA worked fine
    .frame_size = FRAMESIZE_SXGA,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 20, //0-63 lower number means higher quality
    .fb_count = 1,       //if more than one, i2s runs in continuous mode. Use only with JPEG
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_LATEST,
  };
   
  esp_err_t err = esp_camera_init(&config); //Inicialização da câmera
   
  if (err != ESP_OK) {
    Serial.printf("Camera initialization error 0x%x", err);//Informa erro se a câmera não for iniciada corretamente
    // delay(1000);
    ESP.restart();//Reinicia o ESP
  }
  sensor_t *s = esp_camera_sensor_get();
  s->set_special_effect(s, 0);
  s->set_aec2(s, 1);
  s->set_ae_level(s, 0);
  s->set_agc_gain(s, 15);
  s->set_lenc(s, 0);
  s->set_raw_gma(s, 0);
  s->set_reg(s, 0x3A, 0xFF, 0x04);
  // s->set_hmirror(s, 1);
  // sensor_t * s = esp_camera_sensor_get();
  // s->set_brightness(s, 0);     // -2 to 2
  // s->set_contrast(s, 0);       // -2 to 2
  // s->set_saturation(s, 0);     // -2 to 2
  // s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  // s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
  // s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
  // s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  // s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
  // s->set_aec2(s, 0);           // 0 = disable , 1 = enable
  // s->set_ae_level(s, 0);       // -2 to 2
  // s->set_aec_value(s, 0);    // 0 to 1200
  // s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
  // s->set_agc_gain(s, 0);       // 0 to 30
  // s->set_gainceiling(s, (gainceiling_t)2);  // 0 to 6
  // s->set_bpc(s, 1);            // 0 = disable , 1 = enable
  // s->set_wpc(s, 1);            // 0 = disable , 1 = enable
  // s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
  // s->set_lenc(s, 0);           // 0 = disable , 1 = enable
  // s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
  // s->set_vflip(s, 0);          // 0 = disable , 1 = enable
  // s->set_dcw(s, 1);            // 0 = disable , 1 = enable
  // s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
  return;
}

void resetCamera(){

  // workaround to free the image buffer, bad images no more!
  fb = NULL;
  fb = esp_camera_fb_get();
  esp_camera_fb_return(fb);
  fb = NULL;
  return;
}


camera_fb_t* takePicture(){
  
  resetCamera();
  // capturing the image 
  fb = esp_camera_fb_get();

  if(!fb) { 
      Serial.println("Capture error!");
      // ESP.restart();
  }

  return fb;
}
