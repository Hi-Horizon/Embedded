/*
 * ICM20984_driver.c
 *
 *  Created on: Mar 27, 2024
 *      Author: senne
 */
#include "MTU/IMU_parsing.h"

void readoutIMU(I2C_HandleTypeDef* hi2c, uint8_t* rxBuf, IMU_data* imuData) {
	changeUserBank(hi2c, IMU_USER_BANK_0);
	readRegister(hi2c, IMU_address_xacc_h, 20, rxBuf);

	imuData->accelx = accScaleX*(highAndLowBytesToFloat(rxBuf[0], rxBuf[1])	* imuData->accScale) + accBiasX;
	imuData->accely = accScaleY*(highAndLowBytesToFloat(rxBuf[2], rxBuf[3])	* imuData->accScale) + accBiasY;
	imuData->accelz = accScaleZ*(highAndLowBytesToFloat(rxBuf[4], rxBuf[5])	* imuData->accScale) + accBiasZ;
	imuData->gyrox 	= highAndLowBytesToFloat(rxBuf[6], rxBuf[7])	* imuData->gyroScale + 0.022;
	imuData->gyroy 	= highAndLowBytesToFloat(rxBuf[8], rxBuf[9])	* imuData->gyroScale + 0.007;
	imuData->gyroz 	= highAndLowBytesToFloat(rxBuf[10], rxBuf[11])	* imuData->gyroScale - 0.01;
//	_tcounts = (((int16_t)_buffer[12]) << 8) | _buffer[13]; temperature is not needed (yet)
	imuData->magx 	= highAndLowBytesToFloat(rxBuf[14], rxBuf[15]) * magScale;
	imuData->magy 	= highAndLowBytesToFloat(rxBuf[16], rxBuf[17]) * -magScale;
	imuData->magz 	= highAndLowBytesToFloat(rxBuf[18], rxBuf[19]) * -magScale;

	calculatePitchRollYaw(imuData);
	calculateCompassDirection(imuData);
}

float highAndLowBytesToFloat(uint8_t high, uint8_t low) {
	int16_t bytesCombined = (((int16_t) high) << 8) | low;
	return (float) bytesCombined;
}

void configAccel(I2C_HandleTypeDef* hi2c, uint8_t range, uint8_t bandwidth, IMU_data* imuData) {
	changeUserBank(hi2c, IMU_USER_BANK_2);

	//possible register values are 0x00 0x02 0x04 0x06
	uint8_t accelRangeRegValue = 2 * range;

	float rangeScaler = (float) (2 << range);
	imuData->accScale = (G * rangeScaler) /IMU_RAW_SCALING;

	uint8_t configVal = (accelRangeRegValue | bandwidth);
	writeRegister(hi2c, UB2_ACCEL_CONFIG, 1, &configVal);
}

void configGyro(I2C_HandleTypeDef* hi2c, uint8_t range, uint8_t bandwidth, IMU_data* imuData) {
  changeUserBank(hi2c, IMU_USER_BANK_2);

  //possible register values are 0x00 0x02 0x04 0x06
  uint8_t gyroConfigRegValue = 2 * range;

  float rangeScaler = (float) (1 << range);
  imuData->gyroScale = (250.0f * rangeScaler) / IMU_RAW_SCALING * piRadians;

  uint8_t configVal = (gyroConfigRegValue | bandwidth);
  writeRegister(hi2c, UB2_GYRO_CONFIG_1, 1, &configVal);
}

void initIMU(I2C_HandleTypeDef* hi2c, uint8_t* rxBuf, IMU_data* imuData) {
	IMU_pwr_normal_mode(hi2c);
	enableI2cMaster(hi2c);
	powerDownMag(hi2c);

	resetIMU(hi2c);
	HAL_Delay(100);
	resetMag(hi2c);
	HAL_Delay(100);

	IMU_pwr_normal_mode(hi2c);
	enableAccelGyro(hi2c);
	configAccel(hi2c, ACCEL_RANGE_16G, ACCEL_CONFIG_DLPFCFG_246HZ, imuData);
	configGyro(hi2c, GYRO_RANGE_2000DPS, GYRO_CONFIG_1_DLPFCFG_197HZ, imuData);

	enableI2cMaster(hi2c);
	readMagRegister(hi2c, 0x00, 2, rxBuf);
	configMag(hi2c);

	configMagReadout(hi2c, rxBuf);
}

void resetIMU(I2C_HandleTypeDef* hi2c) {
	uint8_t resetCMD = UB0_PWR_MGMNT_1_DEV_RESET;
  	changeUserBank(hi2c, IMU_USER_BANK_0);
  	writeRegister(hi2c, UB0_PWR_MGMNT_1, 1, &resetCMD);
}

void toggleSleepIMU(I2C_HandleTypeDef* hi2c) {
	uint8_t pwr_mgmt_1_val = 0;
	changeUserBank(hi2c, IMU_USER_BANK_0);
	readRegister(hi2c, UB0_PWR_MGMNT_1, 1, &pwr_mgmt_1_val);
	uint8_t pwr_mgmt_1_new = pwr_mgmt_1_val ^ IMU_sleep_bit;
	writeRegister(hi2c, UB0_PWR_MGMNT_1, 1, &pwr_mgmt_1_new);
}

