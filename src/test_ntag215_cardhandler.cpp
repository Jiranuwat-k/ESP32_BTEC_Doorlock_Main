#include <Arduino.h>
#include "Config.h"
#include <SPI.h>
#include <MFRC522.h>
#include <NfcAdapter.h>

MFRC522 mfrc522(SS_PIN, RST_PIN);
NfcAdapter nfc = NfcAdapter(&mfrc522);
// ส่วนประกาศ Function
void dumpNtag215Data();
void checkAuthenticity();
void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("--- NTAG215 Advanced Handler Setup ---");
    SPI.begin();
    mfrc522.PCD_Init();
    nfc.begin();
    delay(4);
    mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
    mfrc522.PCD_DumpVersionToSerial();
    Serial.println(F("Scan NTAG215 to Dump and Decode..."));
}

void loop() {
    if (nfc.tagPresent())
    {
        delay(500);
        Serial.println(F("======================================="));
        NfcTag tag = nfc.read();
        Serial.print("Tag UID: ");
        Serial.println(tag.getUidString());
        Serial.print("Tag Type: ");
        Serial.println(tag.getTagType());
        Serial.print("Is Formatted: ");
        Serial.println(tag.isFormatted() ? "Yes" : "No");
        Serial.print("Has NDEF Message: ");
        Serial.println(tag.hasNdefMessage() ? "Yes" : "No");
        if (tag.hasNdefMessage())
        {
            NdefMessage message = tag.getNdefMessage();
            Serial.print("NDEF Message contains ");
            Serial.print(message.getRecordCount());
            Serial.println(" records.");
            for (int i = 0; i < message.getRecordCount(); i++)
            {
                NdefRecord record = message.getRecord(i);
                Serial.print("Record ");
                Serial.print(i + 1);
                Serial.print(": TNF=");
                Serial.print(record.getTnf());
                Serial.print(", Type Length=");
                Serial.print(record.getTypeLength());
                Serial.print(", Payload Length=");
                Serial.println(record.getPayloadLength());
                Serial.print("Payload: ");
                const byte* payload = record.getPayload();
                for (unsigned int j = 0; j < record.getPayloadLength(); j++)
                {
                    Serial.print((char)payload[j]);
                }
                Serial.println();
                // Further decoding based on record type can be added here
            }
        }
        checkAuthenticity();
        Serial.println("=======================================");
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
    }

    // if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    //     Serial.println(F("\n======================================="));
    //     Serial.println(F("CARD DETECTED!"));
    //     delay(500);
    //     // 1. แสดงข้อมูลดิบทั้งหมด
    //     // dumpNtag215Data();
        
    //     Serial.println(F("======================================="));
    // }
}

void dumpNtag215Data() {
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.print(F("Card Type: "));
    Serial.println(mfrc522.PICC_GetTypeName(piccType));
    Serial.println(F("Page  Data (HEX)           ASCII"));
    Serial.println(F("---------------------------------------"));

    byte buffer[18];
    byte size = sizeof(buffer);
    for (byte page = 0; page < 135; page += 4) {
        int retries = 0; bool success = false;
        while (retries < 3 && !success) {
            size = sizeof(buffer);
            if (mfrc522.MIFARE_Read(page, buffer, &size) == mfrc522.STATUS_OK) success = true;
            else { retries++; delay(10); }
        }
        if (!success) break;

        for (byte i = 0; i < 4; i++) {
            byte currentPage = page + i;
            if (currentPage > 134) break;
            Serial.printf("%03d  ", currentPage);
            for (byte j = 0; j < 4; j++) Serial.printf("%02X ", buffer[i * 4 + j]);
            Serial.print(F("  | "));
            for (byte j = 0; j < 4; j++) {
                byte b = buffer[i * 4 + j];
                Serial.print(isPrintable(b) ? (char)b : '.');
            }
            Serial.println();
        }
    }
    Serial.println(F("---------------------------------------"));
}

void checkAuthenticity() {
    byte command[2] = {0x3C, 0x00}; // คำสั่ง READ_SIG ของ NTAG
    byte response[20]; // รับข้อมูลกลับมา
    byte size = sizeof(response);

    // ส่งคำสั่งแบบระดับต่ำ (Low-level communication)
    MFRC522::StatusCode status = mfrc522.PCD_TransceiveData(command, 2, response, &size);

    if (status == mfrc522.STATUS_OK) {
        Serial.println(F("✅ Signature found! This is likely a genuine NXP chip."));
        Serial.print(F("Signature bytes: "));
        for (byte i = 0; i < size; i++) Serial.printf("%02X ", response[i]);
        Serial.println();
    } else {
        Serial.println(F("❌ Signature NOT found. This might be a Clone/Fake chip."));
    }
}