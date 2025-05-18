#include "WIFI_setup.h"

void connect_wifi(espStatus* status);
void ask_wifi_credentials(espStatus* status);
void startServer(WifiCredentials *wc);

DNSServer dnsServer;
const byte DNS_PORT = 53;
ESP8266WebServer initServer(80);

JsonDocument jsonDocument;

String wifi_ssid = Wifi_SSID;
String wifi_password = Wifi_PASSWORD;
bool wifiCredentialsReceived = false;
unsigned long timeSinceReceived = 0;

void search_wifi(espStatus* status) {
    connect_wifi(status);
    while (WiFi.status() != WL_CONNECTED) {
        yield();
        // ask_wifi_credentials(status);
        // connect_wifi(status);
    }
}

// infinitely try to connect to WiFi given the current wifi credentials
void connect_wifi(espStatus* status, WifiCredentials *wc) {
  status->updateStatus(WIFI_LOGIN_TRY);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wc->ssid, wc->password);

  Serial.println("trying to connect...");
  //connectLoop
  while (WiFi.status() != WL_CONNECTED) {
    yield();
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

//Starts esp in accesspoint mode and hosts a webpage, 
//shuts down when credentials are given
void configure_WiFi(espStatus* status, WifiCredentials *wc) {
  wifiCredentialsReceived = false;
  status->updateStatus(WIFI_SEARCH);

  WiFi.mode(WIFI_AP);
  WiFi.softAP("Hi-Horizon Telemetry", "12345678");
  startServer(wc);
}

//Callback to handle the POST request of the API
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

//Host the webpage, shut down once valid input has been given
void startServer(WifiCredentials *wc) {
    Serial.println("Starting server...");
    initServer.on("/setWiFi", HTTP_POST, handlePost);
    initServer.begin();

    //server loop
    bool done = false;
    while (!done) {
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

