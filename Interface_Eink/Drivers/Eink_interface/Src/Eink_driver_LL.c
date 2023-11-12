/*
 * Eink_driver_LL.c,
 *
 * defines the lower layer interface with the IT8951 driver
 *
 *  Created on: 4 May 2023
 *      Author: senne
 */
#include "../../Eink_interface/Inc/Eink_driver.h"

#define wait 10L

void resetDelay()
{
	for (int i=0;i<10000;i++)
		for (int j=0;j<10;j++);
}

void setWordToArray(uint8_t* wordArray,uint16_t word) {
	wordArray[1]=(word >> 8);
	wordArray[0]= word & 0xff;
}

void resetDriver() {
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_5, GPIO_PIN_RESET);
	resetDelay();
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_5, GPIO_PIN_SET);
	resetDelay();
}

void LCDWaitForReady()
{
	while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) == 0)
	{
	}
}

//-----------------------------------------------------------------
//Host controller function 2   Write command code to host data Bus
//-----------------------------------------------------------------
void LCDWriteCmdCode(SPI_HandleTypeDef *spi ,uint16_t cmd)
{
	uint8_t towrite[2];
	setWordToArray(towrite, 0x6000); //Set Preamble for Write Command, which is 0x6000

	//set CS LOW
	LCDWaitForReady();
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 0);

	HAL_SPI_Transmit(spi, towrite, 1, 1000);

	setWordToArray(towrite, cmd);

	LCDWaitForReady();
	HAL_SPI_Transmit(spi, towrite, 1, 1000);

	HAL_Delay(wait);

	//set CS HIGH
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 1);
}

//-----------------------------------------------------------
//Host controller function 3    Write Data to host data Bus
//-----------------------------------------------------------
void LCDWriteData(SPI_HandleTypeDef *spi, uint16_t data)
{
	uint8_t towrite[2];
	setWordToArray(towrite, 0x0000); //Set Preamble for Write data, which is 0x0000

	//set CS LOW
	LCDWaitForReady();
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 0);

	HAL_SPI_Transmit(spi, towrite, 1, 1000);

	setWordToArray(towrite, data);

	LCDWaitForReady();
	HAL_SPI_Transmit(spi, towrite, 1, 1000);

	HAL_Delay(wait);

	//set CS HIGH
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 1);
}
//
void LCDWriteNData(SPI_HandleTypeDef *spi, uint16_t* dataBuf, uint16_t words)
{
	uint8_t preamble[2];
	//Set Preamble for Write data, which is 0x0000
	setWordToArray(preamble, 0x0000);

	uint8_t dataBufTo8bit[words*2];
	int j = 0;
	for(int i=0;i<words;i++)
	{
		dataBufTo8bit[j+1]	= (dataBuf[i] >> 8);
		dataBufTo8bit[j]	=  dataBuf[i] & 0xff;
		j += 2;
	}

	LCDWaitForReady();
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 0);

	HAL_SPI_Transmit(spi, preamble, 1, 1000);

	//Send Data
	LCDWaitForReady();
	HAL_SPI_Transmit(spi, dataBufTo8bit, words, 1000);

	HAL_Delay(wait);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 1);
}

//-----------------------------------------------------------
//Host controller function 4   Read Data from host data Bus
//-----------------------------------------------------------
uint16_t LCDReadData(SPI_HandleTypeDef *spi)
{
	uint8_t rawResult[2];
	uint8_t dummySend[2]= {0x00,0x00};

	//set type and direction
	uint8_t preamble[2];
	//Set Preamble for Read data, which is 0x1000
	setWordToArray(preamble, 0x1000);



	LCDWaitForReady();
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 0);
	HAL_SPI_Transmit(spi, preamble, 1, 1000);

	//Dummy read
	LCDWaitForReady();
	HAL_SPI_TransmitReceive(spi, dummySend, rawResult, 1, 1000);

	LCDWaitForReady();
	//Read Data
	HAL_SPI_Receive(spi, rawResult, 1, 1000);

	HAL_Delay(wait);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 1);

	uint16_t result = rawResult[0] + (rawResult[1] << 8);

	return result;
}

//-------------------------------------------------------------------
//  Read Burst N words Data
//-------------------------------------------------------------------
void LCDReadNData(SPI_HandleTypeDef *spi, uint16_t* resultBuf, uint16_t resultWordCnt)
{
	uint8_t dummySend[resultWordCnt*2];
	for (int i=0; i < resultWordCnt*2;i++) {
		dummySend[i]= 0x00;
	}

	uint8_t rawResult[resultWordCnt*2];
	//set type and direction
	uint8_t preamble[2];
	//Set Preamble for Read data, which is 0x1000
	setWordToArray(preamble, 0x1000);

	LCDWaitForReady();
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 0);

	//Send Preamble before reading data
	HAL_SPI_Transmit(spi, preamble, 1, 1000);

	LCDWaitForReady();
	HAL_SPI_TransmitReceive(spi, dummySend, rawResult, 1, 1000);

	LCDWaitForReady();
	HAL_SPI_TransmitReceive(spi, dummySend, rawResult, resultWordCnt, 1000);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 1);

	int j = 0;
	for (int i= 0; i < resultWordCnt; i++) {
		resultBuf[i] = rawResult[j] + (rawResult[j + 1] << 8);
		j += 2;
	}
}
