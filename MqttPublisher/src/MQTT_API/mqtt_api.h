#ifndef MQTT_API__H_
#define MQTT_API__H_

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "wifiConfig.h"
#include "CanInbox.h"
#include "CRC.h"
#include "CRC16.h"

#include "DataFrame.h"

#define MSG_BUFFER_SIZE 3000
#define CRC_POLYNOMIAL 0xBAAD

void sendDataToBroker(PubSubClient* client, CanInbox* CanInbox, bool* newDataFlag, unsigned long* lastMsg);
PubSubClient* initMqtt(PubSubClient* client, WiFiClientSecure* bear);
void mqttReconnect(PubSubClient* client, std::function<void ()> idleFn);
void onMQTTReceive(char* topic, byte* payload, unsigned int length);

#endif