#pragma once

// ─── PINY ────────────────────────────────────────────────────────────
#define PIN_BUZZER      25   // Piezo bzučiak (LEDC PWM)
#define PIN_VIBRO       26   // Vibračný modul (IN pin)
#define PIN_LED_RED     27   // Červená LED — alarm / záber (cez 100Ω)
#define PIN_LED_GREEN   14   // Zelená LED — BT status (cez 100Ω)
// Poznámka: PIN_RGB_DATA (WS2812B) — pridaj neskôr na GPIO 13

// ─── LEDC ────────────────────────────────────────────────────────────
#define LEDC_CHANNEL    0
#define LEDC_BITS       8

// ─── BLE — Nordic UART Service (NUS) ────────────────────────────────
// Kompatibilné s: nRF Toolbox, Serial Bluetooth Terminal (BLE móde),
//                 LightBlue, BLE Terminal HC-08
#define BLE_DEVICE_NAME   "FishAlarm-RX"
#define NUS_SERVICE_UUID  "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_CHAR_UUID  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // mobil → ESP
#define NUS_TX_CHAR_UUID  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // ESP → mobil

// ─── PREDVOLENÉ NASTAVENIA ───────────────────────────────────────────
#define DEFAULT_VOLUME      7    // 1–10
#define DEFAULT_TONE_HZ  2500
#define DEFAULT_VIBRO    true

// ─── ALARM SEKVENCIE ─────────────────────────────────────────────────
#define ALARM_BEEP_MS        250
#define ALARM_BEEP_COUNT       4
#define ALARM_BEEP_PAUSE_MS  150
#define ALARM_VIBRO_MS       400   // dĺžka jedného vibračného pulzu
#define ALARM_VIBRO_PAUSE_MS 200
#define ALARM_VIBRO_COUNT      3

// ─── LED ─────────────────────────────────────────────────────────────
#define LED_FLASH_MS        300   // dĺžka jedného záblesku
#define LED_FLASH_COUNT       6   // počet záblesku pri alarme
#define LED_FLASH_PAUSE_MS  200

// ─── TIMEOUT ─────────────────────────────────────────────────────────
// Ak node nepošle heartbeat dlhšie ako toto, označí sa ako offline
#define NODE_TIMEOUT_MS    90000  // 90 sekúnd (3× heartbeat interval)
