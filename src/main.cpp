#include <Arduino.h>
#include "Config.h"
#include <SPI.h>
#include <MFRC522.h>
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
#include "Filehelper.h"

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522 mfrc522_2(SS2_PIN, RST_PIN);  // Create MFRC522 instance for second reader
AsyncWebServer server(80);
Ticker Tick;
DNSServer dns;
ESPAsyncUpdateServer updateServer(&server, LittleFS);
ESPWiFiManager wifimanager(&server,&dns,LittleFS);

const char* http_username = "admin";
const char* http_password = "admin";
long timezone = 7;
byte daysavetime = 0;

const char* PARAM_INPUT_1 = "uid";
const char* PARAM_INPUT_2 = "role";
const char* PARAM_INPUT_3 = "delete";
const char* PARAM_INPUT_4 = "delete-user";

String inputMessage;
String inputParam;

unsigned long currentMillis = 0;
bool doorActive = false;
unsigned long doorTimer = 0;
const long doorOpenDuration = 8000; // เปิดนาน 8 วินาที

bool successEffectActive = false;
unsigned long successEffectTimer = 0;
const long successEffectDuration = 500;

int deniedBeepState = 0; // 0=นิ่ง, 1-6=ขั้นตอนการดัง
unsigned long deniedBeepTimer = 0;
const long deniedBeepInterval = 100;

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

String processor(const String& var){
  return String("HTTP GET request sent to your ESP on input field (" 
                + inputParam + ") with value: " + inputMessage +
                "<br><a href=\"/\"><button class=\"button button-home\">Return to Home Page</button></a>");
}

void initRFIDReader() {
  mfrc522.PCD_Init();    // Init MFRC522 board.
  mfrc522.PCD_DumpVersionToSerial();
  // MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);	// Show details of PCD - MFRC522 Card Reader details.
	Serial.println(F("Scan PICC to see UID"));
}

void initLittleFS() {
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
        return;
  }
  // If the log.txt file doesn't exist, create a file on the LittleFS and write the header
  File file = LittleFS.open("/log.txt");
  if(!file) {
    Serial.println("log.txt file doesn't exist");
    Serial.println("Creating file...");
    writeFile(LittleFS, "/log.txt", "Date,Time,UID,Role\r\n");
  }
  else {
    Serial.println("log.txt file already exists");  
  }
  file.close();
  // If the users.txt file doesn't exist, create a file on the LittleFS and write the header
  file = LittleFS.open("/users.txt");
  if(!file) {
    Serial.println("users.txt file doesn't exist");
    Serial.println("Creating file...");
    writeFile(LittleFS, "/users.txt", "UID,Role\r\n");
  }
  else {
    Serial.println("users.txt file already exists");  
  }
  file.close();
}
void initWifi() {
  // Connect to Wi-Fi
  wifimanager.on(WM_CONFIG,[](){

  });

  wifimanager.on(WM_ASYNC_CONFIG,[](){
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    Tick.attach_ms(200,[](){
      digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
    });
  });
  wifimanager.on(WM_SUCCESS,[](){
    if (MDNS.begin("rfid-doorlock")) {
      Serial.println("MDNS started");
    }
    WiFi.hostname("rfid-doorlock");
    Serial.println("Success!");
    Tick.detach();
    digitalWrite(LED_BUILTIN,HIGH);
  });
  updateServer.begin("/update", "Admin", "Admin");
  wifimanager.begin("Manager");
  
  updateServer.on(UPDATE_BEGIN, [](const OTA_UpdateType type, int &result){
      Serial.print("Update Begin: ");
      Serial.println(type == OTA_UpdateType::OTA_FIRMWARE ? "Firmware" : "Filesystem");
  });
  updateServer.on(UPDATE_END, [](const OTA_UpdateType type, int &result){
      Serial.print("Update End. Result: ");
      Serial.println(result == OTA_UpdateResult::OTA_UPDATE_OK ? "OK" : "Error");
  });
  updateServer.begin("/update" "Admin", "Admin");
}

void initTime() {
  Serial.println("CInitializing Time");
  struct tm tmstruct;
  delay(2000);
  tmstruct.tm_year = 0;
  getLocalTime(&tmstruct, 5000);
  Serial.printf(
    "Time and Date right now is : %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct.tm_year) + 1900, (tmstruct.tm_mon) + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min,
    tmstruct.tm_sec
  );
}

