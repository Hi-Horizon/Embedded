/*
 * Eink_driver_image_generator.c
 *
 *  Created on: 13 May 2023
 *      Author: senne
 */
#include "../../Eink_interface/Inc/Eink_driver.h"

const char* getCharPixelArray(char c) {
	switch (c) {
	case '0':
		return pixel0;
		break;
	case '1':
		return pixel1;
		break;
	case '2':
		return pixel2;
		break;
	case '3':
		return pixel3;
		break;
	case '4':
		return pixel4;
		break;
	case '5':
		return pixel5;
		break;
	case '6':
		return pixel6;
		break;
	case '7':
		return pixel7;
		break;
	case '8':
		return pixel8;
		break;
	case '9':
		return pixel9;
		break;
	case 'a':
	case 'A':
		return pixelA;
		break;
	case 'b':
	case 'B':
		return pixelB;
		break;
	case 'c':
	case 'C':
		return pixelC;
		break;
	case 'd':
	case 'D':
		return pixelD;
		break;
	case 'e':
	case 'E':
		return pixelE;
		break;
	case 'f':
	case 'F':
		return pixelF;
		break;
	case 'g':
	case 'G':
		return pixelG;
		break;
	case 'h':
	case 'H':
		return pixelH;
		break;
	case 'i':
	case 'I':
		return pixelI;
		break;
	case 'j':
	case 'J':
		return pixelJ;
		break;
	case 'k':
	case 'K':
		return pixelK;
		break;
	case 'l':
	case 'L':
		return pixelL;
		break;
	case 'm':
	case 'M':
		return pixelM;
		break;
	case 'n':
	case 'N':
		return pixelN;
		break;
	case 'o':
	case 'O':
		return pixelO;
		break;
	case 'p':
	case 'P':
		return pixelP;
		break;
	case 'q':
	case 'Q':
		return pixelQ;
		break;
	case 'r':
	case 'R':
		return pixelR;
		break;
	case 's':
	case 'S':
		return pixelS;
		break;
	case 't':
	case 'T':
		return pixelT;
		break;
	case 'u':
	case 'U':
		return pixelU;
		break;
	case 'v':
	case 'V':
		return pixelV;
		break;
	case 'w':
	case 'W':
		return pixelW;
		break;
	case 'x':
	case 'X':
		return pixelX;
		break;
	case 'y':
	case 'Y':
		return pixelY;
		break;
	case 'z':
	case 'Z':
		return pixelZ;
		break;
	default:
		return pixelEmpty;
	}
}

uint16_t frameGenerateString(ImageLoadSettings renderSettings, uint16_t* frameBuf, char* stringBuf, int stringSize, int scale) {
	uint16_t width	= 8*stringSize*scale + 2*(stringSize-1)*scale; 	//width of character times amount and scale + whitespace times scale minus end
	uint16_t height = 7*scale;

	char* stringInArray[stringSize];

	//fetch array pointers needed for given String
	for (int i=0; i<stringSize; i++) {
		stringInArray[i] = (char*)getCharPixelArray(stringBuf[i]);
	}

	int frameIndex=0;
	for(int j=0; j<7; j++) {
		//one row of String
		for (int i=0; i<stringSize; i++) {
			char charLine = stringInArray[i][j];
			char mask = 0b00000001;

			//one row of a character
			for (int k=0 ; k<8; k++) {
				if ((charLine & mask) == 0) {
					frameBuf[frameIndex] = 0x00;
				}
				else {
					frameBuf[frameIndex] = 0xFF;
				}
				mask = mask << 1;
				frameIndex++;
			}
			//whitespace
			for (int k=0; k<2; k++) {
				frameBuf[frameIndex] = 0x00;
				frameIndex++;
			}
		}
	}
	renderSettings.xWidth  = width;
	renderSettings.yHeight = height;
	return frameIndex;
}

