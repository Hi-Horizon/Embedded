/*
 * SD_hhrt.h
 *
 *  Created on: 27 Jan 2023
 *      Author: senne
 */

#ifndef INC_HHRTINC_SD_HHRT_H_
#define INC_HHRTINC_SD_HHRT_H_

#include "DataFrame.h"
#include "app_fatfs.h"
#include <stdio.h>
#include <string.h>

FRESULT writeDataHeaderToSD(DataFrame* data, FIL* file) {
	const char* header =
		"\n"
		"time,"
		"gps_fix,"
		"latitude,"
		"longitude,"
		"speed,"
		"pZon,"
		"batteryVoltage,"
		"batteryCurrent,\n";
	f_open(file, "dataLog.txt", FA_OPEN_APPEND | FA_READ | FA_WRITE);
	FRESULT fresult = f_write(file, header, strlen(header), NULL);
	f_close(file);

	return fresult;
}

FRESULT writeDataFrameToSD(DataFrame* data, FIL* file) {
	char row[128];
	int size = sprintf(row, "%lu,%u,%.4f,%.4f,%.3f,%hu,%.3f,%.3f,\n",
		data->telemetry.unixTime,
		data->gps.fix,
		data->gps.lat,
		data->gps.lng,
		data->gps.speed,
		data->mppt.power,
		data->motor.battery_voltage,
		data->motor.battery_current
	);
	f_open(file, "dataLog.txt", FA_OPEN_APPEND | FA_READ | FA_WRITE);
	FRESULT fresult = f_write(file, &row, size, NULL);
	f_close(file);

	return fresult;
}


#endif /* INC_HHRTINC_SD_HHRT_H_ */
