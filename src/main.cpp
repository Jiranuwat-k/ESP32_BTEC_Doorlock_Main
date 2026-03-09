#include <Arduino.h>
#include <ESP32Ping.h>
#include <Preferences.h>
#include "Config.h"
#include "Global.h"
#include "Filehelper.h"
#include "Filehelper.h"
#include "web/style.h"
#include "web/js.h"
#include "web/settings.h"
#include "web/tools.h"
#include "web/index.h"

MFRC522 mfrc522_OUT(SS_PIN, RST_PIN);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
Preferences preferences;

// --- SETTINGS GLOBALS ---
String sys_username = "Admin";
String sys_password = "Admin";
bool sys_antipassback = true;

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

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
void updateUserState(String uid, int newState);
bool isMainKey(String uid);
bool isSuperMainKey(String uid);

// --- NETWORK SCANNER GLOBALS ---
bool isScanning = false;
String scanResults = "[]";
int scanProgress = 0;

// Helpers for MAC and Vendor
#include <lwip/etharp.h>
#include <lwip/netif.h>

String getMacFromIp(IPAddress ip) {
    struct eth_addr *mac_ret;
    const ip4_addr_t *ip_ret;
    ip4_addr_t ipaddr;
    IP4_ADDR(&ipaddr, ip[0], ip[1], ip[2], ip[3]);
    
    // Find in ARP cache (only works if we recently contacted them, e.g. Ping)
    ssize_t idx = etharp_find_addr(NULL, &ipaddr, &mac_ret, &ip_ret);
    
    if (idx >= 0) {
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac_ret->addr[0], mac_ret->addr[1], mac_ret->addr[2],
                 mac_ret->addr[3], mac_ret->addr[4], mac_ret->addr[5]);
        return String(macStr);
    }
    return "";
}

String getVendor(String mac) {
    if(mac.length() < 8) return "";
    String oui = mac.substring(0, 8); // "XX:XX:XX"
    oui.toUpperCase();
    
    // Simple fast lookup for common IoT
    if(oui.startsWith("24:0A:C4") || oui.startsWith("30:AE:A4") || oui.startsWith("3C:71:BF") || 
       oui.startsWith("54:43:B2") || oui.startsWith("60:01:94") || oui.startsWith("84:0D:8E") ||
       oui.startsWith("A0:20:A6") || oui.startsWith("AC:67:B2") || oui.startsWith("EC:FA:BC")) 
       return "Espressif";
       
    if(oui.startsWith("B8:27:EB") || oui.startsWith("DC:A6:32") || oui.startsWith("E4:5F:01"))
       return "Raspberry Pi";
       
    if(oui.startsWith("00:11:32")) return "Synology";
    if(oui.startsWith("00:1A:11") || oui.startsWith("00:1D:AA")) return "Google";
    
    return "";
}

#include <map>

// ... inside scanTask ...
void scanTask(void * parameter) {
    isScanning = true;
    scanProgress = 0;
    scanResults = "[]"; 
    
    String localRes = "[";
    IPAddress local = WiFi.localIP();
    
    // Map to store IP -> Hostname
    std::map<String, String> hostMap;

    // Helper to query and cache
    auto cacheMdns = [&](const char* service, const char* proto) {
        int n = MDNS.queryService(service, proto);
        for(int m=0; m<n; m++) {
            String ip = MDNS.IP(m).toString();
            String name = MDNS.hostname(m);
            if(name.length() > 0) hostMap[ip] = name;
        }
    };
    
    // Query common services to build hostname database
    // This might take a few seconds total (approx 1s per query call depending on lib)
    cacheMdns("http", "tcp");
    cacheMdns("esp", "tcp");
    cacheMdns("arduino", "tcp");
    cacheMdns("printer", "tcp");
    cacheMdns("googlecast", "tcp");
    cacheMdns("smb", "tcp"); // Windows/Samba

    bool first = true;
    for (int i = 1; i < 255; i++) {
        scanProgress = (i * 100) / 255;
        IPAddress target(local[0], local[1], local[2], i);
        if (target == local) continue;
        
        // Use Ping to check aliveness
        if (Ping.ping(target, 1)) {
             String ipStr = target.toString();
             String mac = getMacFromIp(target);
             String vendor = getVendor(mac);
             
             // Lookup Hostname from our mDNS map
             String hostname = "";
             if(hostMap.count(ipStr)) {
                 hostname = hostMap[ipStr];
             } else {
                 if(vendor == "Espressif") hostname = "ESP32/8266";
                 else if(vendor == "Raspberry Pi") hostname = "Raspberry Pi";
                 else hostname = "Device " + String(i);
             }
             
             if (!first) localRes += ",";
             localRes += "{";
             localRes += "\"ip\":\"" + ipStr + "\",";
             localRes += "\"hostname\":\"" + hostname + "\",";
             localRes += "\"port\":80,";
             localRes += "\"mac\":\"" + mac + "\",";
             localRes += "\"vendor\":\"" + vendor + "\"";
             localRes += "}";
             first = false;
        }
        vTaskDelay(5 / portTICK_PERIOD_MS); 
    }
    
    localRes += "]";
    scanResults = localRes;
    
    isScanning = false;
    scanProgress = 100;
    vTaskDelete(NULL);
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void initRFIDReader()
{ 
  Serial.println(F("Initializing RC522 (OUT)..."));
  
  // Ensure Pins are in correct state before SPI
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH); 
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, LOW);
  delay(100);
  digitalWrite(RST_PIN, HIGH);
  delay(100);

  mfrc522_OUT.PCD_Init();
  delay(100);
  
  mfrc522_out_version = mfrc522_OUT.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.print(F("Reader OUT Version: 0x"));
  Serial.println(mfrc522_out_version, HEX);
  
  if (mfrc522_out_version == 0x00 || mfrc522_out_version == 0xFF) {
     Serial.println(F("WARNING: Reader OUT not detected or 0xFF error!"));
  } else {
     mfrc522_OUT.PCD_DumpVersionToSerial();
  }
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
    writeFile(LittleFS, "/userslog.txt", "Date,Time,Event,UID,ModUID,TargetName,Details\n");
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
    digitalWrite(LED_BUILTIN,HIGH); 
  });
  
  updateServer.begin("/update", sys_username.c_str(), sys_password.c_str());
  wifimanager.setRetryInterval(false); // In Mode Config
  wifimanager.begin("Manager");

  updateServer.on(UPDATE_BEGIN, [](const OTA_UpdateType type, int &result)
                  {
      Serial.print("Update Begin: ");
      Serial.println(type == OTA_UpdateType::OTA_FIRMWARE ? "Firmware" : "Filesystem"); });
  updateServer.on(UPDATE_END, [](const OTA_UpdateType type, int &result)
                  {
      Serial.print("Update End. Result: ");
      Serial.println(result == OTA_UpdateResult::OTA_UPDATE_OK ? "OK" : "Error"); });
  updateServer.begin("/update", sys_username.c_str(), sys_password.c_str());
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

