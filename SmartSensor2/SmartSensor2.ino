// @ awake time, SmartSensor.ino is able to :
// - read the soil moisture sensor;
// - watering control via hysteresis, 2 humidity thresholds and activating a water pump, for example;
// - send data and images to cloud; 
// - label images according humidity;
// - fall asleep;

// Some lessons learned:

// ###
// error:
// error when compiling from Arduino IDE:
// .arduino15/packages/esp32/tools/esptool_py/2.6.1/esptool.py", line 38, in
// import serial
// ImportError: No module named serial

// solution: 
// https://github.com/espressif/esptool/issues/528
// $ sudo apt install python-is-python3
// or, in ubuntu 22.04 LTS:
// 1. sudo apt install python3-pip
// 2. pip install esptool

// ###
// error:
// A fatal error occurred: Could not open /dev/ttyACM0, the port doesn't exist

// solution:
// sudo adduser username dialout
// sudo chmod a+rw /dev/ttyACM0
// or:
// sudo usermod -a -G dialout username
// and restart pc

// ###
// Before worked, but, after serious problems with native interruption as 
// https://circuitdigest.com/microcontroller-projects/esp32-timers-and-timer-interrupts 
// (incompability, perhaps) I decided to adopt the ESP32TimerInterrupt library

// ###
// error:
// Osciloscópio no pino 9... não conecta wifi!!!

// ###
// error:
// compilation problems in ESP32TimerInterrupt

// Solution: 
// downgrade esp32 board from 3.0.0-alpha to 2.0.11

// ###
// error:
// with camera integration reseting always

// Solution: 
// enable OPI PSRAM in tools			 

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "Connection.h"
#include "Camera.h"

#define MOISTURE_SENSOR 1 //GPIO1
// #define PUMP 8  //GPIO8 afetado pela placa da câmera
#define PUMP 7  //GPIO7
// based in 30 samples
// #define DRY 680     // at 10 bit resolution, minimum value on the air, void space, at lab with varnished capacitive sensor
// #define SOAKED 584  // at 10 bit resolution, maximum value for full cup of mineral water, at lab with varnished capacitive sensor
// #define DRY 511     // at 9 bit resolution, maximum value at home with HD-38 (in the air, void space)
// #define SOAKED 347  // at 9 bit resolution, minimum value at home with HD-38 (full cup of well water)
// #define DRY 511     // at 9 bit resolution, minimum value at lab with HD-38 (in the air, void space)
// #define SOAKED 210  // at 9 bit resolution, maximum value at lab with HD-38 (full cup of mineral water)
// #define DRY 388     // at 9 bit resolution, minimum value at lab with HD-38 (in a DRY soil)
// #define SOAKED 228  // at 9 bit resolution, maximum value at lab with HD-38 (in a SOAKED soil)
// #define DRY 505     // at 10 bit resolution, minimum value at lab with HD-38 (in a DRY soil)
// #define SOAKED 468  // at 10 bit resolution, maximum value at lab with HD-38 (in a SOAKED soil)
// #define DRY 505     // at 11 bit resolution, minimum value at lab with HD-38 (in a DRY soil)
// #define SOAKED 468  // at 11 bit resolution, maximum value at lab with HD-38 (in a SOAKED soil)
// #define DRY 108     // at 7 bit resolution, minimum value at lab with HL-69 (in a DRY soil)
// #define SOAKED 38  // at 7 bit resolution, maximum value at lab with HL-69 (in a SOAKED soil)
// #define DRY 255     // at 8 bit resolution, minimum value at lab with HL-69 (in a DRY soil)
// #define SOAKED 76  // at 8 bit resolution, maximum value at lab with HL-69 (in a SOAKED soil)
// #define DRY 289     // at 9 bit resolution, minimum value at lab with cap sensor HW-390 v2.0 (in a DRY soil through the Goliah black box)
// #define SOAKED 140  // at 9 bit resolution, maximum value at lab with cap sensor HW-390 v2.0 (in a SOAKED soil through the Goliah black box)
#define DRY 241     // at 9 bit resolution, minimum value at lab with COVERED cap sensor HW-390 v2.0 (in a DRY soil through the mother board)
#define SOAKED 164  // at 9 bit resolution, maximum value at lab with COVERED cap sensor HW-390 v2.0 (in a SOAKED soil through the mother board)

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */

struct Hysteresis {
    int upper_threshold;
    int lower_threshold;
};

struct Hysteresis thresholds = {41,40};
String thresholds_string = "";

int moisture = 0;
const int SAMPLES_EFFECTIVE_NUMBER = 256;
const int SAMPLES_TOTAL_NUMBER = SAMPLES_EFFECTIVE_NUMBER + 2;

const int PUMP_ON_PERIOD = 4000; // miliseconds

unsigned long sleep_period = 40; //minutes
unsigned long sleep_period_aux1 = sleep_period*60;
unsigned long SLEEP_PERIOD = sleep_period_aux1*uS_TO_S_FACTOR;