// void initSDCard() {
//   // CS pin = 15
//   if (!SD.begin(15)) {
//     Serial.println("Card Mount Failed");
//     return;
//   }
//   uint8_t cardType = SD.cardType();

//   if (cardType == CARD_NONE) {
//     Serial.println("No SD card attached");
//     return;
//   }

//   Serial.print("SD Card Type: ");
//   if (cardType == CARD_MMC) {
//     Serial.println("MMC");
//   } else if (cardType == CARD_SD) {
//     Serial.println("SDSC");
//   } else if (cardType == CARD_SDHC) {
//     Serial.println("SDHC");
//   } else {
//     Serial.println("UNKNOWN");
//   }

//   uint64_t cardSize = SD.cardSize() / (1024 * 1024);
//   Serial.printf("SD Card Size: %lluMB\n", cardSize);

//   // If the log.txt file doesn't exist, create a file on the SD card and write the header
//   File file = SD.open("/log.txt");
//   if(!file) {
//     Serial.println("log.txt file doesn't exist");
//     Serial.println("Creating file...");
//     writeFile(SD, "/log.txt", "Date,Time,UID,Role\r\n");
//   }
//   else {
//     Serial.println("log.txt file already exists");  
//   }
//   file.close();

//   // If the users.txt file doesn't exist, create a file on the SD card and write the header
//   file = SD.open("/users.txt");
//   if(!file) {
//     Serial.println("users.txt file doesn't exist");
//     Serial.println("Creating file...");
//     writeFile(SD, "/users.txt", "UID,Role\r\n");
//   }
//   else {
//     Serial.println("users.txt file already exists");  
//   }
//   file.close();
// }

void handlePeripherals() {
  currentMillis = millis();

  // ตรวจสอบเวลาประตู (ถ้าเปิดอยู่ และเวลาครบกำหนด ให้ปิด)
  if (doorActive && (currentMillis - doorTimer >= doorOpenDuration)) {
    digitalWrite(DOORLOCK_PIN, DOORLOCK_OFF);
    doorActive = false;
    Serial.println("Door Locked (Time out)");
  }

  // ตรวจสอบ Effect ผ่าน (เสียง beep ยาว + LED)
  if (successEffectActive && (currentMillis - successEffectTimer >= successEffectDuration)) {
    digitalWrite(BUZZER_PIN, BUZZER_OFF);
    digitalWrite(LED_STA_PIN, LED_STA_OFF);
    successEffectActive = false;
  }

  // ตรวจสอบ Effect ไม่ผ่าน (เสียง beep สั้นๆ 3 ครั้ง)
  if (deniedBeepState > 0 && (currentMillis - deniedBeepTimer >= deniedBeepInterval)) {
    deniedBeepTimer = currentMillis; // รีเซ็ตเวลา
    switch (deniedBeepState) {
      case 1: digitalWrite(BUZZER_PIN, BUZZER_ON); digitalWrite(LED_STA_PIN, LED_STA_ON); break;
      case 2: digitalWrite(BUZZER_PIN, BUZZER_OFF);  digitalWrite(LED_STA_PIN, LED_STA_OFF);  break;
      case 3: digitalWrite(BUZZER_PIN, BUZZER_ON); digitalWrite(LED_STA_PIN, LED_STA_ON); break;
      case 4: digitalWrite(BUZZER_PIN, BUZZER_OFF);  digitalWrite(LED_STA_PIN, LED_STA_OFF);  break;
      case 5: digitalWrite(BUZZER_PIN, BUZZER_ON); digitalWrite(LED_STA_PIN, LED_STA_ON); break;
      case 6: digitalWrite(BUZZER_PIN, BUZZER_OFF);  digitalWrite(LED_STA_PIN, LED_STA_OFF);  break;
    }
    deniedBeepState++;
    if (deniedBeepState > 7) deniedBeepState = 0; // จบการทำงาน
  }
}

