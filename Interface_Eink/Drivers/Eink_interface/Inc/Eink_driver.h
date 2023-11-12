/*
 * Eink_driver.h
 *
 *  Created on: Apr 22, 2023
 *      Author: senne
 */

#ifndef INC_EINK_DRIVER_H_
#define INC_EINK_DRIVER_H_

#include "stm32g4xx.h"
#include "screenCharacters.h"

typedef struct {
	uint16_t screenWidth;
	uint16_t screenHeight;

	uint16_t imageBufferAddress;

	char IT8951version[16];
	char LUTversion[16];
} IT8951Info;

typedef struct {
	uint16_t endianType;
	uint16_t pixelFormat;
	uint16_t rotation;

	uint16_t xBegin;
	uint16_t xWidth;
	uint16_t yBegin;
	uint16_t yHeight;
} ImageLoadSettings;

#define IT8951_TCON_REG_WR       0x0011
#define IT8951_TCON_LD_IMG_AREA  0x0021
#define IT8951_TCON_LD_IMG_END   0x0022
#define USDEF_I80_CMD_DPY_AREA 	 0x0034
#define DISPLAY_REG_BASE 	0x1000
#define LUTAFSR   			0x1224  //LUT Status Reg (status of All LUT Engines)
#define LISAR 				0x0208

//general
void resetDriver();
void getIT8951Info(SPI_HandleTypeDef *spi, IT8951Info *container);
void IT8951WaitForDisplayReady();
void IT8951DisplayArea(SPI_HandleTypeDef *spi, ImageLoadSettings* settings, uint16_t displayMode);
void IT8951HostAreaPackedPixelWrite(SPI_HandleTypeDef *spi, IT8951Info *screenValues, ImageLoadSettings* settings, uint16_t* FrameBuf);
//command
void LCDWriteCmdCode(SPI_HandleTypeDef *spi ,uint16_t cmd);
//write
void LCDWriteData(SPI_HandleTypeDef *spi, uint16_t data);
void LCDWriteNData(SPI_HandleTypeDef *spi, uint16_t* dataBuf, uint16_t words);
//read
uint16_t LCDReadData(SPI_HandleTypeDef *spi);
void LCDReadNData(SPI_HandleTypeDef *spi, uint16_t* resultBuf, uint16_t resultWordCnt);

void writeScreenRegister(SPI_HandleTypeDef *spi, uint16_t address, uint16_t value);

#endif /* INC_EINK_DRIVER_H_ */
