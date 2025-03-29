/*
 * util.h
 *
 *  Created on: 8 Nov 2024
 *      Author: senne
 */

#ifndef INC_MTU_UTIL_H_
#define INC_MTU_UTIL_H_

#include "stm32g4xx_hal.h"
#include "time.h"
#include <DataFrame.h>

void getRTCUnixTime(RTC_HandleTypeDef* hrtc, DataFrame* data);

#endif /* INC_MTU_UTIL_H_ */
