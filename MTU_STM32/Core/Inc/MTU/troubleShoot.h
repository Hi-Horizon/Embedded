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
//	data->gps.fix = rand();
//	data->gps.lat = ((float)rand()/(float)(RAND_MAX)) * a;
//	data->gps.lng = ((float)rand()/(float)(RAND_MAX)) * a;
//	data->gps.speed = ((float)rand()/(float)(RAND_MAX)) * a;
//	data->mppt.power = rand();
//	data->motor.warning = rand();
//	data->motor.failures = rand();
//	data->motor.battery_voltage = ((float)rand()/(float)(RAND_MAX)) * a;
//	data->motor.battery_current = ((float)rand()/(float)(RAND_MAX)) * a;
//	data->bms.battery_voltage = ((float)rand()/(float)(RAND_MAX)) * a;
//	data->bms.battery_current = ((float)rand()/(float)(RAND_MAX)) * a;
//	data->bms.min_cel_voltage = ((float)rand()/(float)(RAND_MAX)) * a;
//	data->bms.max_cel_voltage = ((float)rand()/(float)(RAND_MAX)) * a;

	data->gps.fix = 1;
	data->gps.lat = ((float)rand()/(float)(RAND_MAX)) * a;
	data->gps.lng = ((float)rand()/(float)(RAND_MAX)) * a;
	data->gps.speed = 25.34;
	data->motor.warning = 5;
	data->motor.failures = 6;
	data->motor.battery_voltage = 48.9;
	data->motor.battery_current = 22.5;
	for (int i = 0; i < 14; i++) {
		data->bms.cell_voltage[i] = 50.0/14;
	}
	data->bms.battery_voltage = 50.0;
	data->bms.battery_current = -14.3;
	data->bms.charge_current = 14.3;
//	data->bms.min_cel_voltage = ((float)rand()/(float)(RAND_MAX)) * a;
//	data->bms.max_cel_voltage = ((float)rand()/(float)(RAND_MAX)) * a;
}

#endif /* INC_MTU_TROUBLESHOOT_H_ */
