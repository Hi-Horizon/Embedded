/*
 * SpiConfig.c
 *
 *  Created on: 29 Mar 2025
 *      Author: senne
 */

#include <SpiConfig/SpiConfig.h>

uint8_t calculateChecksumWithStuffing(uint8_t *msg, int32_t messageSize) {
	uint8_t checksum = 0;
    int i = 1;
	while (i < messageSize) {
        if (msg[i] == SpiHeaderByte)  return 0;     //wrong headerbyte
        if (msg[i] == SpiTrailerByte) return 0; 
        if (msg[i] == SpiFlagByte) i++;
		checksum += msg[i];
        i++;
	}
	return checksum;
}

uint8_t calculateChecksum(uint8_t *msg, int32_t messageSize) {
	uint8_t checksum = 0;
	for (int i=0; i < messageSize; i++) {
		checksum += msg[i];
	}
	return checksum;
}

//constructs the dataFrame from the received buffer
void dataFrameFromPayload(DataFrame *dataFrame, uint8_t *buf) {
    int32_t index = 1;

    dataFrame->telemetry.unixTime           = buffer_get_uint32(buf, &index);
    dataFrame->telemetry.wifiSetupControl   = buffer_get_uint8(buf, &index);

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

void espInfoFromPayload(DataFrame *dataFrame, uint8_t *buf) {
    int32_t index = 1;

    dataFrame->telemetry.NTPtime = buffer_get_uint32(buf, &index);
    dataFrame->telemetry.espStatus = buffer_get_uint8(buf, &index);
    dataFrame->telemetry.mqttStatus = buffer_get_uint8(buf, &index);
    dataFrame->telemetry.internetConnection = buffer_get_uint8(buf, &index);
}

void wifiCredentialsFromPayload(WifiCredentials *wifiCredentials, uint8_t *buf) {
    int32_t index = 1;

    uint8_t ssidLength = buffer_get_uint8(buf, &index);
    uint8_t passwordLength = buffer_get_uint8(buf, &index);

    for (uint8_t i = 0; i < ssidLength; i++) {
        wifiCredentials->ssid[i] = buffer_get_uint8(buf, &index);
    }
    for (uint8_t i = 0; i < passwordLength; i++) {
        wifiCredentials->password[i] = buffer_get_uint8(buf, &index);
    }
}

//inplace unpackaging of the payload
bool unpackPayload(uint8_t *data, size_t len) {
    size_t index = 0;
    int32_t unpackIndex = 0;
    int32_t trailerIndex = len - 1;

    //search for the header byte
    while(index < len) { 
        if (data[index] == SpiHeaderByte) break;
        index++;
    }
    if (index == len) return false;
    //search for checksumIndex
    while(trailerIndex >= 0) { 
        if (data[trailerIndex] == SpiTrailerByte) break;
        trailerIndex--;
    }
    if (trailerIndex == -1) return false;

    //parse rest of the frame
    index++;
    while(index < len) { 
        uint8_t next = data[index];

        if (next == SpiHeaderByte)  return false;     //wrong headerbyte
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

    if (calculateChecksum(data, unpackIndex - 1) != data[unpackIndex - 1]) {
        return false;
    }

    return true;
}

//parse a frame into a DataFrame
//returns bool: true if parse is succesfull, false if not.
bool parseFrame(DataFrame *dataFrame, WifiCredentials *wifiCredentials, uint8_t *buf, size_t len) {
    if (unpackPayload(buf, len)) {
        int32_t index = 0;
        //if in the future id's are added for extra messages
        uint8_t id = buffer_get_uint8(buf, &index);
        switch (id) {
            case 1:
                dataFrameFromPayload(dataFrame, buf);
                break;
            case 2:
                wifiCredentialsFromPayload(wifiCredentials, buf);
                break;
            case 3:
                espInfoFromPayload(dataFrame, buf);
                break;
        }       
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

    append_uint8_with_stuffing(buf, 1, &index);
    append_uint32_with_stuffing(buf, dataFrame->telemetry.unixTime, &index);
    append_uint8_with_stuffing(buf, dataFrame->telemetry.wifiSetupControl, &index);
    
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

    append_uint8_with_stuffing(buf, calculateChecksumWithStuffing(buf, index), &index);

    buffer_append_uint8(buf, SpiTrailerByte, &index);
}

void createESPInfoFrame(DataFrame *dataFrame, uint8_t *buf) {
    int32_t index = 0;
    buffer_append_uint8(buf, SpiHeaderByte, &index);

    append_uint8_with_stuffing(buf, 3, &index);
    append_uint32_with_stuffing(buf, dataFrame->telemetry.NTPtime, &index);
    append_uint8_with_stuffing(buf, dataFrame->telemetry.espStatus, &index);
    append_uint8_with_stuffing(buf, dataFrame->telemetry.mqttStatus, &index);
    append_uint8_with_stuffing(buf, dataFrame->telemetry.internetConnection, &index);
    append_uint8_with_stuffing(buf, calculateChecksumWithStuffing(buf, index), &index);

    buffer_append_uint8(buf, SpiTrailerByte, &index);
}

void createWiFiCredentialsFrame(WifiCredentials *wc, uint8_t *buf) {
    int32_t index = 0;

    buffer_append_uint8(buf, SpiHeaderByte, &index);
    //id byte
    buffer_append_uint8(buf, 2, &index);

    append_uint8_with_stuffing(buf, wc->ssidLength, &index);
    append_uint8_with_stuffing(buf, wc->passwordLength, &index);
    for (int i =0; i < wc->ssidLength; i++) {
        append_uint8_with_stuffing(buf, wc->ssid[i], &index);
    }
    for (int i =0; i < wc->passwordLength; i++) {
        append_uint8_with_stuffing(buf, wc->password[i], &index);
    }
    append_uint8_with_stuffing(buf, calculateChecksumWithStuffing(buf, index), &index);

    buffer_append_uint8(buf, SpiTrailerByte, &index);
}