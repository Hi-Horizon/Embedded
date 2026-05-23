//
// Created by senne on 23/05/2026.
//

#include <MPPT_HEX_parsing.h>


bool calculateCheskumMPPT(uint8_t* buf, uint16_t size) {
	int checksum = 0;
	for (int i = 0; i < size; i++) {
	   checksum = (checksum + buf[i]) & 255; /* Take modulo 256 in account */
	}

	if (checksum == 0) {
		return true;
	} else {
		return false;
	}
}

// void parseMPPT(DataFrame* data, uint8_t* buf, uint16_t size) {
// 	bool tagPart = false;
// 	bool valuePart = false;
//
// 	char tags[MPPT_VALUEARRAY_SIZE][MPPT_TAG_LEN] = {};
// 	char values[MPPT_VALUEARRAY_SIZE][MPPT_VALUE_LEN] = {};
// 	uint8_t tagLength = 0;
// 	uint8_t valueLength = 0;
// 	uint8_t variableIndex = 0;
//
// 	for (int i = 0; i < size; i++) {
// 		char c = buf[i];
// 		switch(c) {
// 			case '\r':
// 				if (!tagPart && !valuePart) break;
// 				tagPart = false;
// 				valuePart = false;
// 				tagLength = 0;
// 				valueLength = 0;
// 				variableIndex++;
// 				break;
// 			case '\n':
// 				valuePart = false;
// 				tagPart = true;
// 				break;
// 			case '\t':
// 				tagPart = false;
// 				valuePart = true;
// 				break;
// 			default:
// 				if (tagPart) {
// 					tags[variableIndex][tagLength] = c;
// 					tagLength++;
// 				}
// 				else if (valuePart) {
// 					values[variableIndex][valueLength] = c;
// 					valueLength++;
// 				}
// 		}
// 	}
// 	if (calculateCheskumMPPT(buf, size)) { //checksum doesnt work yet
// 		for (int i = 0; i < MPPT_VALUEARRAY_SIZE; i++) {
// 			if (strcmp(tags[i], "PPV") == 0) {
// 				data->mppt.power = atoi(values[i]);
// 			}
// 			else if (strcmp(tags[i], "V") == 0) {
// 				data->mppt.voltage = atof(values[i]) / 1000.0;
// 			}
// 			else if (strcmp(tags[i], "ERR") == 0) {
// 				data->mppt.error = atoi(values[i]);
// 			}
// 			else if (strcmp(tags[i], "CS") == 0) {
// 				data->mppt.cs = atoi(values[i]);
// 			}
// 		}
// 	}
// }

void swap(char* arr, uint8_t index1, uint8_t index2) {
	uint8_t temp = arr[index1];
	arr[index1] = arr[index2];
	arr[index2] = temp;
}

// void handleMPPTHex(DataFrame* data, char* msg, uint16_t size) {
// 	uint8_t hexLength = size/2;
//
// 	char *tagStart = msg + 1;
// 	uint8_t tagLength = 4;
//
// 	char *valueStart = tagStart + tagLength + 2;
// 	uint32_t value = 0;
// 	if (hexLength == 6) { //payload is un16
// 		char valueRaw[4] = {};
// 		memcpy(valueRaw, valueStart, 4);
// 		swap(valueRaw, 0, 2);
// 		swap(valueRaw, 1, 3);
//
// 		char *endptr;
// 		value = strtoll(valueRaw, &endptr, 16);
// 	}
// 	if (hexLength == 8) { //payload is un32
// 		char valueRaw[8] = {};
// 		memcpy(valueRaw, valueStart, 8);
//
// 		swap(valueRaw, 0, 6);
// 		swap(valueRaw, 1, 7);
//
// 		swap(valueRaw, 2, 4);
// 		swap(valueRaw, 3, 5);
//
// 		char *endptr;
// 		value = strtoll(valueRaw, &endptr, 16);
// 	}
// }
//
// void parseMPPTHex(DataFrame* data, uint8_t* buf, uint16_t size) {
// 	char message[20] = {};
//
// 	bool readMessage = false;
// 	int index = 0;
// 	int copyIndex = 0;
// 	while (index < size){
// 		char c = buf[index];
// 		if (c == ':') {
// 			copyIndex = 0;
// 			readMessage = true;
// 		}
// 		else if (c == '\n' && readMessage) {
// 			handleMPPTHex(data, message, copyIndex);
// 			copyIndex = 0;
// 			readMessage = false;
// 		}
// 		else if (readMessage) {
// 			message[copyIndex] = c;
// 			copyIndex++;
// 		}
// 		index++;
// 	}
// 	//interpret chars as hex values
// }

// uint8_t* buf: char array with the message structure already present
// uint8_t size: buf size
// uint8_t command: tells the MPPT what command is performed, 7 is get and 8 is set
// uint16_t registerId: id of the register to be accessed
// uint32_t value: value to be sent
// uint8_t valueType: defines the amount of bytes to be sent
void buildMPPTHexCommand(uint8_t* buf, uint8_t size, uint8_t command, uint16_t registerId, uint32_t value, uint8_t valueType) {
	//	check right buf size for valueType
	int32_t index = 0;
	buf[index++] = ':';

	// convert Command
	sprintf(buf + index, "%X", command);
	index++;
	//	convert Id to HEX string
	sprintf(buf + index, "%04X", registerId);
	// perform char swaps for correct endianness
	swap(buf, index,		index + 2);
	swap(buf, index + 1,	index + 3);
	index += 4;

	buf[index++] = '0';
	buf[index++] = '0';

	//	convert value to HEX string, make it litte endian
	switch (valueType) {
		case MPPT_uint8:
			sprintf(buf + index, "%02X", value);
			index += 2;
			break;
		case MPPT_uint16:
			sprintf(buf + index, "%04X", value);
			swap(buf, index,		index + 2);
			swap(buf, index + 1,	index + 3);

			index += 4;
			break;
		case MPPT_uint32:
			sprintf(buf + index, "%08X", value);
			swap(buf, index,		index + 6);
			swap(buf, index + 1,	index + 7);
			swap(buf, index + 2,	index + 4);
			swap(buf, index + 3,	index + 5);

			index += 8;
			break;
	}

	// calculate checksum:
	// The sum of all data bytes and the check must equal 0x55.
	uint8_t checksum = 0x55 - ((
		command +
		(uint8_t)(registerId) +
		(uint8_t)(registerId >> 8) +
		(uint8_t)(value) +
		(uint8_t)(value >> 8) +
		(uint8_t)(value >> 16) +
		(uint8_t)(value >> 24)
	) % 256);

	sprintf(buf + index, "%02X", checksum);
	index += 2;
	buf[index] = '\n';
}