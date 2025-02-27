
#include <HTTPClient.h>
#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include "Adafruit_BME680.h"
#include "Adafruit_SHT31.h"
#include "SparkFun_SGP30_Arduino_Library.h"

#define SEALEVELPRESSURE_HPA (1013.25)
#define SENSOR 1
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define LED_BUILTIN 2

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(0x49, 12345);  //sensor 2 = 0x39| sensor1 = 0x49
Adafruit_BME680 bme;                                                   // I2C
Adafruit_SHT31 sht31 = Adafruit_SHT31();
double tempBME = 0, resBME = 0, humBME = 0, pressBME = 0, tlsLUX = 0, tempSHT31 = 0;
double sgpCO2 = 0, sgpEthanol = 0, sgpH2 = 0, sgpTVOC = 0;
int ledGpio = 2;
String url = "";

unsigned long sleep_period = 10;  //minutes
unsigned long sleep_period_aux1 = sleep_period * 60;
unsigned long SLEEP_PERIOD = sleep_period_aux1 * uS_TO_S_FACTOR;

SGP30 mySensor;

void sgp30Init();
void sgp30Readings();

bool enableHeater = false;

void wakeupHandle() {

  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup caused by ULP program"); break;
    default: Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

void sgp30Init() {
  pinMode(ledGpio, OUTPUT);
  digitalWrite(ledGpio, 1);
  Wire.begin();
  uint16_t cmd = 0x2003;
  Wire.beginTransmission(0x58);
  Wire.write(cmd >> 8);
  Wire.write(cmd & 0xFF);
  Wire.endTransmission();
  Wire.begin();
  //Initialize sensor
  if (mySensor.begin() == false) {
    Serial.println("No SGP30 Detected. Check connections.");
    int resetBuf[1];
    int idxReset = 0;
    while (1) {
      resetBuf[idxReset] = idxReset;
      idxReset++;
      ESP.restart();
    }
  }
  digitalWrite(ledGpio, 0);
  //Initializes sensor for air quality readings
  //measureAirQuality should be called in one second increments after a call to initAirQuality
  mySensor.initAirQuality();
  Serial.println("waiting first 20 sensor readings....it's recomended by library");
  for (int i = 0; i < 20; i++) {
    delay(1000);  //Wait 1 second
    //measure CO2 and TVOC levels
    mySensor.measureAirQuality();
    Serial.print("desprezou ");
    Serial.print(i);
    Serial.println(" leituras");
  }
}

void sgp30Readings() {
  mySensor.measureAirQuality();
  Serial.print("CO2: ");
  sgpCO2 = mySensor.CO2;
  sgpTVOC = mySensor.TVOC;
  Serial.print(sgpCO2);
  Serial.print(" ppm\tTVOC: ");
  Serial.print(sgpTVOC);
  Serial.println(" ppb");
  mySensor.measureRawSignals();
  Serial.print("Raw H2: ");
  sgpEthanol = mySensor.ethanol;
  sgpH2 = mySensor.H2;
  Serial.print(mySensor.H2);
  Serial.print(" \tRaw Ethanol: ");
  Serial.println(mySensor.ethanol);
}


void configureSensor(void) {
  /* You can also manually set the gain or enable auto-gain support */
  // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  tsl.enableAutoRange(true); /* Auto-gain ... switches automatically between 1x and 16x */

  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS); /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

  /* Update these values depending on what you've set above! */
  Serial.println("------------------------------------");
  Serial.print("Gain:         ");
  Serial.println("Auto");
  Serial.print("Timing:       ");
  Serial.println("13 ms");
  Serial.println("------------------------------------");
}

//ler tlsLUX
void readtlsLUX() {
  /* Get a new sensor event */
  sensors_event_t event;
  tsl.getEvent(&event);

  /* Display the results (light is measured in tlsLUX) */
  if (event.light) {
    tlsLUX = event.light;
    Serial.print(tlsLUX);
    Serial.println(" tlsLUX");
  } else {
    /* If event.light = 0 tlsLUX the sensor is probably saturated
       and no reliable data could be generated! */
    Serial.println("Sensor overload");
  }
  //delay(250);
}

// ler sht31
void readSHT31() {
  tempSHT31 = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (!isnan(tempSHT31)) {  // check if 'is not a number'
    Serial.print("Temp *C = ");
    Serial.print(tempSHT31);
    Serial.print("\t\t");
  } else {
    Serial.println("Failed to read temperature");
  }

  if (!isnan(h)) {  // check if 'is not a number'
    Serial.print("Hum. % = ");
    Serial.println(h);
  } else {
    Serial.println("Failed to read humidity");
  }
}

