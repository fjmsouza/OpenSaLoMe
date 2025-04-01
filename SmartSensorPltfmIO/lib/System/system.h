#pragma once

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"  // Add this include for rtc_gpio functions
#include "Arduino.h"
#include "esp_camera.h"

// Forward declarations
class ConnectionHandler;
class CameraHandler;

extern RTC_DATA_ATTR int fail_counter;

// // Definições e constantes
#define DRY 338
#define SOAKED 128
#define uS_TO_S_FACTOR 1000000ULL
#define PUMP_ON_PERIOD 30000
#define MOISTURE_SENSOR 1
#define PUMP 7

// Constantes
constexpr int SAMPLES_EFFECTIVE_NUMBER = 256;
constexpr int SAMPLES_TOTAL_NUMBER = SAMPLES_EFFECTIVE_NUMBER + 2;

// Declarações "extern" (definições em system.cpp)
extern String command;
extern unsigned long sleep_period;
extern unsigned long sleep_period_aux1;
extern unsigned long SLEEP_PERIOD;
extern bool turn_on;
extern int moisture;

enum State
{
    moisture_read,
    send_image,
    pump_control,
    send_data,
    deep_sleep
};

extern enum State state;

// Protótipos das funções
void setupPinout();
void pumpControl(bool flag);
int moistureRead();
void updateCommand();
void systemPowerOff();
void handleStates();