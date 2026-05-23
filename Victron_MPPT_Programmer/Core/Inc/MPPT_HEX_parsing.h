//
// Created by senne on 23/05/2026.
//

#ifndef VICTRON_MPPT_PROGRAMMER_MPPT_HEX_PARSING_H
#define VICTRON_MPPT_PROGRAMMER_MPPT_HEX_PARSING_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


#define MPPT_P 0
#define MPPT_V 1
#define MPPT_ERR 2
#define MPPT_CS 3

#define MPPT_VALUEARRAY_SIZE 40
#define MPPT_TAG_LEN 9
#define MPPT_VALUE_LEN 33

enum MPPT_ValueType {
    MPPT_noValue,
    MPPT_uint8,
    MPPT_uint16,
    MPPT_uint32
};

// void parseMPPTHex(DataFrame* data, uint8_t* buf, uint16_t size);
// void handleMPPTHex(DataFrame* data, char* msg, uint16_t size);
void buildMPPTHexCommand(uint8_t* buf, uint8_t size, uint8_t command, uint16_t registerId, uint32_t value, uint8_t valueType);

// void parseMPPT(DataFrame* data, uint8_t* buf, uint16_t size);

#endif //VICTRON_MPPT_PROGRAMMER_MPPT_HEX_PARSING_H