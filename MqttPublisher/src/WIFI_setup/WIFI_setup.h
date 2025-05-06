#ifndef WIFIsetup_h
#define WIFIsetup_h

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <wifiConfig.h>
#include <espStatus/espStatus.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WIFI_setup/website/pages.h>
#include <ArduinoJson.h>

void search_wifi(espStatus* status);
void configure_WiFi(espStatus* status);

#endif