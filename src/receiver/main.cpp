#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <NimBLEDevice.h>
#include <FastLED.h>
#include "config_receiver.h"
#include "espnow_msg.h"

// ════════════════════════════════════════════════════════════════════
// BLE — Nordic UART Service
// ════════════════════════════════════════════════════════════════════
static NimBLECharacteristic* pTxChar    = nullptr;
static bool                  bleConnected = false;

// Odošli správu cez BLE notify
void bleSend(const String& msg) {
    if (!bleConnected || !pTxChar) return;
    pTxChar->setValue(msg.c_str());
    pTxChar->notify();
}

// ─── BLE Server callbacks ─────────────────────────────────────────────
class ServerCB : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pSrv) override {
        bleConnected = true;
        Serial.println("[BLE] Klient pripojený");
    }
    void onDisconnect(NimBLEServer* pSrv) override {
        bleConnected = false;
        Serial.println("[BLE] Klient odpojený — znovu advertisujem");
        NimBLEDevice::getServer()->startAdvertising();
    }
};

// ─── RX Characteristic callbacks (príkazy z mobilu) ──────────────────
void parseBTCommand(const String& cmd);   // forward decl

class RxCB : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pChar) override {
        std::string raw = pChar->getValue();
        String cmd = String(raw.c_str());
        cmd.trim();
        if (cmd.length()) {
            Serial.printf("[BLE RX] %s\n", cmd.c_str());
            parseBTCommand(cmd);
        }
    }
};

// ════════════════════════════════════════════════════════════════════
// NASTAVENIA
// ════════════════════════════════════════════════════════════════════
struct ReceiverSettings {
    uint8_t  volume = DEFAULT_VOLUME;
    uint16_t toneHz = DEFAULT_TONE_HZ;
    bool     vibro  = DEFAULT_VIBRO;
    bool     sound  = true;
    uint8_t  colors[ESPNOW_MAX_NODES][3];

    ReceiverSettings() {
        memcpy(colors, NODE_COLORS, sizeof(NODE_COLORS));
    }
};
ReceiverSettings cfg;

// ════════════════════════════════════════════════════════════════════
// STAV NODOV
// ════════════════════════════════════════════════════════════════════
struct NodeState {
    bool     online   = false;
    uint32_t lastSeen = 0;
    uint8_t  battery  = 255;
};
NodeState nodes[ESPNOW_MAX_NODES];

// ════════════════════════════════════════════════════════════════════
// ALARM — zdieľané medzi ESP-NOW ISR a hlavnou slučkou
// ════════════════════════════════════════════════════════════════════
volatile bool    alarmPending = false;
volatile uint8_t alarmNodeId  = 0;

bool     alarmActive = false;
uint8_t  alarmNode   = 0;
CRGB     alarmColor  = CRGB::Black;

// LED flash
uint8_t  ledFlashCount = 0;
bool     ledOn         = false;
uint32_t ledTimer      = 0;

// Bzučiak
uint8_t  beepCount  = 0;
bool     beepOn     = false;
uint32_t beepTimer  = 0;

// Vibrácia
uint8_t  vibroCount = 0;
bool     vibroOn    = false;
uint32_t vibroTimer = 0;

// ════════════════════════════════════════════════════════════════════
// WS2812B LED
// ════════════════════════════════════════════════════════════════════
CRGB leds[NUM_LEDS];

// ════════════════════════════════════════════════════════════════════
// PROTOTYPY
// ════════════════════════════════════════════════════════════════════
void startAlarm(uint8_t nodeId);
void updateAlarm();
void stopAlarm();
void toneOn(uint16_t freq, uint8_t vol);
void toneOff();
void sendStatus();
void checkNodeTimeouts();
void bleSetup();

// ════════════════════════════════════════════════════════════════════
// ESP-NOW CALLBACK (IRAM — rýchla obsluha)
// ════════════════════════════════════════════════════════════════════
void IRAM_ATTR onEspNowReceive(const uint8_t* mac, const uint8_t* data, int len) {
    if (len != sizeof(AlarmPacket)) return;
    AlarmPacket pkt;
    memcpy(&pkt, data, len);

    if (pkt.nodeId < 1 || pkt.nodeId > ESPNOW_MAX_NODES) return;

    uint8_t idx = pkt.nodeId - 1;
    nodes[idx].online   = true;
    nodes[idx].lastSeen = millis();
    nodes[idx].battery  = pkt.battery;

    if (pkt.event == EVT_BITE) {
        alarmPending = true;
        alarmNodeId  = pkt.nodeId;
    }
}

