import 'package:flutter/material.dart';

// ─── Farby prútov — zhodné s ESP firmware ────────────────────────────
const List<Color> kNodeColors = [
  Color(0xFFFF0000), // Node 1 — Červená
  Color(0xFF00CC00), // Node 2 — Zelená
  Color(0xFF0000FF), // Node 3 — Modrá
  Color(0xFFFFCC00), // Node 4 — Žltá
  Color(0xFFAA00FF), // Node 5 — Fialová
  Color(0xFFFF8800), // Node 6 — Oranžová
];

const List<String> kNodeNames = [
  'Prút 1', 'Prút 2', 'Prút 3',
  'Prút 4', 'Prút 5', 'Prút 6',
];

// ─── Stav jedného nodu ───────────────────────────────────────────────
class NodeState {
  final int    id;
  bool         online  = false;
  int          battery = -1;   // -1 = neznáme
  DateTime?    lastSeen;
  bool         hadBite = false; // od posledného resetovania

  NodeState(this.id);

  Color get color => kNodeColors[(id - 1).clamp(0, kNodeColors.length - 1)];
  String get name => kNodeNames[(id - 1).clamp(0, kNodeNames.length - 1)];
}

// ─── Záznam záberov ──────────────────────────────────────────────────
class BiteEvent {
  final int      nodeId;
  final DateTime time;
  BiteEvent({required this.nodeId, required this.time});

  Color get color => kNodeColors[(nodeId - 1).clamp(0, kNodeColors.length - 1)];
  String get nodeName => kNodeNames[(nodeId - 1).clamp(0, kNodeNames.length - 1)];
}

// ─── Hlavný stav aplikácie ───────────────────────────────────────────
class AlarmState extends ChangeNotifier {
  // BLE spojenie
  bool   isConnected    = false;
  bool   isScanning     = false;
  String connectedName  = '';

  // Aktívny alarm (zobrazí sa overlay)
  bool   alarmActive    = false;
  int    alarmNodeId    = 0;

  // Nody
  final List<NodeState> nodes = List.generate(6, (i) => NodeState(i + 1));

  // Log záberov
  final List<BiteEvent> biteLog = [];

  // Nastavenia receivera
  int    volume = 7;
  int    toneHz = 2500;
  bool   vibro  = true;
  bool   sound  = true;
  List<Color> nodeColors = List.from(kNodeColors);

  // ── BLE spojenie ──────────────────────────────────────────────────
  void setConnected(bool v, {String name = ''}) {
    isConnected   = v;
    connectedName = name;
    if (!v) {
      for (var n in nodes) { n.online = false; }
    }
    notifyListeners();
  }

  void setScanning(bool v) {
    isScanning = v;
    notifyListeners();
  }

  // ── Záber ─────────────────────────────────────────────────────────
  void triggerBite(int nodeId) {
    alarmActive = true;
    alarmNodeId = nodeId;
    final idx = (nodeId - 1).clamp(0, nodes.length - 1);
    nodes[idx].hadBite = true;
    biteLog.insert(0, BiteEvent(nodeId: nodeId, time: DateTime.now()));
    if (biteLog.length > 50) biteLog.removeLast();
    notifyListeners();
  }

  void clearAlarm() {
    alarmActive = false;
    notifyListeners();
  }

  // ── Node stav ─────────────────────────────────────────────────────
  void updateNode(int nodeId, {bool? online, int? battery}) {
    final idx = (nodeId - 1).clamp(0, nodes.length - 1);
    if (online  != null) nodes[idx].online  = online;
    if (battery != null) nodes[idx].battery = battery;
    nodes[idx].lastSeen = DateTime.now();
    notifyListeners();
  }

  void nodeOffline(int nodeId) {
    final idx = (nodeId - 1).clamp(0, nodes.length - 1);
    nodes[idx].online = false;
    notifyListeners();
  }

  // ── Nastavenia ────────────────────────────────────────────────────
  void applyStatus(String raw) {
    // STATUS:VOL:7|TONE:2500|VIBRO:ON|SOUND:ON|ALARM:NO
    final parts = raw.replaceFirst('STATUS:', '').split('|');
    for (final p in parts) {
      final kv = p.split(':');
      if (kv.length < 2) continue;
      switch (kv[0]) {
        case 'VOL':   volume = int.tryParse(kv[1]) ?? volume; break;
        case 'TONE':  toneHz = int.tryParse(kv[1]) ?? toneHz; break;
        case 'VIBRO': vibro  = kv[1] == 'ON'; break;
        case 'SOUND': sound  = kv[1] == 'ON'; break;
      }
    }
    notifyListeners();
  }

  void resetBiteFlags() {
    for (var n in nodes) { n.hadBite = false; }
    notifyListeners();
  }
}
