#ifndef NTP_API__H_
#define NTP_API__H_

#include "Arduino.h"
#include <NTPClient.h>
#include "DataFrame.h"
#include <time.h>
#include <TZ.h>

uint32_t setDateTime(std::function<void()> idleFn);
void initTime(NTPClient* timeClient, DataFrame* dataFrame, std::function<void()> idleFn);

#endif