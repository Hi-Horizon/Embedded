/*
 * ESP_SPI.c
 *
 *  Created on: 14 Aug 2024
 *      Author: senne
 */

#include "MTU/ESP_SPI.h"

//transfer code used for the arduino SPI
uint8_t transfercode[2] = {0x02, 0x00};

uint8_t buf[32] = {};
uint8_t buf2[128] = {};
uint8_t msg[32] = {};

//appends a byte to the the buffer, if this byte is a value used in the framing. It appends a flag byte as well.
void append_with_stuffing(uint8_t *buffer, uint8_t byte, int32_t *index) {
    if (byte == SpiHeaderByte || byte == SpiFlagByte || byte == SpiTrailerByte) {
        buffer_append_uint8(buffer, SpiFlagByte, index);
    }
    buffer_append_uint8(buffer, byte , index);
}

//uint8_t calculateChecksum(uint8_t *msg, uint8_t messageSize) {
//	uint8_t checksum = 0;
//	for (int i=0; i < messageSize; i++) {
//		checksum += msg[i];
//	}
//	return checksum;
//}

void sendFrameToEsp(SPI_HandleTypeDef *spi, uint8_t *buf, uint8_t *msg, int32_t bufLength) {
	int32_t frameLength = 0;
	//create the frame
    buffer_append_uint8(msg, SpiHeaderByte, &frameLength);
    for (int i = 0; i < bufLength; i++) {
  	  append_with_stuffing(msg, buf[i], &frameLength);
    }
    buffer_append_uint8(msg, SpiTrailerByte, &frameLength);
    //send the message
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 1);
    HAL_Delay(5);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 0);

    HAL_SPI_Transmit(spi, transfercode, sizeof(transfercode), 1000);
    HAL_SPI_Transmit(spi, msg, frameLength, 1000);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 1);
    HAL_Delay(5);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 0);
}

void sendDataToEsp(SPI_HandleTypeDef *spi, DataFrame* data) {
	  int32_t index = 0;

	  //frame 1
	  buffer_append_uint8(buf, 1, &index);

	  buffer_append_uint32(buf, data->telemetry.unixTime, &index);
	  buffer_append_uint32(buf, data->mppt.last_msg, &index);
	  buffer_append_uint32(buf, data->gps.last_msg, &index);

	  sendFrameToEsp(spi, buf, msg, index);
	  index = 0;

	  HAL_Delay(50);

	  //Append Id 2
	  buffer_append_uint8(buf, 2, &index);
	  //Append payload
	  buffer_append_float32(buf, data->gps.lat, 100, &index);
	  buffer_append_float32(buf, data->gps.lng, 100, &index);
	  buffer_append_float32(buf, data->gps.speed, 100, &index);

	  sendFrameToEsp(spi, buf, msg, index);
	  index = 0;

	  HAL_Delay(50);

	  //Append Id 3
	  buffer_append_uint8(buf, 3, &index);
	  //Append payload
	  buffer_append_float32(buf, data->motor.battery_current, 100, &index);
	  buffer_append_float32(buf, data->motor.battery_voltage, 100, &index);
	  buffer_append_uint16(buf, data->mppt.power, &index);

	  sendFrameToEsp(spi, buf, msg, index);
	  index = 0;
}

void sendDataToEsp2(SPI_HandleTypeDef *spi, DataFrame* data) {
	dataFrameInBuf(data, buf2);
	sendFrameToEsp(spi, buf2, msg, 19); //1
	HAL_Delay(50);
	sendFrameToEsp(spi, buf2+19, msg, 12); //2
	HAL_Delay(50);
	sendFrameToEsp(spi, buf2+31, msg, 16); //3
	HAL_Delay(50);
	sendFrameToEsp(spi, buf2+47, msg, 14); //4
	HAL_Delay(50);
	sendFrameToEsp(spi, buf2+61, msg, 10); //5
}


void storeResponse(DataFrame* data, uint8_t* buf, int32_t length) {
    int32_t index = 0;
    data->esp.status          = buffer_get_uint8(buf, &index);
    data->esp.internetConnection = buffer_get_uint8(buf, &index);
    if (length == 6) {
        data->telemetry.NTPtime            = buffer_get_uint32(buf, &index);
    }
}

//void receiveResponseFromESP(SPI_HandleTypeDef *spi, DataFrame* data) {
//  uint8_t transfercode[2] = {0x03, 0x00};
//  HAL_SPI_Transmit(spi, transfercode, sizeof(transfercode), 1000);
//
//  if (spi->transfer(0) != SpiHeaderByte) return; //end if first byte is not the header byte
//
//  int32_t index = 0;
//  uint8_t statBuf[RESPONSE_SIZE];
//
//  while(index != RESPONSE_SIZE) {
//    uint8_t next = spi->transfer(0);
//    if (next == SpiHeaderByte) return; //wrong header byte
//    if (next == SpiTrailerByte) break; //end of message
//    if (next == SpiFlagByte) {
//        statBuf[index++] = spi->transfer(0); //frame byte as actual data
//    }
//    else {
//        statBuf[index++] = next;
//    } //generic byte
//  }
//  storeResponse(data, statBuf, index);
//}

void ESPexchangeData(SPI_HandleTypeDef *spi, DataFrame* data) {
	sendDataToEsp(spi, data);
    HAL_Delay(10);
//    receiveResponseFromESP(spi, data);
}


