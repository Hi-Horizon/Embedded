#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <TZ.h>
#include <time.h>
#include <FS.h>
#include <LittleFS.h>
#include <CertStoreBearSSL.h>
#include <WiFiUdp.h>

#include "wifiConfig.h"
#include "WIFI_setup/WIFI_setup.h"
#include <espStatus/espStatus.h>

#include <DataFrame.h>
#include <buffer.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <mcp2515.h>
#include <MQTT_API/mqtt_api.h>
#include <NTP_API/NTP_API.h>
#include <SSLcerts_API/SSLcerts_API.h>
#include <CAN_API/CAN_API.h>

DataFrame dataFrame;
WifiCredentials wifiCredentials;
espStatus status;

// A single, global CertStore which can be used by all connections.
// Needs to stay live the entire time any of the WiFiClientBearSSLs
// are present.
BearSSL::CertStore certStore;
BearSSL::WiFiClientSecure *bear = new BearSSL::WiFiClientSecure();

PubSubClient * client;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200);

unsigned long lastMsg = 0; //msg send timer
unsigned long stalenessTimer = 0; //staleness check timer
uint8_t staleness = 0;
uint8_t oldstaleness = 0;

void updateConnectionStatus();

//control
bool newData = false;

struct can_frame canRxMsg;
struct can_frame canEspTxMsg;
MCP2515 mcp2515(D8);

void setup() {
  //SERIAL INIT
  delay(500);
  Serial.begin(9600);
  delay(500);
  Serial.println();

  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
  
  initCan(&mcp2515, &canEspTxMsg);
  initWiFi(&dataFrame, &status);
  initTime(&timeClient, &dataFrame);
  verifyAndInitCerts(&certStore, bear, &status);
  client = initMqtt(client, bear);

  Serial.println("setup finished");
}

void loop() {
  //MQTT client routine
  client->loop();
  updateConnectionStatus();
  readAndParseCan(&mcp2515, &canRxMsg, &dataFrame, &newData);

  if (newData && (millis() - lastMsg > 1000L)) {
    sendEspInfoToCan(&mcp2515, &canEspTxMsg, &dataFrame);
    sendDataToBroker(client, &dataFrame, &newData, &lastMsg);
  }
  
  // TODO: implement the wifi setup API feature
  // if (dataFrame.telemetry.wifiSetupControl == 1) {
  //   Serial.println("starting WiFi config mode");
  //   wifi_config_mode(&status, &wifiCredentials);
  // }
}


void updateConnectionStatus() {
  //Get diagnostic info
  dataFrame.telemetry.internetConnection = WiFi.RSSI();
  dataFrame.telemetry.mqttStatus = client->state();
  
  // first check if internet is still connected
  if (WiFi.status() != WL_CONNECTED) {              
    dataFrame.telemetry.espStatus = WiFi.status();
  }
  //then check if esp is still connected with Broker
  else if (!client->connected()) { 
    dataFrame.telemetry.espStatus = BROKER_CONNECTION_FAILED;  
    mqttReconnect(client, &status);
  }
  
  // if (millis() - stalenessTimer > 3000L) {
  //   if (oldstaleness == staleness) {
  //     status.updateStatus(HARDWARE_FAULT);
  //   }
  //   else if (status.getStatus() == HARDWARE_FAULT) {
  //     status.updateStatus(CONNECTED);
  //   }
  //   oldstaleness = staleness;
  //   stalenessTimer = millis();
  
  //   Serial.print("status is: ");
  //   Serial.println(status.getStatus());
  // }
}

//TODO: redesign wifi_config_mode to fit current hardware
void requestDataframe() {};
void wifi_config_mode(espStatus* status, WifiCredentials *wifiCredentials) {
  configure_WiFi(&dataFrame, status, wifiCredentials, requestDataframe);
  connect_wifi(&dataFrame, status, wifiCredentials, requestDataframe);
}