# ESPWiFiManager

ESPWiFiManager is a WiFi configuration manager for ESP8266 boards using `ESPAsyncWebServer`. It allows you to configure WiFi credentials and static IP settings via a web portal when the device cannot connect to a known network or upon first boot.

## Features

- **Automatic Fallback to AP Mode**: If no WiFi credentials are saved or connection fails, it starts an Access Point (AP).
- **Web Configuration Portal**: Provides a user-friendly web interface to scan for networks and enter credentials.
- **Static IP Support**: Configure Static IP, Gateway, Subnet, and DNS servers for both Station (STA) and AP modes.
- **Async Web Server**: Built on top of `ESPAsyncWebServer` for non-blocking operation.
- **Filesystem Storage**: Saves configuration in a structured manner on the filesystem (SPIFFS/LittleFS).
- **mDNS Support**: Easily access the device using a hostname (e.g., `http://espwifimanager.local`).
- **Callbacks**: Hooks for various events like successful connection, configuration mode entry, etc.

## Dependencies

This library depends on the following libraries:

- `ESPAsyncWebServer`
- `ESPAsyncTCP`
- `DNSServer`
- `ESP8266mDNS`

## Installation

1. Copy the `ESPWiFiManager` folder to your project's `lib` directory.
2. Ensure you have the required dependencies installed in your `platformio.ini` or Arduino Library Manager.

## Usage

### Basic Setup

```cpp
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <LittleFS.h> // Or SPIFFS
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include "ESPWiFiManager.h"

AsyncWebServer server(80);
DNSServer dns;
ESPWiFiManager manager(&server, &dns, LittleFS);

void setup() {
    Serial.begin(115200);
    LittleFS.begin(); // Initialize filesystem

    // simpler usage
    manager.begin("MyAP", "password123");
}

void loop() {
    
}
```

### Advanced Usage with Callbacks

```cpp
void onConnected() {
    Serial.println("Connected to WiFi!");
}

void onConfigMode() {
    Serial.println("Entered Config Mode (AP started)");
}

void setup() {
    // ... init ...
    manager.on(SUCCESS, onConnected);
    manager.on(CONFIG, onConfigMode);

    // Set static IP for AP mode if needed
    manager.setAPStaticIPConfig(IPAddress(192,168,10,1), IPAddress(192,168,10,1), IPAddress(255,255,255,0));

    manager.begin();
}
```

## API Reference

### Constructor

```cpp
ESPWiFiManager(AsyncWebServer *server, DNSServer *dns, fs::FS &fs);
```

- `server`: Pointer to `AsyncWebServer` instance.
- `dns`: Pointer to `DNSServer` instance.
- `fs`: Reference to Filesystem (LittleFS or SPIFFS).

### Configuration & Control

#### `bool begin(char const* apName = HostName, char const* apPassword = NULL, int Channel = 1)`

Starts the manager. Uses stored config if available, otherwise starts AP with provided credentials.

- `apName`: SSID for AP mode.
- `apPassword`: Password for AP mode (NULL for open).
- `Channel`: WiFi channel.

#### `void loop()`

Must be called in the main `loop()` to handle timeouts and resets.

#### `void reset()`

Clears the stored configuration (SSID) and effectively resets the device to factory settings regarding WiFi.

#### `void setPath(const char* path)`

Sets the path for the index HTML file. Default is `/espwifimanager.html`.

#### `void setMDNS(const char* Name)`

Sets the mDNS hostname. Default is `ESPWiFiManager`.

#### `void setLogOutput(bool log)`

Enable or disable serial logging.

### Static IP Configuration

#### `void setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn)`

Sets static IP configuration for AP mode.

#### `void setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn, IPAddress dns1, IPAddress dns2)`

Sets static IP configuration for Station mode (when connecting to a router).

### Callbacks

#### `void on(int event, std::function<void()> func)`

Register a callback for an event.

- `event`:
  - `SUCCESS` (1): WiFi connected successfully.
  - `CONFIG` (2): Entered AP/Config mode.
  - `ASYNC_CONFIG` (3): Custom async config event.
  - `PRE_CONNECT` (4): Called before attempting to connect.

#### `void add(std::function<void()> func)`

Adds a custom API handler or function to include in the setup process.

### Helper Methods

#### `bool writeFile(const char* path, const char* message)`

Write string content to a file.

#### `String readFile(const char* path)`

Read content from a file.

#### `String getInfo()`

Returns a JSON string containing system info (Chip ID, Flash size, Heap, etc.).

## HTTP Endpoints (Config Mode)

- `GET /`: Serves the configuration page (`/espwifimanager.html`).
- `GET /scan`: Scans for networks and returns JSON list.
- `GET /info`: Returns system info JSON.
- `GET /restart`: Restarts the ESP.
- `POST /save/wificonfig`: Saves WiFi credentials.
  - Params: `ssid`, `password`, `ip`, `gw`, `sn`, `dns1`, `dns2`.
