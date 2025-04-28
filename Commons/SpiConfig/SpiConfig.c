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
void dataFrameInPayload(DataFrame *dataFrame, uint8_t *buf) {
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
void dataFrameFromPayload(DataFrame *dataFrame, uint8_t *buf) {
    int32_t index = 0;
    //if in the future id's are added for extra messages
    // uint8_t id = buffer_get_uint8(buf, &index);
    
    dataFrame->telemetry.unixTime   = buffer_get_uint32(buf, &index);
    dataFrame->gps.fix              = buffer_get_uint8(buf, &index);
    dataFrame->gps.lat              = buffer_get_float32(buf, 100, &index);
    dataFrame->gps.lng              = buffer_get_float32(buf, 100, &index);
    dataFrame->gps.speed            = buffer_get_float32(buf, 100, &index);
    dataFrame->gps.last_msg         = buffer_get_uint32(buf, &index);
    
    dataFrame->mppt.power            = buffer_get_uint16(buf, &index);
    dataFrame->mppt.last_msg         = buffer_get_uint32(buf, &index);
    
    dataFrame->motor.warning         = buffer_get_uint8(buf, &index);
    dataFrame->motor.failures        = buffer_get_uint8(buf, &index);
    dataFrame->motor.battery_current = buffer_get_float32(buf, 100, &index);
    dataFrame->motor.battery_voltage = buffer_get_float32(buf, 100, &index);
    dataFrame->motor.last_msg        = buffer_get_uint32(buf, &index);
    
    dataFrame->bms.battery_voltage = buffer_get_float32(buf, 100, &index);
    dataFrame->bms.battery_current = buffer_get_float32(buf, 100, &index);
    dataFrame->bms.min_cel_voltage = buffer_get_float32(buf, 100, &index);
    dataFrame->bms.max_cel_voltage = buffer_get_float32(buf, 100, &index);
    dataFrame->bms.last_msg        = buffer_get_uint32(buf, &index);
}

void constructESPInfo(DataFrame *dataFrame, uint8_t *buf) {
    int32_t index = 0;
    buffer_append_uint32(buf, dataFrame->telemetry.NTPtime, &index);
    buffer_append_uint8(buf, dataFrame->telemetry.espStatus, &index);
    buffer_append_uint8(buf, dataFrame->telemetry.mqttStatus, &index);
    buffer_append_uint8(buf, dataFrame->telemetry.internetConnection, &index);
    buffer_append_uint8(buf, calculateChecksum(buf, index), &index);
}

bool parseESPInfo(DataFrame *dataFrame, uint8_t *buf) {
    int32_t index = 0;
    dataFrame->telemetry.NTPtime = buffer_get_uint32(buf, &index);
    dataFrame->telemetry.espStatus = buffer_get_uint8(buf, &index);
    dataFrame->telemetry.mqttStatus = buffer_get_uint8(buf, &index);
    dataFrame->telemetry.internetConnection = buffer_get_uint8(buf, &index);
    uint8_t checksum  = buffer_get_uint8(buf, &index);
    if (calculateChecksum(buf, index-1) != checksum) return false;
    return true;
}



//inplace unpackaging of the payload
bool unpackPayload(uint8_t *data, size_t len) {
    size_t index = 0;
    int32_t unpackIndex = 0;

    //search for the header byte
    while(index < len) { 
        if (data[index] == SpiHeaderByte) break;
        index++;
    }
    //parse rest of the frame
    index++;
    while(index < len) { 
        uint8_t next = data[index];

        if (next == SpiHeaderByte)  return false; //wrong headerbyte
        if (next == SpiTrailerByte) break;        //end of message
        if (next == SpiFlagByte) {                //next byte is payload data, not a frame byte
            index++;
            data[unpackIndex++] = data[index];
        }
        else { //generic byte
            data[unpackIndex++] = next;
        }
        index++;
    }

    if (!calculateChecksum(data, unpackIndex)) {
        return false;
    }

    return true;
}

//parse a frame into a DataFrame
//returns bool: true if parse is succesfull, false if not.
bool parseFrame(DataFrame *dataFrame, uint8_t *buf, size_t len) {
    if (unpackPayload(buf, len)) {
        dataFrameFromPayload(dataFrame, buf);
        return true;
    } else return false;
}

void append_uint8_with_stuffing(uint8_t *buffer, uint8_t byte, int32_t *index) {
    if (byte == SpiHeaderByte || byte == SpiFlagByte || byte == SpiTrailerByte) {
        buffer_append_uint8(buffer, SpiFlagByte, index);
    }
    buffer_append_uint8(buffer, byte , index);
}

void append_uint16_with_stuffing(uint8_t *buffer, uint8_t val, int32_t *index) {
    int32_t smallIndex = 0;
    uint8_t buf[2];
    buffer_append_uint16(buf, val , &smallIndex);
    
    append_uint8_with_stuffing(buffer, buf[0], index);
    append_uint8_with_stuffing(buffer, buf[1], index);
}

void append_uint32_with_stuffing(uint8_t *buffer, uint8_t val, int32_t *index) {
    int32_t smallIndex = 0;
    uint8_t buf[4];
    buffer_append_uint32(buf, val , &smallIndex);
    
    append_uint8_with_stuffing(buffer, buf[0], index);
    append_uint8_with_stuffing(buffer, buf[1], index);
    append_uint8_with_stuffing(buffer, buf[2], index);
    append_uint8_with_stuffing(buffer, buf[3], index);
}

void append_float32_with_stuffing(uint8_t *buffer, float val, int acc, int32_t *index) {
    int32_t smallIndex = 0;
    uint8_t buf[4];
    buffer_append_float32(buf, val, acc, &smallIndex);
    
    append_uint8_with_stuffing(buffer, buf[0], index);
    append_uint8_with_stuffing(buffer, buf[1], index);
    append_uint8_with_stuffing(buffer, buf[2], index);
    append_uint8_with_stuffing(buffer, buf[3], index);
}

//construct a frame From a given dataFrame into a buf, with padding
void createFrame(DataFrame *dataFrame, uint8_t *buf, size_t len) {
    int32_t index = 0;

    buffer_append_uint8(buf, SpiHeaderByte, &index);

    append_uint32_with_stuffing(buf, dataFrame->telemetry.unixTime, &index);

    append_uint8_with_stuffing(buf, dataFrame->gps.fix, &index);
    append_float32_with_stuffing(buf, dataFrame->gps.lat, 100, &index);
    append_float32_with_stuffing(buf, dataFrame->gps.lng, 100, &index);
    append_float32_with_stuffing(buf, dataFrame->gps.speed, 100, &index);
    append_uint32_with_stuffing(buf, dataFrame->gps.last_msg, &index);

    append_uint16_with_stuffing(buf, dataFrame->mppt.power, &index);
    append_uint32_with_stuffing(buf, dataFrame->mppt.last_msg, &index);

    append_uint8_with_stuffing(buf, dataFrame->motor.warning, &index);
    append_uint8_with_stuffing(buf, dataFrame->motor.failures, &index);
    append_float32_with_stuffing(buf, dataFrame->motor.battery_voltage, 100, &index);
    append_float32_with_stuffing(buf, dataFrame->motor.battery_current, 100, &index);
    append_uint32_with_stuffing(buf, dataFrame->motor.last_msg, &index);

    append_float32_with_stuffing(buf, dataFrame->bms.battery_voltage, 100, &index);
    append_float32_with_stuffing(buf, dataFrame->bms.battery_current, 100, &index);
    append_float32_with_stuffing(buf, dataFrame->bms.min_cel_voltage, 100, &index);
    append_float32_with_stuffing(buf, dataFrame->bms.max_cel_voltage, 100, &index);
    append_uint32_with_stuffing(buf, dataFrame->bms.last_msg, &index);

    append_uint8_with_stuffing(buf, calculateChecksum(buf, index), &index);

    buffer_append_uint8(buf, SpiTrailerByte, &index);
}
