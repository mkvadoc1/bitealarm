#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <NimBLEDevice.h>
#include "config_receiver.h"
#include "espnow_msg.h"

// ════════════════════════════════════════════════════════════════════
// BLE — Nordic UART Service
// ════════════════════════════════════════════════════════════════════
static NimBLECharacteristic* pTxChar    = nullptr;
static bool                  bleConnected = false;

void bleSend(const String& msg) {
    if (!bleConnected || !pTxChar) return;
    pTxChar->setValue(msg.c_str());
    pTxChar->notify();
}

class ServerCB : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pSrv) override {
        bleConnected = true;
        Serial.println("[BLE] Klient pripojený");
        digitalWrite(PIN_LED_GREEN, HIGH);
    }
    void onDisconnect(NimBLEServer* pSrv) override {
        bleConnected = false;
        Serial.println("[BLE] Klient odpojený — znovu advertisujem");
        NimBLEDevice::getServer()->startAdvertising();
    }
};

void parseBTCommand(const String& cmd);

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
// ALARM
// ════════════════════════════════════════════════════════════════════
volatile bool    alarmPending = false;
volatile uint8_t alarmNodeId  = 0;

bool     alarmActive = false;
uint8_t  alarmNode   = 0;

// LED flash (červená)
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
void updateStatusLed();

// ════════════════════════════════════════════════════════════════════
// ESP-NOW CALLBACK
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
    Serial.println("\n[BOOT] FishAlarm RECEIVER");

    // GPIO
    pinMode(PIN_BUZZER,    OUTPUT);
    pinMode(PIN_VIBRO,     OUTPUT);
    pinMode(PIN_LED_RED,   OUTPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);
    digitalWrite(PIN_VIBRO,     LOW);
    digitalWrite(PIN_LED_RED,   LOW);
    digitalWrite(PIN_LED_GREEN, LOW);

    // LEDC
    ledcSetup(LEDC_CHANNEL, DEFAULT_TONE_HZ, LEDC_BITS);
    ledcAttachPin(PIN_BUZZER, LEDC_CHANNEL);
    ledcWrite(LEDC_CHANNEL, 0);

    // BLE
    bleSetup();

    // ESP-NOW
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] Chyba inicializácie!");
    } else {
        esp_now_register_recv_cb(onEspNowReceive);
        Serial.printf("[ESP-NOW] OK | MAC: %s\n", WiFi.macAddress().c_str());
    }

    // Boot bliknutie — 2× červená + zelená
    for (int i = 0; i < 2; i++) {
        digitalWrite(PIN_LED_RED, HIGH); delay(150);
        digitalWrite(PIN_LED_RED, LOW);  delay(100);
    }
    digitalWrite(PIN_LED_GREEN, HIGH); delay(300);
    digitalWrite(PIN_LED_GREEN, LOW);

    Serial.println("[BOOT] Ready. BLE: " BLE_DEVICE_NAME);
}

// ════════════════════════════════════════════════════════════════════
void loop() {
    // Spracuj záber z ESP-NOW ISR
    if (alarmPending) {
        alarmPending = false;
        uint8_t nid = alarmNodeId;
        Serial.printf("[ALARM] Záber — prút %d\n", nid);
        bleSend("EVENT:BITE:" + String(nid));
        if (!alarmActive) startAlarm(nid);
    }

    updateAlarm();
    checkNodeTimeouts();
    updateStatusLed();
}

// ════════════════════════════════════════════════════════════════════
// STATUS LED (zelená)
// ════════════════════════════════════════════════════════════════════
void updateStatusLed() {
    if (alarmActive) return;  // počas alarmu riadi červená

    if (bleConnected) {
        // BT pripojený → zelená svieti trvale
        digitalWrite(PIN_LED_GREEN, HIGH);
    } else {
        // BT nepripojený → zelená bliká každú sekundu
        digitalWrite(PIN_LED_GREEN, (millis() / 500) % 2);
    }
}

// ════════════════════════════════════════════════════════════════════
// ALARM (non-blocking)
// ════════════════════════════════════════════════════════════════════
void startAlarm(uint8_t nodeId) {
    alarmActive = true;
    alarmNode   = nodeId;

    ledFlashCount = 0; ledOn = true;  ledTimer  = millis();
    beepCount     = 0; beepOn = true; beepTimer = millis();
    vibroCount    = 0; vibroOn = true; vibroTimer = millis();

    digitalWrite(PIN_LED_RED, HIGH);
    if (cfg.sound) toneOn(cfg.toneHz, cfg.volume);
    if (cfg.vibro) digitalWrite(PIN_VIBRO, HIGH);
}

