#pragma once

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Arduino.h"
#include "Storage.h"
#include "Camera.h"
#include "Connection.h"

// Definições e constantes
#define DRY 241                   // valor mínimo para solo seco
#define SOAKED 164                // valor máximo para solo encharcado
#define uS_TO_S_FACTOR 1000000ULL // conversão de microssegundos para segundos
#define PUMP_ON_PERIOD 4000       // tempo de acionamento da bomba (ms)
#define MOISTURE_SENSOR1 1         // exemplo de pino do sensor de umidade
#define MOISTURE_SENSOR2 2         // exemplo de pino do sensor de umidade
#define PUMP 7                    // GPIO7, GPIO8 não pode, afetado pela placa da câmera
// Estrutura para os limiares de histerese
struct Hysteresis
{
    int upper_threshold;
    int lower_threshold;
};

// Declaração das variáveis globais
extern struct Hysteresis thresholds;
extern String thresholds_string;
extern int moisture1;
extern const int SAMPLES_EFFECTIVE_NUMBER;
extern const int SAMPLES_TOTAL_NUMBER;

extern unsigned long sleep_period; // em minutos
extern unsigned long SLEEP_PERIOD; // em microssegundos

extern bool turn_on;
extern bool flash_on;

enum State
{
    moisture_read,
    pump_control,
    publish_data,
    deep_sleep
};
extern State state;

extern int drop_counter;
extern camera_fb_t *image;

// Protótipos das funções de apoio:
void pumpControl(bool flag);
int moistureRead();
void updateHysteresis();
void handleStates();