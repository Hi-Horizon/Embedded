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
		"GPS_fix,"
		"GPS_latitude,"
		"GPS_longitude,"
		"GPS_speed,"
		"MPPT_Pzon,"
		"ESC_warning,"
		"ESC_failures,"
		"ESC_batteryVoltage,"
		"ESC_InputCurrent,"
		"BMS_batteryVoltage,"
		"BMS_batteryCurrent,"
		"min_cell_voltage,"
		"max_cell_voltage,"
		"ESP_status,"
		"ESP_signal_strength,"
		"\n";
	f_open(&file, "dataLog.txt", FA_OPEN_APPEND | FA_READ | FA_WRITE);
	FRESULT fresult = f_write(&file, header, strlen(header), NULL);
	f_close(&file);

	return fresult;
}

FRESULT writeDataFrameToSD(DataFrame* data) {
	char row[256];
	int size = sprintf(row, "%lu,%u,%.4f,%.4f,%.2f,%hu,%u,%u,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%u,\n",
		data->telemetry.unixTime,
		data->gps.fix,
		data->gps.lat,
		data->gps.lng,
		data->gps.speed,
		data->mppt.power,
		data->motor.warning,
		data->motor.failures,
		data->motor.battery_voltage,
		data->motor.battery_current,
		data->bms.battery_voltage,
		data->bms.battery_current,
		data->bms.min_cel_voltage,
		data->bms.max_cel_voltage,
		data->telemetry.espStatus,
		data->telemetry.internetConnection
	);
	f_open(&file, "dataLog.txt", FA_OPEN_APPEND | FA_READ | FA_WRITE);
	FRESULT fresult = f_write(&file, &row, size, NULL);
	f_close(&file);

	return fresult;
}

FRESULT saveWifiCredentials(WifiCredentials *wc) {
	f_open(&file, "wifi.txt", FA_OPEN_APPEND | FA_READ | FA_WRITE);

	//write length+ssid
	FRESULT fresult = f_write(&file, &wc->ssidLength, 1, NULL);
	fresult = f_write(&file, wc->ssid, wc->ssidLength, NULL);
	//write length+password
	fresult = f_write(&file, &wc->passwordLength, 1, NULL);
	fresult = f_write(&file, wc->password, wc->passwordLength, NULL);

	f_close(&file);

	return fresult;
}

FRESULT readWifiCredentials(WifiCredentials *wc) {
	char buf[268];
	uint16_t bytesRead = 0;

	f_open(&file, "wifi.txt", FA_OPEN_APPEND | FA_READ | FA_WRITE);
	FRESULT fresult = f_read(&file, &buf, 268, &bytesRead);
	f_close(&file);

	wc->ssidLength = buf[0];
	for (int i = 0; i < wc->ssidLength; i++) {
		wc->ssid[i] = buf[i+1];
	}

	wc->passwordLength = buf[wc->ssidLength+1];
	for (int i = 0; i < wc->ssidLength + wc->passwordLength + 2; i++) {
		wc->ssid[i] = buf[i + wc->ssidLength + 2];
	}

	return fresult;
}


