/*
 * ICM20984_driver.c
 *
 *  Created on: Mar 27, 2024
 *      Author: senne
 */
#include "MTU/IMU_parsing.h"

void changeUserBank(uint8_t userBank) {
	uint8_t userBankRegValue = 0x00;
	switch(userBank) {
    case USER_BANK_0: {
    	userBankRegValue = REG_BANK_SEL_USER_BANK_0;
  		break;
    }
    case USER_BANK_1: {
    	userBankRegValue = REG_BANK_SEL_USER_BANK_1;
  		break;
    }
    case USER_BANK_2: {
    	userBankRegValue = REG_BANK_SEL_USER_BANK_2;
  		break;
    }
    case USER_BANK_3: {
    	userBankRegValue = REG_BANK_SEL_USER_BANK_3;
  		break;
    }
  }
  writeRegister(REG_BANK_SEL, userBankRegValue);
}

void writeRegister(I2C_HandleTypeDef* hi2c, uint8_t regAddr, uint8_t txLen, uint8_t* txBuf) {
	uint8_t writeMsg[txLen + 1] = {regAddr};
	memcpy(writeMsg + 1, txBuf, txLen);
	HAL_I2C_Master_Transmit(hi2c, IMU_address, regAddr, txLen + 1, 1000);
}

void readRegister(I2C_HandleTypeDef* hi2c, uint8_t regAddr, uint8_t rxLen, uint8_t* rxBuf) {
	HAL_I2C_Master_Transmit(hi2c, IMU_address, regAddr, 1, 1000);
	HAL_I2C_Master_Receive(hi2c, IMU_address, rxBuf, readLen, 1000);
}
