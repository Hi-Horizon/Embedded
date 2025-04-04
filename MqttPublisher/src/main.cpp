#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <TZ.h>
#include <time.h>
#include <FS.h>
#include <LittleFS.h>
#include <CertStoreBearSSL.h>
#include <SoftwareSerial.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "wifiConfig.h"
#include "WIFI_setup/WIFI_setup.h"
#include <espStatus/espStatus.h>

#include <DataFrame.h>
#include <SPISlave.h>
#include <buffer.h>
#include <SpiControl.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

DataFrame dataFrame;

// Update these with values suitable for your network.

// A single, global CertStore which can be used by all connections.
// Needs to stay live the entire time any of the WiFiClientBearSSLs
// are present.
BearSSL::CertStore certStore;
WiFiClientSecure espClient;
PubSubClient * client;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200);

espStatus status;

unsigned long lastMsg = 0; //msg send timer
unsigned long stalenessTimer = 0; //staleness check timer
uint32_t timeSinceNTP = 0;
bool timeSyncDone = false;
uint8_t staleness = 0;
bool validNewMessage = false;
uint8_t oldstaleness = 0;

// String wifi_ssid = "";
// String wifi_password = "";
// bool wifiCredentialsReceived = false;

#define MSG_BUFFER_SIZE (3000)
char msg[MSG_BUFFER_SIZE];

// void connect_wifi();
uint32_t setDateTime();
void reconnect();
void onMQTTReceive(char* topic, byte* payload, unsigned int length);
// void ask_wifi_credentials();

void setup() {
  //SERIAL INIT
  delay(500);
  Serial.begin(9600);
  delay(500);
  Serial.println();

  //SPI INIT
  SPISlave.onData([](uint8_t *data, size_t len) {
    validNewMessage = receiveSpiData(&dataFrame, data, len);
  });

  SPISlave.begin();

  //CERT FILE LOADER INIT
  LittleFS.begin();
  
  //SEARCHING WIFI
  search_wifi(&status);

  timeClient.begin();
  timeClient.update();
  dataFrame.telemetry.NTPtime = timeClient.getEpochTime();
  timeSinceNTP = millis();
  timeSyncDone = true;

  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output

  setDateTime();

  //CHECK CERTS
  status.updateStatus(TESTING_CERTS);
  int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
  Serial.printf("Number of CA certs read: %d\n", numCerts);
  if (numCerts == 0) {
    Serial.printf("No certs found. Did you run certs-from-mozilla.py and upload the LittleFS directory before running?\n");
    return; // Can't connect to anything w/o certs!
  }

  //USE CERTS
  BearSSL::WiFiClientSecure *bear = new BearSSL::WiFiClientSecure();
  // Integrate the cert store with this connection
  bear->setCertStore(&certStore);

  //CONNECT MQTT
  client = new PubSubClient(*bear);

  const char* mqtt_server_prim = mqtt_server;
  client->setServer(mqtt_server_prim, 8883);
  client->setCallback(onMQTTReceive);
}

void loop() {
  //first check if internet is still connected
  if (WiFi.status() != WL_CONNECTED) {              
    status.updateStatus(WiFi.status());
  }
  //then check if esp is still connected with Broker
  else if (!client->connected()) { 
    status.updateStatus(BROKER_CONNECTION_FAILED);  
    reconnect();
  }
  client->loop();

  //TODO: should be in a function 
  if (millis() - stalenessTimer > 3000L) {
    if (oldstaleness == staleness) {
      status.updateStatus(HARDWARE_FAULT);
    }
    else if (status.getStatus() == HARDWARE_FAULT) {
      status.updateStatus(CONNECTED);
    }
    oldstaleness = staleness;
    stalenessTimer = millis();

    Serial.print("status is: ");
    Serial.println(status.getStatus());
  }
  
  if (millis() - lastMsg > 1000L && validNewMessage) {
    digitalWrite(LED_BUILTIN, LOW);
    status.updateConnectionStrength(WiFi.RSSI());
    snprintf (msg, MSG_BUFFER_SIZE, 
      "{"
      "\"mtuT\":%u,"
      "\"gpsT\":%u,"
      "\"mpptT\":%u,"
      "\"lat\":%f,"
      "\"lng\":%f,"
      "\"v\":%f,"
      "\"Pz\":%i,"
      "\"mc\":%f,"
      "\"Pu\":%f,"
      "\"vm\":%f"
      "}"
    , dataFrame.telemetry.unixTime
    , dataFrame.gps.last_msg
    , dataFrame.mppt.last_msg
    , dataFrame.gps.lat
    , dataFrame.gps.lng
    , dataFrame.gps.speed
    , dataFrame.mppt.power
    , dataFrame.motor.battery_current
    , dataFrame.motor.battery_current*dataFrame.motor.battery_voltage
    , dataFrame.motor.battery_voltage);

    digitalWrite(LED_BUILTIN, HIGH);
    client->publish("data", msg);
    lastMsg = millis();
    validNewMessage = false;
    
    //for troubleshooting purposes
    // Serial.println("");
    // Serial.print(msg);
    // Serial.println("");
  }
}

uint32_t setDateTime() {
  // You can use your own timezone, but the exact time is not used at all.
  // Only the date is needed for validating the certificates.
  status.updateStatus(SYNCING_NTP);
  configTime(TZ_Europe_Amsterdam, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(100);
    Serial.print(".");
    now = time(nullptr);  
  }
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  return now;
}

void onMQTTReceive(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if the first character is present
  if ((char)payload[0] != NULL) {
    digitalWrite(LED_BUILTIN, LOW); // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
  } else {
    digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
  }
}

void reconnect() {
  // Loop until we’re reconnected
  status.updateStatus(CONNECTING_BROKER);

  while (!client->connected()) {
    Serial.print("Attempting MQTT connection…");
    String clientId = "ESP8266Client - MyClient";
    // Attempt to connect
    // Insert your password
    if (client->connect(clientId.c_str(), MQTT_USER, MQTT_PWD)) {
      Serial.println("connected");
      // Once connected, publish an announcement…
      client->publish("testTopic", "hello world");
      // … and resubscribe
      client->subscribe("testTopic");
    } 
    else {
      status.updateStatus(BROKER_CONNECTION_FAILED);
      Serial.print("failed, rc = ");
      Serial.print(client->state());
      Serial.println(" try again in 5 seconds");
      Serial.println(WiFi.status());
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  status.updateStatus(CONNECTED);
}
