#ifndef ESP_SPI__H_
#define ESP_SPI__H_

#include <buffer/buffer.h>
#include "DataFrame.h"
#include "SpiConfig.h"

void append_with_stuffing(uint8_t *buffer, uint8_t byte, int32_t *index) {
    if (byte == SpiHeaderByte || byte == SpiFlagByte || byte == SpiTrailerByte) {
        buffer_append_uint8(buffer, SpiFlagByte, index);
    }
    buffer_append_uint8(buffer, byte , index);
}

uint8_t calculateChecksum(uint8_t *msg, uint8_t messageSize) {
	uint8_t checksum = 0;
	for (int i=0; i < messageSize; i++) {
		checksum += msg[i];
	}
	return checksum;
}

void sendFrameToEsp(SPI_HandleTypeDef *spi, DataFrame* data) {
  uint8_t transfercode[2] = {0x02, 0x00};

  uint16_t millis = 0;
  int32_t index = 0;
  int32_t index2 = 0;
  uint8_t buf[32] = {};
  uint8_t msg[32] = {};

//  uint16_t millis = (uint16_t) (HAL_GetTick() && 0x0000FFFF);
  buffer_append_uint16(buf, millis, &index);
  buffer_append_float32(buf, data->gps.lat, 100, &index);
  buffer_append_float32(buf, data->gps.lng, 100, &index);
  buffer_append_float32(buf, data->gps.speed, 100, &index);

  buffer_append_float32(buf, data->motor.battery_current, 100, &index);
  buffer_append_float32(buf, data->motor.battery_voltage, 100, &index);

  buffer_append_uint16(buf, data->mppt.power, &index);

  buffer_append_uint8(buf, calculateChecksum(buf, index), &index);

  buffer_append_uint8(msg, SpiHeaderByte, &index2);
  for (int i =0; i < index; i++) {
	  append_with_stuffing(msg, buf[i], &index2);
  }
  buffer_append_uint8(msg, SpiTrailerByte, &index2);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 0);

  HAL_SPI_Transmit(spi, transfercode, sizeof(transfercode), 1000);
  HAL_SPI_Transmit(spi, msg, sizeof(msg), 1000);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 1);
}

void storeResponse(DataFrame* data, uint8_t* buf, int32_t length) {
    int32_t index = 0;
    data->telemetry.espStatus          = buffer_get_uint8(buf, &index);
    data->telemetry.internetConnection = buffer_get_uint8(buf, &index);
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
    sendFrameToEsp(spi, data);
    HAL_Delay(10);
//    receiveResponseFromESP(spi, data);
}

#endif
