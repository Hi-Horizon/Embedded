/*
 * ICM20984_driver.h
 *
 *  Created on: Mar 27, 2024
 *      Author: senne
 */

#ifndef INC_MTU_IMU_PARSING_H_
#define INC_MTU_IMU_PARSING_H_

#include "stm32g4xx_hal.h"

void changeUserBank(uint8_t userBank);
void writeRegister(I2C_HandleTypeDef* hi2c, uint8_t regAddr, uint8_t txLen, uint8_t* txBuf);
void readRegister(I2C_HandleTypeDef* hi2c, uint8_t regAddr, uint8_t rxLen, uint8_t* rxBuf);

//i2c address of the ICM-20948
#define IMU_address 0b1101000 << 1
#define IMU_address_xacc_l 0x2E

//register bank
#define REG_BANK_SEL 0x7F
#define USER_BANK_0 0x00
#define USER_BANK_1 0x10
#define USER_BANK_2 0x20
#define USER_BANK_3 0x30
