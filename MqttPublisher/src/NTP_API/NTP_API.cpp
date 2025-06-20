#include "NTP_API.h"

void initTime(NTPClient* timeClient, DataFrame* dataFrame) {
  timeClient->begin();
  timeClient->update();
  dataFrame->telemetry.NTPtime = timeClient->getEpochTime();
  setDateTime();
}

uint32_t setDateTime() {
  // You can use your own timezone, but the exact time is not used at all.
  // Only the date is needed for validating the certificates.
  // status.updateStatus(SYNCING_NTP);
  configTime(TZ_Europe_Amsterdam, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(100);
    Serial.print(".");
    now = time(nullptr);  
  }
  Serial.println("time synced");

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  return now;
}