#ifndef INC_DATASET_HHRT__H_
#define INC_DATASET_HHRT__H_

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
        float battery_voltage;
        float battery_current;
        float charge_current;
        float SOC;
        
        //kan weg??
        bool *balancing;
        struct STATUS {
            bool OS;      // BMS online status
            bool LSS;     // Load switch status (0 - OFF, 1 - ON)
            bool CSS;     // Charger switch status (0 - OFF, 1 - ON)
        } status;
        //tot hier
        float cell_voltage[14];
        bool is_Balancing[14];
        float cell_temp[4];
        float balance_temp[2];

        float min_cel_voltage;
        float max_cel_voltage;

        float bms_temp;
        float cells_temp; // kan weg
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

        //kan weg??
        float capacity;
        float term_voltage;
        //tot hier
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

//kan helemaal weg?
typedef struct
{
        uint8_t temp;
        bool fans;
        uint8_t requestWifiSetup;
        uint32_t last_msg;
} Screen_data;

typedef struct
{
        uint32_t unixTime;
        uint32_t NTPtime;
        //kan weg
        uint32_t localRuntime;
        
        uint8_t SD_error;
        float Pmotor;
        //kan weg
        uint32_t strategyRuntime;

        uint8_t MTUtemp;
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

typedef struct 
{
        uint8_t status;
        uint8_t mqttStatus;
        uint8_t internetConnection;
        uint32_t NTPtime;
        // 0 = Normal operation
        // 1 = WiFi Setup mode
        uint8_t wifiSetupControl; 
} ESP_data;

enum ESP_STATE {
    ESP_START,
    ESP_REQUESTING_WIFI_CONFIG,
    ESP_WIFI_CONNECT_ATTEMPT,
    ESP_WIFI_PASSWORD_FAILED,
    ESP_WIFI_CONNECT_FAILED,
    ESP_NTP_TIME_SYNC,
    ESP_CONNECTING_BROKER,
    ESP_OPERATING,
    ESP_WIFI_NEW_CONFIG_MODE,
};

typedef struct {
	BMS_data bms;
	Motorcontroller_data motor;
	MPPT_data mppt;
	GPS_data gps;
	Telemetry_data telemetry;
	Screen_data display;
        ESP_data esp;
} DataFrame;


typedef struct {
    char ssid[128];
    uint8_t ssidLength;
    char password[128];
    uint8_t passwordLength;
} WifiCredentials;


#endif /* INC_DATASET_HHRT__H_ */
