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

void search_wifi(espStatus* status);
void connect_wifi(espStatus* status, WifiCredentials *wifiCredentials);
void configure_WiFi(espStatus* status, WifiCredentials *wifiCredentials);

#endif