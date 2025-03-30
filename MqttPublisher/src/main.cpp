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
#include <buffer.h>
#include <SpiControl.h>
#include <SpiConfig.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <SPI.h>
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
bool validNewMessage = true;
uint8_t oldstaleness = 0;

#define SPI_BUFFER_SIZE
uint8_t spi_tx_buf[SPI_BUFFER_SIZE] = {};
uint8_t spi_rx_buf[SPI_BUFFER_SIZE] = {};

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

  SPI.begin();

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
  
  if (millis() - lastMsg > 1000L) {
    status.updateConnectionStrength(WiFi.RSSI());
    dataFrame.telemetry.espStatus = status.getStatus();
    dataFrame.telemetry.internetConnection = status.getConnectionStrength();
    //sendAndReceivebuffer
    //TODO: put in method in SpiControl
    constructESPInfo(&dataFrame, spi_tx_buf);
    for(int i=0; i < sizeof(spi_tx_buf); i++) {
	    spi_rx_buf[i] = SPI.transfer(spi_tx_buf[i]);
      Serial.print(spi_rx_buf[i]);
      Serial.print(',');
    }
    Serial.println();

    dataFrameFromBuf(&dataFrame, spi_rx_buf);
    dataFrameFromBuf(&dataFrame, spi_rx_buf+19);
    dataFrameFromBuf(&dataFrame, spi_rx_buf+31);
    dataFrameFromBuf(&dataFrame, spi_rx_buf+47);
    dataFrameFromBuf(&dataFrame, spi_rx_buf+61);

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
      "\"vm\":%f"
      "\"mc\":%f,"
      "\"Pu\":%f,"
      "\"escT\":%u,"
      "\"bv\":%f,"
      "\"bc\":%f,"
      "\"bMinv\":%f,"
      "\"bMaxv\":%f,"
      "\"bmsT\":%u,"
      "}"
    , dataFrame.telemetry.unixTime
    , dataFrame.gps.fix
    , dataFrame.gps.lat
    , dataFrame.gps.lng
    , dataFrame.gps.speed
    , dataFrame.gps.last_msg
    , dataFrame.mppt.power
    , dataFrame.mppt.last_msg
    , dataFrame.motor.warning
    , dataFrame.motor.failures
    , dataFrame.motor.battery_voltage
    , dataFrame.motor.battery_current
    , dataFrame.motor.battery_current*dataFrame.motor.battery_voltage
    , dataFrame.motor.last_msg
    , dataFrame.bms.battery_voltage
    , dataFrame.bms.battery_current
    , dataFrame.bms.min_cel_voltage
    , dataFrame.bms.max_cel_voltage
    , dataFrame.bms.last_msg
  );

    digitalWrite(LED_BUILTIN, HIGH);
    bool success = client->publish("data", msg);
    lastMsg = millis();
    
    //for troubleshooting purposes
    Serial.print("message sent: ");
    Serial.println(success);
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
