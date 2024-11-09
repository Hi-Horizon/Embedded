#include "WIFI_setup.h"

void connect_wifi(espStatus* status);
void ask_wifi_credentials(espStatus* status);
void startServer();

DNSServer dnsServer;
const byte DNS_PORT = 53;
ESP8266WebServer initServer(80);

String wifi_ssid = Wifi_SSID;
String wifi_password = Wifi_PASSWORD;
bool wifiCredentialsReceived = false;

void search_wifi(espStatus* status) {
    connect_wifi(status);
    while (WiFi.status() != WL_CONNECTED) {
        yield();
        ask_wifi_credentials(status);
        connect_wifi(status);
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
void ask_wifi_credentials(espStatus* status) {
  wifiCredentialsReceived = false;
  status->updateStatus(WIFI_SEARCH);

  WiFi.mode(WIFI_AP);
  WiFi.softAP("Hi-Horizon Telemetry", "12345678");
  startServer();
}

void submitPage(){
  Serial.println("Sending page...");
  initServer.send(200, "text/html", getIndexPage());
}

void donePage(){
  wifi_ssid = initServer.arg("ssid");
  wifi_password = initServer.arg("password");
  Serial.println(wifi_ssid);
  Serial.println(wifi_password);
  initServer.send(200,"text/html", getClosedPage());
  wifiCredentialsReceived = true;
}

void startServer() {
    Serial.println("Starting server...");
    initServer.on("/", submitPage);
    initServer.on("/done", donePage);

    initServer.begin();

    while (!wifiCredentialsReceived) {
        initServer.handleClient();
    }
    Serial.println("got credentials! shutting down server...");
    initServer.stop();
}

