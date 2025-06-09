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
#include <MQTT_API/mqtt_api.h>

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

#define SPI_BUFFER_SIZE 128
uint8_t spi_tx_buf[SPI_BUFFER_SIZE] = {};
uint8_t spi_rx_buf[SPI_BUFFER_SIZE] = {};

uint32_t setDateTime();
void wifi_config_mode(espStatus* status, WifiCredentials *wifiCredentials);
void requestDataframe();

void initCan();
void initWiFi();
void initTime();
void verifyAndInitCerts();

void sendEspInfoToCan();
void updateConnectionStatus();
void readAndParseCan();

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

  initCan();
  initWiFi();
  initTime();
  verifyAndInitCerts();
  client = initMqtt(client, bear);

  Serial.println("setup finished");
}

void loop() {
  //MQTT client routine
  client->loop();
  updateConnectionStatus();
  readAndParseCan();

  if (newData && (millis() - lastMsg > 1000L)) {
    sendEspInfoToCan();
    sendDataToBroker(client, &dataFrame, &newData, &lastMsg);
  }
  
  // TODO: implement the wifi setup API feature
  // if (dataFrame.telemetry.wifiSetupControl == 1) {
  //   Serial.println("starting WiFi config mode");
  //   wifi_config_mode(&status, &wifiCredentials);
  // }
}

void sendEspInfoToCan() {
  canEspTxMsg.data[0] = dataFrame.telemetry.espStatus;
  canEspTxMsg.data[1] = dataFrame.telemetry.internetConnection;
  canEspTxMsg.data[2] = dataFrame.telemetry.wifiSetupControl;

  mcp2515.sendMessage(&canEspTxMsg);
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

void initCan() {
  canEspTxMsg.can_id  = 0x751;
  canEspTxMsg.can_dlc = 3;

  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
}

void initWiFi() {
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
}

void initTime() {
  timeClient.begin();
  timeClient.update();
  dataFrame.telemetry.NTPtime = timeClient.getEpochTime();

  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output

  setDateTime();
}

void verifyAndInitCerts() {
  //CERT FILE LOADER INIT
  LittleFS.begin();
  //CHECK CERTS
  status.updateStatus(TESTING_CERTS);
  int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
  Serial.printf("Number of CA certs read: %d\n", numCerts);
  if (numCerts == 0) {
    Serial.printf("No certs found. Did you run certs-from-mozilla.py and upload the LittleFS directory before running?\n");
    return; // Can't connect to anything w/o certs!
  }

  // USE CERTS
  // Integrate the cert store with this connection
  bear->setCertStore(&certStore);
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
  Serial.println("time synced");

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  return now;
}