bool turn_on = false;
bool sending_failed = false;
bool connection_status = false;
bool flash_on =false;
enum State { moisture_read,
             pump_control,
             publish_data,
             deep_sleep };
State state = moisture_read;

int drop_counter = 0;

camera_fb_t * image = NULL;

void wakeupHandle(){

  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void setup() {
  
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  // Sets the sample bits resolution,2⁹ = 512 bytes
  analogReadResolution(9);
  pinMode(PUMP, OUTPUT);
  digitalWrite(PUMP, LOW);
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  Serial.println(PUMP_ON_PERIOD);
  Serial.println(SLEEP_PERIOD);

  //Handle the wakeup reason for ESP32
  wakeupHandle();
  /*
  First we configure the wake up source
  We set our ESP32 to wake up every 30 minutes
  */
  esp_sleep_enable_timer_wakeup(SLEEP_PERIOD);
  Serial.println("Setup ESP32 to sleep for every " + String(sleep_period) + " minutes");

  cameraSetup();	
  connection_status = connectionSetup();	
}

void pumpControl(bool flag) {
  if (flag) {
    digitalWrite(PUMP, HIGH);
    Serial.println("flag true: ligou! e espera 10 seg");
    delay(PUMP_ON_PERIOD);
    digitalWrite(PUMP, LOW);
    Serial.println("flag true: desligou!");
  } else {
    digitalWrite(PUMP, LOW);
    Serial.println("flag false: desligou!");
  }
}

int moistureRead(){

  int sum = 0;
  int current = 0;
  int array_samples[SAMPLES_TOTAL_NUMBER]; 

  // read a total number of samples filling an array
  for (int m=0; m < SAMPLES_TOTAL_NUMBER; m++) 
      {
          current = analogRead(MOISTURE_SENSOR);
          array_samples[m] = current;
          sum = sum + current;
      }

  // init min and max values
  int minValue = min(array_samples[0], array_samples[1]);
  int maxValue = max(array_samples[0], array_samples[1]);

  // search for min and max outliers values in the array 
  for (int i = 2; i < SAMPLES_TOTAL_NUMBER; i++) {
    minValue = min(minValue, array_samples[i]);
    maxValue = max(maxValue, array_samples[i]);
  }

  // discard the outliers in sum
  moisture = (sum - minValue - maxValue)/SAMPLES_EFFECTIVE_NUMBER;

  // reap the values out of range
  if (moisture <= SOAKED){
      moisture = SOAKED;
    }
    if (moisture >= DRY){
      moisture = DRY;
    }

  // mapping the moisture between 0-100%
  moisture = map(moisture, DRY, SOAKED, 0, 100);
  return moisture;
}

void updateHysteresis(){
  thresholds_string = receiveData();
  String upper_threshold = thresholds_string.substring(0, 2);
  String lower_threshold = thresholds_string.substring(3, 5);

  if ((upper_threshold.toInt()!=0)&&(lower_threshold.toInt()!=0)&&(upper_threshold.toInt()>=lower_threshold.toInt())){
    thresholds.upper_threshold = upper_threshold.toInt();
    thresholds.lower_threshold = lower_threshold.toInt();
  }
}

void loop() {
  switch (state) {
    
    digitalWrite(LED_BUILTIN, LOW);  // turn the LED on (HIGH is the voltage level)

    case moisture_read:
      Serial.println("moisture_read");
      moisture = moistureRead();
      Serial.print("Moisture: ");
      Serial.println(moisture);
      state = pump_control;
      break;

    case pump_control:
      Serial.println("pump_control");
      if (connection_status){
        updateHysteresis();
        state = publish_data;
      }
      else{
        state = deep_sleep;
      }
      if (moisture <= thresholds.lower_threshold) {
        turn_on = true;
        pumpControl(turn_on);
      } 
      else if (moisture >= thresholds.upper_threshold) {
        turn_on = false;
        pumpControl(turn_on);
      }
      else {
        pumpControl(turn_on);
      }
      
      break;

    case publish_data:
      Serial.println("publish_data");
      sending_failed = true;
      Serial.println("1==========");
      sendData(moisture, turn_on, drop_counter);
      // image = takePicture(flash_on = true);
      // sendImage(moisture, image); //Capture and send image
      // image = takePicture(flash_on = true);
      // sendImage(moisture, image); //Capture and send image
      // image = takePicture(flash_on = true);
      // sendImage(moisture, image); //Capture and send image

      image = takePicture(flash_on = false);
      sendImage(moisture, image); //Capture and send image
      image = takePicture(flash_on = false);
      sendImage(moisture, image); //Capture and send image
      image = takePicture(flash_on = false);
      sendImage(moisture, image); //Capture and send image
      Serial.println("2==========");
      sending_failed = false;
      state = deep_sleep;

      break;

    case deep_sleep:
      if (connection_status){
        connectionClose();
      }
      Serial.println("Going to sleep now");
      digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
      digitalWrite(PUMP, LOW);
      esp_deep_sleep_start();
      Serial.println("This will never be printed");	
      break;

    default:
      break;
  }
}
