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
#include "SpiConfig/SpiConfig.h"
#include <stdio.h>
#include <string.h>

FRESULT initSD(FATFS* fs, uint32_t* total, uint32_t* free_space);
FRESULT writeDataHeaderToSD();
FRESULT writeDataFrameToSD(DataFrame* data);
FRESULT saveWifiCredentials(WifiCredentials *wc);
FRESULT readWifiCredentials(WifiCredentials *wc);
FRESULT readWifiCredentialsRaw(uint8_t *buf, uint8_t *bytesRead);


#endif /* INC_HHRTINC_SD_HHRT_H_ */
