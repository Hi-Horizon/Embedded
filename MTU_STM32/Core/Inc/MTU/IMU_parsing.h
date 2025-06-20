/*
 * ICM20984_driver.h
 *
 *  Created on: Mar 27, 2024
 *      Author: senne
 */

#ifndef INC_MTU_IMU_PARSING_H_
#define INC_MTU_IMU_PARSING_H_

#include "stm32g4xx_hal.h"
#include <string.h>

#include "DataFrame.h"
#include "IMU_calculations.h"

void readoutIMU(I2C_HandleTypeDef* hi2c, uint8_t* rxBuf, IMU_data* imuData);
void initIMU(I2C_HandleTypeDef* hi2c, uint8_t* rxBuf, IMU_data* imuData);

void resetIMU(I2C_HandleTypeDef* hi2c);
void resetMag(I2C_HandleTypeDef* hi2c);
void toggleSleepIMU(I2C_HandleTypeDef* hi2c);

void IMU_pwr_normal_mode(I2C_HandleTypeDef* hi2c);
void enableAccelGyro(I2C_HandleTypeDef* hi2c);
void configGyro(I2C_HandleTypeDef* hi2c, uint8_t range, uint8_t bandwidth, IMU_data* imuData);
void configAccel(I2C_HandleTypeDef* hi2c, uint8_t range, uint8_t bandwidth, IMU_data* imuData);
void configMag(I2C_HandleTypeDef* hi2c);

void configMagReadout(I2C_HandleTypeDef* hi2c, uint8_t* rxBuf);
void enableI2cMaster(I2C_HandleTypeDef* hi2c);
void powerDownMag(I2C_HandleTypeDef* hi2c);

void changeUserBank(I2C_HandleTypeDef* hi2c, uint8_t userBank);
void writeMagRegister(I2C_HandleTypeDef* hi2c, uint8_t subAddress, uint8_t data);
void readMagRegister(I2C_HandleTypeDef* hi2c, uint8_t subRegAddr, uint8_t rxLen, uint8_t* rxBuf);
void writeRegister(I2C_HandleTypeDef* hi2c, uint8_t regAddr, uint8_t txLen, uint8_t* txBuf);
void readRegister(I2C_HandleTypeDef* hi2c, uint8_t regAddr, uint8_t rxLen, uint8_t* rxBuf);

float highAndLowBytesToFloat(uint8_t high, uint8_t low);

#define accBiasX  0.125f
#define accScaleX 1.004f

#define accBiasY  0.162f
#define accScaleY 1.008f

#define accBiasZ -0.002f
#define accScaleZ 0.992f

//i2c address of the ICM-20948
#define IMU_address 0b1101000 << 1
#define IMU_address_xacc_h 0x2D

//i2c address of the magnetometer
#define MAG_address 0x0C
#define MAG_HXL 0x11
#define MAG_DATA_LENGTH 8
#define MAG_CNTL2 0x31
#define MAG_CNTL2_POWER_DOWN 0x00
#define MAG_CNTL2_MODE_100HZ 0x08
#define MAG_CNTL3 0x32
#define MAG_CNTL3_RESET 0x01

#define IMU_sleep_bit 0x20

//register bank 0
#define UB0_PWR_MGMNT_1 0x06
#define UB0_PWR_MGMNT_1_CLOCK_SEL_AUTO 0x01
#define UB0_PWR_MGMNT_1_DEV_RESET 0x80

#define UB0_PWR_MGMNT_2 0x07
#define UB0_PWR_MGMNT_2_SEN_ENABLE 0x00

#define UB0_USER_CTRL 0x03
#define UB0_USER_CTRL_I2C_MST_EN 0x20
#define UB0_EXT_SLV_SENS_DATA_00 0x3B

//register bank 2
#define UB2_ACCEL_CONFIG 0x14
#define UB2_GYRO_CONFIG_1 0x01

//register bank 3
#define UB3_I2C_SLV0_ADDR 0x03
#define UB3_I2C_SLV0_ADDR_READ_FLAG 0x80
#define UB3_I2C_SLV0_REG 0x04
#define UB3_I2C_SLV0_CTRL 0x05
#define UB3_I2C_SLV0_DO 0x06
#define UB3_I2C_SLV0_CTRL_EN 0x80

#define UB3_I2C_MST_CTRL 0x01
#define UB3_I2C_MST_CTRL_CLK_400KHZ 0x07

//register bank
#define IMU_REG_BANK_SEL 0x7F
#define IMU_USER_BANK_0 0x00
#define IMU_USER_BANK_1 0x10
#define IMU_USER_BANK_2 0x20
#define IMU_USER_BANK_3 0x30

//scaling
#define IMU_RAW_SCALING 32767.5f //same value for all sensors
#define G 9.807f
#define piRadians 3.14159265359f/180.0f;
#define magScale 4912.0f/IMU_RAW_SCALING

//---accelconfig macros---
//range
#define ACCEL_RANGE_2G 	0
#define ACCEL_RANGE_4G 	1
#define ACCEL_RANGE_8G 	2
#define ACCEL_RANGE_16G 3

//bandwidth
//first bit sets bypass, bit 5:3 sets sets hz
#define ACCEL_CONFIG_DLPFCFG_1209HZ 0x00
#define ACCEL_CONFIG_DLPFCFG_246HZ 	0x01
#define ACCEL_CONFIG_DLPFCFG_111HZ 	0b00010001
#define ACCEL_CONFIG_DLPFCFG_50HZ  	0b00011001
#define ACCEL_CONFIG_DLPFCFG_24HZ  	0b00100001
#define ACCEL_CONFIG_DLPFCFG_12HZ  	0b00101001
#define ACCEL_CONFIG_DLPFCFG_6HZ   	0b00110001
#define ACCEL_CONFIG_DLPFCFG_473HZ 	0b00111001

//---gyroConfig macros---
//range
#define GYRO_RANGE_250DPS 	0
#define GYRO_RANGE_500DPS	1
#define GYRO_RANGE_1000DPS 	2
#define GYRO_RANGE_2000DPS	3

//bandwidth
//first bit sets bypass, bit 5:3 sets sets hz
#define GYRO_CONFIG_1_DLPFCFG_12106HZ 	0x00
#define	GYRO_CONFIG_1_DLPFCFG_197HZ		0x01
#define GYRO_CONFIG_1_DLPFCFG_152HZ		0b00001001
#define GYRO_CONFIG_1_DLPFCFG_120HZ		0b00010001
#define GYRO_CONFIG_1_DLPFCFG_51HZ		0b00011001
#define GYRO_CONFIG_1_DLPFCFG_24HZ		0b00100001
#define GYRO_CONFIG_1_DLPFCFG_12HZ		0b00101001
#define GYRO_CONFIG_1_DLPFCFG_6HZ		0b00110001
#define GYRO_CONFIG_1_DLPFCFG_361HZ		0b00111001


#endif
