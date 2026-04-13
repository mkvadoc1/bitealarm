#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "config_node.h"
#include "espnow_msg.h"

// ─── ESP-NOW broadcast adresa ────────────────────────────────────────
static const uint8_t BROADCAST_MAC[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// ─── NASTAVENIA ──────────────────────────────────────────────────────
struct Settings {
    uint8_t  sensitivity = DEFAULT_SENSITIVITY;
    uint8_t  volume      = DEFAULT_VOLUME;
    uint16_t toneHz      = DEFAULT_TONE_HZ;
};
Settings cfg;

// ─── STAV ────────────────────────────────────────────────────────────
bool     systemEnabled      = true;
bool     alarmActive        = false;
uint8_t  beepCount          = 0;
bool     beepOn             = false;
uint32_t beepTimer          = 0;

bool     lastSensorState    = HIGH;
bool     stableSensorState  = HIGH;
uint32_t sensorDebTimer     = 0;

bool     lastBtnState       = HIGH;
uint32_t btnPressTime       = 0;
bool     btnHandled         = false;

uint32_t lastHeartbeat      = 0;

// ─── PROTOTYPY ───────────────────────────────────────────────────────
void sendPacket(EventType evt);
void startAlarm();
void updateAlarm();
void stopAlarm();
void toneOn(uint16_t freq, uint8_t vol);
void toneOff();
void handleSensor();
void handleButton();

// ─── ESP-NOW CALLBACK (prijatá správa od iného nodu) ─────────────────
void onReceive(const uint8_t *mac, const uint8_t *data, int len) {
    if (len != sizeof(AlarmPacket)) return;
    AlarmPacket pkt;
    memcpy(&pkt, data, sizeof(pkt));

    // Iný prút dostal záber → aj my pípneme (ak sme zapnutí)
    if (pkt.event == EVT_BITE && pkt.nodeId != NODE_ID && systemEnabled && !alarmActive) {
        Serial.printf("[ESP-NOW] Záber na prúte %d — sympatický alarm\n", pkt.nodeId);
        startAlarm();
    }
}

void onSent(const uint8_t *mac, esp_now_send_status_t status) {
    // voliteľné logovanie
}

// ════════════════════════════════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    Serial.printf("\n[BOOT] FishAlarm NODE %d\n", NODE_ID);

    pinMode(PIN_SENSOR, INPUT_PULLUP);
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    pinMode(PIN_LED,    OUTPUT);
    digitalWrite(PIN_LED, LOW);

    ledcSetup(LEDC_CHANNEL, DEFAULT_TONE_HZ, LEDC_BITS);
    ledcAttachPin(PIN_BUZZER, LEDC_CHANNEL);
    ledcWrite(LEDC_CHANNEL, 0);

    // ESP-NOW vyžaduje WiFi v STA móde
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] Chyba inicializácie!");
        return;
    }
    esp_now_register_recv_cb(onReceive);
    esp_now_register_send_cb(onSent);

    // Registruj broadcast peer
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, BROADCAST_MAC, 6);
    peer.channel = 0;
    peer.encrypt = false;
    esp_now_add_peer(&peer);

    Serial.printf("[ESP-NOW] OK | MAC: %s\n", WiFi.macAddress().c_str());
    Serial.printf("[CFG] SENS:%d VOL:%d TONE:%dHz\n",
                  cfg.sensitivity, cfg.volume, cfg.toneHz);

    // Krátky pípnut = boot OK
    toneOn(2000, 5);
    delay(80);
    toneOff();
}

// ════════════════════════════════════════════════════════════════════
void loop() {
    handleButton();
    handleSensor();
    updateAlarm();

    // Blikanie LED podľa stavu
    if (!systemEnabled) {
        digitalWrite(PIN_LED, (millis() / 1000) % 2);
    } else if (!alarmActive) {
        digitalWrite(PIN_LED, (millis() / 2000) % 2 == 0 ? HIGH : LOW);
    }

    // Heartbeat
    if (millis() - lastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
        lastHeartbeat = millis();
        sendPacket(EVT_HEARTBEAT);
    }
}

