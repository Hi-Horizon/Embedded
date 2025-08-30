#include "CAN_API.h"

void initCan(MCP2515* mcp2515, can_frame* canEspTxMsg, can_frame* canWifiCredentialsTxMsg) {
  Serial.println("Initializing CAN");


  pinMode(D8, OUTPUT);
  digitalWrite(D8, HIGH);

  canEspTxMsg->can_id  = 0x751;
  canEspTxMsg->can_dlc = CAN_MAX_DLEN;

  canWifiCredentialsTxMsg->can_id = 0x753;
  canWifiCredentialsTxMsg->can_dlc = CAN_MAX_DLEN;

  mcp2515->reset();
  mcp2515->setBitrate(CAN_125KBPS, MCP_8MHZ);
  mcp2515->setNormalMode();
}

void sendEspInfoToCan(MCP2515* mcp2515, can_frame* canEspTxMsg, DataFrame* dataFrame) {
  int32_t ind = 0;
  buffer_append_uint8(canEspTxMsg->data, dataFrame->esp.status, &ind);
  buffer_append_uint8(canEspTxMsg->data, dataFrame->esp.mqttStatus, &ind);
  buffer_append_uint8(canEspTxMsg->data, dataFrame->esp.internetConnection, &ind);
  buffer_append_uint32(canEspTxMsg->data, dataFrame->esp.NTPtime, &ind);

  mcp2515->sendMessage(canEspTxMsg);
}

//TODO put this in a seperate WiFiCredentials .c & .h
void parseWifiCredentialsFromBuf(WifiCredentials *wifiCredentials, uint8_t* buf) {
  //get ssid
  for (int i = 0; i < 128; i++) {
    if (buf[i] == '\n') break; //ssid finished
    wifiCredentials->ssid[i] = buf[i];
    wifiCredentials->ssidLength++;
  }
  buf = buf + wifiCredentials->ssidLength + 1;

  //get password
  for (int i = 0; i < 128; i++) {
    if (buf[i] == (char) 0x0 || buf[i] == '\n') break; //password finished
    wifiCredentials->password[i] = buf[i];
    wifiCredentials->passwordLength++;
  }
}

uint8_t listenForWiFiCredentialsCan(MCP2515* mcp2515, can_frame* canRxMsg, WifiCredentials *wifiCredentials, unsigned long timeout) {
  uint8_t buf[256];
  uint8_t seq = 0;

  bool transferComplete=false;
  unsigned long startTime = millis();
  while (!transferComplete && !(millis() - startTime >= timeout)) {
    yield();
    if (mcp2515->readMessage(canRxMsg) == MCP2515::ERROR_OK) {
      // debugging purposes
      // Serial.println(canRxMsg->can_id, HEX);
      // Serial.println(canRxMsg->data[0]);
      if (canRxMsg->can_id == 0x753) { // canBus response id
        // if msg is completely empty, transfer is complete
        transferComplete=true;
        for (int i = 0; i < 8; i++ ) { 
          if (canRxMsg->data[i] == 0) continue;
          else transferComplete = false;
        }
        if (transferComplete) break;

        if (seq != canRxMsg->data[0]) return 0; // out of sequence, this should retrigger a new request
        memcpy(buf + 7*canRxMsg->data[0], canRxMsg->data + 1, 7); // cpy characters from data into buf
        seq++;
      }
      //TODO: add WiFi config mode request check 
    }
  }

  parseWifiCredentialsFromBuf(wifiCredentials, buf);
  return transferComplete;
}

void readAndParseCan(MCP2515* mcp2515, can_frame* canRxMsg, DataFrame* dataFrame, bool* newDataFlag) {
  if (mcp2515->readMessage(canRxMsg) == MCP2515::ERROR_OK) {
    if (canRxMsg->can_id == 0x754) {
      dataFrame->esp.wifiSetupControl = canRxMsg->data[0];
      return;
    }
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

void canListenForWifiConfigToggle(MCP2515* mcp2515, can_frame* canRxMsg, DataFrame* dataFrame) {
  if (mcp2515->readMessage(canRxMsg) == MCP2515::ERROR_OK) {
    if (canRxMsg->can_id == 0x754) {
      dataFrame->esp.wifiSetupControl = canRxMsg->data[0];
      return;
    }
  }
}