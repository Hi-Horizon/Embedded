#ifndef INC_BUFFERa_H_
#define INC_BUFFERa_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

void buffer_append_int8(uint8_t* buffer, int8_t number, int32_t *index);
void buffer_append_uint8(uint8_t* buffer, uint8_t number, int32_t *index);
void buffer_append_int16(uint8_t* buffer, int16_t number, int32_t *index);
void buffer_append_uint16(uint8_t* buffer, uint16_t number, int32_t *index);
void buffer_append_int32(uint8_t* buffer, int32_t number, int32_t *index);
void buffer_append_uint32(uint8_t* buffer, uint32_t number, int32_t *index);
void buffer_append_int64(uint8_t* buffer, int64_t number, int32_t *index);
void buffer_append_uint64(uint8_t* buffer, uint64_t number, int32_t *index);
void buffer_append_float8(uint8_t* buffer, float number, float scale, int32_t *index);
void buffer_append_float16(uint8_t* buffer, float number, float scale, int32_t *index);
void buffer_append_float32(uint8_t* buffer, float number, float scale, int32_t *index);
void buffer_append_double64(uint8_t* buffer, double number, double scale, int32_t *index);
void buffer_append_float32_auto(uint8_t* buffer, float number, int32_t *index);
int8_t buffer_get_int8(const uint8_t *buffer, int32_t *index);
uint8_t buffer_get_uint8(const uint8_t *buffer, int32_t *index);
int16_t buffer_get_int16(const uint8_t *buffer, int32_t *index);
uint16_t buffer_get_uint16(const uint8_t *buffer, int32_t *index);
int32_t buffer_get_int32(const uint8_t *buffer, int32_t *index);
uint32_t buffer_get_uint32(const uint8_t *buffer, int32_t *index);
int64_t buffer_get_int64(const uint8_t *buffer, int32_t *index);
uint64_t buffer_get_uint64(const uint8_t *buffer, int32_t *index);
float buffer_get_float8(const uint8_t *buffer, float scale, int32_t *index);
float buffer_get_float16(const uint8_t *buffer, float scale, int32_t *index);
float buffer_get_float32(const uint8_t *buffer, float scale, int32_t *index);
double buffer_get_double64(const uint8_t *buffer, double scale, int32_t *index);
float buffer_get_float32_auto(const uint8_t *buffer, int32_t *index);

#ifdef __cplusplus
}
#endif

#endif /* INC_BUFFER_H_ */
