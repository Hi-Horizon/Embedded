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

// A single, global CertStore which can be used by all connections.
// Needs to stay live the entire time any of the WiFiClientBearSSLs
// are present.
BearSSL::CertStore certStore;
BearSSL::WiFiClientSecure *bear = new BearSSL::WiFiClientSecure();

PubSubClient * client;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200);

unsigned long lastMsg = 0; //msg send timer
unsigned long lastEspInfoSend = 0;
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
struct can_frame canWifiCredentialsTxMsg;
MCP2515 mcp2515(D8);

void setup() {
  //SERIAL INIT
  delay(500);
  Serial.begin(9600);
  delay(500);
  Serial.println();

  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
  
  dataFrame.esp.status = ESP_START;
  initCan(&mcp2515, &canEspTxMsg, &canWifiCredentialsTxMsg);
  
  dataFrame.esp.status = ESP_REQUESTING_WIFI_CONFIG;
  bool wifiConfigReceived = false;
  while (!wifiConfigReceived) {
    sendEspInfoToCan(&mcp2515, &canEspTxMsg, &dataFrame);
    wifiConfigReceived = getWiFiCredentialsFromCan(&mcp2515, &canRxMsg, &wifiCredentials, 5000);
  }
  Serial.println(wifiCredentials.ssid);
  Serial.println(wifiCredentials.password);
  
  dataFrame.esp.status = ESP_WIFI_CONNECT_ATTEMPT;
  connect_wifi(&dataFrame, &wifiCredentials, wifiConfigModeListener);

  dataFrame.esp.status = ESP_NTP_TIME_SYNC;
  initTime(&timeClient, &dataFrame, wifiConfigModeListener);

  dataFrame.esp.status = ESP_CONNECTING_BROKER;
  verifyAndInitCerts(&certStore, bear);
  client = initMqtt(client, bear);

  Serial.println("setup finished");
  dataFrame.esp.status = ESP_OPERATING;
}

void loop() {
  //MQTT client routine
  client->loop();
  updateConnectionStatus();
  readAndParseCan(&mcp2515, &canRxMsg, &dataFrame, &newData);

  if (millis() - lastEspInfoSend > 1000L) {
    sendEspInfoToCan(&mcp2515, &canEspTxMsg, &dataFrame);
    lastEspInfoSend = millis();
  }

  if (millis() - lastMsg > 1000L) {
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
  dataFrame.esp.NTPtime = timeClient.getEpochTime();
  
  // first check if internet is still connected. if not, reconnect
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("wifi not connected");
    dataFrame.esp.status = ESP_WIFI_CONNECT_ATTEMPT;
    connect_wifi(&dataFrame, &wifiCredentials, wifiConfigModeListener);
  }
  
  //then check if esp is still connected with Broker
  else if (!client->connected()) { 
    Serial.println("mqtt not connected");
    dataFrame.esp.status = ESP_CONNECTING_BROKER;  
    mqttReconnect(client, wifiConfigModeListener);
  }

  if (WiFi.status() == WL_CONNECTED && client->connected()) {
    dataFrame.esp.status = ESP_OPERATING; 
  }
}

//used for init functions
void wifiConfigModeListener() {
  canListenForWifiConfigToggle(&mcp2515, &canRxMsg, &dataFrame);
  //needs to be here as other function cant call WiFi_Config_mode();
  if (dataFrame.esp.wifiSetupControl == 1) {
    wifi_config_mode();
  }

  if (millis() - lastEspInfoSend > 1000L) {
    sendEspInfoToCan(&mcp2515, &canEspTxMsg, &dataFrame);
    lastEspInfoSend = millis();
  }
}

//used in the wifi_config_mode function to avoid recursion
void listenForWifiConfigToggle() {
  canListenForWifiConfigToggle(&mcp2515, &canRxMsg, &dataFrame);
};

//if this is cancelled at any point, reconnection should be done outside the loop
void wifi_config_mode() {
  Serial.println("Starting WiFi New Config Mode");
  dataFrame.esp.status = ESP_WIFI_NEW_CONFIG_MODE;
  configure_WiFi(&dataFrame, &newWifiCredentials, listenForWifiConfigToggle);
  //always perform this, only difference being old WiFi connection or new
  dataFrame.esp.status = ESP_WIFI_CONNECT_ATTEMPT;
  if (dataFrame.esp.wifiSetupControl == 1) {
    if (connect_wifi(&dataFrame, &newWifiCredentials, listenForWifiConfigToggle)) {
      // Connected, write to SD, cancel not possible anymore
      sendWiFICredentialsOverCan(&mcp2515, &canWifiCredentialsTxMsg, &newWifiCredentials);

      // copy new info to current info
      memcpy(wifiCredentials.ssid, newWifiCredentials.ssid, 128);
      memcpy(wifiCredentials.password, newWifiCredentials.password, 128);
      wifiCredentials.ssidLength      = newWifiCredentials.ssidLength;
      wifiCredentials.passwordLength  = newWifiCredentials.passwordLength;
    }
  }
  // If mode is cancelled, attempt to reconnect to old
  if (dataFrame.esp.wifiSetupControl == 0) {
    connect_wifi(&dataFrame, &wifiCredentials, listenForWifiConfigToggle);
  }
  //done, reset control state
  dataFrame.esp.wifiSetupControl = 0;
}