// ─── ODOSLANIE ESP-NOW PAKETU ────────────────────────────────────────
void sendPacket(EventType evt) {
    AlarmPacket pkt;
    pkt.nodeId  = NODE_ID;
    pkt.event   = evt;
    pkt.battery = 255;  // TODO: ADC meranie batérie
    pkt.rssi    = 0;

    esp_now_send(BROADCAST_MAC, (uint8_t*)&pkt, sizeof(pkt));
    Serial.printf("[TX] event=0x%02X nodeId=%d\n", evt, NODE_ID);
}

// ─── SENZOR ZÁBEROV ──────────────────────────────────────────────────
void handleSensor() {
    if (!systemEnabled || alarmActive) return;

    bool raw = digitalRead(PIN_SENSOR);
    uint32_t debMs = SENSOR_DEBOUNCE_BASE_MS * cfg.sensitivity;

    if (raw != lastSensorState) {
        sensorDebTimer = millis();
        lastSensorState = raw;
    }

    if ((millis() - sensorDebTimer) >= debMs) {
        if (stableSensorState == HIGH && raw == LOW) {
            stableSensorState = LOW;
            Serial.printf("[ALARM] Záber! Node %d\n", NODE_ID);
            sendPacket(EVT_BITE);
            startAlarm();
        } else if (stableSensorState == LOW && raw == HIGH) {
            stableSensorState = HIGH;
        }
    }
}

// ─── ALARM (non-blocking) ────────────────────────────────────────────
void startAlarm() {
    alarmActive = true;
    beepCount   = 0;
    beepOn      = true;
    beepTimer   = millis();
    digitalWrite(PIN_LED, HIGH);
    toneOn(cfg.toneHz, cfg.volume);
}

void updateAlarm() {
    if (!alarmActive) return;
    uint32_t now = millis();

    if (beepOn) {
        if ((now - beepTimer) >= ALARM_BEEP_MS) {
            toneOff();
            beepOn = false;
            beepCount++;
            beepTimer = now;
            if (beepCount >= ALARM_BEEP_COUNT) {
                stopAlarm();
            }
        }
    } else {
        if ((now - beepTimer) >= ALARM_BEEP_PAUSE_MS) {
            toneOn(cfg.toneHz, cfg.volume);
            beepOn = true;
            beepTimer = now;
        }
    }
}

void stopAlarm() {
    alarmActive = false;
    toneOff();
    digitalWrite(PIN_LED, LOW);
}

// ─── PWM BZUČIAK ─────────────────────────────────────────────────────
void toneOn(uint16_t freq, uint8_t vol) {
    ledcSetup(LEDC_CHANNEL, freq, LEDC_BITS);
    ledcAttachPin(PIN_BUZZER, LEDC_CHANNEL);
    uint8_t duty = (uint8_t)map(vol, 1, 10, 10, 255);
    ledcWrite(LEDC_CHANNEL, duty);
}

void toneOff() {
    ledcWrite(LEDC_CHANNEL, 0);
}

// ─── TLAČIDLO ────────────────────────────────────────────────────────
void handleButton() {
    bool raw = digitalRead(PIN_BUTTON);

    if (raw == LOW && lastBtnState == HIGH) {
        btnPressTime = millis();
        btnHandled   = false;
    }

    if (raw == LOW && !btnHandled) {
        if ((millis() - btnPressTime) >= BUTTON_LONG_PRESS_MS) {
            // Dlhé stlačenie → reset nastavení
            cfg = Settings{};
            btnHandled = true;
            Serial.println("[BTN] Reset nastavení");
            toneOn(1000, 3); delay(100); toneOff();
            delay(100);
            toneOn(1000, 3); delay(100); toneOff();
        }
    }

    if (raw == HIGH && lastBtnState == LOW && !btnHandled) {
        if ((millis() - btnPressTime) >= 50) {
            if (alarmActive) {
                // Krátke stlačenie počas alarmu → zastav
                stopAlarm();
                sendPacket(EVT_ALARM_OFF);
            } else {
                // Krátke stlačenie = toggle ON/OFF
                systemEnabled = !systemEnabled;
                Serial.printf("[BTN] Systém %s\n", systemEnabled ? "ON" : "OFF");
            }
        }
    }

    lastBtnState = raw;
}
