#include "Camera.h"

CameraHandler Camera;

void CameraHandler::setup()
{
  // Configuração dos pinos da câmera
  camera_config_t config = {
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

      // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
      .xclk_freq_hz = 20000000,
      .ledc_timer = LEDC_TIMER_0,
      .ledc_channel = LEDC_CHANNEL_0,

      .pixel_format = PIXFORMAT_JPEG, // YUV422,GRAYSCALE,RGB565,JPEG
      // FRAMESIZE_QVGA, FRAMESIZE_HVGA, FRAMESIZE_VGA, FRAMESIZE_SVGA worked fine
      // .frame_size = FRAMESIZE_SXGA, // QQVGA-UXGA Do not use sizes above QVGA when not JPEG
      .frame_size = FRAMESIZE_QSXGA,
      .jpeg_quality = 12, // 0-63  (próximos de 0): Menor compressão, ou seja, a imagem tem maior qualidade e, consequentemente, maior tamanho de arquivo.
                          // (próximos de 63): Maior compressão, resultando em imagem com menor qualidade e arquivo menor.
      .fb_count = 1, // if more than one, i2s runs in continuous mode. Use only with JPEG
      .fb_location = CAMERA_FB_IN_PSRAM,
      .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
  };
  esp_err_t err = esp_camera_init(&config); // Inicialização da câmera

  if (err != ESP_OK)
  {
    Serial.printf("Camera initialization error 0x%x", err); // Informa erro se a câmera não for iniciada corretamente
    ESP.restart();                                          // Reinicia o ESP
  }
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
  Serial.println("Câmera initializada com sucesso!");
  return;
}

void CameraHandler::reset()
{

  // workaround to free the image buffer, bad images no more!
  fb = NULL;
  fb = esp_camera_fb_get();
  esp_camera_fb_return(fb);
  fb = NULL;
  return;
}

camera_fb_t *CameraHandler::takePicture()
{

  reset();
  // capturing the image
  fb = esp_camera_fb_get();

  if (!fb)
  {
    Serial.println("Capture error!");
    ESP.restart();
  }
  return fb;
}
