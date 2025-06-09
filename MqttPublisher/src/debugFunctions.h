#ifndef DEBUGFUNCTIONS__H_
#define DEBUGFUNCTIONS__H_

#include "Arduino.h"

void print_buf(uint8_t *buf, uint32_t len) {
  for(unsigned int i=0; i < len; i++) {
    Serial.print(buf[i]);
    Serial.print(',');
  }
  Serial.println();
}

#endif