void IMU_pwr_normal_mode(I2C_HandleTypeDef* hi2c) {
	uint8_t opCmd = UB0_PWR_MGMNT_1_CLOCK_SEL_AUTO;
	changeUserBank(hi2c, IMU_USER_BANK_0);
	writeRegister(hi2c, UB0_PWR_MGMNT_1, 1, &opCmd);
}


void enableAccelGyro(I2C_HandleTypeDef* hi2c) {
	changeUserBank(hi2c, IMU_USER_BANK_0);
	uint8_t enableCMD = UB0_PWR_MGMNT_2_SEN_ENABLE;
	writeRegister(hi2c, UB0_PWR_MGMNT_2, 1, &enableCMD);
}


void powerDownMag(I2C_HandleTypeDef* hi2c) {
	writeMagRegister(hi2c, MAG_CNTL2, MAG_CNTL2_POWER_DOWN);
}

void resetMag(I2C_HandleTypeDef* hi2c) {
	writeMagRegister(hi2c, MAG_CNTL3, MAG_CNTL3_RESET); //reset mag
}

void configMag(I2C_HandleTypeDef* hi2c) {
	writeMagRegister(hi2c, MAG_CNTL2, MAG_CNTL2_MODE_100HZ);
}

void enableI2cMaster(I2C_HandleTypeDef* hi2c) {
	uint8_t i2cMSTenCMD = UB0_USER_CTRL_I2C_MST_EN;
	uint8_t i2cMSTclkCMD = UB3_I2C_MST_CTRL_CLK_400KHZ;

	changeUserBank(hi2c, IMU_USER_BANK_0);
	writeRegister(hi2c, UB0_USER_CTRL, 1, &i2cMSTenCMD);

	changeUserBank(hi2c, IMU_USER_BANK_3);
	writeRegister(hi2c, UB3_I2C_MST_CTRL, 1, &i2cMSTclkCMD);
}

void configMagReadout(I2C_HandleTypeDef* hi2c, uint8_t* rxBuf) {
	readMagRegister(hi2c, MAG_HXL, MAG_DATA_LENGTH, rxBuf);
}

void changeUserBank(I2C_HandleTypeDef* hi2c, uint8_t userBank) {
	writeRegister(hi2c, IMU_REG_BANK_SEL, 1, &userBank);
}

void writeMagRegister(I2C_HandleTypeDef* hi2c, uint8_t subAddress, uint8_t data) {
	uint8_t magI2cAddr = MAG_address;
	uint8_t i2cEnableCmd = (UB3_I2C_SLV0_CTRL_EN | 0x01);

	changeUserBank(hi2c, IMU_USER_BANK_3);
	writeRegister(hi2c, UB3_I2C_SLV0_ADDR, 1, &magI2cAddr);
	// set the register to the desired magnetometer sub address
	writeRegister(hi2c, UB3_I2C_SLV0_REG, 1, &subAddress);
	// store the data for write
	writeRegister(hi2c, UB3_I2C_SLV0_DO, 1, &data);
	HAL_Delay(100);
	// enable I2C and send 1 byte
	writeRegister(hi2c, UB3_I2C_SLV0_CTRL, 1, &i2cEnableCmd);
}

void readMagRegister(I2C_HandleTypeDef* hi2c, uint8_t subRegAddr, uint8_t rxLen, uint8_t* rxBuf) {
	uint8_t setReadFromMag = MAG_address | UB3_I2C_SLV0_ADDR_READ_FLAG;
	uint8_t requestbytesCmd = UB3_I2C_SLV0_CTRL_EN | rxLen;

	changeUserBank(hi2c, IMU_USER_BANK_3);
	writeRegister(hi2c, UB3_I2C_SLV0_ADDR, 1 , &setReadFromMag);

	// set the register to the desired magnetometer sub address
	writeRegister(hi2c, UB3_I2C_SLV0_REG, 1, &subRegAddr);

	// enable I2C and request the bytes
	writeRegister(hi2c, UB3_I2C_SLV0_CTRL, 1 , &requestbytesCmd);

	// takes some time for these registers to fill
	HAL_Delay(100);
	// read the bytes off the ICM-20948 EXT_SLV_SENS_DATA registers
	readRegister(hi2c, UB0_EXT_SLV_SENS_DATA_00, rxLen, rxBuf);

	changeUserBank(hi2c, IMU_USER_BANK_0);
}

void writeRegister(I2C_HandleTypeDef* hi2c, uint8_t regAddr, uint8_t txLen, uint8_t* txBuf) {
	uint8_t writeMsg[txLen + 1];
	writeMsg[0] = regAddr;
	memcpy(writeMsg + 1, txBuf, txLen);
	HAL_I2C_Master_Transmit(hi2c, IMU_address, writeMsg, txLen + 1, 1000);
}

void readRegister(I2C_HandleTypeDef* hi2c, uint8_t regAddr, uint8_t rxLen, uint8_t* rxBuf) {
	HAL_I2C_Master_Transmit(hi2c, IMU_address, &regAddr, 1, 1000);
	HAL_I2C_Master_Receive(hi2c, IMU_address, rxBuf, rxLen, 1000);
}
