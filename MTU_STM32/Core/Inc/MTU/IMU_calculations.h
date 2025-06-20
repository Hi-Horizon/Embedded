/*
 * IMU_calculations.h
 *
 *  Created on: 19 Jun 2025
 *      Author: senne
 */

#ifndef INC_MTU_IMU_CALCULATIONS_H_
#define INC_MTU_IMU_CALCULATIONS_H_

#include "dataFrame.h"
#include <stdio.h>
#include <math.h>

void calculateOrientation(IMU_data* imu);
void calculatePitchRollYaw(IMU_data* imu);
void calculateCompassDirection(IMU_data* imu);
float updateMean(float average, float value, uint32_t size);


#endif /* INC_MTU_IMU_CALCULATIONS_H_ */
