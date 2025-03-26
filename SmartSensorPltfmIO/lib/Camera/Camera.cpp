#include "Camera.h"

CameraHandler Camera;

void CameraHandler::setup(bool high_resolution)
{
  esp_camera_deinit();

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

      .xclk_freq_hz = 10000000,
      .ledc_timer = LEDC_TIMER_0,
      .ledc_channel = LEDC_CHANNEL_0,

      .pixel_format = high_resolution ? PIXFORMAT_JPEG : PIXFORMAT_GRAYSCALE,
      .frame_size = high_resolution ? FRAMESIZE_WQXGA : FRAMESIZE_96X96, // FRAMESIZE_WQXGA para 5MPixels, FRAMESIZE_SXGA para 2MPixels
      .jpeg_quality = 12,
      .fb_count = 1,
      .fb_location = CAMERA_FB_IN_PSRAM,
      .grab_mode = CAMERA_GRAB_LATEST,
  };

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera initialization error 0x%x", err);
    ESP.restart();
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_special_effect(s, 0);
  s->set_aec2(s, 1);
  s->set_ae_level(s, 0);
  s->set_agc_gain(s, 15);
  s->set_lenc(s, 0);
  s->set_raw_gma(s, 0);
  s->set_reg(s, 0x3A, 0xFF, 0x04);

  Serial.printf("Camera initialized successfully in %s resolution!\n", high_resolution ? "HIGH" : "LOW");
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

// Função que captura uma imagem em alta resolução (JPEG) somente se a luminosidade estiver acima do limiar.
// brightnessThreshold é o valor médio mínimo (0-255) necessário para considerar a imagem "clara".
camera_fb_t *CameraHandler::takeDayPicture()
{

  sensor_t *s = esp_camera_sensor_get();
  if (!s)
  {
    Serial.println("Error: sensor not found!");
    return NULL;
  }

  // 1. Configure a câmera para modo grayscale e resolução baixa para análise rápida da luminosidade.
  // s->set_framesize(s, FRAMESIZE_96X96);
  // s->set_pixformat(s, PIXFORMAT_GRAYSCALE);

  reset();
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Error capturing frame for brightness check.");
    return NULL;
  }

  // Calcula a média dos pixels
  uint32_t sum = 0;
  for (size_t i = 0; i < fb->len; i++)
  {
    sum += fb->buf[i];
  }
  float avgBrightness = (float)sum / fb->len;
  int brightnessThreshold = 10;
  Serial.printf("Average brightness: %.2f\n", avgBrightness);

  // Libera o framebuffer usado para análise
  // esp_camera_fb_return(fb_check);

  // Se a média estiver abaixo do limiar, retorna NULL (imagem considerada escura)
  if (avgBrightness < brightnessThreshold)
  {
    Serial.println("Dark image. Discarded.");
    return NULL;
  }

  // 2. Se a luminosidade for suficiente, configure a câmera para alta resolução e JPEG
  setup(true);

  // Captura a imagem final em JPEG
  reset();
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Error capturing JPEG image.");
    return NULL;
  }

  return fb;
}

void CameraHandler::powerOff()
{
  // 1. Desinicializa a câmera (libera recursos)
  esp_camera_deinit();

  // 2. Configura manualmente os pinos críticos para LOW
  pinMode(XCLK_GPIO_NUM, OUTPUT);
  digitalWrite(XCLK_GPIO_NUM, LOW); // Desliga o clock

  // 3. Desativa os pinos de dados (D0-D7)
  const int dataPins[] = {Y2_GPIO_NUM, Y3_GPIO_NUM, Y4_GPIO_NUM, Y5_GPIO_NUM,
                          Y6_GPIO_NUM, Y7_GPIO_NUM, Y8_GPIO_NUM, Y9_GPIO_NUM};
  for (int pin : dataPins)
  {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  Serial.println("Câmera desligada via software!");
}