// Connection Beep Globals
int connectBeepState = 0;
unsigned long connectBeepTimer = 0;

// WiFi Config Button Globals
unsigned long configButtonTimer = 0;
bool configButtonActive = false;

// WiFi Reconnect Timer
unsigned long wifiCheckTimer = 0;
const unsigned long wifiCheckInterval = 30000; // Check every 30 seconds
unsigned long lastReaderCheck = 0;

void handlePeripherals()
{
  currentMillis = millis();

  if (doorActive && (currentMillis - doorTimer >= doorOpenDuration)) {
    Serial.println("Door Locked (Time out)");
    digitalWrite(DOORLOCK_PIN, DOORLOCK_OFF);
    doorActive = false;
    ws.textAll("{\"type\":\"door_status\", \"status\":\"locked\"}");
  }

  if (successEffectActive && (currentMillis - successEffectTimer >= successEffectDuration)) {
    digitalWrite(BUZZER_PIN, BUZZER_OFF);
    digitalWrite(LED_STA_PIN, LED_STA_OFF);
    successEffectActive = false;
  }

  // Denied Beep Sequence (Rapid Beeps)
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

  // Connection Success Sequence (Double Beep)
  if (connectBeepState > 0 && (currentMillis - connectBeepTimer >= 100)) {
       connectBeepTimer = currentMillis;
       switch(connectBeepState) {
           case 1: digitalWrite(BUZZER_PIN, BUZZER_ON); break; // Beep 1
           case 2: digitalWrite(BUZZER_PIN, BUZZER_OFF); break; // Off
           case 3: digitalWrite(BUZZER_PIN, BUZZER_ON); break; // Beep 2
           case 4: digitalWrite(BUZZER_PIN, BUZZER_OFF); break; // Off
       }
       connectBeepState++;
       if(connectBeepState > 4) connectBeepState = 0;
  }

  // WiFi Reset Button Logic (Hold 10s)
  if (digitalRead(CONFIG_BUTTON_PIN) == LOW) {
      if (!configButtonActive) {
          configButtonActive = true;
          configButtonTimer = millis();
          Serial.println("Config Button Pressed...");
      } else {
          unsigned long duration = millis() - configButtonTimer;
          if (duration > 10000) { // 10 Seconds
              Serial.println("Resetting WiFi Settings...");
              // Long Beep Indication
              digitalWrite(BUZZER_PIN, BUZZER_ON); delay(200); digitalWrite(BUZZER_PIN, BUZZER_OFF); delay(100);
              digitalWrite(BUZZER_PIN, BUZZER_ON); delay(200); digitalWrite(BUZZER_PIN, BUZZER_OFF); delay(100);
              digitalWrite(BUZZER_PIN, BUZZER_ON); delay(1000); digitalWrite(BUZZER_PIN, BUZZER_OFF);
              
              wifimanager.reset();
              ESP.restart();
          }
          // Optional: Beep every second to warn user? 
          if(duration > 1000 && (duration / 1000) % 2 == 0) {
             // mild warning tick could go here, but omitted to keep simple
          }
      }
  } else {
      configButtonActive = false;
  }

  // WiFi Connection Check & Reconnect
  if (currentMillis - wifiCheckTimer >= wifiCheckInterval) {
      wifiCheckTimer = currentMillis;
      if (WiFi.status() != WL_CONNECTED) {
          Serial.println("WiFi Connection Lost or Not Connected. Attempting Reconnect...");
          WiFi.reconnect();
      }
  }
}

