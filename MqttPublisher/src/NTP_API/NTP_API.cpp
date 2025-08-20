#include "NTP_API.h"

void initTime(NTPClient* timeClient, DataFrame* dataFrame, std::function<void()> idleFn) {
  timeClient->begin();
  timeClient->update();
  dataFrame->esp.NTPtime = timeClient->getEpochTime();
  setDateTime(idleFn);
}

uint32_t setDateTime(std::function<void()> idleFn) {
  unsigned long lastIdlePerform = 0;

  // You can use your own timezone, but the exact time is not used at all.
  // Only the date is needed for validating the certificates.
  // status.updateStatus(SYNCING_NTP);
  configTime(TZ_Europe_Amsterdam, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    idleFn();
    delay(100);
    Serial.print(".");
    now = time(nullptr);  
  }
  Serial.println("time synced");

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  return now;
}