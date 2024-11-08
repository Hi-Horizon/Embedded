/*
 * SD_API.c
 *
 *  Created on: 8 Nov 2024
 *      Author: senne
 */

#include <MTU/SD_API.h>

//TODO: check if you can just use fs for this
FATFS *pfs;
DWORD fre_clust;
FIL file;

FRESULT initSD(FATFS* fs, uint32_t* total, uint32_t* free_space) {
	FRESULT status = FR_OK;
	status = f_mount(fs,"/",1);

	status = f_getfree("/", &fre_clust, &pfs);

	total[0] = (uint32_t)((pfs->n_fatent - 2) * pfs->csize * 0.5);
	free_space[0] = (uint32_t)(fre_clust * pfs->csize * 0.5);

	writeDataHeaderToSD();
	return status;
}

FRESULT writeDataHeaderToSD() {
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
	f_open(&file, "dataLog.txt", FA_OPEN_APPEND | FA_READ | FA_WRITE);
	FRESULT fresult = f_write(&file, header, strlen(header), NULL);
	f_close(&file);

	return fresult;
}

FRESULT writeDataFrameToSD(DataFrame* data) {
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
	f_open(&file, "dataLog.txt", FA_OPEN_APPEND | FA_READ | FA_WRITE);
	FRESULT fresult = f_write(&file, &row, size, NULL);
	f_close(&file);

	return fresult;
}


