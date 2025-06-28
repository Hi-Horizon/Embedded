/*
 * CAN_API.h
 *
 *  Created on: 8 Nov 2024
 *      Author: senne
 */

#ifndef INC_MTU_CAN_API_H_
#define INC_MTU_CAN_API_H_

#include "stm32g4xx_hal.h"
#include <buffer/buffer.h>
#include <stdint.h>
#include <DataFrame.h>


void setCanTxHeaders();
void sendToCan(FDCAN_HandleTypeDef* hfdcan1, DataFrame* data);
void sendWiFiCredentialsBuf(FDCAN_HandleTypeDef* hfdcan1, uint8_t* buf, uint8_t length);
#endif /* INC_MTU_CAN_API_H_ */
