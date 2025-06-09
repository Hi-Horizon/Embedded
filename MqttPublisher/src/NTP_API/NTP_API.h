#ifndef NTP_API__H_
#define NTP_API__H_

#include "Arduino.h"
#include <NTPClient.h>
#include "DataFrame.h"
#include <time.h>
#include <TZ.h>

uint32_t setDateTime();
void initTime(NTPClient* timeClient, DataFrame* dataFrame);

#endif