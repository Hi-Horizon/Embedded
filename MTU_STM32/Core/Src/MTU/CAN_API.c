/*
 * CAN_API.c
 *
 *  Created on: 8 Nov 2024
 *      Author: senne
 */

#include <MTU/CAN_API.h>

FDCAN_TxHeaderTypeDef MpptHeader;
FDCAN_TxHeaderTypeDef GpsHeader;
FDCAN_TxHeaderTypeDef WiFiCredentialsHeader;
FDCAN_TxHeaderTypeDef WiFiConfigModeControl;

void setCanTxHeaders() {
	MpptHeader.Identifier 		= 0x711;
	MpptHeader.IdType 			= FDCAN_STANDARD_ID;
	MpptHeader.TxFrameType 		= FDCAN_DATA_FRAME;
	MpptHeader.DataLength 		= FDCAN_DLC_BYTES_8;
	MpptHeader.FDFormat			= FDCAN_CLASSIC_CAN;

	GpsHeader.Identifier 		= 0x701;
	GpsHeader.IdType 			= FDCAN_STANDARD_ID;
	GpsHeader.TxFrameType 		= FDCAN_DATA_FRAME;
	GpsHeader.DataLength 		= FDCAN_DLC_BYTES_6;
	GpsHeader.FDFormat			= FDCAN_CLASSIC_CAN;

	WiFiCredentialsHeader.Identifier 		= 0x753;
	WiFiCredentialsHeader.IdType 			= FDCAN_STANDARD_ID;
	WiFiCredentialsHeader.TxFrameType 		= FDCAN_DATA_FRAME;
	WiFiCredentialsHeader.DataLength 		= FDCAN_DLC_BYTES_8;
	WiFiCredentialsHeader.FDFormat			= FDCAN_CLASSIC_CAN;

	WiFiConfigModeControl.Identifier 		= 0x754;
	WiFiConfigModeControl.IdType 			= FDCAN_STANDARD_ID;
	WiFiConfigModeControl.TxFrameType 		= FDCAN_DATA_FRAME;
	WiFiConfigModeControl.DataLength 		= FDCAN_DLC_BYTES_8;
	WiFiConfigModeControl.FDFormat			= FDCAN_CLASSIC_CAN;
}

void sendToCan(FDCAN_HandleTypeDef* hfdcan1, DataFrame* data) {
	uint8_t TxData[8];
	int32_t ind = 0;
	buffer_append_float16(TxData, data->gps.distance, 100, &ind);
	buffer_append_float16(TxData,   data->gps.speed, 100, &ind);
	buffer_append_uint8(TxData,   data->gps.fix, &ind);
	buffer_append_uint8(TxData,   data->gps.antenna, &ind);

	HAL_FDCAN_AddMessageToTxFifoQ(hfdcan1, &GpsHeader, TxData);

	ind = 0;
	buffer_append_float16(TxData,  data->mppt.voltage, 100, &ind);
	buffer_append_uint16(TxData,  data->mppt.power, &ind);
	buffer_append_float16(TxData,  data->mppt.current, 100, &ind);
	buffer_append_uint8(TxData,    data->mppt.error, &ind);
	buffer_append_uint8(TxData,    data->mppt.cs, &ind);

	HAL_FDCAN_AddMessageToTxFifoQ(hfdcan1, &MpptHeader, TxData);
}

void sendWiFiCredentialsBuf(FDCAN_HandleTypeDef* hfdcan1, uint8_t* buf, uint8_t length) {
	uint8_t txBuf[8];
	uint8_t index 	= 0;
	uint8_t seq 	= 0;
	while (index <= length - 7) {
		txBuf[0] = seq;
		memcpy(txBuf+1, buf, 7);
		HAL_Delay(50);
		HAL_FDCAN_AddMessageToTxFifoQ(hfdcan1, &WiFiCredentialsHeader, txBuf);
		buf = buf + 7;
		index += 7;
		seq++;
	}
	uint8_t remainder = length % 7;
	if (remainder != 0) {
		memset(txBuf, 0, 8);

		txBuf[0] = seq;
		memcpy(txBuf+1, buf, remainder);
		HAL_Delay(50);
		HAL_FDCAN_AddMessageToTxFifoQ(hfdcan1, &WiFiCredentialsHeader, txBuf);
	}
	memset(txBuf, 0, 8);
	HAL_Delay(50);
	HAL_FDCAN_AddMessageToTxFifoQ(hfdcan1, &WiFiCredentialsHeader, txBuf);
}

void toggleWifiConfigMode(FDCAN_HandleTypeDef* hfdcan1) {
	uint8_t txBuf[8] = {};
	HAL_FDCAN_AddMessageToTxFifoQ(hfdcan1, &WiFiConfigModeControl, txBuf);
}

uint8_t listenForWiFiCredentialsCan(uint32_t id, uint8_t* rxData, uint8_t* sdBuf, uint32_t* bufLength, bool* msgCompleteFlag, uint8_t* seq) {
	if (id == 0x753) { // canBus response id
		// if msg is completely empty, transfer is complete
		*msgCompleteFlag = true;
		for (int i = 0; i < 8; i++ ) {
		  if (rxData[i] == 0) continue;
		  else *msgCompleteFlag = false;
		}
		if (*msgCompleteFlag) return 1; // transfer complete

		if (*seq != rxData[0]) return 0; // out of sequence, this should retrigger a new request

		memcpy(sdBuf + 7*rxData[0], rxData + 1, 7); // cpy characters from data into buf
		*seq = *seq + 1;
	}
	return 1;
}

