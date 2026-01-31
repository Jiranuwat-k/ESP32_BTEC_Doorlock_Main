#include <Arduino.h>
#include "Config.h"
#include "Global.h"
#include "Filehelper.h"
#include "index.h"

MFRC522 mfrc522_IN(SS_PIN, RST_PIN);
MFRC522 mfrc522_OUT(SS2_PIN, RST_PIN);
AsyncWebServer server(80);

// --- AUTHENTICATION GLOBALS ---
String adminSessionUID = ""; // Stores UID of currently logged in admin
String adminSessionRole = ""; // Stores readable role name (MainKey / Admin)

// --- WEB SESSION GLOBALS (New) ---
String webSessionToken = ""; 
const char* SESSION_COOKIE_NAME = "ESPSESSIONID";

// --- ANTI-PASSBACK ---
// We need to keep track of user state in memory for speed, but also in file for persistence
// For this simple implementation, we check file on every scan (might be slow) or load to RAM?
// Given ESP32, RAM is okay for small user base. Let's use File for persistence.
#define STATE_OUTSIDE 0
#define STATE_INSIDE 1

bool checkUserWebAuth(AsyncWebServerRequest *request) {
  if (request->hasHeader("Cookie")) {
    String cookie = request->header("Cookie");
    if (cookie.indexOf(SESSION_COOKIE_NAME + String("=") + webSessionToken) != -1 && webSessionToken != "") {
      return true;
    }
  }
  return false;
}

unsigned long adminSessionTimer = 0;
const unsigned long ADMIN_SESSION_TIMEOUT = 300000; // 5 Minutes auto logout
bool waitingForAdminLogin = false;
unsigned long waitLoginTimer = 0;

Ticker Tick;
DNSServer dns;
ESPAsyncUpdateServer updateServer(&server, LittleFS);
ESPWiFiManager wifimanager(&server, &dns, LittleFS);

uint8_t mfrc522_in_version = 0;
uint8_t mfrc522_out_version = 0;

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

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void initRFIDReader()
{
  mfrc522_IN.PCD_Init();
  mfrc522_OUT.PCD_Init();
  mfrc522_IN.PCD_DumpVersionToSerial();
  mfrc522_OUT.PCD_DumpVersionToSerial();
  mfrc522_in_version = mfrc522_IN.PCD_ReadRegister(MFRC522::VersionReg);
  mfrc522_out_version = mfrc522_OUT.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.println(F("Scan PICC to see UID"));
}

int initLittleFS()
{
  if (!LittleFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting LittleFS");
    return LittleFS_ERR_MOUNT;
  }
  // 1. readerlogs.txt (Old log.txt)
  File file = LittleFS.open("/readerlogs.txt");
  if (!file)
  {
    Serial.println("readerlogs.txt file doesn't exist");
    Serial.println("Creating file...");
    writeFile(LittleFS, "/readerlogs.txt", "Date,Time,ReaderID,UID,Role,Ref.Verify\n");
  }
  file.close();

  // 2. users.txt
  file = LittleFS.open("/users.txt");
  if (!file)
  {
    Serial.println("users.txt file doesn't exist");
    Serial.println("Creating file...");
    writeFile(LittleFS, "/users.txt", "UID,Role,Prefix,FullNameEN,FullNameTH,Code,Gender,Age,StartDate,ExpireDate\n");
  }
  file.close();

  // 3. userslog.txt
  file = LittleFS.open("/userslog.txt");
  if (!file)
  {
    Serial.println("userslog.txt file doesn't exist");
    Serial.println("Creating file...");
    writeFile(LittleFS, "/userslog.txt", "Date,Time,Event,UID,ModUID\n");
  }
  file.close();

  // 4. systemlog.txt
  file = LittleFS.open("/systemlog.txt");
  if (!file)
  {
    Serial.println("systemlog.txt file doesn't exist");
    Serial.println("Creating file...");
    writeFile(LittleFS, "/systemlog.txt", "Date,Time,Code,Msg\n");
  }
  file.close();

  // 5. usage_stats.txt (New)
  file = LittleFS.open("/usage_stats.txt");
  if (!file)
  {
    Serial.println("usage_stats.txt file doesn't exist");
    Serial.println("Creating file...");
    writeFile(LittleFS, "/usage_stats.txt", "UID,Count,LastAccess\n");
  }
  file.close();

  // 5. usage_stats.txt (New)
  file = LittleFS.open("/usage_stats.txt");
  if (!file)
  {
    Serial.println("usage_stats.txt file doesn't exist");
    Serial.println("Creating file...");
    writeFile(LittleFS, "/usage_stats.txt", "UID,Count,LastAccess\n");
  }
  file.close();

  // 6. user_state.txt (New - Anti-Passback)
  file = LittleFS.open("/user_state.txt");
  if (!file)
  {
    Serial.println("user_state.txt file doesn't exist");
    Serial.println("Creating file...");
    writeFile(LittleFS, "/user_state.txt", "UID,State\n");
  }
  file.close();

  return LittleFS_ERR_OK;
}

