//
// Created by senne on 15/07/2026.
//

#include "NTP.h"

static const char *TAG = "NTP";

void NTP_synced_callback(struct timeval *tv) {
    updateStatus(&espStatus, NTP_STATE_MASK, NTP_SYNCED);

    time_t now;
    struct tm timeInfo;
    time(&now);
    localtime_r(&now, &timeInfo);
    ESP_LOGI(TAG, "Time synced: %s", asctime(&timeInfo));
    esp_netif_sntp_deinit();
}

// Retrieves NTP time
void NTP_fetch_time(void)
{
    // Set timezone
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); //set timezoen europe-amsterdam
    tzset();

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    config.sync_cb = NTP_synced_callback;
    ESP_ERROR_CHECK(esp_netif_sntp_init(&config));
}