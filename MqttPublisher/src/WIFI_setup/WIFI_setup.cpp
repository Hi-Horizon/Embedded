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
  dataFrame->esp.wifiSetupControl = 0;

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

void getWiFiCredentialsFromCan(MCP2515 *mcp2515, can_frame *rxFrame, WifiCredentials *wifiCredentials) {
  struct can_frame credentialsRequestMsg;
  credentialsRequestMsg.can_id  = 0x752;
  credentialsRequestMsg.can_dlc = 0;
  //send request to network for the credentials
  Serial.println("sending request for wifiCredentials");
  mcp2515->sendMessage(&credentialsRequestMsg);
  Serial.println("Sent, listening for response");
  Serial.println(listenForWiFiCredentialsCan(mcp2515, rxFrame, wifiCredentials));
  
  Serial.println(wifiCredentials->ssid);
  Serial.println(wifiCredentials->password);
}

void sendWiFICredentialsOverCan(MCP2515 *mcp2515, can_frame *txFrame, WifiCredentials *wifiCredentials) {
  uint8_t buf[258];
  uint32_t ind = 0;

  //TODO: put in wificredentials library with name Like convertToBuf;
  memcpy(buf, wifiCredentials->ssid, wifiCredentials->ssidLength);
  ind = ind + wifiCredentials->ssidLength;
  buf[ind] = '\n';
  ind = ind + 1;
  memcpy(buf + ind, wifiCredentials->password, wifiCredentials->passwordLength);
  ind = ind + wifiCredentials->passwordLength;

  uint8_t* bufPtr = buf;
  uint32_t length = ind;
	uint8_t seq 	= 0;
	ind	= 0;

	while (ind <= length - 7) {
		txFrame->data[0] = seq;
		memcpy(txFrame->data +1, bufPtr, 7);
		delay(50);
		mcp2515->sendMessage(txFrame);
		bufPtr = bufPtr + 7;
		ind += 7;
		seq++;
	}
	uint8_t remainder = length % 7;
	if (remainder != 0) {
		memset(txFrame->data, 0, 8);
		txFrame->data[0] = seq;
		memcpy(txFrame->data+1, bufPtr, remainder);
		delay(50);
		mcp2515->sendMessage(txFrame);
	}
	memset(txFrame->data, 0, 8);
	delay(50);
	mcp2515->sendMessage(txFrame);
}

//**
//* Connects to wifi given the current wifiConfig
//**
//* DataFrame* data - dataFrame containing all current data
//* espStatus* status - status struct for ESP
//* WifiCredentials* wc - WiFiConfig struct giving the ssid en password
//* std::function<void ()> idleFn - function to perform during the connection loop
bool connect_wifi(DataFrame *data, espStatus* status, WifiCredentials *wc, std::function<void ()> idleFn) {
  status->updateStatus(WIFI_LOGIN_TRY);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wc->ssid, wc->password);

  Serial.println("trying to connect...");

  unsigned long lastIdlePerform = 0;

  //variable for when connection is inside wifi_config routine, 
  uint8_t prevWifiSetupControl = data->esp.wifiSetupControl;
  //connectLoop
  while (WiFi.status() != WL_CONNECTED) {
    yield();
    if (millis() - lastIdlePerform > 1000) {
      idleFn();   
      if (data->esp.wifiSetupControl == 1 && prevWifiSetupControl == 0) {
        return false; //stop searching if WiFiConfigmode is enabled
      }
      if (data->esp.wifiSetupControl == 0 && prevWifiSetupControl == 1) {
        return false; //cancel connection attempt with new WiFi
      }
      prevWifiSetupControl = data->esp.wifiSetupControl;
      lastIdlePerform = millis();
    }
    
    switch(WiFi.status()) {
        case WL_CONNECTED:
            return true;
        case WL_WRONG_PASSWORD:
            return false;
        case WL_CONNECT_FAILED:
            return false;
        default:
            break;
    }
  }
  return true;
}

//**
//* Starts esp in accesspoint mode and hosts a webpage, 
//* shuts down when credentials are given
//**
//* DataFrame* data - dataFrame containing all current data
//* espStatus* status - status struct for ESP
//* WifiCredentials* wc - WiFiConfig struct giving the ssid en password
//* std::function<void ()> idleFn - function to perform during the connection loop
void configure_WiFi(DataFrame *data, espStatus* status, WifiCredentials *wc, std::function<void()> idleFn) {
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
          //if control mode is turned off, shut down server and return immediately
          if (data->esp.wifiSetupControl == 0) {
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