void initWifi()
{
  // Connect to Wi-Fi
  wifimanager.on(WM_CONFIG, []() {
  });

  wifimanager.on(WM_ASYNC_CONFIG, [](){
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    Tick.attach_ms(200,[](){
      digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
    }); });
  wifimanager.on(WM_SUCCESS, []()
                 {
    if (MDNS.begin("rfid-doorlock")) {
      Serial.println("MDNS started");
    }
    WiFi.hostname("rfid-doorlock");
    Serial.println("Success!");
    Tick.detach();
    digitalWrite(LED_BUILTIN,HIGH); });
  updateServer.begin("/update", http_username, http_password);
  wifimanager.begin("Manager");

  updateServer.on(UPDATE_BEGIN, [](const OTA_UpdateType type, int &result)
                  {
      Serial.print("Update Begin: ");
      Serial.println(type == OTA_UpdateType::OTA_FIRMWARE ? "Firmware" : "Filesystem"); });
  updateServer.on(UPDATE_END, [](const OTA_UpdateType type, int &result)
                  {
      Serial.print("Update End. Result: ");
      Serial.println(result == OTA_UpdateResult::OTA_UPDATE_OK ? "OK" : "Error"); });
  updateServer.begin("/update", http_username, http_password);
}

void initTime()
{
  Serial.println("CInitializing Time");
  struct tm tmstruct;
  delay(2000);
  tmstruct.tm_year = 0;
  getLocalTime(&tmstruct, 5000);
  Serial.printf(
      "Time and Date right now is : %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct.tm_year) + 1900, (tmstruct.tm_mon) + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min,
      tmstruct.tm_sec);
}

void handlePeripherals()
{
  currentMillis = millis();

  if (doorActive && (currentMillis - doorTimer >= doorOpenDuration)) {
    digitalWrite(DOORLOCK_PIN, DOORLOCK_OFF);
    doorActive = false;
    Serial.println("Door Locked (Time out)");
  }

  if (successEffectActive && (currentMillis - successEffectTimer >= successEffectDuration)) {
    digitalWrite(BUZZER_PIN, BUZZER_OFF);
    digitalWrite(LED_STA_PIN, LED_STA_OFF);
    successEffectActive = false;
  }

  if (deniedBeepState > 0 && (currentMillis - deniedBeepTimer >= deniedBeepInterval)) {
    deniedBeepTimer = currentMillis; 
    switch (deniedBeepState) {
    case 1: digitalWrite(BUZZER_PIN, BUZZER_ON); digitalWrite(LED_STA_PIN, LED_STA_ON); break;
    case 2: digitalWrite(BUZZER_PIN, BUZZER_OFF); digitalWrite(LED_STA_PIN, LED_STA_OFF); break;
    case 3: digitalWrite(BUZZER_PIN, BUZZER_ON); digitalWrite(LED_STA_PIN, LED_STA_ON); break;
    case 4: digitalWrite(BUZZER_PIN, BUZZER_OFF); digitalWrite(LED_STA_PIN, LED_STA_OFF); break;
    case 5: digitalWrite(BUZZER_PIN, BUZZER_ON); digitalWrite(LED_STA_PIN, LED_STA_ON); break;
    case 6: digitalWrite(BUZZER_PIN, BUZZER_OFF); digitalWrite(LED_STA_PIN, LED_STA_OFF); break;
    }
    deniedBeepState++;
    if (deniedBeepState > 7) deniedBeepState = 0; 
  }
}

