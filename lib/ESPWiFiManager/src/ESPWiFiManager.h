#ifndef ESPWIFIMANAGER_H
#define ESPWIFIMANAGER_H
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#if defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif
#include <FS.h>
#define ESPWIFIMANAGER_VERSION "1.1.0"
//#include <ESPAsyncDNSServer.h>
class ESPWiFiManager{
    private:
      // Helper Macros
      #ifndef XSTR
        #define XSTR(x) #x
      #endif
      #ifndef STR
        #define STR(x) XSTR(x)
      #endif
      AsyncWebServer *server;
      fs::FS &fs;
      DNSServer *dnsserver;
      std::function<void()> onsuccess;
      std::function<void()> onconfig;
      std::function<void()> onasyncconfig;
      std::function<void()> onpreconnect;
      std::function<void()> api;
      String ssid;
      String pass;
      String ip;
      String gw;
      String sn;
      String dns1;
      String dns2;
      IPAddress _ap_static_ip;
      IPAddress _ap_static_gw;
      IPAddress _ap_static_sn;
      IPAddress _sta_static_ip;
      IPAddress _sta_static_gw;
      IPAddress _sta_static_sn;
      IPAddress _sta_static_dns1 = (uint32_t)0x00000000;
      IPAddress _sta_static_dns2 = (uint32_t)0x00000000;
      unsigned long previousMillis = 0;
      unsigned int interval = 10000;
      unsigned long rstTimer_LastTrigger = 0;
      bool rstTimer_status = false;
      bool retryInterval = true;
      const byte DNS_PORT = 53;
      const char* MDNSName;
      const char* indexPath = "/espwifimanager.html";
      bool _log = true;
      template <typename Generic>
      void LOG(Generic text,boolean source = true);
      void scanAPI();
      void getInfoAPI();
      void wificonfigAPI();
      void restartAPI();
    public:
      #define WM_SUCCESS 1
      #define WM_CONFIG 2
      #define WM_ASYNC_CONFIG 3
      #define WM_PRE_CONNECT 4
      static constexpr const char* HostName = "ESPWiFiManager";
      ESPWiFiManager(AsyncWebServer *server,DNSServer *dns,fs::FS &fs);
      bool begin(char const* apName = HostName,char const* apPassword = NULL, int Channel = 1);
      void on(int event,std::function<void()>);
      void add(std::function<void()>);
      void setPath(const char* path);
      String readFile(const char* path);
      bool writeFile(const char* path, const char* message);
      void setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn);
      void setSTAStaticIPConfig(IPAddress ip,
                                IPAddress gw,
                                IPAddress sn,
                                IPAddress dns1 = (uint32_t)0x00000000,
                                IPAddress dns2 = (uint32_t)0x00000000);
      void setMDNS(const char* Name = HostName);
      void setLogOutput(bool log);
      void setRetryInterval(bool Mode = true, unsigned int interval = 10000);
      String getInfo();
      void reset();
      void loop();
  };
#endif
