//
// Created by senne on 15/07/2026.
//

#include "NTP.h"

// Retrieves NTP time
void NTP_fetch_time(void)
{
    static const char *TAG = "NTP";
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    ESP_ERROR_CHECK(esp_netif_sntp_init(&config));

    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK) {
        ESP_LOGW(TAG, "Time Sync took longer than 10s, function will no longer block");
    } else {
        setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); //set timezoen europe-amsterdam
        tzset();
        time_t now;
        struct tm timeInfo;
        time(&now);
        localtime_r(&now, &timeInfo);
        ESP_LOGI(TAG, "Time synced: %s", asctime(&timeInfo));
        esp_netif_sntp_deinit();
    }

}