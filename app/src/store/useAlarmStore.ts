import { create } from 'zustand';
import { NODE_COLORS, NODE_NAMES } from '../constants';

export interface NodeState {
  id:       number;
  online:   boolean;
  battery:  number;   // -1 = neznáme
  hadBite:  boolean;
  lastSeen: Date | null;
  color:    string;
  name:     string;
}

export interface BiteEvent {
  nodeId: number;
  time:   Date;
  color:  string;
  name:   string;
}

interface AlarmStore {
  // BLE
  isConnected:   boolean;
  isScanning:    boolean;
  deviceName:    string;

  // Aktívny alarm
  alarmActive:   boolean;
  alarmNodeId:   number;

  // Nody (len 2 pre aktuálny setup)
  nodes:         NodeState[];

  // Log záberov
  biteLog:       BiteEvent[];

  // Nastavenia receivera
  volume:        number;
  toneHz:        number;
  vibro:         boolean;
  sound:         boolean;

  // Actions
  setConnected:  (v: boolean, name?: string) => void;
  setScanning:   (v: boolean) => void;
  triggerBite:   (nodeId: number) => void;
  clearAlarm:    () => void;
  updateNode:    (nodeId: number, data: Partial<NodeState>) => void;
  applyStatus:   (raw: string) => void;
  resetBiteFlags:() => void;
}

const makeNodes = (): NodeState[] =>
  Array.from({ length: 6 }, (_, i) => ({
    id:       i + 1,
    online:   false,
    battery:  -1,
    hadBite:  false,
    lastSeen: null,
    color:    NODE_COLORS[i],
    name:     NODE_NAMES[i],
  }));

export const useAlarmStore = create<AlarmStore>((set, get) => ({
  isConnected:  false,
  isScanning:   false,
  deviceName:   '',
  alarmActive:  false,
  alarmNodeId:  0,
  nodes:        makeNodes(),
  biteLog:      [],
  volume:       7,
  toneHz:       2500,
  vibro:        true,
  sound:        true,

  setConnected: (v, name = '') => set(s => ({
    isConnected: v,
    deviceName:  name,
    nodes: v ? s.nodes : s.nodes.map(n => ({ ...n, online: false })),
  })),

  setScanning: (v) => set({ isScanning: v }),

  triggerBite: (nodeId) => {
    const node = get().nodes.find(n => n.id === nodeId);
    if (!node) return;
    const event: BiteEvent = {
      nodeId,
      time:  new Date(),
      color: node.color,
      name:  node.name,
    };
    set(s => ({
      alarmActive: true,
      alarmNodeId: nodeId,
      nodes: s.nodes.map(n => n.id === nodeId ? { ...n, hadBite: true } : n),
      biteLog: [event, ...s.biteLog].slice(0, 50),
    }));
  },

  clearAlarm: () => set({ alarmActive: false }),

  updateNode: (nodeId, data) => set(s => ({
    nodes: s.nodes.map(n =>
      n.id === nodeId ? { ...n, ...data, lastSeen: new Date() } : n
    ),
  })),

  applyStatus: (raw) => {
    // STATUS:VOL:7|TONE:2500|VIBRO:ON|SOUND:ON|ALARM:NO
    const parts = raw.replace('STATUS:', '').split('|');
    const update: Partial<AlarmStore> = {};
    for (const p of parts) {
      const [k, v] = p.split(':');
      if (k === 'VOL')   update.volume = parseInt(v) || get().volume;
      if (k === 'TONE')  update.toneHz = parseInt(v) || get().toneHz;
      if (k === 'VIBRO') update.vibro  = v === 'ON';
      if (k === 'SOUND') update.sound  = v === 'ON';
    }
    set(update);
  },

  resetBiteFlags: () => set(s => ({
    nodes: s.nodes.map(n => ({ ...n, hadBite: false })),
  })),
}));
