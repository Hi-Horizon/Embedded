/*
 * GPS_parsing.h
 *
 *  Created on: 22 Jul 2024
 *      Author: senne
 */

#ifndef INC_MTU_GPS_PARSING_H_
#define INC_MTU_GPS_PARSING_H_


#include "DataFrame.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "ctype.h"

#define GPS_PARSE_NONE 0
#define GPS_PARSE_ID 0
#define GPS_PARSE_value 1
#define GPS_PARSE_checksum 2

#define GPS_NONE -1
#define GPS_GPGGA 0
#define GPS_GPVTG 1

void parseGPS(DataFrame* data, uint8_t* buf, uint16_t size);

#endif /* INC_MTU_GPS_PARSING_H_ */
