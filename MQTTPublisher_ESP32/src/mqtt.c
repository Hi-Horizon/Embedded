//
// Created by senne on 09/07/2026.
//

#include "mqtt.h"



// // we komen er wel achter of deze nodig is
// // Note: this function is for testing purposes only publishing part of the active partition
// //       (to be checked against the original binary)
// //
// static void send_binary(esp_mqtt_client_handle_t client)
// {
//     esp_partition_mmap_handle_t out_handle;
//     const void *binary_address;
//     const esp_partition_t *partition = esp_ota_get_running_partition();
//     esp_partition_mmap(partition, 0, partition->size, ESP_PARTITION_MMAP_DATA, &binary_address, &out_handle);
//     // sending only the configured portion of the partition (if it's less than the partition size)
//     int binary_size = MIN(CONFIG_BROKER_BIN_SIZE_TO_SEND, partition->size);
//     int msg_id = esp_mqtt_client_publish(client, "topic/binary", binary_address, binary_size, 0, 0);
//     ESP_LOGI(TAG, "binary sent with msg_id=%d", msg_id);
// }

// build a MQTT message by concatenating all new CANbus messages, format is as follows:
// 4 bytes: canid
// 8 bytes: payload data
// 2 bytes: crc
// returns: size of message
int32_t buildCanDataMQTTMessage(CanInbox* canInbox, uint8_t* msg) {
    int32_t index = 0;
    uint16_t crc = 0;

    for (int i = 0; i < USED_CAN_MESSAGES; i++) {
        if (canInbox->newMsgFlags[i]) {
            // add id to message
            buffer_append_uint32((uint8_t*) msg, canInbox->ids[i], &index);
            // add data to message
            memcpy(msg + index, canInbox->messages[i], 8);
            index += 8;
            canInbox->newMsgFlags[i] = false;
            crc = calcCRC16((uint8_t*) msg + index - 12, 12, CRC_POLYNOMIAL, 0x0, 0x0, false, false, 100);
            buffer_append_uint16((uint8_t*) msg, crc, &index);
        }
    }

    return index;
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
    static const char *TAG = "MQTT_event";
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

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
            // send_binary(client);
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

void mqtt_app_start(esp_mqtt_client_handle_t *client) {
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
    *client = esp_mqtt_client_init(&mqtt_cfg);

    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(*client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(*client);
}

