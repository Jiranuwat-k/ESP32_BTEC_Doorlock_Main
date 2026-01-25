#ifndef ESPAsyncUpdateServer_H
#define ESPAsyncUpdateServer_H
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
enum OTA_UpdateType
{
    OTA_FIRMWARE,
    OTA_FILE_SYSTEM
};
enum OTA_UpdateResult
{
    OTA_UPDATE_OK,
    OTA_UPDATE_ABORT,
    OTA_UPDATE_ERROR,
};
class ESPAsyncUpdateServer{
    private:
        #ifndef XSTR
            #define XSTR(x) #x
        #endif
        #ifndef STR
            #define STR(x) XSTR(x)
        #endif
        String username;
        String password;
        bool authenticated;
        String updaterError;
        int updateResult;
        OTA_UpdateType updateType;
        AsyncWebServer *server;
        fs::FS &fs;
    public:
        std::function<void(const OTA_UpdateType, int&)> onUpdateBegin;
        std::function<void(const OTA_UpdateType, int&)> onUpdateEnd;

        #define UPDATE_BEGIN 1
        #define UPDATE_END 2
        ESPAsyncUpdateServer(AsyncWebServer *server,fs::FS &fs);
        void begin(char const* path = "/update",char const* Username = NULL ,char const* Password = NULL);
        void on(int event,std::function<void(const OTA_UpdateType, int&)>); // Update signature
    
    protected:
        void setUpdaterError();

};
#endif
