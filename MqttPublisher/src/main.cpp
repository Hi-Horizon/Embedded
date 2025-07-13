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
WifiCredentials newWifiCredentials;
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
void wifi_config_mode();
void wifiConfigModeListener();

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
  getWiFiCredentialsFromCan(&mcp2515, &canRxMsg, &wifiCredentials);
  connect_wifi(&dataFrame, &status, &wifiCredentials, wifiConfigModeListener);
  initTime(&timeClient, &dataFrame, wifiConfigModeListener);
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
  
  if (dataFrame.esp.wifiSetupControl == 1) {
    wifi_config_mode();
  }
}


void updateConnectionStatus() {
  //Get diagnostic info
  dataFrame.esp.internetConnection = WiFi.RSSI();
  dataFrame.esp.mqttStatus = client->state();
  
  // first check if internet is still connected. if not, reconnect
  if (WiFi.status() != WL_CONNECTED) {              
    dataFrame.esp.status = WiFi.status();
    connect_wifi(&dataFrame, &status, &wifiCredentials, wifiConfigModeListener);
  }
  //then check if esp is still connected with Broker
  else if (!client->connected()) { 
    dataFrame.esp.status = BROKER_CONNECTION_FAILED;  
    mqttReconnect(client, &status, wifiConfigModeListener);
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

void wifiConfigModeListener() {
  canListenForWifiConfigToggle(&mcp2515, &canRxMsg, &dataFrame);
  //needs to be here as other function cant call WiFi_Config_mode();
  if (dataFrame.esp.wifiSetupControl == 1) {
    wifi_config_mode();
  }
}

void listenForWifiConfigToggle() {
  canListenForWifiConfigToggle(&mcp2515, &canRxMsg, &dataFrame);
};

//if this is cancelled at any point, reconnection should be done outside the loop
void wifi_config_mode() {
  Serial.println("Starting WiFi Config Mode");
  configure_WiFi(&dataFrame, &status, &newWifiCredentials, listenForWifiConfigToggle);
  //always perform this, only difference being old WiFi connection or new
  if (dataFrame.esp.wifiSetupControl == 1) {
    if (connect_wifi(&dataFrame, &status, &newWifiCredentials, listenForWifiConfigToggle)) {
      // Connected, write to network, cancel not possible anymore
      // writeSD()

      // copy new info to current info
      memcpy(wifiCredentials.ssid, newWifiCredentials.ssid, 128);
      memcpy(wifiCredentials.password, newWifiCredentials.password, 128);
      wifiCredentials.ssidLength      = newWifiCredentials.ssidLength;
      wifiCredentials.passwordLength  = newWifiCredentials.passwordLength;
    }
  }
  // If mode is cancelled, attempt to reconnect to old
  if (dataFrame.esp.wifiSetupControl == 0) {
    connect_wifi(&dataFrame, &status, &wifiCredentials, listenForWifiConfigToggle);
  }
  //done, reset control state
  dataFrame.esp.wifiSetupControl = 0;
}

