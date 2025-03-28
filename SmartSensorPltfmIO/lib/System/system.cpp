#include "system.h"

// Variáveis globais
float moisture = 0;

struct Hysteresis thresholds;
String thresholds_string = "";

const int SAMPLES_EFFECTIVE_NUMBER = 256;
const int SAMPLES_TOTAL_NUMBER = SAMPLES_EFFECTIVE_NUMBER + 2;

unsigned long sleep_period = 15; // em minutos
unsigned long sleep_period_aux1 = sleep_period * 60;
unsigned long SLEEP_PERIOD = sleep_period_aux1 * uS_TO_S_FACTOR;

bool turn_on = false;
bool flash_on = false;

State state = moisture_read;
int drop_counter = 0;
camera_fb_t *image = NULL;

// Função que controla a bomba
void pumpControl(bool flag)
{
    if (flag)
    {
        digitalWrite(PUMP, HIGH);
        Serial.printf("Pump ligou! aguarda %s seg", PUMP_ON_PERIOD/1000);
        delay(PUMP_ON_PERIOD);
        digitalWrite(PUMP, LOW);
        Serial.println("Pump desligou!");
    }
    else
    {
        digitalWrite(PUMP, LOW);
        Serial.println("Pump desligou!");
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

// Função para atualizar os limiares de histerese usando a Storage e dados recebidos
void updateHysteresis()
{
    if (Connection.connection_status)
    {
        if ((Storage.fileExists(Storage.UPPER_THRESHOLD_PATH)) && (Storage.fileExists(Storage.LOWER_THRESHOLD_PATH)))
        {
            // Os arquivos já existem
        }
        else
        {
            Storage.createFile(Storage.UPPER_THRESHOLD_PATH);
            Storage.createFile(Storage.LOWER_THRESHOLD_PATH);
            Storage.writeString(Storage.UPPER_THRESHOLD_PATH, "41");
            Storage.writeString(Storage.LOWER_THRESHOLD_PATH, "40");
        }
        thresholds_string = Connection.receiveData();
        String upper_threshold_received = thresholds_string.substring(0, 2);
        String lower_threshold_received = thresholds_string.substring(3, 5);
        String upper_threshold_recorded = Storage.readString(Storage.UPPER_THRESHOLD_PATH);
        String lower_threshold_recorded = Storage.readString(Storage.LOWER_THRESHOLD_PATH);

        if ((upper_threshold_received != upper_threshold_recorded) || (lower_threshold_received != lower_threshold_recorded))
        {
            if ((upper_threshold_received.toInt() >= 0) && (lower_threshold_received.toInt() >= 0) && (upper_threshold_received.toInt() >= lower_threshold_received.toInt()))
            {
                Storage.writeString(Storage.UPPER_THRESHOLD_PATH, upper_threshold_received);
                Storage.writeString(Storage.LOWER_THRESHOLD_PATH, lower_threshold_received);
            }
        }
    }
    else
    {
        if ((Storage.fileExists(Storage.UPPER_THRESHOLD_PATH)) && (Storage.fileExists(Storage.LOWER_THRESHOLD_PATH)))
        {
            // Já existem
        }
        else
        {
            Storage.createFile(Storage.UPPER_THRESHOLD_PATH);
            Storage.createFile(Storage.LOWER_THRESHOLD_PATH);
            Storage.writeString(Storage.UPPER_THRESHOLD_PATH, "41");
            Storage.writeString(Storage.LOWER_THRESHOLD_PATH, "40");
        }
    }
    thresholds.upper_threshold = Storage.readString(Storage.UPPER_THRESHOLD_PATH).toInt();
    thresholds.lower_threshold = Storage.readString(Storage.LOWER_THRESHOLD_PATH).toInt();

    Serial.println("valores dos limiares:");
    Serial.println(thresholds.upper_threshold);
    Serial.println(thresholds.lower_threshold);
}

// Função que gerencia os estados do sistema
void handleStates()
{
    digitalWrite(LED_BUILTIN, LOW); // Garante que o LED esteja apagado

    switch (state)
    {
    case moisture_read:
        Serial.printf("\nnumero de falhas: %d", fail_counter);
        Serial.println("\nmoisture_read");
        moisture = moistureRead(MOISTURE_SENSOR);
        Serial.printf("\nmoisture = %d \n", moisture);
        if (Connection.connection_status)
        {
            state = publish_data;
        }
        else
        {
            state = pump_control;
        }
        break;

    case publish_data:
        Serial.println("publish_data");
        Serial.println("\n=====INÍCIO DE ENVIO DE DADOS=====");
        Connection.sendData(moisture, turn_on);

        // Captura e envia imagens
        image = Camera.takeDayPicture();
        if (image)
        {
            Serial.printf("Tamanho do JPEG: %d bytes\n", image->len);
            Connection.sendImage(moisture, image);
        }
        else
        {
            fail_counter++;
        }

        Serial.println("\n=====FIM DE ENVIO DE DADOS=====");
        state = pump_control;
        break;

    case pump_control:
        Serial.println("pump_control");
        updateHysteresis();
        if (moisture < thresholds.lower_threshold)
        {
            turn_on = true;
            pumpControl(turn_on);
        }
        else if (moisture >= thresholds.upper_threshold)
        {
            turn_on = false;
            pumpControl(turn_on);
        }
        else
        {
            pumpControl(turn_on);
        }
        state = deep_sleep;
        break;

    case deep_sleep:
        if (Connection.connection_status)
        {
            Connection.close();
        }
        if (fail_counter >= 1)
        {
            fail_counter = 0;
            ESP.restart();
        }
        Camera.powerOff();
        Serial.println("Going to sleep now");
        digitalWrite(LED_BUILTIN, HIGH);
        digitalWrite(PUMP, LOW);
        esp_deep_sleep_start();
        Serial.println("This will never be printed");
        break;

    default:
        break;
    }
}
