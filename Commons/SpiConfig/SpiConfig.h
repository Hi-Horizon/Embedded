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

typedef struct {
    char ssid[128];
    uint8_t ssidLength;
    char password[128];
    uint8_t passwordLength;
} WifiCredentials;

bool parseFrame(DataFrame *dataFrame, WifiCredentials *wifiCredentials, uint8_t *buf, size_t len);

void createFrame(DataFrame *dataFrame, uint8_t *buf, size_t len);
void createESPInfoFrame(DataFrame *dataFrame, uint8_t *buf);
void createWiFiCredentialsFrame(WifiCredentials *wc, uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif
