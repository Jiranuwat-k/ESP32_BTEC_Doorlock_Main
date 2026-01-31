#ifndef CONFIG_H
#define CONFIG_H
/* Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino       ESP32
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST           custom 4
 * SPI SS 1    SDA(SS)      **        custom, take a unused pin, only HIGH/LOW required         **(5/VSPI)
 * SPI SS 2    SDA(SS)      **        custom, take a unused pin, only HIGH/LOW required         **(15/HSPI)
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16            23
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14            19
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15            18
 *             IRQ
 */
#define RST_PIN       4          // Configurable, see typical pin layout above
#define SS_PIN        5         // Configurable, see typical pin layout above
#define SS2_PIN       33
#define LED_STA_PIN   2
#define BUZZER_PIN    13
#define DOORLOCK_PIN  27
#define ESPASYNCHTTPUPDATESERVER_LITTLEFS
// Define buzzer and doorlock states
// Assuming active LOW for buzzer
#define BUZZER_ON     LOW
#define BUZZER_OFF    HIGH
// Assuming active HIGH for doorlock
#define DOORLOCK_ON   HIGH
#define DOORLOCK_OFF  LOW
#define LED_STA_ON        HIGH
#define LED_STA_OFF       LOW
// HTTP Auth Credentials
const char *http_username = "Admin";
const char *http_password = "Admin";
// Timezone and Daylight Saving Time settings
long timezone = 7;
byte daysavetime = 0;
// Failed to open file for reading.
#define FILE_OK              0
#define FILE_ERR_OPEN       -1
#define FILE_ERR_READ       -2
#define FILE_ERR_WRITE      -3
#define FILE_ERR_APPEND     -4
#define FILE_ERR_DELETE     -5
#define FILE_ERR_NOTFOUND   -6
#define FILE_ERR_UNKNOWN    -7
// LittleFS error codes
#define LittleFS_ERR_OK            0
#define LittleFS_ERR_MOUNT        -1
// VerifyUID return codes
// *** MAINKEY HARDCODED UID ***
// Replace this with your actual Main Key UID
const String MAIN_KEY_UID = "047318E8360289"; 

const char* Role_MainKey        = "99"; // Master key with all permissions & generate other keys (Stored in code only)
const char* Role_Admin          = "50"; // Full access to manage users & generate User/Guest keys
const char* Role_User           = "10"; // Regular access
const char* Role_Guest          = "01"; // One time access
const char* Role_Unknown        = "00"; // unknown UID
#define READER_IN           1
#define READER_OUT          2
#define UID_NOTVALID        400
#define UID_EXPIRED         401
#define UID_NOTFOUND        404
#define UID_ACCESS_GRANTED  200
#define UID_ACCESS_DENIED   403

// New Verify Codes for Logging
#define VERIFY_OK           1
#define VERIFY_DENIED       2
#define VERIFY_NOTFOUND     3
#define VERIFY_EXPIRED      4
#define VERIFY_NOTVALID     5
#define VERIFY_INVALID      6
#define VERIFY_LOGIN_OK     7
#define VERIFY_LOGIN_FAIL   8

// Gender Constants
#define GENDER_MALE         1
#define GENDER_FEMALE       2
#define GENDER_ETC          'X'

// Event Constants
#define EVENT_CREATE        'C'
#define EVENT_MODIFY        'M'
#define EVENT_DELETE        'D'

// Feature Toggles
const bool AUTO_CLEANUP_EXPIRED_GUESTS = false; // Set to true to enable auto-deletion of expired guests
const bool ENABLE_MAINKEY_SYSTEM = false; // Set to false to allow Admin creation without MainKey

#endif