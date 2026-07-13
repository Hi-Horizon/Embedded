//
// Created by senne on 09/07/2026.
//

#include "mqtt.h"

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
