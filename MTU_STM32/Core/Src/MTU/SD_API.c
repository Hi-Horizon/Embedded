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
		"time_NTP,"
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
		"BMS_batteryCurrentCharge"
		"BMS_batteryCurrent,"
		"min_cell_voltage,"
		"max_cell_voltage,"
		"ESP_status,"
		"ESP_signal_strength,"
		"Cell_Voltage_1,"
		"Cell_Voltage_2,"
		"Cell_Voltage_3,"
		"Cell_Voltage_4,"
		"Cell_Voltage_5,"
		"Cell_Voltage_6,"
		"Cell_Voltage_7,"
		"Cell_Voltage_8,"
		"Cell_Voltage_9,"
		"Cell_Voltage_10,"
		"Cell_Voltage_11,"
		"Cell_Voltage_12,"
		"Cell_Voltage_13,"
		"Cell_Voltage_14,"
		"Cell_Balance_1,"
		"Cell_Balance_2,"
		"Cell_Balance_3,"
		"Cell_Balance_4,"
		"Cell_Balance_5,"
		"Cell_Balance_6,"
		"Cell_Balance_7,"
		"Cell_Balance_8,"
		"Cell_Balance_9,"
		"Cell_Balance_10,"
		"Cell_Balance_11,"
		"Cell_Balance_12,"
		"Cell_Balance_13,"
		"Cell_Balance_14,"
		"Cell_temp_1,"
		"Cell_temp_2,"
		"Cell_temp_3,"
		"Cell_temp_4,"
		"Bal_temp_1,"
		"Bal_temp_2,"
		"\n";
	f_open(&file, "dataLog.txt", FA_OPEN_APPEND | FA_READ | FA_WRITE);
	FRESULT fresult = f_write(&file, header, strlen(header), NULL);
	f_close(&file);

	return fresult;
}

FRESULT writeDataFrameToSD(DataFrame* data) {
	char row[1024];
	int size = sprintf(row, "%lu,%lu,%u,%.4f,%.4f,%.2f,%hu,%u,%u,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%u,",
		data->telemetry.unixTime,
		data->esp.NTPtime,
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
		data->bms.charge_current,
		data->bms.min_cel_voltage,
		data->bms.max_cel_voltage,
		data->esp.status,
		data->esp.internetConnection
	);

	for (int i = 0; i < 14; i++) {
		size += sprintf(row + size, "%.3f,",
			data->bms.cell_voltage[i]
		);
	}
	for (int i = 0; i < 14; i++) {
		size += sprintf(row + size, "%i,",
			data->bms.is_Balancing[i]
		);
	}
	for (int i = 0; i < 4; i++) {
		size += sprintf(row + size, "%.3f,",
			data->bms.cell_temp[i]
		);
	}
	for (int i = 0; i < 2; i++) {
		size += sprintf(row + size, "%.3f,", data->bms.balance_temp[i]);
	}
	row[size] = '\n';
	size++;

	f_open(&file, "dataLog.txt", FA_OPEN_APPEND | FA_READ | FA_WRITE);
	FRESULT fresult = f_write(&file, &row, size, NULL);
	f_close(&file);

	return fresult;
}

FRESULT saveWifiCredentials(WifiCredentials *wc) {
	f_open(&file, "wifi.txt", FA_OPEN_ALWAYS | FA_WRITE);

	//write length+ssid
	FRESULT fresult = f_write(&file, &wc->ssidLength, 1, NULL);
	fresult = f_write(&file, wc->ssid, wc->ssidLength, NULL);
	//write length+password
	fresult = f_write(&file, &wc->passwordLength, 1, NULL);
	fresult = f_write(&file, wc->password, wc->passwordLength, NULL);

	f_close(&file);

	return fresult;
}

FRESULT saveWifiCredentialsRaw(uint8_t *buf, uint32_t length) {
	for (int i = 0; i < 258; i++) {
		if (buf[i] == (char) 0x0) {
			length = i;
			break;
		}
	}

	f_unlink("wifi.txt");

	f_open(&file, "wifi.txt", FA_OPEN_ALWAYS | FA_WRITE);

	FRESULT fresult = f_write(&file, buf, length, NULL);

	f_close(&file);

	return fresult;
}

FRESULT readWifiCredentialsRaw(uint8_t *buf, uint8_t *bytesRead) {
	f_open(&file, "wifi.txt", FA_READ);
	FRESULT fresult = f_read(&file, buf, 258, (UINT*) bytesRead);
	f_close(&file);

	return fresult;
}


FRESULT readWifiCredentials(WifiCredentials *wc) {
	char buf[258];
	UINT bytesRead = 0;

	f_open(&file, "wifi.txt", FA_READ);
	FRESULT fresult = f_read(&file, buf, 258, &bytesRead);
	f_close(&file);

	wc->ssidLength = buf[0];
	memcpy(wc->ssid, buf + 1, wc->ssidLength);

	wc->passwordLength = buf[wc->ssidLength+1];
	memcpy(wc->password, buf + wc->ssidLength + 2, wc->passwordLength);

	return fresult;
}


