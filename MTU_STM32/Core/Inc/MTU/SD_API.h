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

FRESULT initSD(FATFS* fs, uint32_t* total, uint32_t* free_space);
FRESULT writeDataHeaderToSD();
FRESULT writeDataFrameToSD(DataFrame* data);


#endif /* INC_HHRTINC_SD_HHRT_H_ */
