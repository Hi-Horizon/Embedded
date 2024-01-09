/*
 * sensorParser.c
 *
 *  Created on: Sep 28, 2023
 *      Author: senne
 */


#include "MTU/sensorParser.h"
#include "stdio.h"

bool nmeaChecksumCompare(uint8_t* buf, int packetBeginIndex, int packetEndIndex, uint8_t receivedCSbyte1, uint8_t receivedCSbyte2)
    {
      uint8_t calcChecksum = 0;

      for(uint8_t i=packetBeginIndex + 1; i<packetEndIndex-3; ++i) // packetEndIndex is the "size" of the packet minus 1. Loop from 1 to packetEndIndex-4 because the checksum is calculated between $ and *
      {
        calcChecksum = calcChecksum^buf[i];
      }

      uint8_t nibble1 = (calcChecksum&0xF0) >> 4; //"Extracts" the first four bits and shifts them 4 bits to the right. Bitwise AND followed by a bitshift
      uint8_t nibble2 = calcChecksum&0x0F;

      uint8_t translatedByte1 = (nibble1<=0x9) ? (nibble1+'0') : (nibble1-10+'A'); //Converting the number "nibble1" into the ASCII representation of that number
      uint8_t translatedByte2 = (nibble2<=0x9) ? (nibble2+'0') : (nibble2-10+'A'); //Converting the number "nibble2" into the ASCII representation of that number

      if(translatedByte1==receivedCSbyte1 && translatedByte2==receivedCSbyte2) //Check if the checksum calculated from the packet payload matches the checksum in the packet
      {
        return true;
      }
      else
      {
        return false;
      }
    }

void parseNMEA(DataFrame* data, uint8_t* buf, int beginIndex, int endMessage) {
	if (!nmeaChecksumCompare(buf, beginIndex, endMessage, buf[endMessage-2], buf[endMessage-1])) return;

	char nmea[5];
	int current = beginIndex;
	for (int i = 0; i < 5; i++) {
		current++;
		nmea[i] = buf[current];
	}
	char val[20] = {};
	if (strcmp(nmea, "GPGGA") == 0) {
		current++;
		int ind = 0;
		while (buf[current] != ',') {} //skip Time for now
		while (buf[current] != ',') {
			current++;
			val[ind] = buf[current];
			ind++;
		}
		data->gps.lat = atof(val);
		ind = 0;
		memset(val, 0, sizeof val);
		while (buf[current] != ',') {current++;} //skip N/Z for now
		while (buf[current] != ',') {
					current++;
					val[ind] = buf[current];
					ind++;
		}
		data->gps.lng = atof(val);
	}
	if (strcmp(nmea, "GPVTG") == 0) {
			current++;
			int ind = 0;
			for (int i = 0; i < 6; i++) {
				while (buf[current] != ',') {current++;}
			}
			while (buf[current] != ',') {
						current++;
						val[ind] = buf[current];
						ind++;
			}
			data->gps.speed = atof(val);
	}
}

void parseGPS(DataFrame* data, uint8_t* buf, uint16_t size) {
	int beginMessage = 0;
	int endMessage = 0;
	bool insideMessage = false;
	for (int i = 0; i < size; i++) {
		char c = buf[i];
		if (c == '$') {
			insideMessage = true;
			beginMessage = i;
			endMessage = i;
		}
		if (c == '\r' || endMessage - beginMessage >= 80) {
			insideMessage = false;
			if (endMessage - beginMessage < 80) parseNMEA(data, buf, beginMessage, endMessage);
		}

		if (insideMessage == true) {
			endMessage++;
		}
	}
}

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

void parseMPPT(DataFrame* data, uint8_t* buf, uint16_t size) {
	bool tagPart = false;
	bool valuePart = false;

	char tags[MPPT_VALUEARRAY_SIZE][MPPT_TAG_LEN] = {};
	char values[MPPT_VALUEARRAY_SIZE][MPPT_VALUE_LEN] = {};
	uint8_t tagLength = 0;
	uint8_t valueLength = 0;
	uint8_t variableIndex = 0;

	for (int i = 0; i < size; i++) {
		char c = buf[i];
		switch(c) {
			case '\r':
				if (!tagPart && !valuePart) break;
				tagPart = false;
				valuePart = false;
				tagLength = 0;
				valueLength = 0;
				variableIndex++;
				break;
			case '\n':
				valuePart = false;
				tagPart = true;
				break;
			case '\t':
				tagPart = false;
				valuePart = true;
				break;
			default:
				if (tagPart) {
					tags[variableIndex][tagLength] = c;
					tagLength++;
				}
				else if (valuePart) {
					values[variableIndex][valueLength] = c;
					valueLength++;
				}
		}
	}
	if (calculateCheskumMPPT(buf, size)) { //checksum doesnt work yet
		for (int i = 0; i < MPPT_VALUEARRAY_SIZE; i++) {
			if (strcmp(tags[i], "PPV") == 0) {
				data->mppt.power = atoi(values[i]);
			}
			else if (strcmp(tags[i], "V") == 0) {
				data->mppt.voltage = atof(values[i]) / 1000.0;
			}
			else if (strcmp(tags[i], "ERR") == 0) {
				data->mppt.error = atoi(values[i]);
			}
			else if (strcmp(tags[i], "CS") == 0) {
				data->mppt.cs = atoi(values[i]);
			}
		}
	}
}

void handleMPPTHex(DataFrame* data, char* msg, uint16_t size) {
	char *pos = msg + 1;

	uint8_t hexLength = size/2;
	uint8_t valueBufHex[hexLength];

	for (size_t count = 0; count < hexLength; count++) {
		sscanf(pos, "%2hhx", &valueBufHex[count]);
		pos += 2;
	}

	//calculate checksum
	uint8_t checksum = 0x07; //get command
	for (int i = 0; i < hexLength; i++) {
		checksum += valueBufHex[i];
	}
	if (checksum != 0x55) return;

	//copy value into the dataFrame struct
	uint16_t tag 	= ((uint16_t) valueBufHex[1] << 8) | valueBufHex[0];

	uint32_t value 	= 0;
	if (hexLength == 6) {
		value =
			((uint32_t) valueBufHex[4] << 8) | valueBufHex[3];
	}
	if (hexLength == 8) {
		value =
			((uint32_t) valueBufHex[6] << 24) |
			((uint32_t) valueBufHex[5] << 16) |
			((uint32_t) valueBufHex[4] << 8) |
			valueBufHex[3];
	}


	if (tag == 0xEDD5) data->mppt.voltage = value / 100.0f;
	if (tag == 0xEDBC) { //quick type conversion fix
		float fltVal = value / 100.0f;
		if (fltVal >= 65535) data->mppt.power = 65535;
		else data->mppt.power = value / 100;
	}
}

void parseMPPTHex(DataFrame* data, uint8_t* buf, uint16_t size) {
	char message[20] = {};

	bool readMessage = false;
	int index = 0;
	int copyIndex = 0;
	while (index < size){
		char c = buf[index];
		if (c == ':') {
			copyIndex = 0;
			readMessage = true;
		}
		else if (c == '\n' && readMessage) {
			handleMPPTHex(data, message, copyIndex);
			copyIndex = 0;
			readMessage = false;
		}
		else if (readMessage) {
			message[copyIndex] = c;
			copyIndex++;
		}
		index++;
	}
	//interpret chars as hex values
}
