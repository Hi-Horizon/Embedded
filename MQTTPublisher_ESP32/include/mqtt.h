//
// Created by senne on 09/07/2026.
//

#ifndef MQTTPUBLISHER_ESP32_MQTT_H
#define MQTTPUBLISHER_ESP32_MQTT_H

#include "stdio.h"
#include "crc16.h"
#include "buffer.h"
#include "DataFrame.h"
#include "CanInbox.h"
#include <string.h>

#define CRC_POLYNOMIAL 0xBAAD

int32_t buildCanDataMQTTMessage(CanInbox* canInbox, uint8_t* msg);

#endif //MQTTPUBLISHER_ESP32_MQTT_H