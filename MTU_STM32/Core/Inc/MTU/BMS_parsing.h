/*
 * BMS_parsing.h
 *
 *  Created on: Sep 21, 2024
 *      Author: senne
 */

#ifndef INC_MTU_BMS_PARSING_H_
#define INC_MTU_BMS_PARSING_H_

#include "stm32g4xx_hal.h"
#include "DataFrame.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "ctype.h"

void requestBmsData(UART_HandleTypeDef *huart, uint8_t id);
void parseBmsMessage(DataFrame* data, const uint8_t* buf, uint16_t size);

#endif /* INC_MTU_BMS_PARSING_H_ */
