import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:wakelock_plus/wakelock_plus.dart';
import '../models/alarm_state.dart';
import '../services/ble_service.dart';
import '../widgets/node_card.dart';
import '../widgets/settings_panel.dart';
import 'scan_screen.dart';

class DashboardScreen extends StatefulWidget {
  const DashboardScreen({super.key});
  @override
  State<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen>
    with SingleTickerProviderStateMixin {
  late AnimationController _pulseCtrl;
  late Animation<double>   _pulseAnim;

  @override
  void initState() {
    super.initState();
    WakelockPlus.enable();

    _pulseCtrl = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 600),
    )..repeat(reverse: true);

    _pulseAnim = Tween<double>(begin: 0.85, end: 1.0).animate(
      CurvedAnimation(parent: _pulseCtrl, curve: Curves.easeInOut),
    );
  }

  @override
  void dispose() {
    WakelockPlus.disable();
    _pulseCtrl.dispose();
    super.dispose();
  }

  Future<void> _disconnect() async {
    await context.read<BleService>().disconnect();
    if (!mounted) return;
    Navigator.of(context).pushReplacement(
      MaterialPageRoute(builder: (_) => const ScanScreen()),
    );
  }

  void _showSettings() {
    showModalBottomSheet(
      context: context,
      isScrollControlled: true,
      backgroundColor: Colors.transparent,
      builder: (_) => ChangeNotifierProvider.value(
        value: context.read<AlarmState>(),
        child: Provider.value(
          value: context.read<BleService>(),
          child: const SettingsPanel(),
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AlarmState>();

    return Scaffold(
      backgroundColor: const Color(0xFF0A0A0F),
      body: Stack(
        children: [
          // ── Hlavný obsah ─────────────────────────────────────────
          SafeArea(
            child: Column(
              children: [
                _buildHeader(context, state),
                Expanded(
                  child: SingleChildScrollView(
                    padding: const EdgeInsets.fromLTRB(16, 8, 16, 100),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        // Nody grid (len prvé 2 pre 2-node setup)
                        _buildNodeGrid(state),
                        const SizedBox(height: 24),
                        // Log záberov
                        _buildBiteLog(context, state),
                      ],
                    ),
                  ),
                ),
              ],
            ),
          ),

          // ── Alarm overlay ─────────────────────────────────────────
          if (state.alarmActive)
            _AlarmOverlay(
              nodeId:    state.alarmNodeId,
              pulseAnim: _pulseAnim,
              onStop: () {
                context.read<BleService>().send('STOP');
                state.clearAlarm();
              },
            ),

          // ── Bottom bar ────────────────────────────────────────────
          Positioned(
            bottom: 0, left: 0, right: 0,
            child: _buildBottomBar(context, state),
          ),
        ],
      ),
    );
  }

  // ── Header ─────────────────────────────────────────────────────────
  Widget _buildHeader(BuildContext context, AlarmState state) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(16, 12, 16, 4),
      child: Row(
        children: [
          const Icon(Icons.phishing, color: Colors.blueAccent, size: 28),
          const SizedBox(width: 10),
          Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Text('FishAlarm',
                style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold)),
              Text(state.connectedName,
                style: const TextStyle(fontSize: 12, color: Colors.grey)),
            ],
          ),
          const Spacer(),
          // BLE stav
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
            decoration: BoxDecoration(
              borderRadius: BorderRadius.circular(20),
              color: state.isConnected
                  ? Colors.green.withOpacity(0.15)
                  : Colors.red.withOpacity(0.15),
            ),
            child: Row(
              children: [
                Icon(Icons.bluetooth_connected,
                  size: 14,
                  color: state.isConnected ? Colors.greenAccent : Colors.redAccent),
                const SizedBox(width: 4),
                Text(state.isConnected ? 'Connected' : 'Lost',
                  style: TextStyle(
                    fontSize: 11,
                    color: state.isConnected ? Colors.greenAccent : Colors.redAccent,
                  )),
              ],
            ),
          ),
          const SizedBox(width: 8),
          IconButton(
            icon: const Icon(Icons.settings_outlined, size: 22),
            onPressed: _showSettings,
            color: Colors.grey.shade400,
          ),
        ],
      ),
    );
  }

  // ── Node grid ──────────────────────────────────────────────────────
  Widget _buildNodeGrid(AlarmState state) {
    // Zobraz len prvé 2 nody (pre 2-node setup) — ľahko rozšíriteľné
    final activeNodes = state.nodes.take(2).toList();
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text('Prúty', style: TextStyle(
          fontSize: 13, color: Colors.grey.shade500,
          fontWeight: FontWeight.w600, letterSpacing: 0.5,
        )),
        const SizedBox(height: 10),
        GridView.count(
          crossAxisCount: 2,
          shrinkWrap: true,
          physics: const NeverScrollableScrollPhysics(),
          mainAxisSpacing: 10,
          crossAxisSpacing: 10,
          childAspectRatio: 1.5,
          children: activeNodes.map((n) => NodeCard(node: n)).toList(),
        ),
        const SizedBox(height: 10),
        // Reset záberov
        if (state.nodes.any((n) => n.hadBite))
          Align(
            alignment: Alignment.centerRight,
            child: TextButton.icon(
              icon: const Icon(Icons.clear_all, size: 16),
              label: const Text('Resetovať záberov', style: TextStyle(fontSize: 12)),
              style: TextButton.styleFrom(foregroundColor: Colors.grey),
              onPressed: state.resetBiteFlags,
            ),
          ),
      ],
    );
  }

  // ── Bite log ───────────────────────────────────────────────────────
  Widget _buildBiteLog(BuildContext context, AlarmState state) {
    if (state.biteLog.isEmpty) {
      return Center(
        child: Padding(
          padding: const EdgeInsets.symmetric(vertical: 32),
          child: Column(
            children: [
              Icon(Icons.water, size: 48, color: Colors.grey.shade800),
              const SizedBox(height: 8),
              Text('Zatiaľ žiadny záber',
                style: TextStyle(color: Colors.grey.shade600)),
            ],
          ),
        ),
      );
    }
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text('História záberov', style: TextStyle(
          fontSize: 13, color: Colors.grey.shade500,
          fontWeight: FontWeight.w600, letterSpacing: 0.5,
        )),
        const SizedBox(height: 10),
        ...state.biteLog.take(10).map((e) => _BiteLogItem(event: e)),
      ],
    );
  }

  // ── Bottom bar ─────────────────────────────────────────────────────
  Widget _buildBottomBar(BuildContext context, AlarmState state) {
    return Container(
      padding: const EdgeInsets.fromLTRB(16, 10, 16, 24),
      decoration: BoxDecoration(
        color: const Color(0xFF0A0A0F),
        border: Border(top: BorderSide(color: Colors.grey.shade900)),
      ),
      child: Row(
        children: [
          Expanded(
            child: OutlinedButton.icon(
              icon: const Icon(Icons.bluetooth_disabled, size: 18),
              label: const Text('Odpojiť'),
              style: OutlinedButton.styleFrom(foregroundColor: Colors.grey),
              onPressed: _disconnect,
            ),
          ),
          const SizedBox(width: 12),
          Expanded(
            child: ElevatedButton.icon(
              icon: const Icon(Icons.stop_circle_outlined, size: 18),
              label: const Text('STOP Alarm'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.red.shade900,
                foregroundColor: Colors.white,
              ),
              onPressed: state.alarmActive
                  ? () {
                      context.read<BleService>().send('STOP');
                      state.clearAlarm();
                    }
                  : null,
            ),
          ),
        ],
      ),
    );
  }
}

