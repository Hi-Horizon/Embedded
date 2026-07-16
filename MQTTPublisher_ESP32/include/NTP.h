//
// Created by senne on 15/07/2026.
//

#ifndef MQTTPUBLISHER_ESP32_NTP_H
#define MQTTPUBLISHER_ESP32_NTP_H

#include <esp_netif_sntp.h>
#include <esp_log.h>
#include <time.h>
#include "statusCode.h"

extern volatile uint8_t espStatus;

void NTP_fetch_time(void);

#endif //MQTTPUBLISHER_ESP32_NTP_H