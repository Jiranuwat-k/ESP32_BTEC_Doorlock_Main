#include "ESPWiFiManager.h"
ESPWiFiManager::ESPWiFiManager(AsyncWebServer *server, DNSServer *dns, fs::FS &fs) : server(server), fs(fs), dnsserver(dns) {}
boolean ESPWiFiManager::begin(char const *apName, char const *apPassword, int Channel)
{   
    // Ensure wifi config files exist, create if missing
    if (!fs.exists("/wifi")) {
        fs.mkdir("/wifi");
    }
    const char* wifiFiles[] = {"/wifi/ssid.txt", "/wifi/pass.txt", "/wifi/ip.txt", "/wifi/gw.txt", "/wifi/sn.txt", "/wifi/dns1.txt", "/wifi/dns2.txt"};
    for (int i = 0; i < 7; ++i) {
        if (!fs.exists(wifiFiles[i])) {
            File f = fs.open(wifiFiles[i], "w");
            if (f) f.close();
        }
    }
    this->ssid = this->readFile("/wifi/ssid.txt");
    this->pass = this->readFile("/wifi/pass.txt");
    this->ip = this->readFile("/wifi/ip.txt");
    this->gw = this->readFile("/wifi/gw.txt");
    this->sn = this->readFile("/wifi/sn.txt");
    this->dns1 = this->readFile("/wifi/dns1.txt");
    this->dns2 = this->readFile("/wifi/dns2.txt");
    LOG(this->readFile("/Banner.txt"), false);
    if (this->ssid == "")
    {
        if (this->MDNSName == NULL)
        {
            this->MDNSName = HostName;
        }
        WiFi.mode(WIFI_AP);
        if (MDNS.begin(this->MDNSName))
        {
            LOG(F("MDNS started"));
        }
        if (this->_ap_static_ip)
        {
            LOG(F("[AP Mode] Custom IP/GW/Subnet"));
            WiFi.softAPConfig(this->_ap_static_ip, this->_ap_static_gw, this->_ap_static_sn);
        }
        WiFi.softAP(apName, apPassword, Channel);
        IPAddress IP = WiFi.softAPIP();
        LOG("\n", false);
        LOG(F("Start ESPWiFiManager."));
        LOG("[AP Mode] Hostname: " + String(this->MDNSName));
        LOG("[AP Mode] IP address: " + IP.toString());
        if(this->_ap_static_gw){
            LOG("[AP Mode] Gateway: " + this->_ap_static_gw.toString());
        LOG("[AP Mode] Subnet: " + this->_ap_static_sn.toString());
        }
        LOG("\n", false);
        dnsserver->setErrorReplyCode(DNSReplyCode::NoError);
        if (!dnsserver->start(this->DNS_PORT, "*", WiFi.softAPIP()))
        {
            LOG(F("Could not start Captive DNS Server"));
        }
        server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
        { 
            request->send(this->fs, this->indexPath, "text/html"); 
        });
        server->onNotFound([](AsyncWebServerRequest *request)
        { 
            request->redirect("http://" + WiFi.softAPIP().toString()); 
        });
        this->scanAPI();
        this->getInfoAPI();
        this->wificonfigAPI();
        this->restartAPI();
        if (this->api != NULL)
        {
            this->api();
        }
        if (this->onasyncconfig != NULL)
        {
            this->onasyncconfig();
        }

        server->begin();
        while (1)
        {
            #ifndef ESP32
                MDNS.update();
            #endif
            dnsserver->processNextRequest();
            if (this->onconfig != NULL)
            {
                this->onconfig();
            }
            // check rst timer
            if (this->rstTimer_status && (millis() - this->rstTimer_LastTrigger > 3000))
            {
                ESP.restart();
            }
            yield();
        }
    }
    else
    {
        WiFi.mode(WIFI_STA);
        if(ip != "" && sn != "")
        {
            IPAddress _ip, _gw, _sn, _dns1, _dns2;
            _ip.fromString(ip);
            _gw.fromString(gw);
            _sn.fromString(sn);
            if(dns1 != "") _dns1.fromString(dns1);
            if(dns2 != "") _dns2.fromString(dns2);
            this->setSTAStaticIPConfig(_ip, _gw, _sn, _dns1, _dns2);
        }
        if (_sta_static_ip)
        {
            LOG(F("[STA Mode] Custom IP/GW/Subnet/DNS"));
            WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn, _sta_static_dns1, _sta_static_dns2);
            LOG(WiFi.localIP());
        }
