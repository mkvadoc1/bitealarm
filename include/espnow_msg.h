#pragma once
#include <stdint.h>

// ─── ESP-NOW správa (node → receiver + node → node) ─────────────────

#define ESPNOW_MAX_NODES  6

enum EventType : uint8_t {
    EVT_BITE      = 0x01,   // záber detekovaný
    EVT_HEARTBEAT = 0x02,   // keepalive (každých 30s)
    EVT_ALARM_OFF = 0x03,   // alarm bol vypnutý tlačidlom
    EVT_LOW_BAT   = 0x04,   // nízka batéria
};

// Farby prútov — index = nodeId-1  (CRGB formát: R, G, B)
// Rovnaké farby ako na obrázku komerčných alarmov
static const uint8_t NODE_COLORS[ESPNOW_MAX_NODES][3] = {
    { 255,   0,   0 },   // Node 1 — Červená
    { 0,   255,   0 },   // Node 2 — Zelená
    { 0,     0, 255 },   // Node 3 — Modrá
    { 255, 255,   0 },   // Node 4 — Žltá
    { 180,   0, 255 },   // Node 5 — Fialová
    { 255, 128,   0 },   // Node 6 — Oranžová
};

#pragma pack(push, 1)
struct AlarmPacket {
    uint8_t   nodeId;      // 1–6
    EventType event;       // typ udalosti
    uint8_t   battery;     // 0–100 %  (255 = neznáme)
    uint8_t   rssi;        // vyplní receiver pri prijatí
};
#pragma pack(pop)
