#ifndef INC_CANINBOX__H_
#define INC_CANINBOX__H_

#include <stdint.h>
#include <stdbool.h>

#define USED_CAN_MESSAGES 23

typedef struct
{
    uint32_t ids[USED_CAN_MESSAGES];

    bool newMsgFlags[USED_CAN_MESSAGES];
    uint8_t messages[USED_CAN_MESSAGES][8];
} CanInbox;


#endif /* INC_CANINBOX__H_ */