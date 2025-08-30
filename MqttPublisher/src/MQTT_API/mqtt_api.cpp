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
    "\"v\":%.1f,"
    "\"gpsT\":%u,"
    "\"Pz\":%i,"
    "\"mpptT\":%u,"
    "\"escW\":%u,"
    "\"escF\":%u,"
    "\"vm\":%.2f,"
    "\"mc\":%.2f,"
    "\"Pu\":%.2f,"
    "\"escT\":%u,"
    "\"bv\":%.2f,"
    "\"bc\":%.2f,"
    "\"bMinv\":%.2f,"
    "\"bMaxv\":%.2f,"
    "\"vc1\":%.2f,"
    "\"vc2\":%.2f,"
    "\"vc3\":%.2f,"
    "\"vc4\":%.2f,"
    "\"vc5\":%.2f,"
    "\"vc6\":%.2f,"
    "\"vc7\":%.2f,"
    "\"vc8\":%.2f,"
    "\"vc9\":%.2f,"
    "\"vc10\":%.2f,"
    "\"vc11\":%.2f,"
    "\"vc12\":%.2f,"
    "\"vc13\":%.2f,"
    "\"vc14\":%.2f,"
    "\"cb1\":%i,"
    "\"cb2\":%i,"
    "\"cb3\":%i,"
    "\"cb4\":%i,"
    "\"cb5\":%i,"
    "\"cb6\":%i,"
    "\"cb7\":%i,"
    "\"cb8\":%i,"
    "\"cb9\":%i,"
    "\"cb10\":%i,"
    "\"cb11\":%i,"
    "\"cb12\":%i,"
    "\"cb13\":%i,"
    "\"cb14\":%i,"
    "\"cT1\":%.2f,"
    "\"cT2\":%.2f,"
    "\"cT3\":%.2f,"
    "\"cT4\":%.2f,"
    "\"Tbal1\":%.2f,"
    "\"Tbal2\":%.2f,"
    "\"bmsT\":%u"
    "}"
    , dataFrame->telemetry.unixTime
    , dataFrame->gps.fix
    , dataFrame->gps.lat
    , dataFrame->gps.lng
    , dataFrame->gps.speed
    , dataFrame->gps.last_msg
    , (int) std::round(dataFrame->bms.battery_voltage*dataFrame->bms.battery_current)
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
    , dataFrame->bms.cell_voltage[0]
    , dataFrame->bms.cell_voltage[1]
    , dataFrame->bms.cell_voltage[2]
    , dataFrame->bms.cell_voltage[3]
    , dataFrame->bms.cell_voltage[4]
    , dataFrame->bms.cell_voltage[5]
    , dataFrame->bms.cell_voltage[6]
    , dataFrame->bms.cell_voltage[7]
    , dataFrame->bms.cell_voltage[8]
    , dataFrame->bms.cell_voltage[9]
    , dataFrame->bms.cell_voltage[10]
    , dataFrame->bms.cell_voltage[11]
    , dataFrame->bms.cell_voltage[12]
    , dataFrame->bms.cell_voltage[13]
    , dataFrame->bms.is_Balancing[0]
    , dataFrame->bms.is_Balancing[1]
    , dataFrame->bms.is_Balancing[2]
    , dataFrame->bms.is_Balancing[3]
    , dataFrame->bms.is_Balancing[4]
    , dataFrame->bms.is_Balancing[5]
    , dataFrame->bms.is_Balancing[6]
    , dataFrame->bms.is_Balancing[7]
    , dataFrame->bms.is_Balancing[8]
    , dataFrame->bms.is_Balancing[9]
    , dataFrame->bms.is_Balancing[10]
    , dataFrame->bms.is_Balancing[11]
    , dataFrame->bms.is_Balancing[12]
    , dataFrame->bms.is_Balancing[13]
    , dataFrame->bms.cell_temp[0]
    , dataFrame->bms.cell_temp[1]
    , dataFrame->bms.cell_temp[2]
    , dataFrame->bms.cell_temp[3]
    , dataFrame->bms.balance_temp[0]
    , dataFrame->bms.balance_temp[1]
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

void mqttReconnect(PubSubClient* client, std::function<void ()> idleFn) {
  // Loop until we’re reconnected
  unsigned long lastIdlePerform = 0;
  unsigned long lastMqttReconnect = 0;
  unsigned long reconnectWaitTime = 5000;

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
      Serial.print("failed, rc = ");
      Serial.print(client->state());
      Serial.println(" try again in 5 seconds");
      Serial.println(WiFi.status());
    }
  }
}

void onMQTTReceive(char* topic, byte* payload, unsigned int length) {
  //do nothing if MQTT data is received, yet..
};
