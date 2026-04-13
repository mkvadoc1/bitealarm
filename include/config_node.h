#pragma once

// NODE_ID sa nastavuje cez build_flags v platformio.ini (-DNODE_ID=1)
#ifndef NODE_ID
  #error "NODE_ID nie je definované! Použi env:node1 / node2 / node3 ..."
#endif

// ─── PINY ────────────────────────────────────────────────────────────
#define PIN_SENSOR     18   // Reed/switch senzor (INPUT_PULLUP, LOW = záber)
#define PIN_BUTTON     19   // Push button ON/OFF (INPUT_PULLUP)
#define PIN_BUZZER     25   // Piezo bzučiak (LEDC PWM)
#define PIN_LED        26   // Jednofarebná LED (indikácia lokálne)

// ─── LEDC ────────────────────────────────────────────────────────────
#define LEDC_CHANNEL   0
#define LEDC_BITS      8

// ─── PREDVOLENÉ NASTAVENIA ───────────────────────────────────────────
#define DEFAULT_SENSITIVITY  3    // 1–10
#define DEFAULT_VOLUME       7    // 1–10
#define DEFAULT_TONE_HZ      2700

// ─── LIMITY ──────────────────────────────────────────────────────────
#define SENS_MIN   1
#define SENS_MAX  10
#define VOL_MIN    1
#define VOL_MAX   10
#define TONE_MIN   200
#define TONE_MAX  4000

// ─── ČASOVÉ KONŠTANTY ────────────────────────────────────────────────
#define SENSOR_DEBOUNCE_BASE_MS   20    // * sensitivity = debounce čas
#define BUTTON_DEBOUNCE_MS        50
#define BUTTON_LONG_PRESS_MS    2000
#define ALARM_BEEP_MS            300
#define ALARM_BEEP_COUNT           3
#define ALARM_BEEP_PAUSE_MS      200
#define HEARTBEAT_INTERVAL_MS  30000   // každých 30s pošle keepalive
