#ifndef ESP_SPI__H_
#define ESP_SPI__H_

#include <buffer/buffer.h>
#include "DataFrame.h"
#include "SpiConfig/SpiConfig.h"
#include "stm32g4xx_hal.h"

void sendDataToEsp(SPI_HandleTypeDef *spi, DataFrame* data);
void sendDataToEsp2(SPI_HandleTypeDef *spi, DataFrame* data);

#endif
