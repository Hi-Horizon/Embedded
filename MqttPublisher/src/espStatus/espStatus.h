#ifndef espStatus_h
#define espStatus_h

#include "arduino.h"
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include "Wire.h"

#define HARDWARE_FAULT            0
#define WIFI_DISCONNECTED         1
#define WIFI_SEARCH               2
#define WIFI_LOGIN_TRY            3
#define WIFI_LOGIN_WRONG_PASSWORD 4
#define WIFI_LOGIN_FAILED         5
#define NO_SIGNAL_FAULT           6

#define WIFI_CONNECTED            7
#define SYNCING_NTP               8
#define TESTING_CERTS             9
#define CONNECTING_BROKER         10
#define BROKER_CONNECTION_FAILED  11
#define CONNECTED                 12
#define FORCE_SPI_RESET           13

class espStatus {
    public:
        uint8_t getStatus();
        uint8_t getConnectionStrength();
        void updateStatus(uint8_t newStatus);
        void updateStatus(wl_status_t wifi_status);
        void updateConnectionStrength(int32_t newConnectionStrength);
    private:
        uint8_t status = HARDWARE_FAULT;
        uint8_t connectionStrength = 0;
        void sendStatusSerial();
};

#endif