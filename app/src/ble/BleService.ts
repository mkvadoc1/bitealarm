import { BleManager, Device, Characteristic } from 'react-native-ble-plx';
import { Buffer } from 'buffer';
import * as Notifications from 'expo-notifications';
import { useAlarmStore } from '../store/useAlarmStore';
import {
  DEVICE_NAME, NUS_SERVICE_UUID,
  NUS_TX_CHAR_UUID, NUS_RX_CHAR_UUID, NODE_NAMES,
} from '../constants';

class BleService {
  private manager   = new BleManager();
  private device:   Device | null = null;
  private rxChar:   Characteristic | null = null;
  private txSub:    ReturnType<Characteristic['monitor']> | null = null;
  private connSub:  ReturnType<BleManager['onDeviceDisconnected']> | null = null;

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

  startScan(onFound: (device: Device) => void) {
    useAlarmStore.getState().setScanning(true);
    this.manager.startDeviceScan(
      null,
      { allowDuplicates: false },
      (error, device) => {
        if (error) {
          console.warn('[BLE] Scan error:', error);
          useAlarmStore.getState().setScanning(false);
          return;
        }
        if (device?.name === DEVICE_NAME) {
          onFound(device);
        }
      }
    );
    setTimeout(() => this.stopScan(), 15_000);
  }

  stopScan() {
    this.manager.stopDeviceScan();
    useAlarmStore.getState().setScanning(false);
  }

  async connect(device: Device) {
    this.stopScan();
    this.device = await device.connect({ autoConnect: false });
    await this.device.discoverAllServicesAndCharacteristics();

    const services = await this.device.services();
    for (const svc of services) {
      if (svc.uuid.toLowerCase() !== NUS_SERVICE_UUID.toLowerCase()) continue;
      const chars = await svc.characteristics();
      for (const ch of chars) {
        const uuid = ch.uuid.toLowerCase();
        if (uuid === NUS_RX_CHAR_UUID.toLowerCase()) {
          this.rxChar = ch;
        }
        if (uuid === NUS_TX_CHAR_UUID.toLowerCase()) {
          this.txSub = ch.monitor((err, c) => {
            if (err || !c?.value) return;
            const msg = Buffer.from(c.value, 'base64').toString('utf8').trim();
            this.handleMessage(msg);
          });
        }
      }
    }

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

  async send(cmd: string) {
    if (!this.rxChar) return;
    try {
      const encoded = Buffer.from(`${cmd}\n`, 'utf8').toString('base64');
      await this.rxChar.writeWithoutResponse(encoded);
    } catch (e) {
      console.warn('[BLE TX]', e);
    }
  }

  private handleMessage(msg: string) {
    console.log('[BLE RX]', msg);
    const store = useAlarmStore.getState();

    if (msg.startsWith('EVENT:BITE:')) {
      const nodeId = parseInt(msg.split(':')[2]);
      if (nodeId > 0) { store.triggerBite(nodeId); this.notify(nodeId); }
      return;
    }
    if (msg.startsWith('EVENT:OFFLINE:')) {
      const nodeId = parseInt(msg.split(':')[2]);
      if (nodeId > 0) store.updateNode(nodeId, { online: false });
      return;
    }
    if (msg.startsWith('STATUS:')) { store.applyStatus(msg); return; }

    if (msg.startsWith('NODE:')) {
      const parts  = msg.split(':');
      const nodeId = parseInt(parts[1]);
      const online = parts[2] === 'ONLINE';
      const bat    = parts[3] === 'BAT' ? parseInt(parts[4]) : -1;
      if (nodeId > 0) store.updateNode(nodeId, { online, battery: bat === 255 ? -1 : bat });
    }
  }

  private async notify(nodeId: number) {
    const name = NODE_NAMES[(nodeId - 1)] ?? `Prút ${nodeId}`;
    await Notifications.scheduleNotificationAsync({
      content: { title: '🎣 Záber!', body: `${name} zachytil záber`, sound: true },
      trigger: null,
    });
  }
}

export const bleService = new BleService();
