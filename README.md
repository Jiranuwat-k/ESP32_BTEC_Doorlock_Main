
# ESP32_BTEC_DoorLock_Main

This project is an ESP32-based door lock system for BTEC, using PlatformIO and the Arduino Framework.

## Features
- Door control with ESP32
- OTA firmware update support
- User management via Web Interface
- Uses LittleFS for data storage

## Getting Started

### 1. Clone the Project
```bash
git clone https://github.com/Jiranuwat-k/ESP32_BTEC_DoorLock_Main.git
cd ESP32_BTEC_DoorLock_Main
```

### 2. Install PlatformIO
- Install [VS Code](https://code.visualstudio.com/)
- Install the [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) extension

### 3. Open the Project in VS Code
1. Open VS Code and select `Open Folder...` to navigate to this project folder
2. PlatformIO will automatically detect the `platformio.ini` file

### 4. Install Libraries (Automatic)
PlatformIO will automatically install the libraries specified in `platformio.ini` on the first build

### 5. Upload the Firmware to ESP32
Connect your ESP32 to your computer, then click the Upload button in PlatformIO or use:
```bash
pio run --target upload
```

### 6. Upload Filesystem Data (LittleFS)
```bash
pio run --target uploadfs
```

## Project Structure
```
src/           # Main source code
lib/           # Additional libraries
data/          # Web files and LittleFS data
include/       # Header files
platformio.ini # Main PlatformIO config file
```

## More Information
- [PlatformIO Documentation](https://docs.platformio.org/)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)

---
**Maintainer:** Jiranuwat.k
