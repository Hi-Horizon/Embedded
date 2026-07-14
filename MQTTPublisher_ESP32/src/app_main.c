#include <esp_timer.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_system.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "protocol_examples_common.h"
#include "mqtt.h"
#include "esp_twai.h"
#include "esp_twai_onchip.h"
#include "esp_wifi.h"
#include "buffer.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <network_provisioning/manager.h>
#include <network_provisioning/scheme_ble.h>

const int WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t wifi_event_group;

// CANbus
twai_node_handle_t node_hdl;
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

// MQTT
esp_mqtt_client_handle_t client;

#define CONFIG_BROKER_URI                      "mqtts://7f15879e36cf4f3781ca3df1f338b397.s1.eu.hivemq.cloud:8883"
#define CONFIG_BROKER_BIN_SIZE_TO_SEND         512
#define CONFIG_BROKER_CERTIFICATE_OVERRIDDEN   0

static const char *TAG = "MAIN";

#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
static const uint8_t mqtt_eclipseprojects_io_pem_start[]  = "-----BEGIN CERTIFICATE-----\n"
                                                            CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
#else
extern const uint8_t mqtt_eclipseprojects_io_pem_start[]   asm("_binary_cacert_pem_start");
#endif
extern const uint8_t mqtt_eclipseprojects_io_pem_end[]   asm("_binary_cacert_pem_end");

// CANBus Transmission completion callback
static IRAM_ATTR bool twai_sender_tx_done_callback(twai_node_handle_t handle, const twai_tx_done_event_data_t *edata, void *user_ctx)
{
    if (!edata->is_tx_success) {
        ESP_EARLY_LOGW(TAG, "Failed to transmit message, ID: 0x%X", edata->done_tx_frame->header.id);
    }
    return false; // No task wake required
}

// CANBus error callback
static IRAM_ATTR bool twai_sender_on_error_callback(twai_node_handle_t handle, const twai_error_event_data_t *edata, void *user_ctx)
{
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
        // receive ok, do something here
        for (int i = 0; i < USED_CAN_MESSAGES; i++) {
            if (can_inbox.ids[i] == rx_frame.header.id) {
                memcpy(can_inbox.messages[i], rx_frame.buffer, rx_frame.buffer_len);
                can_inbox.newMsgFlags[i] = true;
            }
        }
    }
    return false;
}

static void CANbus_app_start(void) {

    twai_onchip_node_config_t node_config = {
        .io_cfg = {
            .tx = 10,             // TWAI TX GPIO pin
            .rx = 11,             // TWAI RX GPIO pin
        },
        .bit_timing.bitrate = 125000,  // 200 kbps bitrate
        .tx_queue_depth = 5,        // Transmit queue depth set to 5
        .intr_priority = 0,
        .flags = {
            .enable_self_test = 1,
            .enable_loopback = 0,
        }
    };
    // Create a new TWAI controller driver instance
    ESP_ERROR_CHECK(twai_new_node_onchip(&node_config, &node_hdl));

    twai_event_callbacks_t user_cbs = {
        .on_rx_done = CAN_rx_cb,
        .on_tx_done = twai_sender_tx_done_callback,
        .on_error = twai_sender_on_error_callback,
    };
    ESP_ERROR_CHECK(twai_node_register_event_callbacks(node_hdl, &user_cbs, NULL));

    // Start the TWAI controller
    ESP_ERROR_CHECK(twai_node_enable(node_hdl));
    ESP_LOGI(TAG, "TWAI Sender started successfully");
}

// Retrieves SNTP time
static void obtain_time(void)
{
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    ESP_ERROR_CHECK(esp_netif_sntp_init(&config));

    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to sync time within timeout, TLS handshake may fail");
    } else {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        ESP_LOGI(TAG, "Time synced: %s", asctime(&timeinfo));
        esp_netif_sntp_deinit();
    }

}

//
// Note: this function is for testing purposes only publishing part of the active partition
//       (to be checked against the original binary)
//
static void send_binary(esp_mqtt_client_handle_t client)
{
    esp_partition_mmap_handle_t out_handle;
    const void *binary_address;
    const esp_partition_t *partition = esp_ota_get_running_partition();
    esp_partition_mmap(partition, 0, partition->size, ESP_PARTITION_MMAP_DATA, &binary_address, &out_handle);
    // sending only the configured portion of the partition (if it's less than the partition size)
    int binary_size = MIN(CONFIG_BROKER_BIN_SIZE_TO_SEND, partition->size);
    int msg_id = esp_mqtt_client_publish(client, "topic/binary", binary_address, binary_size, 0, 0);
    ESP_LOGI(TAG, "binary sent with msg_id=%d", msg_id);
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_publish(client, "connection", "esp32 CONNECTED", 0, 0, 0);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d, return code=0x%02x ", event->msg_id, (uint8_t)*event->data);
        // subscription is not needed yet
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        // unsubscription is not needed yet
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        if (strncmp(event->data, "send binary please", event->data_len) == 0) {
            ESP_LOGI(TAG, "Sending the binary");
            send_binary(client);
        }

        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");

        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        } else {
            ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }

        break;

    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}