// ── Bite log položka ──────────────────────────────────────────────────
class _BiteLogItem extends StatelessWidget {
  final BiteEvent event;
  const _BiteLogItem({required this.event});

  @override
  Widget build(BuildContext context) {
    final t = event.time;
    final time = '${t.hour.toString().padLeft(2,'0')}:'
                 '${t.minute.toString().padLeft(2,'0')}:'
                 '${t.second.toString().padLeft(2,'0')}';
    return Container(
      margin: const EdgeInsets.only(bottom: 6),
      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(10),
        color: event.color.withOpacity(0.08),
        border: Border.all(color: event.color.withOpacity(0.25)),
      ),
      child: Row(
        children: [
          Container(
            width: 10, height: 10,
            decoration: BoxDecoration(shape: BoxShape.circle, color: event.color),
          ),
          const SizedBox(width: 10),
          Text(event.nodeName, style: const TextStyle(fontWeight: FontWeight.w600)),
          const Spacer(),
          Text(time, style: TextStyle(fontSize: 12, color: Colors.grey.shade500)),
          const SizedBox(width: 6),
          Icon(Icons.water_drop, size: 14, color: event.color),
        ],
      ),
    );
  }
}

// ── Alarm Overlay ─────────────────────────────────────────────────────
class _AlarmOverlay extends StatelessWidget {
  final int             nodeId;
  final Animation<double> pulseAnim;
  final VoidCallback    onStop;

  const _AlarmOverlay({
    required this.nodeId,
    required this.pulseAnim,
    required this.onStop,
  });

  @override
  Widget build(BuildContext context) {
    final idx   = (nodeId - 1).clamp(0, kNodeColors.length - 1);
    final color = kNodeColors[idx];
    final name  = kNodeNames[idx];

    return AnimatedBuilder(
      animation: pulseAnim,
      builder: (ctx, _) => Container(
        color: color.withOpacity(0.15 * pulseAnim.value),
        child: Center(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              ScaleTransition(
                scale: pulseAnim,
                child: Container(
                  width: 160, height: 160,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    color: color.withOpacity(0.2),
                    border: Border.all(color: color, width: 3),
                    boxShadow: [
                      BoxShadow(color: color.withOpacity(0.5),
                        blurRadius: 40, spreadRadius: 10),
                    ],
                  ),
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Icon(Icons.water_drop, color: color, size: 48),
                      const SizedBox(height: 6),
                      Text('ZÁBER!', style: TextStyle(
                        color: color, fontSize: 22, fontWeight: FontWeight.bold,
                      )),
                    ],
                  ),
                ),
              ),
              const SizedBox(height: 24),
              Text(name, style: const TextStyle(
                fontSize: 20, fontWeight: FontWeight.bold, color: Colors.white)),
              const SizedBox(height: 32),
              ElevatedButton.icon(
                icon: const Icon(Icons.stop_circle),
                label: const Text('ZASTAVIŤ ALARM', style: TextStyle(fontSize: 16)),
                style: ElevatedButton.styleFrom(
                  backgroundColor: color,
                  foregroundColor: Colors.white,
                  padding: const EdgeInsets.symmetric(horizontal: 32, vertical: 16),
                  shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(30)),
                ),
                onPressed: onStop,
              ),
            ],
          ),
        ),
      ),
    );
  }
}
