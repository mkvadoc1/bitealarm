#pragma once

// ─── PIN KONFIGURÁCIA ───────────────────────────────────────────────
#define PIN_SENSOR      18    // Reed/switch senzor záberov (INPUT_PULLUP)
#define PIN_BUTTON      19    // Push button ON/OFF (INPUT_PULLUP)
#define PIN_BUZZER      25    // Piezo bzučiak (PWM, LEDC channel 0)
#define PIN_LED_ALARM   26    // LED - alarm / zaber
#define PIN_LED_STATUS  27    // LED - BT / stav systému

// ─── LEDC PWM ───────────────────────────────────────────────────────
#define LEDC_CHANNEL    0
#define LEDC_TIMER_BITS 8     // 8-bit rozlíšenie (0-255)
#define LEDC_BASE_FREQ  2000  // bude prepísaná nastaveným tónom

// ─── PREDVOLENÉ NASTAVENIA ──────────────────────────────────────────
#define DEFAULT_SENSITIVITY   3     // 1-10 (čím vyššie, tým dlhší debounce → menej citlivý)
#define DEFAULT_VOLUME        7     // 1-10
#define DEFAULT_TONE_HZ       2700  // Hz, frekvencia bzučania

// ─── LIMITY ─────────────────────────────────────────────────────────
#define SENSITIVITY_MIN  1
#define SENSITIVITY_MAX  10
#define VOLUME_MIN       1
#define VOLUME_MAX       10
#define TONE_MIN_HZ      200
#define TONE_MAX_HZ      4000

// ─── ČASOVÉ KONŠTANTY ───────────────────────────────────────────────
// Debounce pre senzor záberov: sensitivity 1 = 20ms, 10 = 200ms
#define SENSOR_DEBOUNCE_BASE_MS  20
// Debounce tlačidla ON/OFF
#define BUTTON_DEBOUNCE_MS       50
// Dlhé stlačenie tlačidla → reset nastavení
#define BUTTON_LONG_PRESS_MS    2000
// Dĺžka jedného pípnutia pri zábere
#define ALARM_BEEP_MS           300
// Počet pípnutí pri zábere
#define ALARM_BEEP_COUNT         3
// Pauza medzi pípnutiami
#define ALARM_BEEP_PAUSE_MS     200

// ─── BLUETOOTH ──────────────────────────────────────────────────────
#define BT_DEVICE_NAME  "FishAlarm"