void updateAlarm() {
    if (!alarmActive) return;
    uint32_t now = millis();

    // LED flash (červená)
    if (ledFlashCount < LED_FLASH_COUNT) {
        if (ledOn && (now - ledTimer) >= LED_FLASH_MS) {
            digitalWrite(PIN_LED_RED, LOW);
            ledOn = false; ledTimer = now;
        } else if (!ledOn && (now - ledTimer) >= LED_FLASH_PAUSE_MS) {
            digitalWrite(PIN_LED_RED, HIGH);
            ledOn = true; ledTimer = now; ledFlashCount++;
        }
    } else {
        // Po skončení flash sekvencie — červená ostáva zapnutá
        digitalWrite(PIN_LED_RED, HIGH);
    }

    // Bzučiak
    if (cfg.sound && beepCount < ALARM_BEEP_COUNT) {
        if (beepOn && (now - beepTimer) >= ALARM_BEEP_MS) {
            toneOff(); beepOn = false; beepTimer = now; beepCount++;
        } else if (!beepOn && beepCount < ALARM_BEEP_COUNT
                   && (now - beepTimer) >= ALARM_BEEP_PAUSE_MS) {
            toneOn(cfg.toneHz, cfg.volume); beepOn = true; beepTimer = now;
        }
    }

    // Vibrácia
    if (cfg.vibro && vibroCount < ALARM_VIBRO_COUNT) {
        if (vibroOn && (now - vibroTimer) >= ALARM_VIBRO_MS) {
            digitalWrite(PIN_VIBRO, LOW); vibroOn = false; vibroTimer = now; vibroCount++;
        } else if (!vibroOn && vibroCount < ALARM_VIBRO_COUNT
                   && (now - vibroTimer) >= ALARM_VIBRO_PAUSE_MS) {
            digitalWrite(PIN_VIBRO, HIGH); vibroOn = true; vibroTimer = now;
        }
    }

    // Koniec sekvencie
    bool done = (ledFlashCount >= LED_FLASH_COUNT)
             && (!cfg.sound || beepCount  >= ALARM_BEEP_COUNT)
             && (!cfg.vibro || vibroCount >= ALARM_VIBRO_COUNT);

    if (done) {
        alarmActive = false;
        toneOff();
        digitalWrite(PIN_VIBRO, LOW);
        // Červená zostáva zapnutá — vizuálna pamäť záberov
    }
}

void stopAlarm() {
    alarmActive = false;
    toneOff();
    digitalWrite(PIN_VIBRO,   LOW);
    digitalWrite(PIN_LED_RED, LOW);
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
// BLE SETUP
// ════════════════════════════════════════════════════════════════════
void bleSetup() {
    NimBLEDevice::init(BLE_DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCB());

    NimBLEService* pSvc = pServer->createService(NUS_SERVICE_UUID);

    pTxChar = pSvc->createCharacteristic(NUS_TX_CHAR_UUID, NIMBLE_PROPERTY::NOTIFY);

    NimBLECharacteristic* pRxChar = pSvc->createCharacteristic(
        NUS_RX_CHAR_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    pRxChar->setCallbacks(new RxCB());

    pSvc->start();

    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    pAdv->addServiceUUID(NUS_SERVICE_UUID);
    pAdv->setScanResponse(true);
    pAdv->start();

    Serial.println("[BLE] Advertising: " BLE_DEVICE_NAME);
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
    if (cmd.equalsIgnoreCase("STATUS"))  { sendStatus(); return; }
    if (cmd.equalsIgnoreCase("STOP"))    { stopAlarm(); bleSend("OK:STOP"); return; }

    if (cmd.startsWith("VOL:")) {
        int v = cmd.substring(4).toInt();
        if (v >= 1 && v <= 10) { cfg.volume = v; bleSend("OK:VOL:" + String(v)); }
        else bleSend("ERR:VOL 1-10");
        return;
    }
    if (cmd.startsWith("TONE:")) {
        int v = cmd.substring(5).toInt();
        if (v >= 200 && v <= 4000) { cfg.toneHz = v; bleSend("OK:TONE:" + String(v)); }
        else bleSend("ERR:TONE 200-4000");
        return;
    }
    if (cmd.startsWith("VIBRO:")) {
        cfg.vibro = cmd.substring(6).equalsIgnoreCase("ON");
        bleSend("OK:VIBRO:" + String(cfg.vibro ? "ON" : "OFF"));
        return;
    }
    if (cmd.startsWith("SOUND:")) {
        cfg.sound = cmd.substring(6).equalsIgnoreCase("ON");
        bleSend("OK:SOUND:" + String(cfg.sound ? "ON" : "OFF"));
        return;
    }
    if (cmd.equalsIgnoreCase("NODES")) {
        for (int i = 0; i < ESPNOW_MAX_NODES; i++) {
            bleSend("NODE:" + String(i+1)
                  + ":" + (nodes[i].online ? "ONLINE" : "OFFLINE")
                  + ":BAT:" + String(nodes[i].battery));
        }
        return;
    }

    bleSend("ERR:? Prikazy: STATUS|STOP|NODES|VOL:n|TONE:n|VIBRO:ON/OFF|SOUND:ON/OFF");
}