//ler bme
void readBME680() {
  tempBME = 0;
  resBME = 0;
  humBME = 0;
  pressBME = 0;
  // Tell BME680 to begin measurement.
  unsigned long endTime = bme.beginReading();
  if (endTime == 0) {
    Serial.println(F("Failed to begin reading :("));
    return;
  }
  Serial.print(F("Reading started at "));
  Serial.print(millis());
  Serial.print(F(" and will finish at "));
  Serial.println(endTime);

  Serial.println(F("You can do other work during BME680 measurement."));
  // There's no need to delay() until millis() >= endTime: bme.endReading()
  // takes care of that. It's okay for parallel work to take longer than
  // BME680's measurement time.

  // Obtain measurement results from BME680. Note that this operation isn't
  // instantaneous even if milli() >= endTime due to I2C/SPI latency.
  if (!bme.endReading()) {
    Serial.println(F("Failed to complete reading :("));
    return;
  }
  Serial.print(F("Reading completed at "));
  Serial.println(millis());
  tempBME = bme.temperature;
  resBME = bme.gas_resistance / 1000.0;
  humBME = bme.humidity;
  pressBME = bme.pressure / 100.0;

  Serial.print(F("Temperature = "));
  Serial.print(bme.temperature);
  Serial.println(F(" *C"));

  Serial.print(F("Pressure = "));
  Serial.print(bme.pressure / 100.0);
  Serial.println(F(" hPa"));

  Serial.print(F("Humidity = "));
  Serial.print(bme.humidity);
  Serial.println(F(" %"));

  Serial.print(F("Gas = "));
  Serial.print(bme.gas_resistance / 1000.0);
  Serial.println(F(" KOhms"));

  Serial.print(F("Approx. Altitude = "));
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(F(" m"));
  Serial.println();
}

void sendDataToServer() {
  Serial.println("Conectando ao Wi-Fi para enviar dados...");

  // WiFi.mode(WIFI_STA);
  WiFi.begin("CINGUESTS", "acessocin");

  int contador = 0;
  while (WiFi.status() != WL_CONNECTED && contador < 10) {
    Serial.print('.');
    delay(1000);
    contador++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi conectado!");
    Serial.println(WiFi.localIP());

    HTTPClient http;
    Serial.println("Enviando dados para o servidor:");
    Serial.println(url);

    http.begin(url.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println(httpCode);
      Serial.println(payload);
    } else {
      Serial.println("Erro na requisição HTTP.");
    }

    http.end();
  } else {
    Serial.println("Falha ao conectar no Wi-Fi.");
  }

  WiFi.disconnect();
}
void readSensors() {
  readBME680();
  sgp30Readings();
  readSHT31();
  readtlsLUX();
}

void updateUrl() {
  char *GScriptId = "AKfycbzVvNmoZzZ5MK2XKpeqguDhdCyOSSvhzsuYKbTb5aV6DBQE7crH7pDhoYEpNHIJVpXa0A";
  url = "https://script.google.com/macros/s/" + String(GScriptId) + "/exec?";
  url += "tempBME=" + String(tempBME);
  url += "&humBME=" + String(humBME);
  url += "&pressBME=" + String(pressBME);
  url += "&tlsLUX=" + String(tlsLUX);
  url += "&tempSHT31=" + String(tempSHT31);
  url += "&co2SGP=" + String(sgpCO2);
  url += "&sgpTVOC=" + String(sgpTVOC);
  url += "&resBME=" + String(resBME);
  url += "&sgpH2=" + String(sgpH2);
  url += "&sgpEthanol=" + String(sgpEthanol);
  url += "&macAddress=" + String(SENSOR);
}
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  Serial.begin(115200);
  //Handle the wakeup reason for ESP32
  wakeupHandle();
  esp_sleep_enable_timer_wakeup(SLEEP_PERIOD);
  Serial.println("Setup ESP32 to sleep for every " + String(sleep_period) + " minutes");
  // Serial.println(F("BME680 async test"));
  sgp30Init();

  Serial.println("SHT31 test");
  if (!sht31.begin(0x44)) {  // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1)
      ;
  }

  Serial.print("Heater Enabled State: ");
  if (sht31.isHeaterEnabled())
    Serial.println("ENABLED");
  else
    Serial.println("DISABLED");

  if (!bme.begin()) {
    Serial.println(F("Could not find a valid BME680 sensor, check wiring!"));
    while (1)
      ;
  }
  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);  // 320*C for 150 ms

  if (!tsl.begin()) {
    /* There was a problem detecting the TSL2561 ... check your connections */
    Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
    while (1)
      ;
  }
  configureSensor();
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  readSensors();
  updateUrl();
  sendDataToServer();
  Serial.println("Going to sleep now");

  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}
