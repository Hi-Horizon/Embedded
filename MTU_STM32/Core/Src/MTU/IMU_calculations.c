/*
 * IMU_calculations.c
 *
 *  Created on: 19 Jun 2025
 *      Author: senne
 */

#include "MTU/IMU_calculations.h"

void calculateOrientation(IMU_data* imu) {
}

void calculatePitchRollYaw(IMU_data* imu) {
	imu->pitch 	= atan2(imu->accely, imu->accelz + 0.05*imu->accelx);
	imu->roll 	= atan2(-1*imu->accelx, sqrt(imu->accely*imu->accely + imu->accelz*imu->accelz));

	float magLength = sqrt(imu->magx*imu->magx + imu->magy*imu->magy + imu->magz*imu->magz);
	float normMagx = imu->magx/magLength;
	float normMagy = imu->magy/magLength;
	float normMagz = imu->magz/magLength;

	imu->yaw = atan2(
			sin(imu->roll)*normMagz - cos(imu->roll)*normMagy,
			cos(imu->pitch)*normMagx + sin(imu->roll)*sin(imu->pitch)*normMagy + cos(imu->roll)*sin(imu->pitch)*normMagz
		  ) * 57.2957795;
}

void calculateCompassDirection(IMU_data* imu) {
	float Xhorizontal = imu->magx*cos(imu->pitch) + imu->magy*sin(imu->roll)*sin(imu->pitch) - imu->magz*cos(imu->roll)*sin(imu->pitch);
	float Yhorizontal = imu->magy*cos(imu->roll) + imu->magz*sin(imu->roll);
	imu->compass = atan2(Yhorizontal, Xhorizontal);
}

float updateMean(float average, float value, uint32_t size) {
	return (size * average + value) / (size + 1);
}
