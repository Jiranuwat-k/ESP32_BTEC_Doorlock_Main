#ifndef APP_GLOBAL_H
#define APP_GLOBAL_H
    // Include necessary libraries
    #include <SPI.h>
    #include <MFRC522.h>
    #include <NfcAdapter.h>
    #include <WiFi.h>
    #include <AsyncTCP.h>
    #include <ESPAsyncWebServer.h>
    #include <ESPAsyncUpdateServer.h>
    #include <ESPWiFiManager.h>
    #include <LittleFS.h>
    #include "FS.h"
    #include <time.h>
    #include <ESPmDNS.h>
    #include <Ticker.h>
    #include "Config.h"

    void handleReader(int ID, MFRC522 &mfrc522);
    int verifyUID(String uid, String &role, bool isExit = false);
    void updateUserState(String uid, int newState);
    bool isUserInside(String uid);
#endif