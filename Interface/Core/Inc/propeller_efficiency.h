/*
 * propeller_efficiency.h
 *
 *  Created on: 6 May 2026
 *      Author: senne
 */

#ifndef INC_PROPELLER_EFFICIENCY_H_
#define INC_PROPELLER_EFFICIENCY_H_

#include "math.h"
#include <stdio.h>

#define PROP_BEUKER 0
#define PROP_WERKPAARD 1

#define WERKPAARD_JS_KQ_LEN 29
#define BEUKER_JS_KQ_LEN 27

float calc_propeller_efficiency(float P_motor, float rpm, float v_s, uint8_t propeller);

#endif /* INC_PROPELLER_EFFICIENCY_H_ */
