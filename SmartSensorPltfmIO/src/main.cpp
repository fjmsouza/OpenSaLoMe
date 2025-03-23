#include "system.h"

void setup()
{
    // Desativa a detecção de brown-out
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    Serial.begin(115200);

    // Configura resolução do ADC e pinos
    analogReadResolution(9); // 2^9 = 512 bytes
    pinMode(PUMP, OUTPUT);
    digitalWrite(PUMP, LOW);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PWDN_GPIO_NUM, OUTPUT);

    // Configura deep sleep para despertar a cada sleep_period minutos
    esp_sleep_enable_timer_wakeup(SLEEP_PERIOD);
    delay(5000);
    Serial.println("\nSetup ESP32 to sleep for every " + String(sleep_period) + " minutes");
    Serial.printf("PSRAM: %dMB\n", esp_spiram_get_size() / 1048576);

    // Inicializa storage, conexão e câmera
    Storage.setup();

    if (Connection.setup()){
        Camera.setup(false);
    }
}

void loop()
{
    // Gerencia a máquina de estados
    handleStates();
}