// *** RESTORED LOGIN HANDLERS (Fix for Logout & Login Page) ***
void setup()
{
  Serial.begin(115200);
  SPI.begin();
  initLittleFS();
  initWifi();
  configTime(3600 * timezone, daysavetime * 3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
  initTime();
  initRFIDReader();

  initRFIDReader();

  // Log Startup with Reason
  esp_reset_reason_t reason = esp_reset_reason();
  String reasonStr;
  switch (reason) {
      case ESP_RST_POWERON: reasonStr = "Power On"; break;
      case ESP_RST_EXT: reasonStr = "External Pin (RST)"; break;
      case ESP_RST_SW: reasonStr = "Software Reset"; break;
      case ESP_RST_PANIC: reasonStr = "Exception/Panic"; break;
      case ESP_RST_INT_WDT: reasonStr = "Interrupt WDT"; break;
      case ESP_RST_TASK_WDT: reasonStr = "Task WDT"; break;
      case ESP_RST_WDT: reasonStr = "Other WDT"; break;
      case ESP_RST_DEEPSLEEP: reasonStr = "Wake Deep Sleep"; break;
      case ESP_RST_BROWNOUT: reasonStr = "Brownout"; break;
      case ESP_RST_SDIO: reasonStr = "SDIO"; break;
      default: reasonStr = "Unknown"; break;
  }

  maintainLogFile(LittleFS, "/systemlog.txt", "Date,Time,Code,Msg");
  appendFile(LittleFS, "/systemlog.txt", (String("INFO,Boot: ") + reasonStr).c_str());

  if (!MDNS.begin("rfid-doorlock")) Serial.println("Error setting up MDNS responder!");
  else { Serial.println("mDNS responder started : rfid-doorlock.local"); MDNS.addService("http", "tcp", 80); }
  
  pinMode(LED_STA_PIN, OUTPUT); pinMode(BUZZER_PIN, OUTPUT); pinMode(DOORLOCK_PIN, OUTPUT);
  digitalWrite(LED_STA_PIN, LED_STA_OFF); digitalWrite(BUZZER_PIN, BUZZER_OFF); digitalWrite(DOORLOCK_PIN, DOORLOCK_OFF);

  // 1. Login Page (GET)
  server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request) {
     String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>Login</title>";
     html += "<style>";
     html += "body{font-family:'Segoe UI',sans-serif;background:#f0f2f5;display:flex;align-items:center;justify-content:center;height:100vh;margin:0;}";
     html += ".login-card{background:white;padding:40px;border-radius:12px;box-shadow:0 8px 24px rgba(0,0,0,0.1);width:100%;max-width:320px;text-align:center;}";
     html += "h2{margin:0 0 20px;color:#1a73e8;}";
     html += "input{width:100%;padding:12px;margin-bottom:15px;border:1px solid #ddd;border-radius:8px;box-sizing:border-box;font-size:16px;}";
     html += "button{width:100%;padding:12px;background:#1a73e8;color:white;border:none;border-radius:8px;font-size:16px;cursor:pointer;font-weight:600;transition:0.3s;}";
     html += "button:hover{background:#1557b0;}";
     html += ".error{color:red;font-size:14px;display:none;margin-bottom:15px;}";
     html += "</style></head><body>";
     html += "<div class='login-card'><h2>Web Console Login</h2>";
     html += "<p class='error' id='err'>Invalid Username or Password</p>";
     html += "<form onsubmit='event.preventDefault(); login();'>";
     html += "<input type='text' id='user' placeholder='Username' required autocomplete='username'>";
     html += "<input type='password' id='pass' placeholder='Password' required autocomplete='current-password'>";
     html += "<button type='submit'>Login</button></form></div>";
     html += "<script>";
     html += "async function login(){";
     html += " document.getElementById('err').style.display='none';";
     html += " let u=document.getElementById('user').value; let p=document.getElementById('pass').value;";
     html += " try{ let res = await fetch('/api/login/web', {method:'POST', body: new URLSearchParams({user:u, pass:p})});";
     html += " if(res.ok){ window.location.href='/'; } else { document.getElementById('err').style.display='block'; }";
     html += " }catch(e){ alert('Network Error'); }}";
     html += "</script></body></html>";
     request->send(200, "text/html", html);
  });

  // 2. Login Process (POST)
  server.on("/api/login/web", HTTP_POST, [](AsyncWebServerRequest *request) {
     String user = request->hasParam("user", true) ? request->getParam("user", true)->value() : "";
     String pass = request->hasParam("pass", true) ? request->getParam("pass", true)->value() : "";
     
     if(user == http_username && pass == http_password) {
        // Generate Token
        webSessionToken = String(millis()) + String(random(1000,9999));
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "OK");
        response->addHeader("Set-Cookie", String(SESSION_COOKIE_NAME) + "=" + webSessionToken + "; Path=/; Max-Age=3600"); 
        request->send(response);
     } else {
        request->send(401, "text/plain", "Invalid Auth");
     }
  });

  // 3. Logout
  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request) {
      webSessionToken = ""; 
      AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
      response->addHeader("Set-Cookie", String(SESSION_COOKIE_NAME) + "=0; Path=/; Max-Age=0");
      response->addHeader("Location", "/login");
      request->send(response);
  });

  // Route for root / web page (SECURED)
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      if(!checkUserWebAuth(request)) { request->redirect("/login"); return; }
      request->send(200, "text/html", index_html); 
  });

  // PROTECT VIEW ROUTES
  auto checkAuth = [](AsyncWebServerRequest *request, const char* file) {
      if(!checkUserWebAuth(request)) return request->send(401);
      request->send(LittleFS, file, "text/plain", false);
  };

  server.on("/view-log", HTTP_GET, [checkAuth](AsyncWebServerRequest *r){ checkAuth(r, "/readerlogs.txt"); });
  server.on("/view-users", HTTP_GET, [checkAuth](AsyncWebServerRequest *r){ checkAuth(r, "/users.txt"); });
  server.on("/view-userslog", HTTP_GET, [checkAuth](AsyncWebServerRequest *r){ checkAuth(r, "/userslog.txt"); });
  server.on("/view-systemlog", HTTP_GET, [checkAuth](AsyncWebServerRequest *r){ checkAuth(r, "/systemlog.txt"); });
  server.on("/view-usage", HTTP_GET, [checkAuth](AsyncWebServerRequest *r){ checkAuth(r, "/usage_stats.txt"); });
  
  // AUTHENTICATION API (Card)
  server.on("/api/login/wait", HTTP_POST, [](AsyncWebServerRequest *request) {
       waitingForAdminLogin = true;
       waitLoginTimer = millis();
       request->send(200, "text/plain", "Waiting for Admin Card...");
  });

  server.on("/api/login/status", HTTP_GET, [](AsyncWebServerRequest *request) {
       // Check timeout
       if(adminSessionUID != "" && millis() - adminSessionTimer > ADMIN_SESSION_TIMEOUT) {
           adminSessionUID = ""; // Auto logout
       }
       String status = adminSessionUID != "" ? "logged_in" : (waitingForAdminLogin ? "waiting" : "logged_out");
       String display = adminSessionUID != "" ? (adminSessionRole + " (" + adminSessionUID + ")") : "";
       String json = "{\"status\":\"" + status + "\", \"uid\":\"" + display + "\"}";
       request->send(200, "application/json", json);
  });

  server.on("/api/login/logout", HTTP_POST, [](AsyncWebServerRequest *request) {
       adminSessionUID = "";
       adminSessionRole = "";
       request->send(200, "text/plain", "Logged Out");
  });

  // Board Info
  server.on("/update/boardinfo", HTTP_GET, [](AsyncWebServerRequest *request) {
       long rssi = WiFi.RSSI();
       uint32_t freeHeap = ESP.getFreeHeap();
       uint32_t totalHeap = ESP.getHeapSize();
       unsigned long uptimeSec = millis() / 1000;
       int d = uptimeSec / 86400; int h = (uptimeSec % 86400) / 3600; int m = (uptimeSec % 3600) / 60; int s = uptimeSec % 60;
       String uptime = String(d) + "d " + String(h) + "h " + String(m) + "m " + String(s) + "s";
       String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>Board Info</title><style>body{font-family:sans-serif;background:#f0f2f5;display:flex;align-items:center;justify-content:center;height:100vh;margin:0}.card{background:white;padding:2rem;border-radius:15px;box-shadow:0 4px 15px rgba(0,0,0,0.1);max-width:400px;width:90%}h2{margin-top:0;color:#333;text-align:center;border-bottom:2px solid #eee;padding-bottom:10px}.item{display:flex;justify-content:space-between;padding:10px 0;border-bottom:1px dashed #eee}.label{color:#666;font-weight:500}.val{font-weight:bold;color:#1a73e8}.btn{display:block;width:100%;padding:10px;text-align:center;background:#1a73e8;color:white;text-decoration:none;border-radius:8px;margin-top:20px;transition:0.3s}.btn:hover{background:#1557b0}</style></head><body><div class='card'><h2>ℹ️ Board Info</h2><div class='item'><span class='label'>Firmware Ver</span><span class='val'>1.0.0</span></div><div class='item'><span class='label'>Free Heap</span><span class='val'>" + String(freeHeap/1024) + " KB</span></div><div class='item'><span class='label'>Total Heap</span><span class='val'>" + String(totalHeap/1024) + " KB</span></div><div class='item'><span class='label'>Uptime</span><span class='val'>" + uptime + "</span></div><div class='item'><span class='label'>WiFi Signal</span><span class='val'>" + String(rssi) + " dBm</span></div><div class='item'><span class='label'>IP Address</span><span class='val'>" + WiFi.localIP().toString() + "</span></div><a href='/' class='btn' onclick='window.close()'>Close</a></div></body></html>";
       request->send(200, "text/html", html);
  });

  // --- API Action Handler (POST) - SECURED ---
  server.on("/api/action", HTTP_POST, [](AsyncWebServerRequest *request) {
      if(!checkUserWebAuth(request)) return request->send(401, "text/plain", "Session Expired"); // Web Auth First
      
      // Smart Card Auth Check
      if(ENABLE_MAINKEY_SYSTEM && adminSessionUID == "") {
          request->send(401, "text/plain", "Unauthorized: Please scan Admin Card");
          return;
      }
      
      adminSessionTimer = millis(); // Refresh Card Session

      String action = request->hasParam("action", true) ? request->getParam("action", true)->value() : "";
      
      String operatorStr;
      if (adminSessionUID != "") {
          operatorStr = adminSessionRole + " (" + adminSessionUID + ")";
      } else {
          operatorStr = "System/Bypass"; // Default for when MainKey System is disabled
      }

      if(action == "clear_log") {
          String type = request->hasParam("type", true) ? request->getParam("type", true)->value() : "";
          
          if(type == "reader") {
              deleteFile(LittleFS, "/readerlogs.txt");
              maintainLogFile(LittleFS, "/systemlog.txt", "Date,Time,Code,Msg");
              appendFile(LittleFS, "/systemlog.txt", (String("INFO,Reader Log Cleared by ") + operatorStr).c_str());
          }
          else if(type == "users") {
              deleteFile(LittleFS, "/userslog.txt");
              maintainLogFile(LittleFS, "/systemlog.txt", "Date,Time,Code,Msg");
              appendFile(LittleFS, "/systemlog.txt", (String("INFO,User Log Cleared by ") + operatorStr).c_str());
          }
          else if(type == "system") {
              if (adminSessionUID != MAIN_KEY_UID) {
                  request->send(403, "text/plain", "Permission Denied: Only MainKey can clear System Logs");
                  return;
              }
              deleteFile(LittleFS, "/systemlog.txt");
          }
          else if(type == "usage") {
              // Only allow clearing if needed, or by admin
              deleteFile(LittleFS, "/usage_stats.txt"); 
              maintainLogFile(LittleFS, "/systemlog.txt", "Date,Time,Code,Msg");
              appendFile(LittleFS, "/systemlog.txt", (String("INFO,Usage Stats Cleared by ") + operatorStr).c_str());
          }
          request->send(200, "text/plain", "Log Cleared by " + operatorStr);
      }
      else if(action == "delete_user") {
          String uid = request->hasParam("uid", true) ? request->getParam("uid", true)->value() : "";
          if(uid != "") {
             deleteUserByUID(LittleFS, "/users.txt", uid);
             maintainLogFile(LittleFS, "/userslog.txt", "Date,Time,Event,UID,ModUID");
             appendFile(LittleFS, "/userslog.txt", (String("D") + "," + operatorStr + "," + uid).c_str());
             request->send(200, "text/plain", "User Deleted");
          } else {
             request->send(400, "text/plain", "Missing UID");
          }
      }
      else if(action == "save_user") {
          String uid = request->hasParam("uid", true) ? request->getParam("uid", true)->value() : "";
          String role = request->hasParam("role", true) ? request->getParam("role", true)->value() : "10";
          if(uid == "") { request->send(400, "text/plain", "Missing UID"); return; }
          if(role == "50" && ENABLE_MAINKEY_SYSTEM && adminSessionUID != MAIN_KEY_UID) {
              request->send(403, "text/plain", "Permission Denied: Only MainKey can create Admins");
              return;
          }
          String prefix = request->hasParam("prefix", true) ? request->getParam("prefix", true)->value() : "XX";
          String fname_en = request->hasParam("fname_en", true) ? request->getParam("fname_en", true)->value() : "-";
          String fname_th = request->hasParam("fname_th", true) ? request->getParam("fname_th", true)->value() : "-";
          String code = request->hasParam("code", true) ? request->getParam("code", true)->value() : "-";
          String gender = request->hasParam("gender", true) ? request->getParam("gender", true)->value() : "-";
          String age = request->hasParam("age", true) ? request->getParam("age", true)->value() : "-";
          String start_date = request->hasParam("start_date", true) ? request->getParam("start_date", true)->value() : "-";
          String end_date = request->hasParam("end_date", true) ? request->getParam("end_date", true)->value() : "-";
          
          fname_en.replace(",", " "); fname_th.replace(",", " "); code.replace(",", " ");
          start_date.replace("T", " "); end_date.replace("T", " ");

          String finalData = uid + "," + role + "," + prefix + "," + fname_en + "," + fname_th + "," + code + "," + gender + "," + age + "," + start_date + "," + end_date;
          String existingUser = getUserDataFromFile(LittleFS, "/users.txt", uid);
          bool isNew = (existingUser == String(FILE_ERR_NOTFOUND) || existingUser == String(FILE_ERR_OPEN));
          upsertUser(LittleFS, "/users.txt", uid, finalData);
          
          // Reset Anti-Passback state to allow immediate re-entry
          updateUserState(uid, STATE_OUTSIDE);

          String evt = isNew ? "C" : "M";
          maintainLogFile(LittleFS, "/userslog.txt", "Date,Time,Event,UID,ModUID");
          appendFile(LittleFS, "/userslog.txt", (evt + "," + operatorStr + "," + uid).c_str());
          request->send(200, "text/plain", "User Saved");
      }
      else if(action == "cleanup_expired") {
          if (!AUTO_CLEANUP_EXPIRED_GUESTS) {
              request->send(403, "text/plain", "Auto Cleanup Disabled in Config");
              return;
          }

          File file = LittleFS.open("/users.txt", "r");
          if (!file) { request->send(500, "text/plain", "File Error"); return; }
          
          File temp = LittleFS.open("/users.temp", "w");
          if (!temp) { file.close(); request->send(500, "text/plain", "Temp File Error"); return; }
          
          int count = 0;
          struct tm tmstruct;
          getLocalTime(&tmstruct, 100);
          char currentStr[20];
          snprintf(currentStr, sizeof(currentStr), "%04d-%02d-%02d %02d:%02d", tmstruct.tm_year+1900, tmstruct.tm_mon+1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min);
          String nowStr = String(currentStr);

          // Keep Header
          String header = file.readStringUntil('\n');
          temp.println(header);

          while (file.available()) {
              String line = file.readStringUntil('\n');
              line.trim();
              if (line.length() == 0) continue;
              
              int c1 = line.indexOf(','); 
              int c2 = line.indexOf(',', c1+1);
              
              String role = "";
              if(c2 > 0) role = line.substring(c1+1, c2);
              else if(c1 > 0) role = line.substring(c1+1);
              role.trim();

              bool remove = false;
              if (role == "01") { // Guest
                   // Get Dates (Index 8, 9)
                   int idx = 0; int start = 0;
                   for(int i=0; i<9; i++) {
                       start = line.indexOf(',', start); 
                       if(start == -1) break; 
                       start++;
                   }
                   if(start != -1) {
                       String expireDate = line.substring(start);
                       expireDate.trim();
                       if(nowStr >= expireDate) remove = true;
                   }
              }

              if (remove) {
                  count++;
                  // Log
                  String uid = line.substring(0, c1);
                  maintainLogFile(LittleFS, "/userslog.txt", "Date,Time,Event,UID,ModUID");
                  appendFile(LittleFS, "/userslog.txt", (String("D") + "," + operatorStr + " (Cleanup)," + uid).c_str());
              } else {
                  temp.println(line);
              }
          }
          file.close();
          temp.close();
          LittleFS.remove("/users.txt");
          LittleFS.rename("/users.temp", "/users.txt");
          request->send(200, "text/plain", "Cleanup Complete: Removed " + String(count) + " users");
      }
      else {
          request->send(400, "text/plain", "Unknown Action");
      }
  });

  server.serveStatic("/", LittleFS, "/");
  server.begin();
}

