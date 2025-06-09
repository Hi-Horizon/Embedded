#ifndef __CAN_API__H
#define __CAN_API__H

#include "Arduino.h"
#include "mcp2515.h"
#include "CANparser.h"
#include "DataFrame.h"

void initCan(MCP2515* mcp2515, can_frame* canEspTxMsg);
void sendEspInfoToCan(MCP2515* mcp2515, can_frame* canEspTxMsg, DataFrame* dataFrame);
void readAndParseCan(MCP2515* mcp2515, can_frame* canRxMsg, DataFrame* dataFrame, bool* newDataFlag);

#endif