#include "system.h"
#include "Connection.h"
#include "Camera.h"

// Definições (com inicialização)
String command = "";
enum State state = moisture_read;
int moisture = 0;
bool turn_on = false;
unsigned long sleep_period = 15;
unsigned long sleep_period_aux1 = sleep_period * 60;
unsigned long SLEEP_PERIOD = sleep_period_aux1 * uS_TO_S_FACTOR;

RTC_DATA_ATTR int fail_counter = 0;

void setupPinout(){
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PWDN_GPIO_NUM, OUTPUT);
    pinMode(PUMP, OUTPUT); // GPIO for LED flash
    digitalWrite(PUMP, LOW);
    rtc_gpio_hold_dis(GPIO_NUM_4); // disable pin hold if it was enabled before sleeping
}

// Função que controla a bomba
void pumpControl(bool flag)
{

    if (flag)
    {
        delay(1000);
        digitalWrite(PUMP, HIGH);
        Serial.printf("Pump turned on! Wait %d secs", PUMP_ON_PERIOD / 1000);
        delay(PUMP_ON_PERIOD);
        digitalWrite(PUMP, LOW);
        Serial.println("Pump turned off!");
    }
    else
    {
        digitalWrite(PUMP, LOW);
        Serial.println("Pump turned off!");
    }
}

// Função que lê o sensor de umidade e processa os valores
int moistureRead(int moisture_sensor)
{
    int moisture = 0;
    int sum = 0;
    int current = 0;
    int array_samples[SAMPLES_TOTAL_NUMBER];

    for (int m = 0; m < SAMPLES_TOTAL_NUMBER; m++)
    {
        current = analogRead(moisture_sensor);
        array_samples[m] = current;
        sum += current;
    }

    int minValue = min(array_samples[0], array_samples[1]);
    int maxValue = max(array_samples[0], array_samples[1]);

    for (int i = 2; i < SAMPLES_TOTAL_NUMBER; i++)
    {
        minValue = min(minValue, array_samples[i]);
        maxValue = max(maxValue, array_samples[i]);
    }

    moisture = (sum - minValue - maxValue) / SAMPLES_EFFECTIVE_NUMBER;

    if (moisture <= SOAKED)
    {
        moisture = SOAKED;
    }
    if (moisture >= DRY)
    {
        moisture = DRY;
    }

    // Mapeia o valor de umidade para uma escala de 0 a 100%
    moisture = map(moisture, DRY, SOAKED, 0, 100);
    return moisture;
}

// Função para capturar comando de rega
void updateCommand()
{

    if (Connection.connection_status)
    {

        command = Connection.receiveData();
        Serial.println(command);

        if (command != "water")
        {
            command = "no water";
        }
    }
    else
    {
        command = "no water";
    }
}

void systemPowerOff()
{
    Camera.powerOff();
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(PUMP, LOW);
    rtc_gpio_hold_en(GPIO_NUM_7); // make sure pump is held LOW in sleep
    Serial.println("Going to sleep now");
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
}


// Função que gerencia os estados do sistema
void handleStates()
{
    digitalWrite(LED_BUILTIN, LOW); // Garante que o LED esteja apagado

    switch (state)
    {
    case moisture_read:
        Serial.println("\nmoisture_read");
        moisture = moistureRead(MOISTURE_SENSOR);
        Serial.printf("\nmoisture = %d \n", moisture);
        if (Connection.connection_status)
        {
            state = send_image;
        }
        else
        {
            state = pump_control;
        }
        break;

    case send_image:
        Serial.println("send_image");

        // Captura e envia imagens
        image = Camera.takeDayPicture();
        if (image)
        {
            Serial.printf("JPEG size: %d bytes\n", image->len);
            Connection.sendImage(moisture, image);
        }

        state = pump_control;
        break;

    case pump_control:
        Serial.println("pump_control");

        updateCommand();

        if (command == "water")
        {
            turn_on = true;
            pumpControl(turn_on);
        }
        else
        {
            turn_on = false;
            pumpControl(turn_on);
        }

        if (Connection.connection_status)
        {
            state = send_data;
        }
        else
        {
            state = deep_sleep;
        }
        break;
    case send_data:
        Serial.println("send_data");
        Connection.sendData(moisture, turn_on);
        state = deep_sleep;
        break;
    case deep_sleep:
        Serial.println("deep_sleep");
        Serial.printf("\nnumero de falhas: %d", fail_counter);
        if (Connection.connection_status)
        {
            Connection.close();
        }
        if (fail_counter >= 1)
        {
            fail_counter = 0;
            ESP.restart();
        }

        systemPowerOff();

        break;

    default:
        break;
    }
}
