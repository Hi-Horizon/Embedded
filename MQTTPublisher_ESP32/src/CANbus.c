//
// Created by senne on 15/07/2026.
//

#include "CANbus.h"

#include <esp_log.h>

#include "buffer.h"

static const char *TAG = "CANbus";

void CANbus_app_start(twai_node_handle_t *node_hdl, twai_event_callbacks_t *user_cbs) {
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
    ESP_ERROR_CHECK(twai_new_node_onchip(&node_config, node_hdl));
    ESP_ERROR_CHECK(twai_node_register_event_callbacks(*node_hdl, user_cbs, NULL));

    // Start the TWAI controller
    ESP_ERROR_CHECK(twai_node_enable(*node_hdl));
    ESP_LOGI(TAG, "TWAI Sender started successfully");
}

void buildEspStatusFrame(uint8_t *buf, uint8_t espStatus, uint32_t currentTime) {
    int32_t index = 0;
    buffer_append_uint8(buf, espStatus, &index);
    buffer_append_uint8(buf, 0, &index);
    buffer_append_uint8(buf, 0, &index);
    buffer_append_uint32(buf, currentTime, &index);
    buffer_append_uint8(buf, 0, &index);
}
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