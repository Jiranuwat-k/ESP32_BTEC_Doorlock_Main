#include "ESPAsyncUpdateServer.h"
#include "OTAWebPage.h"
#include "BoardInfoPage.h"
#include "StreamString.h"
#include <Ticker.h>

#ifdef ESP32
    #ifdef ESPASYNCHTTPUPDATESERVER_LITTLEFS
        #include <LittleFS.h>
    #else
        #include <SPIFFS.h>
    #endif
    #include <Update.h>
    #include <WiFi.h>
    #include <esp_system.h>
#elif defined(ESP8266)
    #include <flash_hal.h>
    #include <FS.h>
    #include <ESP8266WiFi.h>
#else
  #error "This library only supports boards with an ESP8266 or ESP32 processor."
#endif

#ifndef ESPASYNCUPDATESERVER_COUT
    #define ESPASYNCUPDATESERVER_COUT Serial
#endif

#ifdef ESPASYNCUPDATESERVER_DEBUG
    #define Log(...) ESPASYNCUPDATESERVER_COUT.printf(__VA_ARGS__)
#else
    #define Log(...) 
#endif


static const char successResponse[] PROGMEM =
    R"(<meta content="15;URL=/"http-equiv=refresh>Update Success! Rebooting...)";

Ticker restartTimer;
ESPAsyncUpdateServer::ESPAsyncUpdateServer(AsyncWebServer *server, fs::FS &fs) : server(server), fs(fs) {}
void ESPAsyncUpdateServer::begin(char const* path ,char const* Username ,char const* Password)
{
    username = Username;
    password = Password;

    // handler for the /update form page
    server->on(path, HTTP_GET, [this](AsyncWebServerRequest *request){
        if(username != "" && password != ""){
            if( !request->authenticate(username.c_str(), password.c_str())){
                return request->requestAuthentication();
            }
        }
        
        #ifdef ESP32
        request->send(200, "text/html", serverIndex);
        #else
        request->send_P(200, "text/html", serverIndex);
        #endif
    });

    // handler for the /info API
    server->on("/info", HTTP_GET, [this](AsyncWebServerRequest *request){
    
    // Authentication Check
    if(username != "" && password != ""){
        if( !request->authenticate(username.c_str(), password.c_str())){
            return request->requestAuthentication();
        }
    }

    // Prepare JSON
    String json;
    json.reserve(1024);
    json = "{";

    String boardName;
    String chipIdStr;
    String resetReasonStr;
    
    #ifdef ESP32
        boardName = "ESP32";
        uint64_t chipid = ESP.getEfuseMac();
        char buf[17];
        sprintf(buf, "%04X%08X", (uint16_t)(chipid>>32), (uint32_t)chipid);
        chipIdStr = String(buf);

        // Reset Reason (Enum -> String)
        esp_reset_reason_t reason = esp_reset_reason();
        switch (reason) {
            case ESP_RST_UNKNOWN: resetReasonStr = "UNKNOWN"; break;
            case ESP_RST_POWERON: resetReasonStr = "POWERON"; break;
            case ESP_RST_EXT: resetReasonStr = "EXT"; break;
            case ESP_RST_SW: resetReasonStr = "SW"; break;
            case ESP_RST_PANIC: resetReasonStr = "PANIC"; break;
            case ESP_RST_INT_WDT: resetReasonStr = "INT_WDT"; break;
            case ESP_RST_TASK_WDT: resetReasonStr = "TASK_WDT"; break;
            case ESP_RST_WDT: resetReasonStr = "WDT"; break;
            case ESP_RST_DEEPSLEEP: resetReasonStr = "DEEPSLEEP"; break;
            case ESP_RST_BROWNOUT: resetReasonStr = "BROWNOUT"; break;
            case ESP_RST_SDIO: resetReasonStr = "SDIO"; break;
            default: resetReasonStr = "OTHER"; break;
        }

    #else
        boardName = "ESP8266";
        chipIdStr = String(ESP.getChipId(), HEX);
        resetReasonStr = ESP.getResetReason();
    #endif

    #ifdef ARDUINO_BOARD
        boardName = STR(ARDUINO_BOARD);
        boardName.replace("\"", "");
    #endif

    json += "\"board\":\"" + boardName + "\",";
    json += "\"chip_id\":\"" + chipIdStr + "\",";
    json += "\"cpu_freq\":" + String(ESP.getCpuFreqMHz()) + ",";
    json += "\"reset_reason\":\"" + resetReasonStr + "\",";

    // Memory Info flash
    json += "\"chip_size\":" + String(ESP.getFlashChipSize()) + ",";
    json += "\"flash_speed\":" + String(ESP.getFlashChipSpeed()) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"sketch_size\":" + String(ESP.getSketchSize()) + ",";
    json += "\"free_sketch_space\":" + String(ESP.getFreeSketchSpace()) + ",";

    // only ESP8266 Real Flash Size
    #ifdef ESP8266
        json += "\"flash_real_size\":" + String(ESP.getFlashChipRealSize()) + ",";
        json += "\"heap_frag\":" + String(ESP.getHeapFragmentation()) + ",";
    #endif

    // Filesystem Info (support both LittleFS / SPIFFS)
    size_t fsTotal = 0;
    size_t fsUsed = 0;
    #ifdef ESPASYNCHTTPUPDATESERVER_LITTLEFS
        fsTotal = LittleFS.totalBytes();
        fsUsed = LittleFS.usedBytes();
    #else
        #ifdef ESP32
             // ESP32 SPIFFS
             fsTotal = SPIFFS.totalBytes();
             fsUsed = SPIFFS.usedBytes();
        #else
             // ESP8266 SPIFFS
             FSInfo fs_info;
             SPIFFS.info(fs_info);
             fsTotal = fs_info.totalBytes;
             fsUsed = fs_info.usedBytes;
        #endif
    #endif
    json += "\"fs_size\":" + String(fsTotal) + ",";
    json += "\"fs_used\":" + String(fsUsed) + ",";

    // Software Info
    json += "\"sdk_ver\":\"" + String(ESP.getSdkVersion()) + "\",";
    #ifdef ESP8266
        json += "\"core_ver\":\"" + ESP.getCoreVersion() + "\",";
    #endif

    #ifdef APP_NAME
        json += "\"app_name\":\"" + String(APP_NAME) + "\",";
    #endif
    #ifdef APP_VERSION
        json += "\"app_version\":\"" + String(APP_VERSION) + "\",";
    #endif
    json += "\"firmware_file\":\"" + String(__FILE__) + "\",";
    json += "\"compile_date\":\"" + String(__DATE__) + "\",";
    json += "\"compile_time\":\"" + String(__TIME__) + "\",";

    // Network Info
    String wifiMode = "OFF";
    if (WiFi.getMode() == WIFI_STA) wifiMode = "STA";
    else if (WiFi.getMode() == WIFI_AP) wifiMode = "AP";
    else if (WiFi.getMode() == WIFI_AP_STA) wifiMode = "AP_STA";
    
    json += "\"wifi_mode\":\"" + wifiMode + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"mac\":\"" + WiFi.macAddress() + "\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"gateway\":\"" + WiFi.gatewayIP().toString() + "\",";
    json += "\"subnet\":\"" + WiFi.subnetMask().toString() + "\",";
    json += "\"dns1\":\"" + WiFi.dnsIP(0).toString() + "\",";

    String hostname;
    #ifdef ESP32
        hostname = String(WiFi.getHostname());
    #else
        hostname = String(WiFi.hostname());
    #endif
    json += "\"hostname\":\"" + hostname + "\",";
    json += "\"ssid\":\"" + String(WiFi.SSID()) + "\"";
    json += "}";
    request->send(200, "application/json", json);
});

    // handler for the /boardinfo HTML page
    server->on("/boardinfo", HTTP_GET, [this](AsyncWebServerRequest *request){
        if(username != "" && password != ""){
            if( !request->authenticate(username.c_str(), password.c_str())){
                return request->requestAuthentication();
            }
        }
        #ifdef ESP32
        request->send(200, "text/html", boardInfoPage);
        #else
        request->send_P(200, "text/html", boardInfoPage);
        #endif
    });

    // handler for the /update form page - preflight options
    server->on(path, HTTP_OPTIONS, [this](AsyncWebServerRequest *request){
        AsyncWebServerResponse* response = request->beginResponse(200,F("text/html"), String(F("y")));
        response->addHeader("Access-Control-Allow-Headers", "*");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response); 
        
        authenticated = (username == "" || password == "" || request -> authenticate(username.c_str(), password.c_str()));
        if (!authenticated)
        {
            Log("Unauthenticated Update\n");
            return;
        } 
    });

    // handler for the /update form POST (once file upload finishes)
    server->on(path, HTTP_POST, [this](AsyncWebServerRequest *request){
        // if requestAuthentication() is false second handler will run, else it wont.
        if (!authenticated){
            return request->requestAuthentication();
        }
        if (updateResult != OTA_UpdateResult::OTA_UPDATE_OK){
            AsyncWebServerResponse *response = request->beginResponse(200, F("text/html"), Update.hasError() ? String(F("Update error: ")) + updaterError : "Update aborted by server.");
            response->addHeader("Access-Control-Allow-Headers", "*");
            response->addHeader("Access-Control-Allow-Origin", "*");
            response->addHeader("Connection", "close");
            request->send(response);
        }else{
            #ifdef ESP32
                request->send(200, PSTR("text/html"), successResponse);
            #else
                request->send_P(200, PSTR("text/html"), successResponse);
            #endif
            
            if (request->hasArg("reboot") && request->arg("reboot") == "false") {
                Log("Reboot skipped by request\n");
            } else {
                Log("Rebooting...\n");
                restartTimer.once_ms(1000,[]{ ESP.restart(); });
            }
        }},[this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
            // handler for the file upload, gets the sketch bytes, and writes
            // them through the Update object
            const AsyncWebParameter* nameParam = request->getParam("name");
            String updateName = nameParam ? nameParam->value() : "firmware";
            
            if (updateName == "filesystem") {
                updateType = OTA_UpdateType::OTA_FILE_SYSTEM;
            } else {
                updateType = OTA_UpdateType::OTA_FIRMWARE;
            }

            if (!index){
                updaterError.clear();

                #ifdef ESPASYNCUPDATESERVER_DEBUG
                    ESPASYNCUPDATESERVER_COUT.setDebugOutput(true);
                #endif
                authenticated = (username == emptyString || password == emptyString || request->authenticate(username.c_str(), password.c_str()));
                if (!authenticated){
                    Log("Unauthenticated Update\n");
                    return;
                }

                if (onUpdateBegin){
                    updateResult = OTA_UpdateResult::OTA_UPDATE_OK;
                    onUpdateBegin(updateType, updateResult);
                    if (updateResult != OTA_UpdateResult::OTA_UPDATE_OK){
                        Log("Update aborted by server: %d\n", updateResult);
                        if(onUpdateEnd)
                            onUpdateEnd(updateType, updateResult);
                        return;
                    }
                }

                Log("Update: %s\n", filename.c_str());
#ifdef ESP8266
                Update.runAsync(true);
#endif
                if (updateType == OTA_UpdateType::OTA_FILE_SYSTEM){
                    Log("updating filesystem\n");
#ifdef ESP8266
                    int command = U_FS;
                    size_t fsSize = ((size_t)FS_end - (size_t)FS_start);
                    close_all_fs();
#elif defined(ESP32)
                    int command = U_SPIFFS;
    #ifdef ESPASYNCHTTPUPDATESERVER_LITTLEFS
                    size_t fsSize = LittleFS.totalBytes();
    #else
                    size_t fsSize = SPIFFS.totalBytes();
    #endif
#endif
                    if (!Update.begin(fsSize, command))
                    { // start with max available size
#ifdef ESPASYNCHTTPUPDATESERVER_DEBUG
                        Update.printError(ESPASYNCHTTPUPDATESERVER_SerialOutput);
#endif
                    }
                }
                else{
                    Log("updating flash\n");
                    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
                    if (!Update.begin(maxSketchSpace, U_FLASH)) // start with max available size
                        setUpdaterError();
                }
            }

            if (authenticated && len && updateResult == OTA_UpdateResult::OTA_UPDATE_OK){
                Log(".");
                if (Update.write(data, len) != len)
                    setUpdaterError();
            }

            if (authenticated && final && updateResult == OTA_UpdateResult::OTA_UPDATE_OK){
                if (Update.end(true)){
                    // true to set the size to the current progress
                    Log("Update Success.\n");
                    updateResult = OTA_UpdateResult::OTA_UPDATE_OK;
                    if(onUpdateEnd){
                        onUpdateEnd(updateType, updateResult);
                    }
                }
                else{
                    setUpdaterError();
                }
#ifdef ESPASYNCHTTPUPDATESERVER_DEBUG
                ESPASYNCHTTPUPDATESERVER_SerialOutput.setDebugOutput(false);
#endif
            }
            else{
                return;
            }
        });
}

void ESPAsyncUpdateServer::setUpdaterError()
{
#ifdef ESPASYNCUPDATESERVER_DEBUG
    Update.printError(ESPASYNCUPDATESERVER_COUT);
#endif
    StreamString str;
    Update.printError(str);
    updaterError = str.c_str();
    
    updateResult = OTA_UpdateResult::OTA_UPDATE_ERROR;
    if(onUpdateEnd){
        onUpdateEnd(updateType, updateResult);
    }
}

void ESPAsyncUpdateServer::on(int event, std::function<void(const OTA_UpdateType, int&)> func)
{
    if (event == UPDATE_BEGIN)
    {
        this->onUpdateBegin = func;
    }
    else if (event == UPDATE_END)
    {
        this->onUpdateEnd = func;
    }
}