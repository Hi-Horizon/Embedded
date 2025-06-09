#ifndef WIFIsetup_h
#define WIFIsetup_h

#include <SpiConfig.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <wifiConfig.h>
#include <espStatus/espStatus.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WIFI_setup/website/pages.h>
#include <ArduinoJson.h>

void initWiFi(DataFrame* dataFrame, espStatus* status);
void connect_wifi(DataFrame *data, espStatus* status, WifiCredentials *wifiCredentials, std::function<void ()> idleFn);
void configure_WiFi(DataFrame *data, espStatus* status, WifiCredentials *wifiCredentials, std::function<void ()> idleFn);

#endif