// *** RESTORED LOGIN HANDLERS (Fix for Logout & Login Page) ***
void setup()
{
  Serial.begin(115200);
  preferences.begin("system", false);
  sys_username = preferences.getString("http_user", "Admin");
  sys_password = preferences.getString("http_pass", "Admin");
  sys_antipassback = preferences.getBool("anti_pass", true);
  SPI.begin();
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_STA_PIN, OUTPUT);
  pinMode(DOORLOCK_PIN, OUTPUT);
  pinMode(READER_IN_RST_PIN, OUTPUT); // Configure Reader IN Reset Pin
  pinMode(SS_PIN, OUTPUT);           // Ensure SS is Output
  pinMode(RST_PIN, OUTPUT);          // Ensure RST is Output
  digitalWrite(SS_PIN, HIGH);        // Deselect Reader
  pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP); // BOOT Button
  digitalWrite(BUZZER_PIN, BUZZER_OFF);
  digitalWrite(LED_STA_PIN, LED_STA_OFF);
  digitalWrite(DOORLOCK_PIN, DOORLOCK_OFF);
  digitalWrite(READER_IN_RST_PIN, HIGH); // Default HIGH (Run mode)
  initLittleFS();
  initWifi();
  configTime(3600 * timezone, daysavetime * 3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
  initTime();
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
     if(checkUserWebAuth(request)) { 
         request->redirect("/"); 
         return; 
     }
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
     
     // Special Developer Account (High Security Credentials)
     // Rights: Equivalent to MainKey (Super Admin)
     bool isDev = (user == "Developer" && pass == "Dx6MZ2ESOjNO9HyYIjguT70TDvITpkZu");
     
     if((user == sys_username && pass == sys_password) || isDev) {
        // Generate Web Token
        webSessionToken = String(millis()) + String(random(1000,9999));
        
        // If developer, auto-initiate Admin/MainKey Session so they don't need a card scan
        if (isDev) {
            adminSessionUID = "DEVELOPER";
            adminSessionRole = "MainKey";
            adminSessionTimer = millis(); 
        }

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
      if(!checkUserWebAuth(request)) { 
          request->redirect("/login"); 
          return; 
      }
      
      // Manual Replacement to prevent % conflicts in CSS/JS
      String html = String(index_html);
      html.replace("%SETTINGS_SECTION%", String(settings_html));
      html.replace("%TOOLS_SECTION%", String(tools_html));
      request->send(200, "text/html", html);
  });

  // Serve Static Assets from PROGMEM
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/css", style_css);
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "application/javascript", script_js);
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
       if(!checkUserWebAuth(request)) return request->send(401, "text/plain", "Session Expired");

       // Check timeout
       if(adminSessionUID != "" && millis() - adminSessionTimer > ADMIN_SESSION_TIMEOUT) {
           adminSessionUID = ""; // Auto logout
       }
       String status = adminSessionUID != "" ? "logged_in" : (waitingForAdminLogin ? "waiting" : "logged_out");
       String display = adminSessionUID != "" ? (adminSessionRole + " (" + adminSessionUID + ")") : "";
       bool isSuper = isSuperMainKey(adminSessionUID);
       String json = "{\"status\":\"" + status + "\", \"uid\":\"" + display + "\", \"is_super\":" + (isSuper ? "true" : "false") + "}";
       request->send(200, "application/json", json);
  });

  server.on("/api/login/logout", HTTP_POST, [](AsyncWebServerRequest *request) {
       adminSessionUID = "";
       adminSessionRole = "";
       request->send(200, "text/plain", "Logged Out");
  });

  // Settings Save API
  server.on("/api/settings/save", HTTP_POST, [](AsyncWebServerRequest *request) {
      if(!checkUserWebAuth(request)) return request->send(401, "text/plain", "Session Expired");
      if(adminSessionUID == "") return request->send(401, "text/plain", "Unauthorized: Please scan Admin Card");
      
      if(request->hasParam("sys_username", true)) {
          sys_username = request->getParam("sys_username", true)->value();
          preferences.putString("http_user", sys_username);
      }
      if(request->hasParam("sys_password", true)) {
          String newPass = request->getParam("sys_password", true)->value();
          if (newPass.length() > 0) {
              sys_password = newPass;
              preferences.putString("http_pass", sys_password);
          }
      }
      if(request->hasParam("sys_antipassback", true)) {
          String val = request->getParam("sys_antipassback", true)->value();
          sys_antipassback = (val == "true" || val == "1");
          preferences.putBool("anti_pass", sys_antipassback);
      }
      
      // Re-init update server with new credentials
      updateServer.begin("/update", sys_username.c_str(), sys_password.c_str());
      
      request->send(200, "text/plain", "Settings Updated Successfully");
  });

  server.on("/api/settings/get", HTTP_GET, [](AsyncWebServerRequest *request) {
      if(!checkUserWebAuth(request)) return request->send(401);
      String json = "{";
      json += "\"sys_username\":\"" + sys_username + "\",";
      json += "\"sys_antipassback\":" + String(sys_antipassback ? "true" : "false");
      json += "}";
      request->send(200, "application/json", json);
  });

  // API to get Reader Status
  server.on("/api/readers", HTTP_GET, [](AsyncWebServerRequest *request) {
       if(!checkUserWebAuth(request)) return request->send(401, "text/plain", "Session Expired");

       auto getVerStr = [](uint8_t v) {
           String s = "0x" + String(v, HEX);
           s.toUpperCase();
           if(v == 0x92) s += " (v2.0)";
           else if(v == 0x91) s += " (v1.0)";
           else if(v == 0x90) s += " (v0.0)";
           else if(v == 0x88) s += " (Clone)";
           else s += " (Unknown)";
           return s;
       };
       String json = "{";
       json += "\"in\":\"" + getVerStr(mfrc522_in_version) + "\",";
       json += "\"out\":\"" + getVerStr(mfrc522_out_version) + "\"";
       json += "}";
       request->send(200, "application/json", json);
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

  // Start Background Scan
  server.on("/api/scan/start", HTTP_POST, [](AsyncWebServerRequest *request) {
      if(isScanning) return request->send(200, "text/plain", "Busy");
      
      // Create Task
      xTaskCreate(
          scanTask,       /* Task function. */
          "ScanTask",     /* String with name of task. */
          4096,           /* Stack size in bytes. */
          NULL,           /* Parameter passed as input of the task */
          1,              /* Priority of the task. */
          NULL);          /* Task handle. */
          
      request->send(200, "text/plain", "Started");
  });

  // Check Status
  server.on("/api/scan/status", HTTP_GET, [](AsyncWebServerRequest *request) {
       if(!checkUserWebAuth(request)) return request->send(401, "text/plain", "Session Expired");

       // Return JSON with status
       String json = "{";
       json += "\"scanning\":" + String(isScanning ? "true" : "false") + ",";
       json += "\"progress\":" + String(scanProgress) + ",";
       json += "\"data\":" + scanResults;
       json += "}";
       request->send(200, "application/json", json);
  });

  // WS Server Status API
  server.on("/api/ws/status", HTTP_GET, [](AsyncWebServerRequest *request) {
      if(!checkUserWebAuth(request)) return request->send(401);
      
      String json = "{";
      json += "\"status\":\"Active\",";
      json += "\"count\":" + String(ws.count());
      json += "}";
      request->send(200, "application/json", json);
  });

  // WS Broadcast API (For Remote Control)
  server.on("/api/ws/broadcast", HTTP_POST, [](AsyncWebServerRequest *request) {
      if(!checkUserWebAuth(request)) return request->send(401);
      
      String msg = request->hasParam("msg", true) ? request->getParam("msg", true)->value() : "";
      if(msg.length() > 0) {
          ws.textAll(msg);
          request->send(200, "text/plain", "Sent: " + msg);
      } else {
          request->send(400, "text/plain", "Empty Message");
      }
  });

  // --- API Action Handler (POST) - SECURED ---
  server.on("/api/action", HTTP_POST, [](AsyncWebServerRequest *request) {
      if(!checkUserWebAuth(request)) return request->send(401, "text/plain", "Session Expired"); // Web Auth First
      
      String action = request->hasParam("action", true) ? request->getParam("action", true)->value() : "";

      // Allow Reset actions (Hardware/Self/Local Reader) WITHOUT Admin Card
      if (action == "rst_hardware" || action == "rst_self" || action == "rst_reader_out") {
           // Proceed directly
      } 
      // For other sensitive actions, require Admin Card if Enabled
      else if(ENABLE_MAINKEY_SYSTEM && adminSessionUID == "") {
          request->send(401, "text/plain", "Unauthorized: Please scan Admin Card");
          return;
      } else {
          adminSessionTimer = millis(); // Refresh Card Session
      }
      
      String operatorStr;
      if (adminSessionUID != "") {
          operatorStr = adminSessionRole + " " + adminSessionUID;
      } else {
          // If bypassing card check (e.g. for reset), we can use Web User or just System
          operatorStr = "WebAdmin"; 
      }

      if(action == "rst_hardware") {
           // Reset External Reader via GPIO
           digitalWrite(READER_IN_RST_PIN, LOW);
           delay(200);
           digitalWrite(READER_IN_RST_PIN, HIGH);
           request->send(200, "text/plain", "Hardware Reset Sent to Reader IN");
      }
      else if(action == "rst_reader_out") {
           // Reset Local Reader via GPIO & Init
           digitalWrite(RST_PIN, LOW);
           delay(200);
           digitalWrite(RST_PIN, HIGH);
           delay(100);
           mfrc522_OUT.PCD_Init();
           mfrc522_out_version = mfrc522_OUT.PCD_ReadRegister(MFRC522::VersionReg);
           request->send(200, "text/plain", "Local Reader Reset. Ver: 0x" + String(mfrc522_out_version, HEX));
      }
      else if(action == "rst_self") {
           request->send(200, "text/plain", "Restarting Main Unit...");
           delay(500);
           ESP.restart();
      }
      else if(action == "clear_log") {
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
              if (!isMainKey(adminSessionUID)) {
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
             // Fetch role before deleting to verify permissions
             String targetRole = getRoleFromFile(LittleFS, "/users.txt", uid);
             if ((targetRole == Role_SecondaryKey || targetRole == Role_MainKey || targetRole == "50") && !isSuperMainKey(adminSessionUID)) {
                 request->send(403, "text/plain", "Permission Denied: Only MainKey can delete Admins/SecondaryKeys");
                 return;
             }

             // Get Comprehensive Info before deleting for Log
             String targetData = getUserDataFromFile(LittleFS, "/users.txt", uid);
             String snapshot = "-";
             if(targetData != "" && !targetData.startsWith("-")) {
                 auto getIdx = [](String data, int idx) -> String {
                     int found = 0; int start = 0;
                     for (int i = 0; i < idx; i++) {
                         start = data.indexOf(',', start); if (start == -1) return "-"; start++;
                     }
                     int end = data.indexOf(',', start); if (end == -1) end = data.length();
                     String s = data.substring(start, end); s.trim(); return s;
                 };
                 // Format: [Pre] Name EN (Name TH) ID:Code Age:XX Sex:XX
                 snapshot = "[" + getIdx(targetData, 2) + "] " + getIdx(targetData, 3) + " (" + getIdx(targetData, 4) + ") ID:" + getIdx(targetData, 5) + " Age:" + getIdx(targetData, 7) + " Sex:" + getIdx(targetData, 6);
             }

             deleteUserByUID(LittleFS, "/users.txt", uid);
             maintainLogFile(LittleFS, "/userslog.txt", "Date,Time,Event,UID,ModUID,TargetName,Details");
             appendFile(LittleFS, "/userslog.txt", (String("D") + "," + operatorStr + "," + uid + "," + snapshot + ",User Deleted").c_str());
             request->send(200, "text/plain", "User Deleted");
          } else {
             request->send(400, "text/plain", "Missing UID");
          }
      }
      else if(action == "diag_reader") {
          Serial.println(F("Running Reader Self-Test..."));
          bool result = mfrc522_OUT.PCD_PerformSelfTest();
          mfrc522_out_version = mfrc522_OUT.PCD_ReadRegister(MFRC522::VersionReg);
          
          String json = "{";
          json += "\"test\":" + String(result ? "true" : "false") + ",";
          json += "\"version\":\"0x" + String(mfrc522_out_version, HEX) + "\"";
          json += "}";
          request->send(200, "application/json", json);
      }
      else if(action == "save_user") {
          String uid = request->hasParam("uid", true) ? request->getParam("uid", true)->value() : "";
          String role = request->hasParam("role", true) ? request->getParam("role", true)->value() : "10";
          if(uid == "") { request->send(400, "text/plain", "Missing UID"); return; }
          
          if (ENABLE_MAINKEY_SYSTEM) {
              String existingRole = getRoleFromFile(LittleFS, "/users.txt", uid);
              if ((existingRole == Role_SecondaryKey || existingRole == Role_MainKey || existingRole == "50") && !isSuperMainKey(adminSessionUID)) {
                  request->send(403, "text/plain", "Permission Denied: Only MainKey can modify Admins/Secondary Keys");
                  return;
              }
              if((role == "50" || role == Role_SecondaryKey || role == Role_MainKey) && !isSuperMainKey(adminSessionUID)) {
                  request->send(403, "text/plain", "Permission Denied: Only MainKey can create Admin/Secondary Keys");
                  return;
              }
          }
          String prefix = request->hasParam("prefix", true) ? request->getParam("prefix", true)->value() : "XX";
          String fname_en = request->hasParam("fname_en", true) ? request->getParam("fname_en", true)->value() : "-";
          String fname_th = request->hasParam("fname_th", true) ? request->getParam("fname_th", true)->value() : "-";
          String code = request->hasParam("code", true) ? request->getParam("code", true)->value() : "-";
          String gender = request->hasParam("gender", true) ? request->getParam("gender", true)->value() : "-";
          String age = request->hasParam("age", true) ? request->getParam("age", true)->value() : "-";
          String start_date = request->hasParam("start_date", true) ? request->getParam("start_date", true)->value() : "-";
          String end_date = request->hasParam("end_date", true) ? request->getParam("end_date", true)->value() : "-";
          
          String finalData = uid + "," + role + "," + prefix + "," + fname_en + "," + fname_th + "," + code + "," + gender + "," + age + "," + start_date + "," + end_date;
          
          // Check for Duplicate ID Code (Excluding current UID)
          if(code != "" && code != "-") {
              File fCheck = LittleFS.open("/users.txt", "r");
              if(fCheck) {
                  fCheck.readStringUntil('\n'); // skip header
                  while(fCheck.available()) {
                      String line = fCheck.readStringUntil('\n');
                      int c1 = line.indexOf(','); if(c1==-1) continue;
                      String f_uid = line.substring(0, c1);
                      if(f_uid == uid) continue; // It's us

                      // Extract code (idx 5)
                      int start = c1;
                      for(int i=0; i<4; i++) { start = line.indexOf(',', start+1); if(start==-1) break; }
                      if(start != -1) {
                          int end = line.indexOf(',', start+1); if(end==-1) end = line.length();
                          String f_code = line.substring(start+1, end);
                          f_code.trim();
                          if(f_code == code) {
                              fCheck.close();
                              request->send(400, "text/plain", "ID Code already assigned to UID: " + f_uid);
                              return;
                          }
                      }
                  }
                  fCheck.close();
              }
          }

          String existingUser = getUserDataFromFile(LittleFS, "/users.txt", uid);
          bool isNew = (existingUser == String(FILE_ERR_NOTFOUND) || existingUser == String(FILE_ERR_OPEN));
          
          String detail = "-";
          bool hasChanges = false;
          if(!isNew) {
              auto getF = [](String data, int idx) -> String {
                  int found = 0; int start = 0;
                  for (int i = 0; i < idx; i++) {
                      start = data.indexOf(',', start); if (start == -1) return ""; start++;
                  }
                  int end = data.indexOf(',', start); if (end == -1) end = data.length();
                  String s = data.substring(start, end); s.trim(); return s;
              };

              String changes = "";
              if(role != getF(existingUser, 1)) changes += "Role ";
              if(prefix != getF(existingUser, 2)) changes += "Pre ";
              if(fname_en != getF(existingUser, 3)) changes += "Name ";
              if(code != getF(existingUser, 5)) changes += "ID ";
              if(gender != getF(existingUser, 6)) changes += "Sex ";
              if(age != getF(existingUser, 7)) changes += "Age ";
              if(start_date != getF(existingUser, 8) || end_date != getF(existingUser, 9)) changes += "Time ";
              
              if(changes != "") {
                  detail = "Changed: " + changes;
                  hasChanges = true;
              } else {
                  detail = "No changes";
              }
          } else {
              detail = "Initial Create";
              hasChanges = true;
          }

          upsertUser(LittleFS, "/users.txt", uid, finalData);
          updateUserState(uid, STATE_OUTSIDE);

          // Only Log if it's NEW or has CHANGES
          if (hasChanges) {
              String evt = isNew ? "C" : "M";
              maintainLogFile(LittleFS, "/userslog.txt", "Date,Time,Event,UID,ModUID,TargetName,Details");
              String snapshot = "[" + prefix + "] " + fname_en + " (" + fname_th + ") ID:" + code + " Age:" + age + " Sex:" + gender;
              appendFile(LittleFS, "/userslog.txt", (evt + "," + operatorStr + "," + uid + "," + snapshot + "," + detail).c_str());
          }
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
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();

  // Reset Reader IN when Main Unit is ready to ensure fresh connection
  Serial.println("Main Unit Ready. Resetting Reader IN...");
  digitalWrite(READER_IN_RST_PIN, LOW);
  delay(500); 
  digitalWrite(READER_IN_RST_PIN, HIGH);
  Serial.println("Reader IN Reset Complete.");
}

void openDoor()
{
  digitalWrite(DOORLOCK_PIN, DOORLOCK_ON);
  doorActive = true;
  doorTimer = millis();
  Serial.println("Door Unlocked");
  ws.textAll("{\"type\":\"door_status\", \"status\":\"unlocked\"}");
}

bool isSuperMainKey(String uid) {
    if (uid == "DEVELOPER") return true; // Developer Bypass
    if (uid == MAIN_KEY_UID_1 && MAIN_KEY_UID_1 != "") return true;
    if (uid == MAIN_KEY_UID_2 && MAIN_KEY_UID_2 != "") return true;
    if (uid == MAIN_KEY_UID_3 && MAIN_KEY_UID_3 != "") return true;
    return false;
}

bool isMainKey(String uid) {
    if (isSuperMainKey(uid)) return true;
    
    String roleFromFile = getRoleFromFile(LittleFS, "/users.txt", uid);
    if (roleFromFile == Role_SecondaryKey || roleFromFile == Role_MainKey) {
        return true;
    }
    return false;
}

int verifyUID(String uid, String &role, bool isExit) {
  // 1. MAIN KEY CHECK (Always Allowed - Highest Priority)
  if (isMainKey(uid)) {
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



void processScan(int ID, String uidString) {
  String role = "";
  long durationSec = 0; // Initialize duration variable
  Serial.print("[" + String(ID) + "] Card UID: ");
  Serial.println(uidString);

  // --- ANTI-PASSBACK CHECK ---
  // Reader 1 = IN, Reader 2 = OUT
  bool allow = false;
  int currentCode = VERIFY_DENIED;

  // Check Permissions First
  String tempRole = "";
  // Pass isExit = true if Reader ID is OUT
  int permCode = verifyUID(uidString, tempRole, (ID == READER_OUT));
  
  // ... inside processScan ...
  String denyReason = "Access Denied"; // Default reason

  // If permission OK, check Direction Logic
  if (permCode == VERIFY_OK) {
      // MainKey bypasses Anti-Passback logic
      if (isMainKey(uidString)) {
          allow = true;
          role = tempRole; // Should be "MainKey"
          currentCode = VERIFY_OK;
      } else {
          bool isInside = isUserInside(uidString);
          
          if (ID == READER_IN) { // Tapping IN
               if (sys_antipassback && isInside) {
                   Serial.println("Anti-Passback: Already Inside!");
                   currentCode = VERIFY_DENIED; 
                   role = tempRole; 
                   denyReason = "Already Inside"; // REASON: Anti-passback
                   // Beep Error
                   deniedBeepState = 1; deniedBeepTimer = millis();
               } else {
                   allow = true;
                   role = tempRole;
                   currentCode = VERIFY_OK;
                   if (sys_antipassback) updateUserState(uidString, STATE_INSIDE);
               }
          } 
          else if (ID == READER_OUT) { // Tapping OUT
               if (sys_antipassback && !isInside) {
                   Serial.println("Anti-Passback: Already Outside (or didn't tap in)!");
                   // STRICT MODE: Deny. 
                   allow = false; 
                   role = tempRole;
                   currentCode = VERIFY_DENIED;
                   denyReason = "Already Outside"; // REASON: Anti-passback
                   deniedBeepState = 1; deniedBeepTimer = millis();
               } else {
                   allow = true; 
                   role = tempRole;
                   currentCode = VERIFY_OK;

                   // --- CALCULATE DURATION BEFORE EXIING ---
                   if (sys_antipassback) {
                       String stateLine = getUserDataFromFile(LittleFS, "/user_state.txt", uidString);
                       // Line: "AABBCC,1,1712345678"
                       if(stateLine.length() > 0) {
                           int lastComma = stateLine.lastIndexOf(',');
                           if(lastComma > 0) {
                               long timeIn = stateLine.substring(lastComma+1).toInt();
                               struct tm tmstruct;
                               if(getLocalTime(&tmstruct, 10)) {
                                   time_t nowTime = mktime(&tmstruct);
                                   if(nowTime > timeIn) {
                                       durationSec = (long)(nowTime - timeIn);
                                   }
                               }
                           }
                       }
                       // Reset State to OUTSIDE
                       updateUserState(uidString, STATE_OUTSIDE);
                   }
                   
                   // *** COUNT USAGE ON EXIT ONLY ***
                   updateUsageStats(LittleFS, "/usage_stats.txt", uidString);
               }
          }
      }
  } else {
      currentCode = permCode;
      role = tempRole; 
      // Map Verify Codes to Text
      if(permCode == VERIFY_NOTFOUND) denyReason = "User Not Found";
      else if(permCode == VERIFY_EXPIRED) denyReason = "Card Expired";
      else if(permCode == VERIFY_INVALID) denyReason = "Time Invalid";
      else if(permCode == VERIFY_LOGIN_FAIL) denyReason = "Login Failed";
      else denyReason = "No Permission";
  }

  if (allow) {
      Serial.println("Access Permitted");
      digitalWrite(BUZZER_PIN, BUZZER_ON); delay(200); digitalWrite(BUZZER_PIN, BUZZER_OFF);
      openDoor();

      // --- BROADCAST TO WEBSOCKET DISPLAY ---
      // Fetch User Details explicitly since they might not be in scope or passed in fully
      String userDetails = getUserDataFromFile(LittleFS, "/users.txt", uidString);
      
      if (userDetails != "" && userDetails != String(FILE_ERR_NOTFOUND) && userDetails != String(FILE_ERR_OPEN)) {
            // Helper to get Nth field
            auto getField = [](String data, int index) -> String {
                int found = 0;
                int strIndex[] = {0, -1};
                int maxIndex = data.length() - 1;
                for (int i = 0; i <= maxIndex && found <= index; i++) {
                    if (data.charAt(i) == ',' || i == maxIndex) {
                        found++;
                        strIndex[0] = strIndex[1] + 1;
                        strIndex[1] = (i == maxIndex) ? i + 1 : i;
                    }
                }
                return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
            };
            
            String uPrefix = getField(userDetails, 2); // Prefix
            String uName = getField(userDetails, 3); // Name EN
            String uNameTH = getField(userDetails, 4); // Name TH
            String uRole = getField(userDetails, 1); // Role
            String uCode = getField(userDetails, 5); // Code
            String uGender = getField(userDetails, 6); // Gender
            String uAge = getField(userDetails, 7); // Age
            String uExpire = getField(userDetails, 9); // Expire Date (Guest)
            
            // 1. Get Usage Count Helper
            auto getUsageCount = [](fs::FS &fs, const char* path, String uid) -> int {
                 File file = fs.open(path, "r");
                 if(!file) return 0;
                 int count = 0;
                 while(file.available()) {
                    String line = file.readStringUntil('\n');
                    if(line.startsWith(uid + ",")) {
                        int p1 = line.indexOf(',');
                        if(p1 > 0) count = line.substring(p1+1).toInt();
                        break;
                    }
                 }
                 file.close();
                 return count;
            };
            
            // Fetch Count (Naive read)
            int usageCount = getUsageCount(LittleFS, "/usage_stats.txt", uidString);

            // Construct JSON
            String wsJson = "{";
            wsJson += "\"type\":\"scan_event\",";
            wsJson += "\"reader\":" + String(ID) + ",";
            wsJson += "\"uid\":\"" + uidString + "\",";
            wsJson += "\"prefix\":\"" + uPrefix + "\",";
            wsJson += "\"name\":\"" + uName + "\",";
            wsJson += "\"name_th\":\"" + uNameTH + "\",";
            wsJson += "\"role\":\"" + uRole + "\",";
            wsJson += "\"code\":\"" + uCode + "\",";
            wsJson += "\"gender\":\"" + uGender + "\",";
            wsJson += "\"age\":\"" + uAge + "\",";
            wsJson += "\"expire\":\"" + uExpire + "\","; // ADDED EXPIRE DATE
            wsJson += "\"count\":" + String(usageCount) + ",";
            wsJson += "\"duration\":" + String(durationSec); 
            wsJson += "}";
            
            ws.textAll(wsJson);
      }
      
  } else {
      Serial.println("Access Denied");
      
       // Send Denied Event WITH REASON
       String wsJson = "{";
       wsJson += "\"type\":\"scan_deny\",";
       wsJson += "\"reader\":" + String(ID) + ",";
       wsJson += "\"uid\":\"" + uidString + "\",";
       wsJson += "\"code\":" + String(currentCode) + ",";
       wsJson += "\"msg\":\"" + denyReason + "\"";
       wsJson += "}";
       ws.textAll(wsJson);
  }

  maintainLogFile(LittleFS, "/readerlogs.txt", "Date,Time,ReaderID,UID,Role,Ref.Verify");
  int sta = appendFile(LittleFS, "/readerlogs.txt", (String(ID) + "," + uidString + "," + role + "," + String(currentCode)).c_str());

  if (sta != FILE_OK) {
    Serial.printf("Failed to write to readerlogs.txt: %d\n", sta);
  }
  
  // Visual Feedback
  digitalWrite(LED_STA_PIN, LED_STA_ON);
  delay(500); // Visual hold
  digitalWrite(LED_STA_PIN, LED_STA_OFF);
}

// Updated State Manager to handle Timestamp
void updateUserState(String uid, int newState) {
   struct tm tmstruct;
   getLocalTime(&tmstruct, 0);
   time_t now = mktime(&tmstruct);
   
   upsertUser(LittleFS, "/user_state.txt", uid, uid + "," + String(newState) + "," + String(now));
}

void handleReader(int ID, MFRC522 &mfrc522)
{
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  // No delay here, handled in processScan or loop? Original had delay(500). 
  // Let's rely on standard debouncing or just let it flow. 
  // Original had delay(500) AFTER reading.
  // We can put it after process.

  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidString += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    uidString += String(mfrc522.uid.uidByte[i], HEX);
  }
  uidString.toUpperCase();
  
  processScan(ID, uidString);

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

// Global to track Reader IN client ID
uint32_t readerInClientId = 0;

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
      Serial.printf("WS Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      // Audible Notification (Double Beep via State Machine)
      connectBeepState = 1; 
      connectBeepTimer = millis();
  }
  else if(type == WS_EVT_DISCONNECT){
      Serial.printf("WS Client #%u disconnected\n", client->id());
      if(client->id() == readerInClientId) {
          Serial.printf("Reader IN Disconnected! (Client ID: %u)\n", readerInClientId);
          mfrc522_in_version = 0; // Reset version to indicate offline
          readerInClientId = 0;
          
          // Broadcast Reader Status to Web
          String wsJson = "{\"type\":\"reader_status\", \"status\":\"offline\", \"reader\":1}";
          ws.textAll(wsJson);
      }
  }
  else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    if(info->final && info->index == 0 && info->len == len){
      String msg = "";
      for(size_t i=0; i<len; i++) msg += (char)data[i];
      
      // Syntax: "UID:AABBCC" or just "AABBCC"
      // Syntax: "VER:146" for version 0x92 (146)
      if(msg.startsWith("VER:")) {
          mfrc522_in_version = msg.substring(4).toInt();
          readerInClientId = client->id(); // Register this client as Reader IN
          Serial.printf("Reader IN Connected! Client ID: %u\n", readerInClientId);
          
          // Broadcast Reader Status to Web
          String wsJson = "{\"type\":\"reader_status\", \"status\":\"online\", \"reader\":1}";
          // Don't send back to the reader itself, only to browsers? Or just textAll (reader will ignore)
          ws.textAll(wsJson);
      }
      else {
          if(msg.startsWith("UID:")) msg = msg.substring(4);
          msg.trim();
          msg.toUpperCase();
          if(msg.length() > 0) processScan(READER_IN, msg);
      }
    }
  }

}

void checkReaderHealth() {
  if (millis() - lastReaderCheck > 10000) { // Check every 10 seconds
    lastReaderCheck = millis();
    
    // Read version to check if reader is still responding
    uint8_t v = mfrc522_OUT.PCD_ReadRegister(MFRC522::VersionReg);
    
    if (v == 0x00 || v == 0xFF) {
      Serial.println(F("‼️ Reader Critical Error (0xFF). Hard Resetting SPI & Reader..."));
      
      // 1. Force Pins State
      digitalWrite(SS_PIN, HIGH); 
      digitalWrite(RST_PIN, LOW);
      delay(150);
      digitalWrite(RST_PIN, HIGH);
      delay(200);
      
      // 2. Cycle SPI Bus (Helpful if bus is hung)
      SPI.end();
      delay(50);
      SPI.begin();
      
      // 3. Re-initialize Reader
      mfrc522_OUT.PCD_Init();
      delay(50);
      mfrc522_out_version = mfrc522_OUT.PCD_ReadRegister(MFRC522::VersionReg);
      
      if (mfrc522_out_version != 0x00 && mfrc522_out_version != 0xFF) {
        Serial.printf("✅ Reader OUT Recovered! Version: 0x%02X\n", mfrc522_out_version);
        maintainLogFile(LittleFS, "/systemlog.txt", "Date,Time,Code,Msg");
        appendFile(LittleFS, "/systemlog.txt", "INFO,Reader OUT Recovered");
      } else {
        Serial.println(F("❌ Recovery Failed. Card scanning will not work."));
      }
    }
  }
}

void loop()
{
  ws.cleanupClients();
  handlePeripherals();
  checkReaderHealth();
  // handleReader(READER_IN, mfrc522_IN); // Handled by WS
  handleReader(READER_OUT, mfrc522_OUT);
}