void openDoor()
{
  digitalWrite(DOORLOCK_PIN, DOORLOCK_ON);
  doorActive = true;
  doorTimer = millis();
  Serial.println("Door Unlocked");
}

int verifyUID(String uid, String &role, bool isExit) {
  // 1. MAIN KEY CHECK (Always Allowed - Highest Priority)
  if (uid == MAIN_KEY_UID) {
      // If we are in Login Mode (Waiting for Admin Scan)
      if(waitingForAdminLogin) {
           adminSessionUID = uid;
           adminSessionRole = "MainKey"; 
           adminSessionTimer = millis();
           waitingForAdminLogin = false;
           // Double Beep for Login Success
           digitalWrite(BUZZER_PIN, BUZZER_ON); delay(100); digitalWrite(BUZZER_PIN, BUZZER_OFF); delay(100);
           digitalWrite(BUZZER_PIN, BUZZER_ON); delay(200); digitalWrite(BUZZER_PIN, BUZZER_OFF);
           role = "MainKey"; 
           return VERIFY_LOGIN_OK;
      } else {
           // Normal Access - Open Door
            Serial.println("Access Granted (Main Key)");
            // We still allow MainKey to control hardware directly OR let it pass?
            // MainKey bypasses logic? 
            // If MainKey, return OK. But logic below handles AntiPassback for everyone. 
            // Let's assume MainKey OVERRIDES AntiPassback.
            // So we return special code? Or just let it calculate?
            // "MainKey" usually bypasses restrictions.
            // Removed direct hardware control from here. handleReader will do it.
      }
      role = "MainKey"; // IMPORTANT: Set role so log is correct
      return VERIFY_OK;
  }

  // 2. ADMIN LOGIN MODE (Specific Flow for Login Page)
  if (waitingForAdminLogin) {
      if(millis() - waitLoginTimer > 30000) {
           waitingForAdminLogin = false;
           return VERIFY_DENIED; 
      }
      
      // Check if Admin (Read from File)
      String tempRole = getRoleFromFile(LittleFS, "/users.txt", uid);
      if (tempRole.startsWith("-") || tempRole.length() == 0) {
          role = "Unknown";
      } else {
          role = tempRole;
      }

      if (role == Role_Admin) {
           adminSessionUID = uid;
           adminSessionRole = "Admin"; 
           adminSessionTimer = millis();
           waitingForAdminLogin = false;
           digitalWrite(BUZZER_PIN, BUZZER_ON); delay(100); digitalWrite(BUZZER_PIN, BUZZER_OFF);
           return VERIFY_LOGIN_OK;
      }
      
      // Login Failed
      deniedBeepState = 1; deniedBeepTimer = millis();
      return VERIFY_LOGIN_FAIL;
  }

  // 3. NORMAL ACCESS FLOW (Open Door)
  String userData = getUserDataFromFile(LittleFS, "/users.txt", uid);
  
  // ERROR HANDLING: If file error (-6, -1), treat as Not Found
  if (userData.startsWith("-") || userData.length() < 2) { 
       role = "Unknown"; 
       deniedBeepState = 1; deniedBeepTimer = millis();
       return VERIFY_NOTFOUND;
  }

  // Parse Role
  int firstComma = userData.indexOf(',');
  int secondComma = userData.indexOf(',', firstComma + 1);
  if(firstComma > 0) {
       if(secondComma > 0) role = userData.substring(firstComma + 1, secondComma);
       else role = userData.substring(firstComma + 1); // Case: UID,Role (End)
       role.trim();
  } else {
       role = "Unknown";
       return VERIFY_NOTFOUND;
  }

  // 3.1 Normal Users / Admins
  if (role == Role_Admin || role == Role_User) {
       // Removed direct hardware control
       return VERIFY_OK;
  }
  
  // 3.2 Guest (Time Limited)
  if (role == Role_Guest) {
      auto getCol = [&](int index) -> String {
          int start = 0;
          for(int i=0; i<index; i++) {
             start = userData.indexOf(',', start); if(start == -1) return ""; start++;
          }
          int end = userData.indexOf(',', start); if (end == -1) end = userData.length();
          return userData.substring(start, end);
      };

      String startDateStr = getCol(8); startDateStr.trim();
      String expireDateStr = getCol(9); expireDateStr.trim();
      
      struct tm tmstruct;
      if(!getLocalTime(&tmstruct, 100)) return VERIFY_INVALID; // Time Sync Error
      
      char currentStr[20];
      snprintf(currentStr, sizeof(currentStr), "%04d-%02d-%02d %02d:%02d", tmstruct.tm_year+1900, tmstruct.tm_mon+1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min);
      String nowStr = String(currentStr);
      
      // Strict Expiry: Expire AS SOON AS the minute matches (Exclusive)
      if (nowStr >= startDateStr && nowStr < expireDateStr) {
          // Removed direct hardware control
          return VERIFY_OK;
      } else {
          deniedBeepState = 1; deniedBeepTimer = millis();

          if (nowStr >= expireDateStr) {
             // Check Grace Period for EXIT
             if (isExit) {
                 int y, M, d, h, m;
                 if(sscanf(expireDateStr.c_str(), "%d-%d-%d %d:%d", &y, &M, &d, &h, &m) == 5) {
                      struct tm expireTm = {0};
                      expireTm.tm_year = y - 1900;
                      expireTm.tm_mon = M - 1;
                      expireTm.tm_mday = d;
                      expireTm.tm_hour = h;
                      expireTm.tm_min = m;
                      expireTm.tm_isdst = -1;
                      
                      time_t tExpire = mktime(&expireTm);
                      time_t tNow = mktime(&tmstruct); // tmstruct populated earlier
                      
                      double diff = difftime(tNow, tExpire);
                      // Grace Period: 30 Minutes (1800 Seconds)
                      if(diff >= 0 && diff <= 1800) {
                           Serial.printf("Guest Expiry Grace Period: %.0f sec (Allowed Exit)\n", diff);
                           return VERIFY_OK; 
                      }
                 }
             }



             // Auto Delete Expired Guest (Verified Expired & No Grace)
             if (AUTO_CLEANUP_EXPIRED_GUESTS) {
                 Serial.println("Guest Expired. Deleting...");
                 deleteUserByUID(LittleFS, "/users.txt", uid);
              
                 // Log deletion
                 maintainLogFile(LittleFS, "/userslog.txt", "Date,Time,Event,UID,ModUID");
                 appendFile(LittleFS, "/userslog.txt", (String("D") + ",System (Expired)," + uid).c_str());
             } else {
                 Serial.println("Guest Expired (Auto Delete Disabled)");
             }
          
             return VERIFY_EXPIRED;
          } else {
             Serial.println("Guest Access: Not Yet Active (or Invalid Range)");
             return VERIFY_DENIED;
          }
      }
  }

  // Fallback (Invalid Role)
  deniedBeepState = 1; deniedBeepTimer = millis();
  return VERIFY_DENIED;
}

