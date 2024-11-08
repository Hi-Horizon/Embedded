/*
 * GPS_parsing.c
 *
 *  Created on: 22 Jul 2024
 *      Author: senne
 */

#include <MTU/GPS_API.h>

uint8_t GPS_work_buf[GPS_BUF_SIZE];
uint16_t GPS_buf_index = 0;
uint16_t GPS_RX_msg_size = 0;

int8_t counter = -1;
uint8_t arraypos = 0;
bool counting = false;
bool new_msg = false;

char raw_time[10]; // hhmmss.sss
char raw_status[1]; // A = data valid, V = data not valid
char raw_latitude[9]; // ddmm.mmmm
char raw_NS_indicator[1]; // N = north, S = south
char raw_longitude[10]; // dddmm.mmmm
char raw_EW_indicator[1]; // E = east, W = west
char raw_speed_knots[7]; // Max velocity = 1001.08 knots
char raw_course[6]; //Course over ground in degrees
char raw_date[6]; // ddmmyy

char *GPS_data_raw[9] = {raw_time, raw_status, raw_latitude, raw_NS_indicator, raw_longitude, raw_EW_indicator, raw_speed_knots, raw_course, raw_date};

void parseGpsMessage(uint8_t* buf, uint16_t size) {
	for (uint16_t i = 0; i < size; i++) {
		uint8_t data = buf[i];
		if(data == 'R') {
			counting = true;
			counter = -3;
		}
		else if(data == ',' && counting) {//Next data
			counter++;
			arraypos = 0;
		} else if(counting) { // Save data
			if(counter >= 0 && counter < 9) { // RMC data
				GPS_data_raw[counter][arraypos] = data;
				arraypos++;
			} else if (counter >= 9) { // Data complete
				counting = false;
				new_msg = true;
			}
			else {
				counter++;
			}
		}
	}
}

//puts the raw data from gps into a dataframe
void GPS_bufferToDataFrame(DataFrame* data) {
	//checks if a new message has been parsed from the buffer
	if (!new_msg) return;
	data->gps.last_msg = data->telemetry.unixTime;
	new_msg = false;

	//A is fix, V is no fix
	if (raw_status[0] == 'A') {
		data->gps.fix = 1;
	} else {
		data->gps.fix = 0;
	}
	data->telemetry.strategyRuntime = atof(raw_time);
	data->gps.speed = atof(raw_speed_knots) * 1.852; //conversion from knots to km/h
	data->gps.lat = atof(raw_latitude);
	data->gps.lng = atof(raw_longitude);
}

bool nmeaChecksumCompare(uint8_t* buf, int packetBeginIndex, int packetEndIndex, uint8_t receivedCSbyte1, uint8_t receivedCSbyte2)
    {
      uint8_t calcChecksum = 0;

      for(uint16_t i=packetBeginIndex + 1; i<packetEndIndex-3; ++i) // packetEndIndex is the "size" of the packet minus 1. Loop from 1 to packetEndIndex-4 because the checksum is calculated between $ and *
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
		while (buf[current] != ',') {
			current++;
		} //skip Time for now
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

void parseGPS(DataFrame* data, uint8_t* GPS_buf, uint16_t Size) {
	if (Size != GPS_buf_index) { // check if new data has been received
		/* Check if position of index in reception buffer has simply be increased
		   of if end of buffer has been reached */

		if (Size > GPS_buf_index) { /* Current position is higher than previous one */

			GPS_RX_msg_size = Size - GPS_buf_index;

			/* Copy received data in "User" buffer for evacuation */
			for (uint16_t i = 0; i < GPS_RX_msg_size; i++) {
				GPS_work_buf[i] = GPS_buf[GPS_buf_index + i];
			}
		}
		else { /* Current position is lower than previous one : end of buffer has been reached */

		  /* First copy data from current position till end of buffer */
		  GPS_RX_msg_size = GPS_BUF_SIZE - GPS_buf_index;
		  /* Copy received data in "User" buffer for evacuation */
		  for (uint16_t i = 0; i < GPS_RX_msg_size; i++) {
			  GPS_work_buf[i] = GPS_buf[GPS_buf_index + i];
		  }
		  /* Check and continue with beginning of buffer */
		  if (Size > 0)
		  {
			for (uint16_t i = 0; i < Size; i++) {
				GPS_work_buf[GPS_RX_msg_size + i] = GPS_buf[i];
			}
			GPS_RX_msg_size += Size;
		  }
		}

		parseGpsMessage(GPS_work_buf, GPS_RX_msg_size);
		GPS_buf_index = Size;
	}
}

void oldParseGPS(DataFrame* data, uint8_t* buf, uint16_t size) {
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
