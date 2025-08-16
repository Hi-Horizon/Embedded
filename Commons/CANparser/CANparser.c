#include "CANparser.h"

// ** Parse a CAN msg and put data into a DataFrame struct **
// 
// uint32_t id:             Id of CAN msg
// const uint8_t *payload:  Pointer to the array with the msg data
// DataFrame *dataset:      Pointer to where the parsed output will be stored
void CAN_parseMessage(uint32_t id, const uint8_t *payload, DataFrame *dataset) 
{
	int32_t ind = 0;
	switch (id)
	{
		//bms cell voltage
		case 0x200:
		{
			ind = 0;
			for (int i = 0; i < 4; i++) {
				dataset->bms.cell_voltage[i] = buffer_get_uint16_rev_endian(payload, &ind)*0.0001f;
			}
			break;
		}
		case 0x201:
		{
			ind = 0;
			for (int i = 4; i < 8; i++) {
				dataset->bms.cell_voltage[i] = buffer_get_uint16_rev_endian(payload, &ind)*0.0001;
			}
			break;
		}
		case 0x202:
		{
			ind = 0;
			for (int i = 8; i < 12; i++) {
				dataset->bms.cell_voltage[i] = buffer_get_uint16_rev_endian(payload, &ind)*0.0001;
			}
			break;
		}
		case 0x203:
		{
			ind = 0;
			dataset->bms.cell_voltage[12] = buffer_get_uint16_rev_endian(payload, &ind)*0.0001;
			dataset->bms.cell_voltage[13] = buffer_get_uint16_rev_endian(payload, &ind)*0.0001;

			dataset->bms.battery_voltage = buffer_get_uint16_rev_endian(payload, &ind)*0.001;
			break;
		}
		//bms current
		case 0x204:
		{
			ind = 0;
			dataset->bms.battery_current 	= (buffer_get_uint16_rev_endian(payload, &ind)*0.01) - 326.7f;
			dataset->bms.charge_current 	= (buffer_get_uint16_rev_endian(payload, &ind)*0.01) - 250.0f;
			break;
		}
		//bms cell temp
		case 0x2A0:
		{
			ind = 0;
			for (int i = 0; i < 4; i++) {
				(dataset->bms.cell_temp[i] = buffer_get_uint16(payload, &ind)*0.01) - 50;
			}
			break;
		}
		case 0x2A1: //balanceTemp
		{
			ind = 0;
			for (int i = 0; i < 2; i++) {
				(dataset->bms.balance_temp[i] = buffer_get_uint16(payload, &ind)*0.01) - 50;
			}
			break;
		}
		case 0x2A2: // balancing
		{
			ind = 0;
			uint16_t isBalancingNum = buffer_get_uint16(payload, &ind);
			generate_bit_list(14, isBalancingNum, dataset->bms.is_Balancing);
		}
		//bms (old)
		case 0x601:
			{
				ind = 0;
				uint16_t cells_balancing = buffer_get_uint16(payload, &ind);
				generate_bit_list(16, cells_balancing, dataset->bms.balancing);
				uint8_t status = buffer_get_uint8(payload, &ind);
				bool status_array[3]; generate_bit_list(3, status, status_array);
				dataset->bms.status.OS = status_array[0];
				dataset->bms.status.LSS = status_array[1];
				dataset->bms.status.CSS = status_array[2];
				dataset->bms.min_cel_voltage = buffer_get_float16(payload, 100, &ind);
				dataset->bms.max_cel_voltage = buffer_get_float16(payload, 100, &ind);
				dataset->bms.last_msg = dataset->telemetry.unixTime;
				break;
			}

		case 0x611:
			{
				ind = 0;
				dataset->bms.battery_voltage = buffer_get_float16(payload, 500, &ind);
				dataset->bms.battery_current = buffer_get_float16(payload, 100, &ind);
				dataset->bms.bms_temp = buffer_get_float16(payload, 100, &ind);
				dataset->bms.SOC = buffer_get_float16(payload, 100, &ind);
				break;
			}
		case 0x621:
			{
				ind = 0;
				dataset->bms.cells_temp   = buffer_get_float8(payload, 1, &ind);
				dataset->bms.env_temp     = buffer_get_float8(payload, 1, &ind);
				break;
			}

		// Motorcontroller
		case 0x14A10191:
			{
				dataset->motor.battery_voltage = ((payload[0]) + 256*(payload[1]))/57.45;
				dataset->motor.battery_current = ((payload[2]) + 256*(payload[3]))/10;
				dataset->motor.rpm = ((payload[4]) + 256*(payload[5]) + 65536*(payload[6]))*10;
				dataset->motor.last_msg = dataset->telemetry.unixTime;
				break;
			}

		case 0x14A10192:
			{
				dataset->motor.odometer = ((payload[0]) + 256*(payload[1]) + 65536*(payload[2]) + 16777216*(payload[3]));
				dataset->motor.controller_temp = payload[4];
				dataset->motor.motor_temp = payload[5];
				dataset->motor.battery_temp = payload[6];
				break;
			}

		case 0x14A10193:
			{
				dataset->motor.req_power = ((payload[0]) + 256*(payload[1]))/10;
				dataset->motor.power_out = ((payload[2]) + 256*(payload[3]))/10;
				dataset->motor.warning = payload[4] + 256*(payload[5]);
				dataset->motor.failures = payload[6] + 256*(payload[7]);
				break;
			}

		case 0x14A10194:
			{
				dataset->motor.capacity = payload[0];
				dataset->motor.term_voltage = ((payload[6]) + 256*(payload[7]))/10;
				break;
			}

		// Telemetry data

		case 0x701:
			{
				ind = 0;
				dataset->gps.distance   = buffer_get_float16(payload, 100, &ind);
				dataset->gps.speed      = buffer_get_float16(payload, 100, &ind);
				dataset->gps.fix		= buffer_get_uint8(payload, &ind);
				dataset->gps.antenna    = buffer_get_uint8(payload, &ind);
				break;
			}

		case 0x702:
			{
				ind = 0;
				dataset->gps.lat = buffer_get_float32(payload, 10000, &ind);
				dataset->gps.lng = buffer_get_float32(payload, 10000, &ind);
				break;
			}

		case 0x711:
			{
				ind = 0;
				dataset->mppt.voltage = buffer_get_float16(payload, 100, &ind);
				dataset->mppt.power   = buffer_get_uint16(payload, &ind);
				dataset->mppt.current = buffer_get_float16(payload, 100, &ind);
				dataset->mppt.error   = buffer_get_uint8(payload, &ind);
				dataset->mppt.cs      = buffer_get_uint8(payload, &ind);
				break;
			}

		case 0x721:
			{
				ind = 0;
				dataset->telemetry.unixTime   = buffer_get_uint32(payload, &ind);
				dataset->telemetry.MTUtemp       = buffer_get_uint8(payload, &ind);
				break;
			}

		case 0x731:
			{
				ind = 0;
				uint8_t status = buffer_get_uint8(payload, &ind);
				bool status_array[8];
				generate_bit_list(8, status, status_array);
				dataset->display.fans = status_array[0];

				dataset->display.temp = buffer_get_uint8(payload, &ind);
				break;
			}

		case 0x741:
			{
				ind = 0;
				dataset->telemetry.strategyRuntime = buffer_get_uint16(payload, &ind);
				dataset->telemetry.Pmotor = buffer_get_float16(payload, 100, &ind);
				break;
			}
		
		// esp 

		case 0x751:
			{
				ind = 0;
				dataset->esp.status = buffer_get_uint8(payload, &ind);
				dataset->esp.mqttStatus = buffer_get_uint8(payload, &ind);
				dataset->esp.internetConnection = buffer_get_uint8(payload, &ind);
  				dataset->esp.NTPtime = buffer_get_uint32(payload, &ind);
				break;
			}

		default:
			{
			break;
			}
	}
}

// ** extracts booleans from an int by performing bitwise operations **
// 
// int len:                 length of the amount of booleans stored
// unsigned long data:      integer to be extracted
// DataFrame *dataset:      Pointer to an output array with all the booleans
void generate_bit_list(int len, unsigned long data, bool* return_array)
{
    for(int i = 0; i < len; i++) {
        return_array[i] = (bool) (((uint32_t) 1) << i & data);
    }
}

// ** extracts a uint16_t from a buffer, the bytes will be read in reverse endian order from the buffer **
// 
// const uint8_t *buffer:   Pointer of the buffer to be read
// int32_t *index:          Pointer to the index from where to start the data extraction
// returns:                 uint16_t with the result from the buffer
uint16_t buffer_get_uint16_rev_endian(const uint8_t *buffer, int32_t *index) {
	uint16_t res = 	((uint16_t) buffer[*index + 1]) << 8 |
					((uint16_t) buffer[*index]);
	*index += 2;
	return res;
}