// ════════════════════════════════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    Serial.println("\n[BOOT] FishAlarm RECEIVER (BLE)");

    // ─── GPIO ────────────────────────────────────────────────────────
    pinMode(PIN_VIBRO, OUTPUT);
    digitalWrite(PIN_VIBRO, LOW);

    // ─── LEDC (bzučiak) ───────────────────────────────────────────────
    ledcSetup(LEDC_CHANNEL, DEFAULT_TONE_HZ, LEDC_BITS);
    ledcAttachPin(PIN_BUZZER, LEDC_CHANNEL);
    ledcWrite(LEDC_CHANNEL, 0);

    // ─── WS2812B ──────────────────────────────────────────────────────
    FastLED.addLeds<WS2812B, PIN_RGB_DATA, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(LED_BRIGHTNESS);
    leds[0] = CRGB::Black;
    FastLED.show();

    // ─── BLE ──────────────────────────────────────────────────────────
    bleSetup();

    // ─── ESP-NOW — WiFi musí byť STA, BLE funguje paralelne ──────────
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] Chyba inicializácie!");
    } else {
        esp_now_register_recv_cb(onEspNowReceive);
        Serial.printf("[ESP-NOW] OK | MAC: %s\n", WiFi.macAddress().c_str());
    }

    // Boot animácia — prechod farbami nodov
    for (int i = 0; i < ESPNOW_MAX_NODES; i++) {
        leds[0] = CRGB(cfg.colors[i][0], cfg.colors[i][1], cfg.colors[i][2]);
        FastLED.show();
        delay(180);
    }
    leds[0] = CRGB::Black;
    FastLED.show();

    Serial.println("[BOOT] Ready. BLE: " BLE_DEVICE_NAME);
}

// ════════════════════════════════════════════════════════════════════
void loop() {
    // ─── Spracuj záber z ESP-NOW ISR ─────────────────────────────────
    if (alarmPending) {
        alarmPending = false;
        uint8_t nid = alarmNodeId;
        Serial.printf("[ALARM] Záber — prút %d\n", nid);
        bleSend("EVENT:BITE:" + String(nid));
        if (!alarmActive) startAlarm(nid);
    }

    updateAlarm();
    checkNodeTimeouts();

    // ─── Idle LED — jemný pulz (len keď alarm nebeží) ────────────────
    if (!alarmActive) {
        static uint32_t idleTimer = 0;
        if (millis() - idleTimer > 30) {
            idleTimer = millis();
            float s = (1.0f + sinf(millis() / 1500.0f)) / 2.0f;
            uint8_t v = (uint8_t)(s * 12);   // max jas 12/255 — едva viditeľný
            leds[0] = bleConnected ? CRGB(0, v, 0) : CRGB(v, v, v);
            FastLED.show();
        }
    }
}

// ════════════════════════════════════════════════════════════════════
// BLE INIT
// ════════════════════════════════════════════════════════════════════
void bleSetup() {
    NimBLEDevice::init(BLE_DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);   // max dosah

    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCB());

    // Nordic UART Service
    NimBLEService* pSvc = pServer->createService(NUS_SERVICE_UUID);

    // TX — ESP → mobil (notify)
    pTxChar = pSvc->createCharacteristic(
        NUS_TX_CHAR_UUID,
        NIMBLE_PROPERTY::NOTIFY
    );

    // RX — mobil → ESP (write)
    NimBLECharacteristic* pRxChar = pSvc->createCharacteristic(
        NUS_RX_CHAR_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pRxChar->setCallbacks(new RxCB());

    pSvc->start();

    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    pAdv->addServiceUUID(NUS_SERVICE_UUID);
    pAdv->setScanResponse(true);
    pAdv->start();

    Serial.println("[BLE] Advertising štart: " BLE_DEVICE_NAME);
}