void setup() {
  Serial.begin(115200);  // Initialize serial communication
  while (!Serial);       // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4).
  SPI.begin();
  initRFIDReader();
  initLittleFS();
  initWifi();
  configTime(3600 * timezone, daysavetime * 3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
  initTime();
  // initSDCard();
  if (!MDNS.begin("rfid-doorlock")) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started : rfid-doorlock.local");
    MDNS.addService("http", "tcp", 80);
  }
  pinMode(LED_STA_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(DOORLOCK_PIN, OUTPUT);
  digitalWrite(LED_STA_PIN, LED_STA_OFF);
  digitalWrite(BUZZER_PIN, BUZZER_OFF);
  digitalWrite(DOORLOCK_PIN, DOORLOCK_OFF);
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->authenticate(http_username, http_password)) 
      return request->requestAuthentication();
    request->send(LittleFS, "/full-log.html");
  });
  // Route for root /add-user web page
  server.on("/add-user", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->authenticate(http_username, http_password)) 
      return request->requestAuthentication();
    request->send(LittleFS, "/add-user.html");
  });
  // Route for root /manage-users web page
  server.on("/manage-users", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->authenticate(http_username, http_password)) 
      return request->requestAuthentication();
    request->send(LittleFS, "/manage-users.html");
  });

  // Loads the log.txt file
  server.on("/view-log", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->authenticate(http_username, http_password)) 
      return request->requestAuthentication();
    request->send(LittleFS, "/log.txt", "text/plain", false);
  });
  // Loads the users.txt file
  server.on("/view-users", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->authenticate(http_username, http_password)) 
      return request->requestAuthentication();
    request->send(LittleFS, "/users.txt", "text/plain", false);
  });
  
  // Receive HTTP GET requests on <ESP_IP>/get?input=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if(!request->authenticate(http_username, http_password)) 
      return request->requestAuthentication();
    // GET input1 and input2 value on <ESP_IP>/get?input1=<inputMessage1>&input2=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = String(PARAM_INPUT_1);
      inputMessage += " " + request->getParam(PARAM_INPUT_2)->value();
      inputParam += " " + String(PARAM_INPUT_2);

      String finalMessageInput = String(request->getParam(PARAM_INPUT_1)->value()) + "," + String(request->getParam(PARAM_INPUT_2)->value());
      appendUserFile(LittleFS, "/users.txt", finalMessageInput.c_str());
    }
    else if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputParam = String(PARAM_INPUT_3);
      if(request->getParam(PARAM_INPUT_3)->value()=="users") {
        deleteFile(LittleFS, "/users.txt");
      }
      else if(request->getParam(PARAM_INPUT_3)->value()=="log") {
        deleteFile(LittleFS, "/log.txt");
      }
    }
    else if (request->hasParam(PARAM_INPUT_4)) {
      inputMessage = request->getParam(PARAM_INPUT_4)->value();
      inputParam = String(PARAM_INPUT_4);
      deleteLineFromFile(LittleFS, "/users.txt", inputMessage.toInt());
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    request->send(LittleFS, "/get.html", "text/html", false, processor);
  });

   // Serve Static files
  server.serveStatic("/style.css", LittleFS, "/style.css");
  // server.serveStatic("/style.css", LittleFS, "/style.css").setCacheControl("max-age=600");
  // Start server
  server.begin();
}

void loop() {
  handlePeripherals();
	// Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
	if (!mfrc522.PICC_IsNewCardPresent()) {
		return;
	}

	// Select one of the cards.
	if (!mfrc522.PICC_ReadCardSerial()) {
		return;
	}

  // Save the UID on a String variable
  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      uidString += "0"; 
    }
    uidString += String(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.print("Card UID: ");
  Serial.println(uidString);

  String role = getRoleFromFile(LittleFS, "/users.txt", uidString);
  if (role != "") {
    Serial.println("Access Granted");
    Serial.print("Role for UID: ");
    Serial.print(uidString);
    Serial.print(" is ");
    Serial.println(role);
    digitalWrite(BUZZER_PIN, BUZZER_ON);
    digitalWrite(LED_STA_PIN, LED_STA_ON);
    digitalWrite(DOORLOCK_PIN, DOORLOCK_ON);
    doorActive = true;
    doorTimer = millis();
    successEffectActive = true;
    successEffectTimer = millis();
  } else {
    role = "unknown";
    Serial.println("Access Denied");
    Serial.print("UID: ");
    Serial.print(uidString);
    Serial.println(" not found, set user role to unknown");
    deniedBeepState = 1;
    deniedBeepTimer = millis();

  }
  String sdMessage = uidString + "," + role;
  appendFile(LittleFS, "/log.txt", sdMessage.c_str());
  digitalWrite(LED_STA_PIN, LED_STA_ON);
  delay(500);
  digitalWrite(LED_STA_PIN, LED_STA_OFF);
  mfrc522.PICC_HaltA();

  mfrc522.PCD_StopCrypto1();
}