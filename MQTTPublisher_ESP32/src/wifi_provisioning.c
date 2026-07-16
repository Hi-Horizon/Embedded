//
// Created by senne on 15/07/2026.
//

#include "wifi_provisioning.h"

uint8_t custom_service_uuid[] = {
    /* LSB <---------------------------------------
     * ---------------------------------------> MSB */
    0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
    0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
};

// 0 is simply plain text communication.
network_prov_security_t security = 0;

/* prov_Event handler for catching system events */
static void prov_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    static const char *TAG = "WiFi_provision_event";
    if (event_base == NETWORK_PROV_EVENT) {
        switch (event_id) {
        case NETWORK_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;
        case NETWORK_PROV_WIFI_CRED_RECV: {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG, "Received Wi-Fi credentials"
                     "\n\tSSID     : %s\n\tPassword : %s",
                     (const char *) wifi_sta_cfg->ssid,
                     (const char *) wifi_sta_cfg->password);
            break;
        }
        case NETWORK_PROV_WIFI_CRED_FAIL: {
            updateStatus(&espStatus, WIFI_STATE_MASK, WIFI_FAILED);
            network_prov_wifi_sta_fail_reason_t *reason = (network_prov_wifi_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                     "\n\tPlease reset to factory and retry provisioning",
                     (*reason == NETWORK_PROV_WIFI_STA_AUTH_ERROR) ?
                     "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
            break;
        }
        case NETWORK_PROV_WIFI_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning successful");
            break;
        case NETWORK_PROV_END:
            // /* De-initialize manager once provisioning is finished */
            // esp_err_t err = network_prov_mgr_deinit();
            //     ESP_LOGI(TAG, "Skibidi Provisioning completed");
            // if (err != ESP_OK) {
            //     ESP_LOGE(TAG, "Failed to de-initialize provisioning manager: %s", esp_err_to_name(err));
            // }
            // break;
        default:
            break;
        }
    } else if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            updateStatus(&espStatus, WIFI_STATE_MASK, WIFI_CONNECTING);
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            updateStatus(&espStatus, WIFI_STATE_MASK, WIFI_FAILED);
            // only attempted to reconnect if there is no provisioning happening
            if (provisionCMD != 1) {
                ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
                updateStatus(&espStatus, WIFI_STATE_MASK, WIFI_CONNECTING);
                esp_wifi_connect();
            }
            break;
        default:
            break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        ESP_LOGI(TAG, "sending wifi connected signal");
        updateStatus(&espStatus, WIFI_STATE_MASK, WIFI_CONNECTED);
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
    } else if (event_base == PROTOCOMM_TRANSPORT_BLE_EVENT) {
        switch (event_id) {
        case PROTOCOMM_TRANSPORT_BLE_CONNECTED:
            ESP_LOGI(TAG, "BLE transport: Connected!");
            break;
        case PROTOCOMM_TRANSPORT_BLE_DISCONNECTED:
            ESP_LOGI(TAG, "BLE transport: Disconnected!");
            break;
        default:
            break;
        }
    } else if (event_base == PROTOCOMM_SECURITY_SESSION_EVENT) {
        switch (event_id) {
        case PROTOCOMM_SECURITY_SESSION_SETUP_OK:
            ESP_LOGI(TAG, "Secured session established!");
            break;
        case PROTOCOMM_SECURITY_SESSION_INVALID_SECURITY_PARAMS:
            ESP_LOGE(TAG, "Received invalid security parameters for establishing secure session!");
            break;
        case PROTOCOMM_SECURITY_SESSION_CREDENTIALS_MISMATCH:
            ESP_LOGE(TAG, "Received incorrect username and/or PoP for establishing secure session!");
            break;
        default:
            break;
        }
    }
}

void init_wifi(void) {
    static const char *TAG = "WiFi_init";
    ESP_ERROR_CHECK(esp_event_handler_register(NETWORK_PROV_EVENT, ESP_EVENT_ANY_ID, &prov_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_TRANSPORT_BLE_EVENT, ESP_EVENT_ANY_ID, &prov_event_handler, NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_SECURITY_SESSION_EVENT, ESP_EVENT_ANY_ID, &prov_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &prov_event_handler, NULL));

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Start Wi-Fi in station mode */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &prov_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void start_wifi_provisioning() {
    static const char *TAG = "WiFi_Provisioning";

    esp_wifi_disconnect();
    updateStatus(&espStatus, WIFI_STATE_MASK, WIFI_PROVISIONING);

    network_prov_mgr_config_t config = {
        .scheme = network_prov_scheme_ble,
        .scheme_event_handler = NETWORK_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
    };
    ESP_ERROR_CHECK(network_prov_mgr_init(config));

    network_prov_scheme_ble_set_service_uuid(custom_service_uuid);
    ESP_ERROR_CHECK(network_prov_mgr_start_provisioning(security, nullptr, "Hi-Horizon Bluetooth provisioning", nullptr));
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_EVENT);
    while (xEventGroupGetBits(wifi_event_group) != WIFI_CONNECTED_EVENT) {
        // if stop signal from canbus is received, stop provisioning and ext function, check if wifi is still intact
        if (provisionCMD == 2) {
            network_prov_mgr_stop_provisioning();
            ESP_LOGI(TAG, "cancelling provision");
            ESP_ERROR_CHECK(esp_wifi_disconnect());
            ESP_ERROR_CHECK(esp_wifi_stop());
            ESP_ERROR_CHECK(esp_wifi_deinit());

            ESP_LOGI(TAG, "WiFi deinit");

            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK(esp_wifi_init(&cfg));

            ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            ESP_ERROR_CHECK(esp_wifi_start());

            ESP_LOGI(TAG, "WiFi init");
            break;
        }
        vTaskDelay(10);
    }
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_EVENT);
    vTaskDelay(100);
    network_prov_mgr_deinit();
    provisionCMD = 0;
}