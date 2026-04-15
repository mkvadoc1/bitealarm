import { BleManager, Device, Characteristic, BleError } from 'react-native-ble-plx';
import { Buffer } from 'buffer';
import * as Notifications from 'expo-notifications';
import { useAlarmStore } from '../store/useAlarmStore';
import {
  DEVICE_NAME, NUS_SERVICE_UUID,
  NUS_TX_CHAR_UUID, NUS_RX_CHAR_UUID,
  NODE_NAMES,
} from '../constants';

class BleService {
  private manager    = new BleManager();
  private device:    Device | null = null;
  private rxChar:    Characteristic | null = null;
  private txSub:     ReturnType<Characteristic['monitor']> | null = null;
  private connSub:   ReturnType<BleManager['onDeviceDisconnected']> | null = null;

  // ─── INIT notifikácií ─────────────────────────────────────────────
  async init() {
    await Notifications.requestPermissionsAsync();
    Notifications.setNotificationHandler({
      handleNotification: async () => ({
        shouldShowAlert:  true,
        shouldPlaySound:  true,
        shouldSetBadge:   true,
      }),
    });
  }

  // ─── SCAN ─────────────────────────────────────────────────────────
  startScan(onFound: (device: Device) => void) {
    const store = useAlarmStore.getState();
    store.setScanning(true);

    this.manager.startDeviceScan(
      [NUS_SERVICE_UUID],
      { allowDuplicates: false },
      (error, device) => {
        if (error) {
          console.warn('[BLE] Scan error:', error);
          store.setScanning(false);
          return;
        }
        if (device?.name === DEVICE_NAME) {
          onFound(device);
        }
      }
    );

    // Auto-stop po 15s
    setTimeout(() => this.stopScan(), 15_000);
  }

  stopScan() {
    this.manager.stopDeviceScan();
    useAlarmStore.getState().setScanning(false);
  }

  // ─── CONNECT ──────────────────────────────────────────────────────
  async connect(device: Device): Promise<void> {
    this.stopScan();
    this.device = await device.connect({ autoConnect: false });
    await this.device.discoverAllServicesAndCharacteristics();

    // Nájdi RX charakteristiku
    const services = await this.device.services();
    for (const svc of services) {
      if (svc.uuid.toLowerCase() !== NUS_SERVICE_UUID.toLowerCase()) continue;
      const chars = await svc.characteristics();
      for (const ch of chars) {
        if (ch.uuid.toLowerCase() === NUS_RX_CHAR_UUID.toLowerCase()) {
          this.rxChar = ch;
        }
        if (ch.uuid.toLowerCase() === NUS_TX_CHAR_UUID.toLowerCase()) {
          // Subscribe na notifikácie
          this.txSub = ch.monitor((err, c) => {
            if (err || !c?.value) return;
            const msg = Buffer.from(c.value, 'base64').toString('utf8').trim();
            this.handleMessage(msg);
          });
        }
      }
    }

    // Odpojenie handler
    this.connSub = this.manager.onDeviceDisconnected(device.id, () => {
      useAlarmStore.getState().setConnected(false);
      this.cleanup();
    });

    useAlarmStore.getState().setConnected(true, device.name ?? DEVICE_NAME);
    this.send('STATUS');
    this.send('NODES');
  }

  async disconnect() {
    await this.device?.cancelConnection();
    useAlarmStore.getState().setConnected(false);
    this.cleanup();
  }

  private cleanup() {
    this.txSub?.remove();
    this.connSub?.remove();
    this.rxChar  = null;
    this.txSub   = null;
    this.connSub = null;
    this.device  = null;
  }

  // ─── ODOSLANIE ────────────────────────────────────────────────────
  async send(cmd: string) {
    if (!this.rxChar) return;
    try {
      const encoded = Buffer.from(`${cmd}\n`, 'utf8').toString('base64');
      await this.rxChar.writeWithoutResponse(encoded);
    } catch (e) {
      console.warn('[BLE TX]', e);
    }
  }

  // ─── PRÍJEM SPRÁV ─────────────────────────────────────────────────
  private handleMessage(msg: string) {
    console.log('[BLE RX]', msg);
    const store = useAlarmStore.getState();

    if (msg.startsWith('EVENT:BITE:')) {
      const nodeId = parseInt(msg.split(':')[2]);
      if (nodeId > 0) {
        store.triggerBite(nodeId);
        this.sendBiteNotification(nodeId);
      }
      return;
    }

    if (msg.startsWith('EVENT:OFFLINE:')) {
      const nodeId = parseInt(msg.split(':')[2]);
      if (nodeId > 0) store.updateNode(nodeId, { online: false });
      return;
    }

    if (msg.startsWith('STATUS:')) {
      store.applyStatus(msg);
      return;
    }

    // NODE:1:ONLINE:BAT:85
    if (msg.startsWith('NODE:')) {
      const parts  = msg.split(':');
      const nodeId = parseInt(parts[1]);
      const online = parts[2] === 'ONLINE';
      const bat    = parts[3] === 'BAT' ? parseInt(parts[4]) : -1;
      if (nodeId > 0) {
        store.updateNode(nodeId, {
          online,
          battery: bat === 255 ? -1 : bat,
        });
      }
    }
  }

  // ─── PUSH NOTIFIKÁCIA ─────────────────────────────────────────────
  private async sendBiteNotification(nodeId: number) {
    const name = NODE_NAMES[(nodeId - 1)] ?? `Prút ${nodeId}`;
    await Notifications.scheduleNotificationAsync({
      content: {
        title: '🎣 Záber!',
        body:  `${name} zachytil záber`,
        sound: true,
      },
      trigger: null,
    });
  }
}

export const bleService = new BleService();
