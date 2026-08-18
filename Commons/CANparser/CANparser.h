#ifndef __CAN_HHRT_H_
#define __CAN_HHRT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <buffer/buffer.h>
#include <stdint.h>
#include <DataFrame.h>

void CAN_parseMessage(uint32_t id, const uint8_t *payload, DataFrame *dataset, uint32_t receiveTime);
void generate_bit_list(int len, unsigned long data, bool* return_array);
uint16_t buffer_get_uint16_rev_endian(const uint8_t *buffer, int32_t *index);

#ifdef __cplusplus
}
#endif

#endif
