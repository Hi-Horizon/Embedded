/*
 * SpiConfig.c
 *
 *  Created on: 29 Mar 2025
 *      Author: senne
 */

#include <SpiConfig/SpiConfig.h>

uint8_t calculateChecksum(uint8_t *msg, int32_t messageSize) {
	uint8_t checksum = 0;
	for (int i=0; i < messageSize; i++) {
		checksum += msg[i];
	}
	return checksum;
}

//Puts all values from the dataFrame into a buffer, creates the frame structure
void dataFrameInBuf(DataFrame *dataFrame, uint8_t *buf) {
    int32_t index = 0;
    int32_t beginMsg = 0;

    buffer_append_uint8(buf, 1, &index);
    buffer_append_uint32(buf, dataFrame->telemetry.unixTime, &index);
    buffer_append_uint8(buf, dataFrame->gps.fix, &index);
    buffer_append_float32(buf, dataFrame->gps.lat, 100, &index);
    buffer_append_float32(buf, dataFrame->gps.lng, 100, &index);
    buffer_append_float32(buf, dataFrame->gps.speed, 100, &index);
    buffer_append_uint8(buf, calculateChecksum(buf + beginMsg, index - beginMsg), &index);
    beginMsg = index;

    buffer_append_uint8(buf, 2, &index);
    buffer_append_uint32(buf, dataFrame->gps.last_msg, &index);
    buffer_append_uint16(buf, dataFrame->mppt.power, &index);
    buffer_append_uint32(buf, dataFrame->mppt.last_msg, &index);
    buffer_append_uint8(buf, calculateChecksum(buf + beginMsg, index - beginMsg), &index);
    beginMsg = index;

    buffer_append_uint8(buf, 3, &index);
    buffer_append_uint8(buf, dataFrame->motor.warning, &index);
    buffer_append_uint8(buf, dataFrame->motor.failures, &index);
    buffer_append_float32(buf, dataFrame->motor.battery_voltage, 100, &index);
    buffer_append_float32(buf, dataFrame->motor.battery_current, 100, &index);
    buffer_append_uint32(buf, dataFrame->motor.last_msg, &index);
    buffer_append_uint8(buf, calculateChecksum(buf + beginMsg, index - beginMsg), &index);
    beginMsg = index;

    buffer_append_uint8(buf, 4, &index);
    buffer_append_float32(buf, dataFrame->bms.battery_voltage, 100, &index);
    buffer_append_float32(buf, dataFrame->bms.battery_current, 100, &index);
    buffer_append_float32(buf, dataFrame->bms.min_cel_voltage, 100, &index);
    buffer_append_uint8(buf, calculateChecksum(buf + beginMsg, index - beginMsg), &index);
    beginMsg = index;

    buffer_append_uint8(buf, 5, &index);
    buffer_append_float32(buf, dataFrame->bms.max_cel_voltage, 100, &index);
    buffer_append_uint32(buf, dataFrame->bms.last_msg, &index);
    buffer_append_uint8(buf, calculateChecksum(buf + beginMsg, index - beginMsg), &index);
    beginMsg = index;
}

//constructs the dataFrame from the received buffer
void dataFrameFromBuf(DataFrame *dataFrame, uint8_t *buf) {
    int32_t index = 0;
    uint8_t id = buffer_get_uint8(buf, &index);

    switch(id) {
        case 1:
            dataFrame->telemetry.unixTime   = buffer_get_uint32(buf, &index);
            dataFrame->gps.fix              = buffer_get_uint8(buf, &index);
            dataFrame->gps.lat              = buffer_get_float32(buf, 100, &index);
            dataFrame->gps.lng              = buffer_get_float32(buf, 100, &index);
            dataFrame->gps.speed            = buffer_get_float32(buf, 100, &index);
            break;
        case 2:
            dataFrame->gps.last_msg         = buffer_get_uint32(buf, &index);
            dataFrame->mppt.power           = buffer_get_uint16(buf, &index);
            dataFrame->mppt.last_msg        = buffer_get_uint32(buf, &index);
            break;
        case 3:
            dataFrame->motor.warning        = buffer_get_uint8(buf, &index);
            dataFrame->motor.failures       = buffer_get_uint8(buf, &index);
            dataFrame->motor.battery_current = buffer_get_float32(buf, 100, &index);
            dataFrame->motor.battery_voltage = buffer_get_float32(buf, 100, &index);
            dataFrame->motor.last_msg       = buffer_get_uint32(buf, &index);
            break;
        case 4:
            dataFrame->bms.battery_voltage = buffer_get_float32(buf, 100, &index);
            dataFrame->bms.battery_current = buffer_get_float32(buf, 100, &index);
            dataFrame->bms.min_cel_voltage = buffer_get_float32(buf, 100, &index);
            break;
        case 5:
            dataFrame->bms.max_cel_voltage = buffer_get_float32(buf, 100, &index);
            dataFrame->bms.last_msg        = buffer_get_uint32(buf, &index);
            break;
    }
}

void parseESPInfo(DataFrame *dataFrame, uint8_t *buf) {
    int32_t index = 0;
    dataFrame->telemetry.espStatus = buffer_get_uint8(buf, &index);
    dataFrame->telemetry.internetConnection = buffer_get_uint8(buf, &index);
}
