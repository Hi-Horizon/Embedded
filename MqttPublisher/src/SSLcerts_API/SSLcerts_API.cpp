#include "SSLcerts_API.h"

void verifyAndInitCerts(BearSSL::CertStore* certStore, BearSSL::WiFiClientSecure* bear) {
  //CERT FILE LOADER INIT
  LittleFS.begin();
  //CHECK CERTS
  int numCerts = certStore->initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
  Serial.printf("Number of CA certs read: %d\n", numCerts);
  if (numCerts == 0) {
    Serial.printf("No certs found. Did you run certs-from-mozilla.py and upload the LittleFS directory before running?\n");
    return; // Can't connect to anything w/o certs!
  }

  // USE CERTS
  // Integrate the cert store with this connection
  bear->setCertStore(certStore);
}