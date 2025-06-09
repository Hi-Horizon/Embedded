#ifndef MQTT_API__H_
#define MQTT_API__H_

#include "Arduino.h"
#include <PubSubClient.h>
#include "wifiConfig.h"
#include <espStatus/espStatus.h>

#include "DataFrame.h"

#define MSG_BUFFER_SIZE 3000

void sendDataToBroker(PubSubClient* client, DataFrame* dataFrame, bool* newDataFlag, unsigned long* lastMsg);
PubSubClient* initMqtt(PubSubClient* client, WiFiClientSecure* bear);
void mqttReconnect(PubSubClient* client, espStatus* status);
void onMQTTReceive(char* topic, byte* payload, unsigned int length);

#endif