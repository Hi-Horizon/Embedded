#include <esp_timer.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "mqtt.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "NTP.h"
#include "CANbus.h"
#include "wifi_provisioning.h"

EventGroupHandle_t wifi_event_group;

// 0 = idle, 1 = enter provision mode, 2 leave provision mode
uint8_t provisionCMD = 0;

CanInbox can_inbox = {
    .ids = {
        0x200, 0x201, 0x202, 0x203, 0x204,
        0x2A0, 0x2A1, 0x2A2,
        0x601, 0x611, 0x621,
        0x701, 0x702, 0x711, 0x721, 0x731, 0x741, 0x751,
        0x14A10191, 0x14A10192, 0x14A10193, 0x14A10194, 0x14A10190
    },
    .newMsgFlags = { false },   // rest zero-initialized
    .messages    = { { 0 } }        // rest zero-initialized
};

// CANBus Transmission completion callback
static IRAM_ATTR bool twai_sender_tx_done_callback(twai_node_handle_t handle, const twai_tx_done_event_data_t *edata, void *user_ctx)
{
    static const char *TAG = "CANbus";
    if (!edata->is_tx_success) {
        ESP_EARLY_LOGW(TAG, "Failed to transmit message, ID: 0x%X", edata->done_tx_frame->header.id);
    }
    return false; // No task wake required
}

// CANBus error callback
static IRAM_ATTR bool twai_sender_on_error_callback(twai_node_handle_t handle, const twai_error_event_data_t *edata, void *user_ctx)
{
    static const char *TAG = "CANbus";
    ESP_EARLY_LOGW(TAG, "TWAI node error: 0x%x", edata->err_flags.val);
    return false; // No task wake required
}

// Callback function for CANbus rx_receive, puts message in CANinbox
static IRAM_ATTR bool CAN_rx_cb(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx)
{
    uint8_t recv_buff[8];
    twai_frame_t rx_frame = {
        .buffer = recv_buff,
        .buffer_len = sizeof(recv_buff),
    };
    if (ESP_OK == twai_node_receive_from_isr(handle, &rx_frame)) {
        if (rx_frame.header.id == 0x754) {
            provisionCMD = rx_frame.buffer[0];
        }
        for (int i = 0; i < USED_CAN_MESSAGES; i++) {
            if (can_inbox.ids[i] == rx_frame.header.id) {
                memcpy(can_inbox.messages[i], rx_frame.buffer, rx_frame.buffer_len);
                can_inbox.newMsgFlags[i] = true;
            }
        }
    }
    return false;
}

static void CANbus_task(void *arg) {
    // static const char *TAG = "CANbus_task";

    twai_node_handle_t node_hdl;
    twai_event_callbacks_t user_cbs = {
        .on_rx_done = CAN_rx_cb,
        .on_tx_done = twai_sender_tx_done_callback,
        .on_error = twai_sender_on_error_callback,
    };
    CANbus_app_start(&node_hdl, &user_cbs);

    // esp stat frame
    uint8_t esp_stats_tx_buffer[8];
    twai_frame_t esp_stats_tx_frame = {
        .header.id = 0x751,
        .header.ide = false,
        .header.dlc = 8,
        .buffer = esp_stats_tx_buffer,
        .buffer_len = 8,
    };

    while (true) {
        //get current time
        time_t now;
        time(&now);
        uint32_t currentTime = (int32_t) now;

        buildEspStatusFrame(esp_stats_tx_buffer, currentTime);
        ESP_ERROR_CHECK(twai_node_transmit(node_hdl, &esp_stats_tx_frame, 500));

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void MQTT_task(void *arg) {
    static const char *TAG = "MQTT_task";
    // MQTT
    esp_mqtt_client_handle_t client;

    uint8_t msg[512];
    int32_t msgSize = 0;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    init_wifi();

    ESP_LOGI(TAG, "starting init sntp and mqtt");
    NTP_fetch_time();
    mqtt_app_start(&client);

    while (true) {
        if (provisionCMD == 1) {
            start_wifi_provisioning();
            esp_mqtt_client_reconnect(client);
        }
        //if there is at least 1 new CAN message, send all new can messages over MQTT
        portDISABLE_INTERRUPTS();
        msgSize = buildCanDataMQTTMessage(&can_inbox, msg);
        portENABLE_INTERRUPTS();
        if (msgSize > 0) { // only send if there are new messages
            esp_mqtt_client_publish(client, "data", (const char*)msg, msgSize, 0, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

#define MQTT_TASK_STACK_SIZE   3584
#define CANBUS_TASK_STACK_SIZE 1024   // bumped from 1024 - too small even before this change

static StackType_t mqtt_task_stack[MQTT_TASK_STACK_SIZE];
static StaticTask_t mqtt_task_tcb;

static StackType_t canbus_task_stack[CANBUS_TASK_STACK_SIZE];
static StaticTask_t canbus_task_tcb;

void app_main(void)
{
    static const char *TAG = "MAIN";

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    // esp_log_level_set("*", ESP_LOG_INFO);
    // esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("mbedtls", ESP_LOG_DEBUG);
    esp_log_level_set("esp-tls", ESP_LOG_DEBUG);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    wifi_event_group = xEventGroupCreate();

    xTaskCreateStatic(CANbus_task, "CANbus_task", CANBUS_TASK_STACK_SIZE, NULL, 0, canbus_task_stack, &canbus_task_tcb);
    xTaskCreateStatic(MQTT_task, "MQTT_task", MQTT_TASK_STACK_SIZE, NULL, 1, mqtt_task_stack, &mqtt_task_tcb);
}
