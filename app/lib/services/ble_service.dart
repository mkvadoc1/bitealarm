import 'dart:async';
import 'dart:convert';
import 'package:flutter/foundation.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:flutter_local_notifications/flutter_local_notifications.dart';
import '../models/alarm_state.dart';

// Nordic UART Service UUIDs — zhodné s ESP firmware
const _kNusService = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const _kNusTx      = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // notify (ESP→app)
const _kNusRx      = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // write  (app→ESP)

class BleService {
  final AlarmState state;
  BleService(this.state);

  BluetoothDevice?       _device;
  BluetoothCharacteristic? _rxChar;
  StreamSubscription?    _notifySub;
  StreamSubscription?    _connSub;

  final _notifications = FlutterLocalNotificationsPlugin();

  // ─── INIT notifikácií ─────────────────────────────────────────────
  Future<void> init() async {
    const android = AndroidInitializationSettings('@mipmap/ic_launcher');
    const ios     = DarwinInitializationSettings();
    await _notifications.initialize(
      const InitializationSettings(android: android, iOS: ios),
    );
  }

  // ─── SCAN & CONNECT ───────────────────────────────────────────────
  Future<void> startScan() async {
    state.setScanning(true);
    await FlutterBluePlus.startScan(
      timeout: const Duration(seconds: 10),
      withNames: ['FishAlarm-RX'],
    );
    state.setScanning(false);
  }

  void stopScan() => FlutterBluePlus.stopScan();

  Stream<List<ScanResult>> get scanResults => FlutterBluePlus.scanResults;

  Future<void> connect(BluetoothDevice device) async {
    _device = device;
    await device.connect(autoConnect: false);

    _connSub = device.connectionState.listen((s) {
      if (s == BluetoothConnectionState.disconnected) {
        state.setConnected(false);
        _notifySub?.cancel();
      }
    });

    final services = await device.discoverServices();
    for (final svc in services) {
      if (svc.uuid.toString().toLowerCase() == _kNusService) {
        for (final ch in svc.characteristics) {
          final uuid = ch.uuid.toString().toLowerCase();
          if (uuid == _kNusTx) {
            await ch.setNotifyValue(true);
            _notifySub = ch.lastValueStream.listen(_onData);
          }
          if (uuid == _kNusRx) {
            _rxChar = ch;
          }
        }
      }
    }

    state.setConnected(true, name: device.platformName);
    send('STATUS');   // načítaj aktuálne nastavenia
    send('NODES');    // načítaj stav nodov
  }

  Future<void> disconnect() async {
    await _device?.disconnect();
    _notifySub?.cancel();
    _connSub?.cancel();
    _device  = null;
    _rxChar  = null;
  }

  // ─── ODOSLANIE PRÍKAZU ────────────────────────────────────────────
  Future<void> send(String cmd) async {
    if (_rxChar == null) return;
    try {
      final bytes = utf8.encode('$cmd\n');
      await _rxChar!.write(bytes, withoutResponse: true);
      debugPrint('[BLE TX] $cmd');
    } catch (e) {
      debugPrint('[BLE TX ERR] $e');
    }
  }

  // ─── PRÍJEM SPRÁV OD ESP ──────────────────────────────────────────
  void _onData(List<int> raw) {
    if (raw.isEmpty) return;
    final msg = utf8.decode(raw).trim();
    debugPrint('[BLE RX] $msg');

    if (msg.startsWith('EVENT:BITE:')) {
      final nodeId = int.tryParse(msg.split(':').last) ?? 0;
      if (nodeId > 0) {
        state.triggerBite(nodeId);
        _sendBiteNotification(nodeId);
      }
      return;
    }

    if (msg.startsWith('EVENT:OFFLINE:')) {
      final nodeId = int.tryParse(msg.split(':').last) ?? 0;
      if (nodeId > 0) state.nodeOffline(nodeId);
      return;
    }

    if (msg.startsWith('EVENT:POWER:')) return;

    if (msg.startsWith('STATUS:')) {
      state.applyStatus(msg);
      return;
    }

    // NODE:1:ONLINE:BAT:85
    if (msg.startsWith('NODE:')) {
      final parts = msg.split(':');
      if (parts.length >= 3) {
        final nodeId = int.tryParse(parts[1]) ?? 0;
        final online = parts[2] == 'ONLINE';
        int? bat;
        if (parts.length >= 5 && parts[3] == 'BAT') {
          bat = int.tryParse(parts[4]);
          if (bat == 255) bat = null;
        }
        if (nodeId > 0) state.updateNode(nodeId, online: online, battery: bat);
      }
      return;
    }
  }

  // ─── PUSH NOTIFIKÁCIA ─────────────────────────────────────────────
  Future<void> _sendBiteNotification(int nodeId) async {
    final name = kNodeNames[(nodeId - 1).clamp(0, kNodeNames.length - 1)];
    await _notifications.show(
      nodeId,
      '🎣 Záber!',
      '$name zachytil záber',
      const NotificationDetails(
        android: AndroidNotificationDetails(
          'fish_alarm', 'FishAlarm',
          importance: Importance.max,
          priority: Priority.high,
          enableVibration: true,
          playSound: true,
        ),
        iOS: DarwinNotificationDetails(presentSound: true, presentBadge: true),
      ),
    );
  }
}
