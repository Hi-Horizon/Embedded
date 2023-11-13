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

bool receiveSpiData(DataFrame *dataFrame, uint8_t *data) {
    int32_t index = 0;
    int32_t bufIndex = 0;
    uint8_t buf[32] = {};

    while(index < 32) { //search for the header byte
        if (data[index] == SpiHeaderByte) break;
        index++;
    }
    index++;
    while(index < 32) { 
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
    if (bufIndex < 24) { //parts of the frame is missing
        Serial.println("parts of the frame is missing");
        return false;
    } 

    if (!isChecksumValid(buf, bufIndex)) {
        Serial.println("checksum failed");
        return false;
    }


    index = 0;

    uint8_t staleness = buffer_get_uint16(buf, &index);
    dataFrame->gps.lat = buffer_get_float32(buf, 100, &index);
    dataFrame->gps.lng = buffer_get_float32(buf, 100, &index);
    dataFrame->gps.speed = buffer_get_float32(buf, 100, &index);

    dataFrame->motor.battery_current = buffer_get_float32(buf, 100, &index);
    dataFrame->motor.battery_voltage = buffer_get_float32(buf, 100, &index);

    dataFrame->mppt.power = buffer_get_uint16(buf, &index);
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