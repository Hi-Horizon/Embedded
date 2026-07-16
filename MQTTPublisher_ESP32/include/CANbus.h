//
// Created by senne on 15/07/2026.
//

#ifndef MQTTPUBLISHER_ESP32_CANBUS_H
#define MQTTPUBLISHER_ESP32_CANBUS_H

#include <esp_twai_onchip.h>
#include <esp_twai.h>

void CANbus_app_start(twai_node_handle_t *node_hdl, twai_event_callbacks_t *user_cbs);
void buildEspStatusFrame(uint8_t *buf, uint8_t espStatus, uint32_t currentTime);

#endif //MQTTPUBLISHER_ESP32_CANBUS_H