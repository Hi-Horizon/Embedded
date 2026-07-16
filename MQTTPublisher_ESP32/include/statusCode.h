//
// Created by senne on 16/07/2026.
//

#ifndef MQTTPUBLISHER_ESP32_STATUSCODE_H
#define MQTTPUBLISHER_ESP32_STATUSCODE_H

//esp status is defined in a byte, the bits are arranged as follows:
// - bit 0, CAN TX state: 0 = working, 1 = failed
#define CAN_RX_STATE_MASK   BIT0
#define CAN_RX_WORKING      0b0
#define CAN_RX_FAILED       BIT0

// - bit 1, CAN RX state: 0 = working, 1 = failed
#define CAN_TX_STATE_MASK   BIT1
#define CAN_TX_WORKING      0b0
#define CAN_TX_FAILED       BIT1

// - bit 2, NTP state:    0 = synced, 1 = not synced
#define NTP_STATE_MASK      BIT2
#define NTP_SYNCED          0b0
#define NTP_NOT_SYNCED      BIT2

// - bit 3-5, Wi-Fi state:
#define WIFI_STATE_MASK             (BIT3 | BIT4 | BIT5)
#define WIFI_CONNECTED              (0b0)
#define WIFI_PROVISIONING           (BIT3)
#define WIFI_CONNECTING             (BIT4)
#define WIFI_FAILED_INVALID_CRED    (BIT3 | BIT4)
#define WIFI_FAILED_STA_NOT_FOUND   (BIT5)
#define WIFI_FAILED                 (BIT3 | BIT5)
#define WIFI_NOT_INIT               (BIT3 | BIT4 | BIT5)

// - bit 6-7, MQTT state
#define MQTT_STATE_MASK             (BIT6 | BIT7)
#define MQTT_CONNECTED              (0b0)
#define MQTT_FAILED_INVALID_CRED    (BIT6)
#define MQTT_FAILED                 (BIT7)
#define MQTT_NOT_INIT               (BIT6 | BIT7)

inline void updateStatus(volatile uint8_t *status, uint8_t mask, uint8_t newState) {
    *status = (*status & ~mask) | (newState & mask);
}

#endif //MQTTPUBLISHER_ESP32_STATUSCODE_H