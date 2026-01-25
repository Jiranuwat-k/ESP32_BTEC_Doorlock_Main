#ifndef CONFIG_H
#define CONFIG_H
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

#endif