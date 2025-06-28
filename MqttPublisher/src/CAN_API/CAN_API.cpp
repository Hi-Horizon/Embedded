#include "CAN_API.h"

void initCan(MCP2515* mcp2515, can_frame* canEspTxMsg) {
  Serial.println("Initializing CAN");

  canEspTxMsg->can_id  = 0x751;
  canEspTxMsg->can_dlc = 3;

  mcp2515->reset();
  mcp2515->setBitrate(CAN_125KBPS, MCP_8MHZ);
  mcp2515->setNormalMode();
}

void sendEspInfoToCan(MCP2515* mcp2515, can_frame* canEspTxMsg, DataFrame* dataFrame) {
  canEspTxMsg->data[0] = dataFrame->esp.status;
  canEspTxMsg->data[1] = dataFrame->esp.internetConnection;
  canEspTxMsg->data[2] = dataFrame->esp.wifiSetupControl;

  mcp2515->sendMessage(canEspTxMsg);
}

//TODO put this in a seperate WiFiCredentials .c & .h
void parseWifiCredentialsFromBuf(WifiCredentials *wifiCredentials, uint8_t* buf) {
  //get ssid
  for (int i = 0; i < 128; i++) {
    Serial.print(buf[i]);
    if (buf[i] == '\n') break; //ssid finished
    wifiCredentials->ssid[i] = buf[i];
    wifiCredentials->ssidLength++;
  }
  buf = buf + wifiCredentials->ssidLength + 1;

  //get password
  for (int i = 0; i < 128; i++) {
    Serial.print(buf[i]);
    if (buf[i] == (char) 0x0) break; //password finished
    wifiCredentials->password[i] = buf[i];
    wifiCredentials->passwordLength++;
  }
  Serial.println();
}

uint8_t listenForWiFiCredentialsCan(MCP2515* mcp2515, can_frame* canRxMsg, WifiCredentials *wifiCredentials) {
  uint8_t buf[256];
  uint8_t seq = 0;

  bool transferComplete=false;
  while (!transferComplete) {
    yield();
    if (mcp2515->readMessage(canRxMsg) == MCP2515::ERROR_OK) {
      Serial.println(canRxMsg->can_id);
      Serial.println(canRxMsg->data[0]);
      if (canRxMsg->can_id == 0x753) { // canBus response id
        // if msg is completely empty, transfer is complete
        transferComplete=true;
        for (int i = 0; i < 8; i++ ) { 
          if (canRxMsg->data[i] == 0) continue;
          else transferComplete = false;
        }
        if (transferComplete) break;

        if (seq != canRxMsg->data[0]) return 0; // out of sequence, this should retrigger a new request
        memcpy(buf, canRxMsg->data + 1, 7); // cpy characters from data into buf
        seq++;
      }
      //TODO: add WiFi config mode request check 
    }
  }

  parseWifiCredentialsFromBuf(wifiCredentials, buf);
  return 1;
}

void readAndParseCan(MCP2515* mcp2515, can_frame* canRxMsg, DataFrame* dataFrame, bool* newDataFlag) {
  if (mcp2515->readMessage(canRxMsg) == MCP2515::ERROR_OK) {
    CAN_parseMessage(canRxMsg->can_id, canRxMsg->data, dataFrame);
    *newDataFlag = true;
          
    Serial.print(canRxMsg->can_id, HEX); // print ID
    Serial.print(" "); 
    Serial.print(canRxMsg->can_dlc, HEX); // print DLC
    Serial.print(" ");
    
    for (int i = 0; i<canRxMsg->can_dlc; i++)  {  // print the data
      Serial.print(canRxMsg->data[i],HEX);
      Serial.print(" ");
    }

    Serial.println();      
  }
}