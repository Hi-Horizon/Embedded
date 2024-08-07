/*
 * sensorParser.h
 *
 *  Created on: Sep 28, 2023
 *      Author: senne
 */

#ifndef INC_SENSORPARSER_H_
#define INC_SENSORPARSER_H_

#include "DataFrame.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "ctype.h"

#define MPPT_P 0
#define MPPT_V 1
#define MPPT_ERR 2
#define MPPT_CS 3

#define MPPT_VALUEARRAY_SIZE 40
#define MPPT_TAG_LEN 9
#define MPPT_VALUE_LEN 33

void parseMPPT(DataFrame* data, uint8_t* buf, uint16_t size);

void parseMPPTHex(DataFrame* data, uint8_t* buf, uint16_t size);
void handleMPPTHex(DataFrame* data, char* msg, uint16_t size);

#endif /* INC_SENSORPARSER_H_ */
