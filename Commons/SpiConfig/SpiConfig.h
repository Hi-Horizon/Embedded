#ifndef INC_SPICONFIGa_H_
#define INC_SPICONFIGa_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <buffer/buffer.h>
#include <stdint.h>
#include <DataFrame.h>

#define SpiHeaderByte 172
#define SpiTrailerByte 69
#define SpiFlagByte 74
#define RESPONSE_SIZE 14
#define MSG_MAX_SIZE = 32

void dataFrameInBuf(DataFrame *dataFrame, uint8_t *buf);
void dataFrameFromBuf(DataFrame *dataFrame, uint8_t *buf);
void constructESPInfo(DataFrame *dataFrame, uint8_t *buf);
void parseESPInfo(DataFrame *dataFrame, uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif
