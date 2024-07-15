#include "Arduino.h"

int start;
void wait(int interval){
  start = millis();
  while((millis() - start) < interval){
  }
}

  