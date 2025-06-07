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
#include <mcp2515.h>
#include <CANparser.h>


DataFrame dataFrame;

// Update these with values suitable for your network.

// A single, global CertStore which can be used by all connections.
// Needs to stay live the entire time any of the WiFiClientBearSSLs
// are present.
WifiCredentials wifiCredentials;
BearSSL::CertStore certStore;
WiFiClientSecure espClient;
PubSubClient * client;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200);

espStatus status;

unsigned long lastMsg = 0; //msg send timer
unsigned long stalenessTimer = 0; //staleness check timer
uint8_t staleness = 0;
uint8_t oldstaleness = 0;

#define SPI_BUFFER_SIZE 128
uint8_t spi_tx_buf[SPI_BUFFER_SIZE] = {};
uint8_t spi_rx_buf[SPI_BUFFER_SIZE] = {};

#define MSG_BUFFER_SIZE (3000)
char msg[MSG_BUFFER_SIZE];

uint32_t setDateTime();
void reconnect();
void onMQTTReceive(char* topic, byte* payload, unsigned int length);
void wifi_config_mode(espStatus* status, WifiCredentials *wifiCredentials);
void print_buf(uint8_t *buf, uint32_t len);
void requestDataframe();

void sendDataToBroker();
void sendEspInfoToCan();
void updateConnectionStatus();
void readAndParseCan();

//control
bool newData = false;

struct can_frame canRxMsg;
MCP2515 mcp2515(D8);

void setup() {
  //SERIAL INIT
  delay(500);
  Serial.begin(9600);
  delay(500);
  Serial.println();

  //CERT FILE LOADER INIT
  LittleFS.begin();

  //Try to connect to wifi
  dataFrame.telemetry.wifiSetupControl = 0;

  status.updateStatus(WIFI_LOGIN_TRY);

  WiFi.begin(Wifi_SSID, Wifi_PASSWORD);

  Serial.println("trying to connect...");
  //connectLoop
  while (WiFi.status() != WL_CONNECTED) { 
    yield();
    // switch(WiFi.status()) {
    //     case WL_CONNECTED:
    //         return;
    //     case WL_WRONG_PASSWORD:
    //         return;
    //     case WL_CONNECT_FAILED:
    //         return;
    //     default:
    //         break;
    // }
  }

  Serial.println("connected");

  timeClient.begin();
  timeClient.update();
  dataFrame.telemetry.NTPtime = timeClient.getEpochTime();

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
  client->setBufferSize(3000);

  const char* mqtt_server_prim = mqtt_server;
  client->setServer(mqtt_server_prim, 8883);
  client->setCallback(onMQTTReceive);
  digitalWrite(LED_BUILTIN, HIGH);

  //CAN Init
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Serial.println("setup finished");
}

void loop() {
  //MQTT client routine
  client->loop();
  updateConnectionStatus();
  readAndParseCan();

  if (newData && millis() - lastMsg > 1000L) {
    sendEspInfoToCan();
    sendDataToBroker();
  } 
  
  // TODO: implement the wifi setup API feature
  // if (dataFrame.telemetry.wifiSetupControl == 1) {
  //   Serial.println("starting WiFi config mode");
  //   wifi_config_mode(&status, &wifiCredentials);
  // }
}

void sendDataToBroker() {
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
  bool success = client->publish("data", msg);  
  digitalWrite(LED_BUILTIN, HIGH);

  lastMsg = millis();
  newData = false;

  //debug for troubleshooting purposes
  Serial.print("message sent: ");
  Serial.println(success);
}

void sendEspInfoToCan() {

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
    reconnect();
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

void readAndParseCan() {
  if (mcp2515.readMessage(&canRxMsg) == MCP2515::ERROR_OK) {
    CAN_parseMessage(canRxMsg.can_id, canRxMsg.data, &dataFrame);
    newData = true;

    Serial.print(canRxMsg.can_id, HEX); // print ID
    Serial.print(" "); 
    Serial.print(canRxMsg.can_dlc, HEX); // print DLC
    Serial.print(" ");
    
    for (int i = 0; i<canRxMsg.can_dlc; i++)  {  // print the data
      Serial.print(canRxMsg.data[i],HEX);
      Serial.print(" ");
    }

    Serial.println();      
  }
}

void wifi_config_mode(espStatus* status, WifiCredentials *wifiCredentials) {
  configure_WiFi(&dataFrame, status, wifiCredentials, requestDataframe);

  connect_wifi(&dataFrame, status, wifiCredentials, requestDataframe);

  //send wifiCredentials to MTU
  spi_tx_buf[0] = 3;
  createWiFiCredentialsFrame(wifiCredentials, spi_tx_buf + 1);
  dataFrame.telemetry.espStatus = CONNECTED;
  SPI.beginTransaction(SPISettings(16000000, MSBFIRST, SPI_MODE0));
  for (unsigned long i=0; i < sizeof(spi_tx_buf); i++) {
    spi_rx_buf[i] = SPI.transfer(spi_tx_buf[i]);
  }
  SPI.endTransaction();
}

// callback function, requests dataFrame to be sent by MTU and parses this, 
// sends ESP diagnostics as well
void requestDataframe() {
  Serial.println("Requesting data from MTU in the mean time...");
  //Send ESP status to MTU through SPI
  createESPInfoFrame(&dataFrame, spi_tx_buf + 1);
  dataFrame.telemetry.espStatus = CONNECTED;
  SPI.beginTransaction(SPISettings(16000000, MSBFIRST, SPI_MODE0));
  spi_tx_buf[0] = 1;
  for (unsigned long i=0; i < sizeof(spi_tx_buf); i++) {
    spi_rx_buf[i] = SPI.transfer(spi_tx_buf[i]);
  }
  SPI.endTransaction();
  parseFrame(&dataFrame, &wifiCredentials, spi_rx_buf, sizeof(spi_tx_buf));
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

void onMQTTReceive(char* topic, byte* payload, unsigned int length) {
  //do nothing if MQTT data is received, yet..
};

void print_buf(uint8_t *buf, uint32_t len) {
  for(unsigned int i=0; i < len; i++) {
    Serial.print(buf[i]);
    Serial.print(',');
  }
  Serial.println();
}

