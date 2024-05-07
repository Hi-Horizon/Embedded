/*
 * troubleShoot.h
 *
 *  Created on: 15 Mar 2024
 *      Author: senne
 */

#ifndef INC_MTU_TROUBLESHOOT_H_
#define INC_MTU_TROUBLESHOOT_H_

#include "DataFrame.h"

float a = 60;

void fillRandomData(DataFrame* data) {
	data->mppt.power = rand();
	data->motor.battery_voltage = ((float)rand()/(float)(RAND_MAX)) * a;
	data->motor.battery_current = ((float)rand()/(float)(RAND_MAX)) * a;
	data->gps.speed = ((float)rand()/(float)(RAND_MAX)) * a;
}

#endif /* INC_MTU_TROUBLESHOOT_H_ */
