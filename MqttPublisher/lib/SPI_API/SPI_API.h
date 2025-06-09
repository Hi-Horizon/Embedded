#ifndef SPI_API__H_
#define SPI_API__H_

#include "Arduino.h"
#include "SPI.h"
#include <SpiControl.h>
#include <SpiConfig.h>
#include <espStatus/espStatus.h>

#define SPI_BUFFER_SIZE 128
uint8_t spi_tx_buf[SPI_BUFFER_SIZE] = {};
uint8_t spi_rx_buf[SPI_BUFFER_SIZE] = {};

//TODO: refactor to current setup
void spi_send_wifiCredentials_to_MTU(DataFrame* dataFrame, espStatus* status, WifiCredentials *wifiCredentials) {
  //send wifiCredentials to MTU
  spi_tx_buf[0] = 3;
  createWiFiCredentialsFrame(wifiCredentials, spi_tx_buf + 1);
  dataFrame->telemetry.espStatus = CONNECTED;
  SPI.beginTransaction(SPISettings(16000000, MSBFIRST, SPI_MODE0));
  for (unsigned long i=0; i < sizeof(spi_tx_buf); i++) {
    spi_rx_buf[i] = SPI.transfer(spi_tx_buf[i]);
  }
  SPI.endTransaction();
}
// callback function, requests dataFrame to be sent by MTU and parses this, 
// sends ESP diagnostics as well
void requestDataframe(DataFrame* dataFrame, WifiCredentials* wifiCredentials) {
  Serial.println("Requesting data from MTU in the mean time...");
  //Send ESP status to MTU through SPI
  createESPInfoFrame(dataFrame, spi_tx_buf + 1);
  dataFrame->telemetry.espStatus = CONNECTED;
  SPI.beginTransaction(SPISettings(16000000, MSBFIRST, SPI_MODE0));
  spi_tx_buf[0] = 1;
  for (unsigned long i=0; i < sizeof(spi_tx_buf); i++) {
    spi_rx_buf[i] = SPI.transfer(spi_tx_buf[i]);
  }
  SPI.endTransaction();
  parseFrame(dataFrame, wifiCredentials, spi_rx_buf, sizeof(spi_tx_buf));
}

#endif