// Check if user is already inside (returns true if INSIDE)
bool isUserInside(String uid) {
   String state = getUserDataFromFile(LittleFS, "/user_state.txt", uid);
   if(state.startsWith("-")) return false; // Not found -> default outside
   int comma = state.indexOf(',');
   if(comma > 0) {
       int val = state.substring(comma+1).toInt();
       return (val == STATE_INSIDE);
   }
   return false;
}

void updateUserState(String uid, int newState) {
   upsertUser(LittleFS, "/user_state.txt", uid, uid + "," + String(newState));
}

void handleReader(int ID, MFRC522 &mfrc522)
{
  String uidString = "";
  String role = "";
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  delay(500);
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidString += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    uidString += String(mfrc522.uid.uidByte[i], HEX);
  }
  uidString.toUpperCase();
  Serial.print("[" + String(ID) + "] Card UID: ");
  Serial.println(uidString);
  Serial.println(uidString);

  // --- ANTI-PASSBACK CHECK ---
  // Reader 1 = IN, Reader 2 = OUT
  bool allow = false;
  int currentCode = VERIFY_DENIED;

  // Check Permissions First
  String tempRole = "";
  // Pass isExit = true if Reader ID is OUT
  int permCode = verifyUID(uidString, tempRole, (ID == READER_OUT));
  
  // If permission OK, check Direction Logic
  if (permCode == VERIFY_OK) {
      // MainKey bypasses Anti-Passback logic
      if (uidString == MAIN_KEY_UID) {
          allow = true;
          role = tempRole; // Should be "MainKey"
          currentCode = VERIFY_OK;
      } else {
          bool isInside = isUserInside(uidString);
          
          if (ID == READER_IN) { // Tapping IN
               if (isInside) {
                   Serial.println("Anti-Passback: Already Inside!");
                   currentCode = VERIFY_DENIED; // Or custom code?
                   role = tempRole; // Log the role even if denied by logic
                   // Beep Error
                   deniedBeepState = 1; deniedBeepTimer = millis();
               } else {
                   allow = true;
                   role = tempRole;
                   currentCode = VERIFY_OK;
                   updateUserState(uidString, STATE_INSIDE);
               }
          } 
          else if (ID == READER_OUT) { // Tapping OUT
               if (!isInside) {
                   Serial.println("Anti-Passback: Already Outside (or didn't tap in)!");
                   // STRICT MODE: Deny. 
                   allow = false; 
                   role = tempRole;
                   currentCode = VERIFY_DENIED;
                   deniedBeepState = 1; deniedBeepTimer = millis();
               } else {
                   allow = true; 
                   role = tempRole;
                   currentCode = VERIFY_OK;
                   updateUserState(uidString, STATE_OUTSIDE);
                   
                   // *** COUNT USAGE ON EXIT ONLY ***
                   // "นับเข้าออกประตูโดยใบแตะเข้าและออกนับเป็น 1 ครั้ง" -> Count on OUT
                   updateUsageStats(LittleFS, "/usage_stats.txt", uidString);
               }
          }
      }
  } else {
      currentCode = permCode;
      role = tempRole; // Role IS found (e.g. Unknown/Guest), but denied. We need to log it.
  }

  // Override verifyUID's direct hardware control (It opens door inside verifyUID... wait)
  // verifyUID() in previous code calls openDoor() directly! We need to Refactor verifyUID to NOT open door, only return status.
  // OR we prevent verifyUID call? 
  // Refactoring verifyUID is safer.
  
  // WAIT: My verifyUID logic above calls openDoor(). I need to change verifyUID to NOT open door.
  // I will Modify verifyUID below.
  
  if (allow) {
      Serial.println("Access Permitted (Passback OK)");
      digitalWrite(BUZZER_PIN, BUZZER_ON); delay(200); digitalWrite(BUZZER_PIN, BUZZER_OFF);
      openDoor();
  } else {
      Serial.println("Access Denied (Passback/Auth Fail)");
  }

  // Log format: ReaderID, UID, Role, Ref.Verify
  // appendFile adds Date,Time automatically
  maintainLogFile(LittleFS, "/readerlogs.txt", "Date,Time,ReaderID,UID,Role,Ref.Verify");
  int sta = appendFile(LittleFS, "/readerlogs.txt", (String(ID) + "," + uidString + "," + role + "," + String(currentCode)).c_str());

  if (sta != FILE_OK) {
    Serial.printf("Failed to write to readerlogs.txt: %d\n", sta);
  }
  digitalWrite(LED_STA_PIN, LED_STA_ON);
  delay(500);
  digitalWrite(LED_STA_PIN, LED_STA_OFF);
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void loop()
{
  handlePeripherals();
  handleReader(READER_IN, mfrc522_IN);
  handleReader(READER_OUT, mfrc522_OUT);
}