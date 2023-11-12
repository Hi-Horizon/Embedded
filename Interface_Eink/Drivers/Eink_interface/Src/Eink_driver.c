/*
 * Eink_driver.c
 *
 *  Created on: Apr 22, 2023
 *      Author: senne
 */
#include "../../Eink_interface/Inc/Eink_driver.h"

void getIT8951Info(SPI_HandleTypeDef *spi, IT8951Info *container) {
	uint16_t rawData[20];

	LCDWriteCmdCode(spi, 0x0302);
	LCDReadNData(spi,rawData,20);
	container->screenWidth 	= rawData[0];
	container->screenHeight = rawData[1];

	container->imageBufferAddress 	= rawData[2] | (rawData[3] << 16);

	int j=0;
	for (int i = 4; i <= 11; i++) {
		container->IT8951version[j] 	= (rawData[i] >> 8);
		container->IT8951version[j + 1] = (rawData[i] & 0xff);
		j += 2;
	}

	j = 0;
	for (int i = 12; i <= 19; i++) {
		container->LUTversion[j] 	 = (rawData[i] >> 8);
		container->LUTversion[j + 1] = (rawData[i] & 0xff);
		j += 2;
	}
}

uint16_t readScreenRegister(SPI_HandleTypeDef *spi, uint16_t address) {
	LCDWriteCmdCode(spi, 0x0010);
	LCDWriteData(spi, address);
	return LCDReadData(spi);
}

void writeScreenRegister(SPI_HandleTypeDef *spi, uint16_t address, uint16_t value)
{
	//I80 Mode
	//Send Cmd , Register Address and Write Value
	LCDWriteCmdCode(spi, IT8951_TCON_REG_WR);
	LCDWriteData(spi, address);
	LCDWriteData(spi, value);
}

void IT8951WaitForDisplayReady(SPI_HandleTypeDef *spi)
{
	//Check IT8951 Register LUTAFSR => NonZero - Busy, 0 - Free
	while(readScreenRegister(spi, LUTAFSR));
}

void IT8951SetImgBufBaseAddr(SPI_HandleTypeDef *spi, uint16_t ulImgBufAddr)
{
	uint16_t usWordH = (uint16_t)((ulImgBufAddr >> 16) & 0x0000FFFF);
	uint16_t usWordL = (uint16_t)( ulImgBufAddr & 0x0000FFFF);
	//Write LISAR Reg
	writeScreenRegister(spi, LISAR + 2 ,usWordH);
	writeScreenRegister(spi, LISAR ,usWordL);
}

void LCDSendCmdArg(SPI_HandleTypeDef *spi, uint16_t cmd,uint16_t* args, uint16_t numArgs)
{
     //Send Cmd code
     LCDWriteCmdCode(spi, cmd);
     //Send Data
     for(uint16_t i=0;i<numArgs;i++)
     {
         LCDWriteData(spi, args[i]);
     }
}

void IT8951LoadImgAreaStart(SPI_HandleTypeDef *spi, ImageLoadSettings* settings)
{
	uint16_t usArg[5];
    //Setting Argument for Load image start
    usArg[0] = (settings->endianType << 8 )
    |(settings->pixelFormat << 4)
    |(settings->rotation);
    usArg[1] = settings->xBegin;
    usArg[2] = settings->xWidth;
    usArg[3] = settings->yBegin;
    usArg[4] = settings->yHeight;
    //Send Cmd and Args
    LCDSendCmdArg(spi, IT8951_TCON_LD_IMG_AREA , usArg , 5);
}

void IT8951LoadImgEnd(SPI_HandleTypeDef *spi)
{
    LCDWriteCmdCode(spi, IT8951_TCON_LD_IMG_END);
}

void IT8951HostAreaPackedPixelWrite(SPI_HandleTypeDef *spi, IT8951Info *screenValues, ImageLoadSettings* settings, uint16_t* FrameBuf)
{
	//Source buffer address of Host
	uint16_t* pusFrameBuf = (uint16_t*)FrameBuf;

	//Set Image buffer(IT8951) Base address
	IT8951SetImgBufBaseAddr(spi, screenValues->imageBufferAddress);
	//Send Load Image start Cmd
	IT8951LoadImgAreaStart(spi, settings);
	//Host Write Data
	for(uint16_t j=0;j< settings->yHeight;j++)
	{
		 for(uint16_t i=0;i< settings->xWidth/2;i++)
			{
					//Write a Word(2-Bytes) for each time
					LCDWriteData(spi ,*pusFrameBuf);
					pusFrameBuf++;
			}
	}
	//Send Load Img End Command
	IT8951LoadImgEnd(spi);
}

void IT8951DisplayArea(SPI_HandleTypeDef *spi, ImageLoadSettings* settings, uint16_t displayMode)
{
	//Send I80 Display Command (User defined command of IT8951)
	LCDWriteCmdCode(spi, USDEF_I80_CMD_DPY_AREA); //0x0034
	//Write arguments
	LCDWriteData(spi, settings->xBegin);
	LCDWriteData(spi, settings->yBegin);
	LCDWriteData(spi, settings->xWidth);
	LCDWriteData(spi, settings->yHeight);
	LCDWriteData(spi, displayMode);
}




