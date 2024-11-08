/*
 * ICM20984_driver.h
 *
 *  Created on: Mar 27, 2024
 *      Author: senne
 */

#define IMU_address 0x68 << 1

#define IMU_address_xacc_l 0x2E

//register bank
#define REG_BANK_SEL 0x7F
#define USER_BANK_0 0x00
#define USER_BANK_1 0x10
#define USER_BANK_2 0x20
#define USER_BANK_3 0x30
