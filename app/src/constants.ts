// ─── BLE — Nordic UART Service ───────────────────────────────────────
export const NUS_SERVICE_UUID = '6E400001-B5A3-F393-E0A9-E50E24DCCA9E';
export const NUS_RX_CHAR_UUID = '6E400002-B5A3-F393-E0A9-E50E24DCCA9E'; // app → ESP
export const NUS_TX_CHAR_UUID = '6E400003-B5A3-F393-E0A9-E50E24DCCA9E'; // ESP → app
export const DEVICE_NAME      = 'FishAlarm-RX';

// ─── Farby prútov — zhodné s ESP firmware ────────────────────────────
export const NODE_COLORS = [
  '#FF2020', // Node 1 — Červená
  '#00CC44', // Node 2 — Zelená
  '#2060FF', // Node 3 — Modrá
  '#FFCC00', // Node 4 — Žltá
  '#AA00FF', // Node 5 — Fialová
  '#FF8800', // Node 6 — Oranžová
] as const;

export const NODE_NAMES = [
  'Prút 1', 'Prút 2', 'Prút 3',
  'Prút 4', 'Prút 5', 'Prút 6',
] as const;

// ─── Farby UI ────────────────────────────────────────────────────────
export const Colors = {
  bg:         '#0A0A0F',
  surface:    '#141420',
  border:     '#1E1E2E',
  text:       '#FFFFFF',
  textMuted:  '#6B7280',
  primary:    '#3B82F6',
  success:    '#22C55E',
  danger:     '#EF4444',
} as const;
