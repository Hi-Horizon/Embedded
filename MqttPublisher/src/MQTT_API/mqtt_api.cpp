#include "mqtt_api.h"

PubSubClient* initMqtt(PubSubClient* client, WiFiClientSecure* bear) {
  //CONNECT MQTT
  client = new PubSubClient(*bear);
  client->setBufferSize(MSG_BUFFER_SIZE);

  const char* mqtt_server_prim = mqtt_server;
  client->setServer(mqtt_server_prim, 8883);
  client->setCallback(onMQTTReceive);
  digitalWrite(LED_BUILTIN, HIGH);
  return client;
}

//buffer for sending mqtt messages
char msg[MSG_BUFFER_SIZE];

//send an mqtt message with relevant data to the broker at topic "data"
void sendDataToBroker(PubSubClient* client, DataFrame* dataFrame, bool* newDataFlag, unsigned long* lastMsg) {
  // If MTU gave data as response, parse and send data with MQTT
  digitalWrite(LED_BUILTIN, LOW);
  snprintf (msg, MSG_BUFFER_SIZE, 
    "{"
    "\"mtuT\":%u,"
    "\"fix\":%u,"
    "\"lat\":%f,"
    "\"lng\":%f,"
    "\"v\":%f,"
    "\"gpsT\":%u,"
    "\"Pz\":%i,"
    "\"mpptT\":%u,"
    "\"escW\":%u,"
    "\"escF\":%u,"
    "\"vm\":%f,"
    "\"mc\":%f,"
    "\"Pu\":%f,"
    "\"escT\":%u,"
    "\"bv\":%f,"
    "\"bc\":%f,"
    "\"bMinv\":%f,"
    "\"bMaxv\":%f,"
    "\"bmsT\":%u"
    "}"
    , dataFrame->telemetry.unixTime
    , dataFrame->gps.fix
    , dataFrame->gps.lat
    , dataFrame->gps.lng
    , dataFrame->gps.speed
    , dataFrame->gps.last_msg
    , dataFrame->mppt.power
    , dataFrame->mppt.last_msg
    , dataFrame->motor.warning
    , dataFrame->motor.failures
    , dataFrame->motor.battery_voltage
    , dataFrame->motor.battery_current
    , dataFrame->motor.battery_current*dataFrame->motor.battery_voltage
    , dataFrame->motor.last_msg
    , dataFrame->bms.battery_voltage
    , dataFrame->bms.battery_current
    , dataFrame->bms.min_cel_voltage
    , dataFrame->bms.max_cel_voltage
    , dataFrame->bms.last_msg
  );  

  bool success = client->publish("data", msg);  
  digitalWrite(LED_BUILTIN, HIGH);

  *lastMsg = millis();
  *newDataFlag = false;

  //debug for troubleshooting purposes
  Serial.print("message sent: ");
  Serial.println(success);
}

void mqttReconnect(PubSubClient* client, espStatus* status, std::function<void ()> idleFn) {
  // Loop until we’re reconnected
  status->updateStatus(CONNECTING_BROKER);

  unsigned long lastIdlePerform = 0;
  unsigned long lastMqttReconnect = 0;
  unsigned long reconnectWaitTime = 5000;

  while (!client->connected()) {
    if (millis() - lastIdlePerform > 1000) {
      idleFn();
      lastIdlePerform = millis();
    }

    Serial.print("Attempting MQTT connection…");
    String clientId = "ESP8266Client - MyClient"; // TODO:why in this loop?
    // Attempt to connect
    // Insert your password
    if (millis() - lastMqttReconnect > reconnectWaitTime) {
      if (client->connect(clientId.c_str(), MQTT_USER, MQTT_PWD)) {
        Serial.println("connected");
        // Once connected, publish an announcement…
        client->publish("testTopic", "hello world");
        // … and resubscribe
        client->subscribe("testTopic");
      } 
      else {
        status->updateStatus(BROKER_CONNECTION_FAILED);
        Serial.print("failed, rc = ");
        Serial.print(client->state());
        Serial.println(" try again in 5 seconds");
        Serial.println(WiFi.status());
      }
    }
  }

  status->updateStatus(CONNECTED);
}

void onMQTTReceive(char* topic, byte* payload, unsigned int length) {
  //do nothing if MQTT data is received, yet..
};