// ════════════════════════════════════════════════════════════════════
// ALARM (non-blocking)
// ════════════════════════════════════════════════════════════════════
void startAlarm(uint8_t nodeId) {
    alarmActive = true;
    alarmNode   = nodeId;

    uint8_t idx = nodeId - 1;
    alarmColor = CRGB(cfg.colors[idx][0], cfg.colors[idx][1], cfg.colors[idx][2]);

    ledFlashCount = 0; ledOn = true;  ledTimer = millis();
    beepCount     = 0; beepOn = true; beepTimer = millis();
    vibroCount    = 0; vibroOn = true; vibroTimer = millis();

    leds[0] = alarmColor;
    FastLED.show();
    if (cfg.sound) toneOn(cfg.toneHz, cfg.volume);
    if (cfg.vibro) digitalWrite(PIN_VIBRO, HIGH);
}

void updateAlarm() {
    if (!alarmActive) return;
    uint32_t now = millis();

    // ── LED flash ──────────────────────────────────────────────────
    if (ledFlashCount < LED_FLASH_COUNT) {
        if (ledOn && (now - ledTimer) >= LED_FLASH_MS) {
            leds[0] = CRGB::Black; FastLED.show();
            ledOn = false; ledTimer = now;
        } else if (!ledOn && (now - ledTimer) >= LED_FLASH_PAUSE_MS) {
            leds[0] = alarmColor; FastLED.show();
            ledOn = true; ledTimer = now;
            ledFlashCount++;
        }
    } else {
        leds[0] = alarmColor; FastLED.show();  // po skončení svieti
    }

    // ── Bzučiak ────────────────────────────────────────────────────
    if (cfg.sound && beepCount < ALARM_BEEP_COUNT) {
        if (beepOn && (now - beepTimer) >= ALARM_BEEP_MS) {
            toneOff(); beepOn = false; beepTimer = now; beepCount++;
        } else if (!beepOn && beepCount < ALARM_BEEP_COUNT
                   && (now - beepTimer) >= ALARM_BEEP_PAUSE_MS) {
            toneOn(cfg.toneHz, cfg.volume); beepOn = true; beepTimer = now;
        }
    }

    // ── Vibrácia ───────────────────────────────────────────────────
    if (cfg.vibro && vibroCount < ALARM_VIBRO_COUNT) {
        if (vibroOn && (now - vibroTimer) >= ALARM_VIBRO_MS) {
            digitalWrite(PIN_VIBRO, LOW); vibroOn = false; vibroTimer = now; vibroCount++;
        } else if (!vibroOn && vibroCount < ALARM_VIBRO_COUNT
                   && (now - vibroTimer) >= ALARM_VIBRO_PAUSE_MS) {
            digitalWrite(PIN_VIBRO, HIGH); vibroOn = true; vibroTimer = now;
        }
    }

    // ── Koniec sekvencie ───────────────────────────────────────────
    bool done = (ledFlashCount >= LED_FLASH_COUNT)
             && (!cfg.sound || beepCount  >= ALARM_BEEP_COUNT)
             && (!cfg.vibro || vibroCount >= ALARM_VIBRO_COUNT);

    if (done) {
        alarmActive = false;
        toneOff();
        digitalWrite(PIN_VIBRO, LOW);
        // LED ostáva vo farbe prútu — vizuálna pamäť záberov
    }
}

void stopAlarm() {
    alarmActive = false;
    toneOff();
    digitalWrite(PIN_VIBRO, LOW);
    leds[0] = CRGB::Black;
    FastLED.show();
}

// ════════════════════════════════════════════════════════════════════
// PWM BZUČIAK
// ════════════════════════════════════════════════════════════════════
void toneOn(uint16_t freq, uint8_t vol) {
    ledcSetup(LEDC_CHANNEL, freq, LEDC_BITS);
    ledcAttachPin(PIN_BUZZER, LEDC_CHANNEL);
    ledcWrite(LEDC_CHANNEL, (uint8_t)map(vol, 1, 10, 10, 255));
}
void toneOff() { ledcWrite(LEDC_CHANNEL, 0); }

// ════════════════════════════════════════════════════════════════════
// NODE TIMEOUT
// ════════════════════════════════════════════════════════════════════
void checkNodeTimeouts() {
    static uint32_t lastCheck = 0;
    if ((millis() - lastCheck) < 10000) return;
    lastCheck = millis();

    for (int i = 0; i < ESPNOW_MAX_NODES; i++) {
        if (nodes[i].online && (millis() - nodes[i].lastSeen) > NODE_TIMEOUT_MS) {
            nodes[i].online = false;
            Serial.printf("[TIMEOUT] Node %d offline\n", i + 1);
            bleSend("EVENT:OFFLINE:" + String(i + 1));
        }
    }
}

