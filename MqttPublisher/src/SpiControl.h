#ifndef SPICONTROL__H_
#define SPICONTROL__H_

#include "DataFrame.h"
#include "buffer.h"
#include "Arduino.h"
#include "SpiConfig.h"

bool isChecksumValid(uint8_t *msg, uint8_t messageSize) {
    uint8_t checksum = msg[messageSize - 1];
    uint8_t messageSum = 0;
    for (int i = 0; i < messageSize - 1; i++) {
        messageSum += msg[i];
    }
    if (messageSum == checksum) return true;
    else return false; 
}


//for debugging purposes
// bool receiveSpiData(DataFrame *dataFrame, uint8_t *data) {
//     for (int i=0; i<32;i++) {
//         Serial.println(data[i]);
//     }
//     return true;
// }

void parsePayload(DataFrame *dataFrame, uint8_t *buf) {
    int32_t index = 0;
    uint8_t id = buffer_get_uint8(buf, &index);

    switch(id) {
        case 1:
            dataFrame->telemetry.unixTime   = buffer_get_uint32(buf, &index);
            dataFrame->mppt.last_msg        = buffer_get_uint32(buf, &index);
            dataFrame->gps.last_msg         = buffer_get_uint32(buf, &index);
            break;
        case 2:
            dataFrame->gps.lat      = buffer_get_float32(buf, 100, &index);
            dataFrame->gps.lng      = buffer_get_float32(buf, 100, &index);
            dataFrame->gps.speed    = buffer_get_float32(buf, 100, &index);
            break;
        case 3:
            dataFrame->motor.battery_current = buffer_get_float32(buf, 100, &index);
            dataFrame->motor.battery_voltage = buffer_get_float32(buf, 100, &index);
            dataFrame->mppt.power = buffer_get_uint16(buf, &index);
            break;
    }
}

bool receiveSpiData(DataFrame *dataFrame, uint8_t *data, size_t len) {
    size_t index = 0;
    int32_t bufIndex = 0;
    uint8_t buf[32] = {};

    while(index < len) { //search for the header byte
        if (data[index] == SpiHeaderByte) break;
        index++;
    }
    index++;
    while(index < len) { 
        uint8_t next = data[index];

        if (next == SpiHeaderByte) { //wrong header byte
            Serial.println("wrong header byte");
            return false;
        } 
        if (next == SpiTrailerByte) break; //end of message
        if (next == SpiFlagByte) {
            index++;
            buf[bufIndex++] = data[index]; //frame byte as actual data
        }
        else {
            buf[bufIndex++] = next;
        } //generic byte
        index++;
    }

    // Serial.println(buf[0]);
    if (!isChecksumValid(buf, bufIndex)) {
        // Serial.println("checksum failed");
        // Serial.println(buf[bufIndex]);
        return false;
    }
    
    parsePayload(dataFrame, buf);
    return true;
}

void append_with_stuffing(uint8_t *buffer, uint8_t byte, int32_t *index) {
    if (byte == SpiHeaderByte || byte == SpiFlagByte || byte == SpiTrailerByte) {
        buffer_append_uint8(buffer, SpiFlagByte, index);
    }
    buffer_append_uint8(buffer, byte , index);
}

int32_t generateMessageWithStuffing(uint8_t *buf, uint32_t NTPtime, bool timeSync, uint8_t status, uint8_t signal) {
    int32_t index = 0;
    buffer_append_uint8(buf, SpiHeaderByte, &index);

    append_with_stuffing(buf, status , &index);
    append_with_stuffing(buf, signal, &index);
    if (timeSync) {
        append_with_stuffing(buf, NTPtime >> 24, &index);
        append_with_stuffing(buf, NTPtime >> 16, &index);
        append_with_stuffing(buf, NTPtime >> 8, &index);
        append_with_stuffing(buf, NTPtime, &index);
    }

    buffer_append_uint8(buf, SpiTrailerByte, &index);
    return index;
}

#endif