#if defined(ESP8266)
        // fix connection in progress hanging
        ETS_UART_INTR_DISABLE();
        wifi_station_disconnect();
        ETS_UART_INTR_ENABLE();
#else
        WiFi.disconnect(false);
#endif
        WiFi.begin(this->ssid.c_str(), this->pass.c_str());
        LOG("\n", false);
        LOG(F("Connecting to WiFi..."));
        if (this->onpreconnect != NULL)
        {
            this->onpreconnect();
        }
        dnsserver->stop();
        this->previousMillis = millis();
        while (WiFi.status() != WL_CONNECTED)
        {
            LOG(F("wait a moment"));
            delay(10);
            if (this->retryInterval && (millis() - this->previousMillis >= this->interval))
            {
                LOG("\n", false);
                LOG(F("Failed to connect."));
                this->reset();
                ESP.restart();
            }
        }
        LOG("\n", false);
        LOG(WiFi.localIP());
        LOG("\n", false);
        if (this->onsuccess != NULL)
        {
            this->onsuccess();
        }
    }
    return true;
}
void ESPWiFiManager::wificonfigAPI()
{
    server->on("/save/wificonfig", HTTP_POST, [this](AsyncWebServerRequest *request)
               {
    if (request->hasParam("ssid", true)) {
      const AsyncWebParameter* p = request->getParam("ssid", true);
      this->ssid = p->value().c_str();
      this->writeFile("/wifi/ssid.txt", this->ssid.c_str());
    }
    if (request->hasParam("password", true)) {
      const AsyncWebParameter* p = request->getParam("password", true);
      this->pass = p->value().c_str();
      this->writeFile("/wifi/pass.txt", this->pass.c_str());
    }
    if(request->hasParam("ip", true)){
      const AsyncWebParameter* p = request->getParam("ip", true);
      this->ip = p->value().c_str();
      this->writeFile("/wifi/ip.txt", this->ip.c_str());
    }
    if(request->hasParam("gw", true)){
      const AsyncWebParameter* p = request->getParam("gw", true);
      this->gw = p->value().c_str();
      this->writeFile("/wifi/gw.txt", this->gw.c_str());
    }
    if(request->hasParam("sn", true)){
      const AsyncWebParameter* p = request->getParam("sn", true);
      this->sn = p->value().c_str();
      this->writeFile("/wifi/sn.txt", this->sn.c_str());
    }
    if(request->hasParam("dns1", true)){
      const AsyncWebParameter* p = request->getParam("dns1", true);
      this->dns1 = p->value().c_str();
      this->writeFile("/wifi/dns1.txt", this->dns1.c_str());
    }
    if(request->hasParam("dns2", true)){
      const AsyncWebParameter* p = request->getParam("dns2", true);
      this->dns2 = p->value().c_str();
      this->writeFile("/wifi/dns2.txt", this->dns2.c_str());
    }
    LOG(F("Save Config Success."));
    request->send(200, "text/plain", "Done."); });
}
void ESPWiFiManager::scanAPI()
{
    server->on("/scan", HTTP_GET, [this](AsyncWebServerRequest *request)
    {
        LOG(F("WiFi Scan."));
        int n = WiFi.scanComplete();
    
        // If scan not started (-2), start it
        if (n == -2) {
        WiFi.scanNetworks(true);
        request->send(200, "application/json", "[]");
        return;
        }
    
        // If scanning (-1), return empty
        if (n == -1) {
        request->send(200, "application/json", "[]");
        return;
        }

        String json;
        json.reserve(2048); // Reserve memory to prevent fragmentation
        json = "[";
    
        if (n > 0) {
            for (int i = 0; i < n; ++i) {
                if (i) json += ",";
                json += "{";
                json += "\"rssi\":" + String(WiFi.RSSI(i));
                json += ",\"ssid\":\"" + WiFi.SSID(i) + "\"";
                json += ",\"bssid\":\"" + WiFi.BSSIDstr(i) + "\"";
                json += ",\"channel\":" + String(WiFi.channel(i));
                json += ",\"secure\":" + String(WiFi.encryptionType(i));
                #if defined(ESP8266)
                json += ",\"hidden\":" + String(WiFi.isHidden(i) ? "true" : "false");
                #else
                json += ",\"hidden\":false";
                #endif
                json += "}";
            }
            WiFi.scanDelete();
            // Start next scan for next refresh
            WiFi.scanNetworks(true);
        }
        json += "]";
        request->send(200, "application/json", json);
        json = ""; });
}
void ESPWiFiManager::getInfoAPI(){
    server->on("/info", HTTP_GET, [this](AsyncWebServerRequest *request)
    {
        request->send(200, "application/json", this->getInfo());
    });
}
void ESPWiFiManager::restartAPI()
{
    server->on("/restart", HTTP_GET, [this](AsyncWebServerRequest *request)
    {
        request->send(200, "text/html", "Done.");
        LOG(F("Restart."));
        this->rstTimer_LastTrigger = millis();
        this->rstTimer_status = true; });
}
void ESPWiFiManager::reset()
{
    LOG(F("Reset Config Success."), 1);
    this->writeFile("/wifi/ssid.txt", "");
    this->writeFile("/wifi/pass.txt", "");
    this->writeFile("/wifi/ip.txt", "");
    this->writeFile("/wifi/gw.txt", "");
    this->writeFile("/wifi/sn.txt", "");
    this->writeFile("/wifi/dns1.txt", "");
    this->writeFile("/wifi/dns2.txt", "");
}
String ESPWiFiManager::readFile(const char *path)
{
    File file = fs.open(path, "r");
    if (!file || file.isDirectory())
    {
        return "";
    }
    String fileContent;
    while (file.available())
    {
        fileContent = file.readStringUntil('\0');
        break;
    }
    file.close();
    return fileContent;
}
void ESPWiFiManager::add(std::function<void()> func)
{
    this->api = func;
}
void ESPWiFiManager::on(int event, std::function<void()> func)
{
    if (event == WM_SUCCESS)
    {
        this->onsuccess = func;
    }
    else if (event == WM_CONFIG)
    {
        this->onconfig = func;
    }
    else if (event == WM_ASYNC_CONFIG)
    {
        this->onasyncconfig = func;
    }
    else if (event == WM_PRE_CONNECT)
    {
        this->onpreconnect = func;
    }
}
boolean ESPWiFiManager::writeFile(const char *path, const char *message)
{
    File file = fs.open(path, "w");
    if (!file)
    {
        return 1;
    }
    if (file.print(message))
    {
        file.close();
        return 0;
    }
    else
    {
        return 1;
    }
}
template <typename Generic>
void ESPWiFiManager::LOG(Generic text, boolean source)
{
    if (_log)
    {
        if (source)
        {
            Serial.print(F("\nESPWiFiManager -> "));
        }
        Serial.print(text);
    }
}
void ESPWiFiManager::setAPStaticIPConfig(IPAddress ip,
                                         IPAddress gw,
                                         IPAddress sn)
{
    this->_ap_static_ip = ip;
    this->_ap_static_gw = gw;
    this->_ap_static_sn = sn;
}
void ESPWiFiManager::setSTAStaticIPConfig(IPAddress ip,
                                          IPAddress gw,
                                          IPAddress sn,
                                          IPAddress dns1,
                                          IPAddress dns2)
{
    this->_sta_static_ip = ip;
    this->_sta_static_gw = gw;
    this->_sta_static_sn = sn;
    this->_sta_static_dns1 = dns1;
    this->_sta_static_dns2 = dns2;
}
void ESPWiFiManager::setMDNS(const char *Name)
{
    this->MDNSName = Name;
}
void ESPWiFiManager::setLogOutput(boolean log)
{
    this->_log = log;
}
void ESPWiFiManager::setPath(const char *path)
{
    this->indexPath = path;
}
String ESPWiFiManager::getInfo()
{
    // Reserve memory to prevent heap fragmentation
    String json;
    json.reserve(600); 

    json = "{";


    #if defined(ESP32)
    String boardName = "ESP32";
    #ifdef ARDUINO_BOARD
        boardName = STR(ARDUINO_BOARD);
        boardName.replace("\"", "");
    #endif
    json += "\"board\":\"" + boardName + "\",";
    // --- Chip Information ---
    json += "\"chipRevision\":\"" + String(ESP.getChipRevision()) + "\",";
    json += "\"cpuFreqMHz\":" + String(ESP.getCpuFreqMHz()) + ",";
    // --- Flash Memory Information ---
    json += "\"flashSize\":" + String(ESP.getFlashChipSize()) + ",";
    json += "\"flashSpeed\":" + String(ESP.getFlashChipSpeed()) + ",";
    // --- RAM / Heap Information ---
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    // --- Sketch (Firmware) Information ---
    json += "\"sketchSize\":" + String(ESP.getSketchSize()) + ",";
    json += "\"freeSketchSpace\":" + String(ESP.getFreeSketchSpace()) + ",";
    // --- Version Information ---
    json += "\"sdkVersion\":\"" + String(ESP.getSdkVersion()) + "\",";
    // --- Reset Information ---
    #include <esp_system.h>
    esp_reset_reason_t reason = esp_reset_reason();
    String reasonStr;
    switch (reason) {
        case ESP_RST_UNKNOWN: reasonStr = "UNKNOWN"; break;
        case ESP_RST_POWERON: reasonStr = "POWERON"; break;
        case ESP_RST_EXT: reasonStr = "EXT"; break;
        case ESP_RST_SW: reasonStr = "SW"; break;
        case ESP_RST_PANIC: reasonStr = "PANIC"; break;
        case ESP_RST_INT_WDT: reasonStr = "INT_WDT"; break;
        case ESP_RST_TASK_WDT: reasonStr = "TASK_WDT"; break;
        case ESP_RST_WDT: reasonStr = "WDT"; break;
        case ESP_RST_DEEPSLEEP: reasonStr = "DEEPSLEEP"; break;
        case ESP_RST_BROWNOUT: reasonStr = "BROWNOUT"; break;
        case ESP_RST_SDIO: reasonStr = "SDIO"; break;
        default: reasonStr = "OTHER"; break;
    }
    json += "\"resetReason\":\"" + reasonStr + "\",";
    #else
    String boardName = "ESP8266 NodeMCU";
    #ifdef ARDUINO_BOARD
        boardName = STR(ARDUINO_BOARD);
        boardName.replace("\"", "");
    #endif
    json += "\"board\":\"" + boardName + "\",";
    // --- Chip Information ---
    json += "\"chipId\":\"" + String(ESP.getChipId(), HEX) + "\",";
    json += "\"cpuFreqMHz\":" + String(ESP.getCpuFreqMHz()) + ",";
    json += "\"cycleCount\":" + String(ESP.getCycleCount()) + ",";
    // --- Flash Memory Information ---
    json += "\"flashChipId\":\"" + String(ESP.getFlashChipId(), HEX) + "\",";
    json += "\"flashSizeSdk\":" + String(ESP.getFlashChipSize()) + ",";
    json += "\"flashSizeReal\":" + String(ESP.getFlashChipRealSize()) + ",";
    json += "\"flashSpeed\":" + String(ESP.getFlashChipSpeed()) + ",";
    // --- RAM / Heap Information ---
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"maxFreeBlock\":" + String(ESP.getMaxFreeBlockSize()) + ",";
    json += "\"heapFrag\":" + String(ESP.getHeapFragmentation()) + ",";
    // --- Sketch (Firmware) Information ---
    json += "\"sketchSize\":" + String(ESP.getSketchSize()) + ",";
    json += "\"freeSketchSpace\":" + String(ESP.getFreeSketchSpace()) + ",";
    json += "\"sketchMD5\":\"" + ESP.getSketchMD5() + "\",";
    // --- Version Information ---
    json += "\"coreVersion\":\"" + ESP.getCoreVersion() + "\",";
    json += "\"sdkVersion\":\"" + String(ESP.getSdkVersion()) + "\",";
    // --- Reset Information ---
    json += "\"resetReason\":\"" + ESP.getResetReason() + "\",";
    #endif

    // --- Build Metadata (File & Time) ---
    // __FILE__ : Full source file path
    // __DATE__ : Compilation date
    // __TIME__ : Compilation time
    
    // Replace backslashes with forward slashes for valid JSON format (Windows paths)
    // (custom_app_name)
    #ifdef APP_NAME
        json += "\"appName\":\"" + String(APP_NAME) + "\",";
    #endif

    // (custom_app_version)
    #ifdef APP_VERSION
        json += "\"appVersion\":\"" + String(APP_VERSION) + "\",";
    #endif

    // (FIRMWARE_FILENAME)
    #ifdef FIRMWARE_FILENAME
        json += "\"firmwareFile\":\"" + String(FIRMWARE_FILENAME) + "\",";
    #endif
    json += "\"compileDate\":\"" + String(__DATE__) + "\",";
    json += "\"compileTime\":\"" + String(__TIME__) + "\""; // No comma for the last element

    json += "}";
    return json;
}
void ESPWiFiManager::setRetryInterval(bool Mode, unsigned int interval){
    // true = enable
    // false = disable
    this->retryInterval = Mode;
    this->interval = interval;
}
void ESPWiFiManager::loop()
{
    // check rst timer
    if (this->rstTimer_status && (millis() - this->rstTimer_LastTrigger > 3000))
    {
        ESP.restart();
    }
}