static void mqtt_app_start(void) {
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = CONFIG_BROKER_URI,
            .verification.certificate = (const char *)mqtt_eclipseprojects_io_pem_start,
        },
        .credentials = {
            .username = "admin",
            .authentication = {
                .password = "H1hrtFTW",
            }
        }
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

static void CANbus_task(void *arg) {
    static const char *TAG = "CANbus";
    CANbus_app_start();

    // esp stat frame
    uint8_t esp_stats_tx_buffer[8];
    twai_frame_t esp_stats_tx_frame = {
        .header.id = 0x751,
        .header.ide = false,
        .header.dlc = 8,
        .buffer = esp_stats_tx_buffer,
        .buffer_len = 8,
    };

    int32_t index = 0;
    while (true) {
        time_t now;
        time(&now);
        uint32_t currentTime = (int32_t) now;

        index = 0;
        buffer_append_uint8(esp_stats_tx_buffer, 0, &index);
        buffer_append_uint8(esp_stats_tx_buffer, 0, &index);
        buffer_append_uint8(esp_stats_tx_buffer, 0, &index);
        buffer_append_uint32(esp_stats_tx_buffer, currentTime, &index);
        buffer_append_uint8(esp_stats_tx_buffer, 0, &index);

        // ESP_ERROR_CHECK(twai_node_transmit(node_hdl, &tx_frame, 500));
        ESP_ERROR_CHECK(twai_node_transmit(node_hdl, &esp_stats_tx_frame, 500));

        // todo: turn this into a util function
        // for (int i = 0; i < USED_CAN_MESSAGES; i++) {
        //     if (can_inbox.newMsgFlags[i]) {
        //         ESP_LOGI(TAG, "RX: %x [%d] %x %x %x %x %x %x %x %x",
        //             can_inbox.ids[i], 8,
        //             can_inbox.messages[i][0], can_inbox.messages[i][1], can_inbox.messages[i][2], can_inbox.messages[i][3],
        //             can_inbox.messages[i][4], can_inbox.messages[i][5], can_inbox.messages[i][6], can_inbox.messages[i][7]
        //         );
        //     }
        // }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* prov_Event handler for catching system events */
static void prov_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
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
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
            esp_wifi_connect();
            break;
        default:
            break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        ESP_LOGI(TAG, "sending wifi connected signal");
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

static void connect_wifi_with_provisioning() {
    ESP_ERROR_CHECK(esp_event_handler_register(NETWORK_PROV_EVENT, ESP_EVENT_ANY_ID, &prov_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_TRANSPORT_BLE_EVENT, ESP_EVENT_ANY_ID, &prov_event_handler, NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_SECURITY_SESSION_EVENT, ESP_EVENT_ANY_ID, &prov_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &prov_event_handler, NULL));

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    network_prov_mgr_config_t config = {
        .scheme = network_prov_scheme_ble,
        .scheme_event_handler = NETWORK_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
    };
    ESP_ERROR_CHECK(network_prov_mgr_init(config));

    bool wifiProvisioned = false;
    // uncomment to always trigger provisioning on startup
    // network_prov_mgr_reset_wifi_provisioning();

    ESP_ERROR_CHECK(network_prov_mgr_is_wifi_provisioned(&wifiProvisioned));
    //
    if (!wifiProvisioned) {
        ESP_LOGI(TAG, "Starting provisioning");
        uint8_t custom_service_uuid[] = {
            /* LSB <---------------------------------------
             * ---------------------------------------> MSB */
            0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
            0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
        };
        // 0 is simply plain text communication.
        network_prov_security_t security = 0;
        network_prov_scheme_ble_set_service_uuid(custom_service_uuid);

        ESP_ERROR_CHECK(network_prov_mgr_start_provisioning(security, nullptr, "Hi-Horizon Bluetooth provisioning", nullptr));
        network_prov_mgr_wait();
        vTaskDelay(100); //wait to make sure all rtos locks are released correctly
        network_prov_mgr_deinit();
    } else {
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");

        /* We don't need the manager as device is already provisioned,
         * so let's release it's resources */
        ESP_ERROR_CHECK(network_prov_mgr_deinit());

        heap_caps_print_heap_info(MALLOC_CAP_8BIT);
        heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);
        heap_caps_print_heap_info(MALLOC_CAP_DMA);

        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &prov_event_handler, NULL));

        // esp_wifi_set_default_wifi_sta_handlers();
        /* Start Wi-Fi in station mode */
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
    }
    // /* Wait for Wi-Fi connection */
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, true, true, portMAX_DELAY);
    esp_wifi_set_ps(WIFI_PS_NONE);
}

static void MQTT_task(void *arg) {
    static const char *TAG = "MQTT";
    uint8_t msg[512];
    int32_t msgSize = 0;
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
    err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGE(TAG, "NVS failed to initialize");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    connect_wifi_with_provisioning();
    // ESP_ERROR_CHECK(example_connect());

    ESP_LOGI(TAG, "starting init sntp and mqtt");
    obtain_time();
    mqtt_app_start();

    while (true) {
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

void app_main(void)
{
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

    xTaskCreate(CANbus_task, "CANbus_task", 1024, NULL, 0, NULL);
    xTaskCreate(MQTT_task, "MQTT_task", 4096, NULL, 1, NULL);
}
