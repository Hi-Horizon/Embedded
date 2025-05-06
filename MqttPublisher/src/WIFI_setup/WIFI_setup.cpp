#include "WIFI_setup.h"

void connect_wifi(espStatus* status);
void ask_wifi_credentials(espStatus* status);
void startServer();

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

void connect_wifi(espStatus* status) {
  status->updateStatus(WIFI_LOGIN_TRY);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  Serial.println("trying to connect...");
  unsigned long timoutTimer = millis();
  while (millis() - timoutTimer < 30000uL ) { //timeout after 5 seconds
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

//starts esp in accesspoint mode and hosts a webpage
void configure_WiFi(espStatus* status) {
  wifiCredentialsReceived = false;
  status->updateStatus(WIFI_SEARCH);

  WiFi.mode(WIFI_AP);
  WiFi.softAP("Hi-Horizon Telemetry", "12345678");
  startServer();
}

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

void startServer() {
    Serial.println("Starting server...");
    initServer.on("/setWiFi", HTTP_POST, handlePost);
    initServer.begin();

    bool done = false;
    while (!done) {
        initServer.handleClient();
        //wait for a second to close the server, to handle responses
        if (wifiCredentialsReceived && (millis() - timeSinceReceived > 1000)) {
          done = true;
        }
    }

    Serial.println("got credentials! shutting down server...");
    initServer.stop();
}

