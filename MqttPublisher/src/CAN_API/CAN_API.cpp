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
  canEspTxMsg->data[0] = dataFrame->telemetry.espStatus;
  canEspTxMsg->data[1] = dataFrame->telemetry.internetConnection;
  canEspTxMsg->data[2] = dataFrame->telemetry.wifiSetupControl;

  mcp2515->sendMessage(canEspTxMsg);
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