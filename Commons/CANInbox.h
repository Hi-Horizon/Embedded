#ifndef INC_CANINBOX__H_
#define INC_CANINBOX__H_

#include <stdint.h>
#include <stdbool.h>

#define USED_CAN_MESSAGES 23

typedef struct
{
    uint32_t ids[USED_CAN_MESSAGES] = {
        0x200,
        0x201,
        0x202,
        0x203,
        0x204,
        0x2A0,
        0x2A1,
        0x2A2,
        0x601,
        0x611,
        0x621,
        0x701,
        0x702,
        0x711,
        0x721,
        0x731,
        0x741,
        0x751,
        0x14A10191,
        0x14A10192,
        0x14A10193,
        0x14A10194,
        0x14A10190
    };

    bool newMsgFlags[USED_CAN_MESSAGES]     = { false };
    uint8_t messages[USED_CAN_MESSAGES][8]  = { 0 };
} CanInbox;


#endif /* INC_CANINBOX__H_ */