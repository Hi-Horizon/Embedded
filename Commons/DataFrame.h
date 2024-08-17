#ifndef INC_DATASET_HHRT__H_
#define INC_DATASET_HHRT__H_

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
        float battery_voltage;
        float battery_current;
        float SOC;

        bool *balancing;
        struct STATUS {
            bool OS;      // BMS online status
            bool LSS;     // Load switch status (0 - OFF, 1 - ON)
            bool CSS;     // Charger switch status (0 - OFF, 1 - ON)
        } status;
        float min_cel_voltage;
        float max_cel_voltage;

        float bms_temp;
        float cells_temp;
        float env_temp;
        uint32_t last_msg;
} BMS_data;

typedef struct
{
        float battery_voltage;
        float battery_current;
        float rpm;

        uint32_t odometer;
        uint8_t controller_temp;
        uint8_t motor_temp;
        uint8_t battery_temp;

        float req_power;
        float power_out;

        uint8_t warning;
        uint8_t failures;

        float capacity;
        float term_voltage;
        uint32_t last_msg;
} Motorcontroller_data;

typedef struct
{
        float voltage;
        uint16_t power;
        float current;

        uint8_t error;
        uint8_t cs;
        uint32_t last_msg;
} MPPT_data;

typedef struct
{
        uint8_t temp;
        bool fans;
        uint32_t last_msg;
} Screen_data;

typedef struct
{
        uint32_t unixTime;
        uint32_t localRuntime;
        uint8_t MTUtemp;

        uint8_t SD_error;

        uint8_t espStatus;
        uint8_t internetConnection;
        uint32_t NTPtime;

        float Pmotor;
        uint32_t strategyRuntime;
        uint32_t last_msg;
} Telemetry_data;

typedef struct
{
        float speed;
        float lat;
        float lng;
        float distance;

        uint8_t fix;
        uint8_t antenna;
        uint32_t last_msg;
} GPS_data;

typedef struct {
	BMS_data bms;
	Motorcontroller_data motor;
	MPPT_data mppt;
	GPS_data gps;
	Telemetry_data telemetry;
	Screen_data display;
} DataFrame;


#endif /* INC_DATASET_HHRT__H_ */
