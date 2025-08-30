#ifndef SSLCERTS_API__H_
#define SSLCERTS_API__H_

#include "Arduino.h"
#include "FS.h"
#include "LittleFS.h"
#include <ESP8266WiFi.h>

void verifyAndInitCerts(BearSSL::CertStore* certStore, BearSSL::WiFiClientSecure* bear);

#endif