/*
 * CAN_API.c
 *
 *  Created on: 8 Nov 2024
 *      Author: senne
 */

#include <MTU/CAN_API.h>

FDCAN_TxHeaderTypeDef MpptHeader;
FDCAN_TxHeaderTypeDef GpsHeader;
FDCAN_TxHeaderTypeDef EspHeader;

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

	EspHeader.Identifier 		= 0x751;
	EspHeader.IdType 			= FDCAN_STANDARD_ID;
	EspHeader.TxFrameType 		= FDCAN_DATA_FRAME;
	EspHeader.DataLength 		= FDCAN_DLC_BYTES_2;
	EspHeader.FDFormat			= FDCAN_CLASSIC_CAN;
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

	ind = 0;
	buffer_append_uint8(TxData, data->telemetry.espStatus, &ind);
	buffer_append_uint8(TxData, data->telemetry.internetConnection, &ind);

	HAL_FDCAN_AddMessageToTxFifoQ(hfdcan1, &EspHeader, TxData);
}

