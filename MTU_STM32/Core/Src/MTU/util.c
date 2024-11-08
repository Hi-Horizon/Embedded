/*
 * util.c
 *
 *  Created on: 8 Nov 2024
 *      Author: senne
 */

#include <MTU/util.h>

//RTC
RTC_TimeTypeDef timer;
RTC_DateTypeDef date;

//converts timestamp to unixTime
void getRTCUnixTime(RTC_HandleTypeDef* hrtc, DataFrame* data) {
	HAL_RTC_GetTime(hrtc, &timer, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(hrtc, &date, RTC_FORMAT_BIN);
	struct tm tm = {};
	tm.tm_sec = timer.Seconds;
	tm.tm_min = timer.Minutes;
	tm.tm_hour = timer.Hours;
	//these below are not important but are needed for correct conversion
	tm.tm_isdst = timer.DayLightSaving;
	tm.tm_mday = 1;
	tm.tm_mon = 0;
	tm.tm_year = 70;
	data->telemetry.unixTime = mktime(&tm); //convert a segmented timestamp to a unixTimeStamp
}
