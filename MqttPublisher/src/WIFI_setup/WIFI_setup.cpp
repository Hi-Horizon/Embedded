#include "WIFI_setup.h"

void startServer(DataFrame *data, WifiCredentials *wc, std::function<void ()> idleFn);

DNSServer dnsServer;
const byte DNS_PORT = 53;
ESP8266WebServer initServer(80);

JsonDocument jsonDocument;

String wifi_ssid = Wifi_SSID;
String wifi_password = Wifi_PASSWORD;
bool wifiCredentialsReceived = false;
unsigned long timeSinceReceived = 0;


void initWiFi(DataFrame* dataFrame, espStatus* status) {
  //Try to connect to wifi
  dataFrame->telemetry.wifiSetupControl = 0;

  status->updateStatus(WIFI_LOGIN_TRY);

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

//**
//* Connects to wifi given the current wifiConfig
//**
//* DataFrame* data - dataFrame containing all current data
//* espStatus* status - status struct for ESP
//* WifiCredentials* wc - WiFiConfig struct giving the ssid en password
//* std::function<void ()> idleFn - function to perform during the connection loop
void connect_wifi(DataFrame *data, espStatus* status, WifiCredentials *wc, std::function<void ()> idleFn) {
  status->updateStatus(WIFI_LOGIN_TRY);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wc->ssid, wc->password);

  Serial.println("trying to connect...");

  unsigned long lastIdlePerform = 0;
  uint8_t prevWifiSetupControl = data->telemetry.wifiSetupControl;
  //connectLoop
  while (WiFi.status() != WL_CONNECTED) {
    yield();
    if (millis() - lastIdlePerform > 1000) {
      idleFn();   
      if (data->telemetry.wifiSetupControl == 1 && prevWifiSetupControl == 0) {
        Serial.println("stopping wifi search");
        return; //stop searching if WiFiConfigmode is enabled
      }
      prevWifiSetupControl = data->telemetry.wifiSetupControl;
      lastIdlePerform = millis();
    }
    
    switch(WiFi.status()) {
        case WL_CONNECTED:
            return;
        case WL_WRONG_PASSWORD:
            return;
        case WL_CONNECT_FAILED:
            return;
        default:
            break;
    }
  }
}

//**
//* Starts esp in accesspoint mode and hosts a webpage, 
//* shuts down when credentials are given
//**
//* DataFrame* data - dataFrame containing all current data
//* espStatus* status - status struct for ESP
//* WifiCredentials* wc - WiFiConfig struct giving the ssid en password
//* std::function<void ()> idleFn - function to perform during the connection loop
void configure_WiFi(DataFrame *data, espStatus* status, WifiCredentials *wc, std::function<void ()> idleFn) {
  wifiCredentialsReceived = false;
  status->updateStatus(WIFI_SEARCH);

  WiFi.mode(WIFI_AP);
  WiFi.softAP("Hi-Horizon Telemetry", "12345678");
  startServer(data, wc, idleFn);
}

//**
//* Callback to handle the POST request of the API
//**
void handlePost(){
  if (initServer.hasArg("plain") == false) {
    initServer.send(400, "application/json", "error: data not sent using JSON format");
  }
  String body = initServer.arg("plain");
  deserializeJson(jsonDocument, body);

  wifi_ssid     = (String) jsonDocument["ssid"];
  wifi_password = (String) jsonDocument["password"];

  initServer.send(200, "application/json", "connecting..");
  wifiCredentialsReceived = true;
  timeSinceReceived = millis();
}

//**
//* Host the webpage, 
//* shut down once valid input has been given
//**
//* DataFrame* data - dataFrame containing all current data
//* espStatus* status - status struct for ESP
//* WifiCredentials* wc - WiFiConfig struct giving the ssid en password
//* std::function<void ()> idleFn - function to perform during the connection loop
void startServer(DataFrame *data, WifiCredentials *wc, std::function<void ()> idleFn) {
    Serial.println("Starting server...");
    initServer.on("/setWiFi", HTTP_POST, handlePost);
    initServer.begin();

    unsigned long lastIdlePerform = 0;

    //server loop
    bool done = false;
    while (!done) {
        if (millis() - lastIdlePerform > 1000) {
          idleFn();
          //if control mode is turned of, shut down server and return immediately
          if (data->telemetry.wifiSetupControl == 0) {
            initServer.stop();
            return;
          }
          lastIdlePerform = millis();
        }

        initServer.handleClient();
        //wait for a second to close the server, to handle responses
        if (wifiCredentialsReceived && (millis() - timeSinceReceived > 1000)) {
          done = true;
        }
    }

    //put values into the WifiCredentials struct
    wc->ssidLength = wifi_ssid.length();
    wc->passwordLength = wifi_password.length();

    wifi_ssid.toCharArray(wc->ssid, wifi_ssid.length() + 1);
    wifi_password.toCharArray(wc->password, wifi_password.length() + 1);

    Serial.println("got credentials! shutting down server...");
    initServer.stop();
}