// ════════════════════════════════════════════════════════════════════
// BLE PRÍKAZY
// ════════════════════════════════════════════════════════════════════
void sendStatus() {
    bleSend("STATUS:VOL:" + String(cfg.volume)
          + "|TONE:"  + String(cfg.toneHz)
          + "|VIBRO:" + String(cfg.vibro ? "ON" : "OFF")
          + "|SOUND:" + String(cfg.sound ? "ON" : "OFF")
          + "|ALARM:" + String(alarmActive ? "YES" : "NO"));
}

void parseBTCommand(const String& cmd) {
    if (cmd.equalsIgnoreCase("STATUS")) { sendStatus(); return; }

    if (cmd.equalsIgnoreCase("STOP"))   { stopAlarm(); bleSend("OK:STOP"); return; }

    // NODES — stav všetkých nodov
    if (cmd.equalsIgnoreCase("NODES")) {
        for (int i = 0; i < ESPNOW_MAX_NODES; i++) {
            bleSend("NODE:" + String(i+1)
                  + ":" + (nodes[i].online ? "ONLINE" : "OFFLINE")
                  + ":BAT:" + String(nodes[i].battery));
        }
        return;
    }

    // VOL:1-10
    if (cmd.startsWith("VOL:")) {
        int v = cmd.substring(4).toInt();
        if (v >= 1 && v <= 10) { cfg.volume = v; bleSend("OK:VOL:" + String(v)); }
        else bleSend("ERR:VOL 1-10");
        return;
    }

    // TONE:200-4000
    if (cmd.startsWith("TONE:")) {
        int v = cmd.substring(5).toInt();
        if (v >= 200 && v <= 4000) { cfg.toneHz = v; bleSend("OK:TONE:" + String(v)); }
        else bleSend("ERR:TONE 200-4000");
        return;
    }

    // VIBRO:ON|OFF
    if (cmd.startsWith("VIBRO:")) {
        cfg.vibro = cmd.substring(6).equalsIgnoreCase("ON");
        bleSend("OK:VIBRO:" + String(cfg.vibro ? "ON" : "OFF"));
        return;
    }

    // SOUND:ON|OFF
    if (cmd.startsWith("SOUND:")) {
        cfg.sound = cmd.substring(6).equalsIgnoreCase("ON");
        bleSend("OK:SOUND:" + String(cfg.sound ? "ON" : "OFF"));
        return;
    }

    // COLOR:nodeId:RRGGBB  napr. COLOR:1:FF0000
    if (cmd.startsWith("COLOR:")) {
        int sep = cmd.indexOf(':', 6);
        if (sep < 0) { bleSend("ERR:COLOR format COLOR:id:RRGGBB"); return; }
        int nodeId = cmd.substring(6, sep).toInt();
        if (nodeId < 1 || nodeId > ESPNOW_MAX_NODES) {
            bleSend("ERR:COLOR node 1-" + String(ESPNOW_MAX_NODES)); return;
        }
        String hex = cmd.substring(sep + 1);
        if (hex.length() != 6) { bleSend("ERR:COLOR hex = 6 znakov"); return; }
        long rgb = strtol(hex.c_str(), nullptr, 16);
        uint8_t idx = nodeId - 1;
        cfg.colors[idx][0] = (rgb >> 16) & 0xFF;
        cfg.colors[idx][1] = (rgb >>  8) & 0xFF;
        cfg.colors[idx][2] =  rgb        & 0xFF;
        bleSend("OK:COLOR:" + String(nodeId) + ":" + hex);
        // Krátka ukážka farby
        leds[0] = CRGB(cfg.colors[idx][0], cfg.colors[idx][1], cfg.colors[idx][2]);
        FastLED.show();
        delay(600);
        if (!alarmActive) { leds[0] = CRGB::Black; FastLED.show(); }
        return;
    }

    bleSend("ERR:? Prikazy: STATUS|STOP|NODES|VOL:n|TONE:n|VIBRO:ON/OFF|SOUND:ON/OFF|COLOR:id:RRGGBB");
}
