//
// Created by senne on 15/07/2026.
//

#ifndef MQTTPUBLISHER_ESP32_WIFI_PROVISIONING_H
#define MQTTPUBLISHER_ESP32_WIFI_PROVISIONING_H

#include <esp_event.h>
#include <esp_netif_types.h>
#include <esp_wifi.h>
#include <esp_wifi_default.h>
#include <esp_wifi_types_generic.h>
#include <protocomm_ble.h>
#include <protocomm_security.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "network_provisioning/manager.h"
#include "network_provisioning/scheme_ble.h"
#include "statusCode.h"

#define WIFI_CONNECTED_EVENT BIT0

extern EventGroupHandle_t wifi_event_group;
extern uint8_t provisionCMD;
extern volatile uint8_t espStatus;

void init_wifi(void);
void start_wifi_provisioning();

#endif //MQTTPUBLISHER_ESP32_WIFI_PROVISIONING_H