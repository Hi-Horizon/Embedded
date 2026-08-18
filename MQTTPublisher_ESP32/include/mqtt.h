//
// Created by senne on 09/07/2026.
//

#ifndef MQTTPUBLISHER_ESP32_MQTT_H
#define MQTTPUBLISHER_ESP32_MQTT_H

#include "stdio.h"
#include "crc16.h"
#include "buffer.h"
#include "CANInbox.h"
#include <string.h>
#include <esp_log.h>
#include "mqtt_client.h"
#include "statusCode.h"

extern volatile uint8_t espStatus;

// config options
#define CONFIG_BROKER_URI                      "mqtts://7f15879e36cf4f3781ca3df1f338b397.s1.eu.hivemq.cloud:8883"
#define CONFIG_BROKER_BIN_SIZE_TO_SEND         512

// TLS Certficate
extern const uint8_t mqtt_eclipseprojects_io_pem_start[]    asm("_binary_cacert_pem_start");
extern const uint8_t mqtt_eclipseprojects_io_pem_end[]      asm("_binary_cacert_pem_end");

#define CRC_POLYNOMIAL 0xBAAD

int32_t buildCanDataMQTTMessage(CanInbox* canInbox, uint8_t* msg);
void mqtt_app_start(esp_mqtt_client_handle_t *client);

#endif //MQTTPUBLISHER_ESP32_MQTT_H