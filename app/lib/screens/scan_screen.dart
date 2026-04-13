import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:provider/provider.dart';
import '../services/ble_service.dart';
import '../models/alarm_state.dart';
import 'dashboard_screen.dart';

class ScanScreen extends StatefulWidget {
  const ScanScreen({super.key});
  @override
  State<ScanScreen> createState() => _ScanScreenState();
}

class _ScanScreenState extends State<ScanScreen> {
  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      context.read<BleService>().startScan();
    });
  }

  @override
  void dispose() {
    context.read<BleService>().stopScan();
    super.dispose();
  }

  Future<void> _connect(BluetoothDevice device) async {
    final ble = context.read<BleService>();
    context.read<BleService>().stopScan();
    try {
      await ble.connect(device);
      if (!mounted) return;
      Navigator.of(context).pushReplacement(
        MaterialPageRoute(builder: (_) => const DashboardScreen()),
      );
    } catch (e) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Chyba spojenia: $e'), backgroundColor: Colors.red),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AlarmState>();

    return Scaffold(
      backgroundColor: const Color(0xFF0A0A0F),
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        title: const Text('FishAlarm', style: TextStyle(fontWeight: FontWeight.bold)),
        centerTitle: true,
      ),
      body: Column(
        children: [
          const SizedBox(height: 32),

          // ── Logo / ikona ──────────────────────────────────────────
          Container(
            width: 100, height: 100,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              color: const Color(0xFF1A1A2E),
              border: Border.all(color: Colors.blue.shade900, width: 2),
            ),
            child: const Icon(Icons.phishing, size: 52, color: Colors.blueAccent),
          ),
          const SizedBox(height: 16),
          const Text('Hľadám FishAlarm-RX…',
            style: TextStyle(fontSize: 16, color: Colors.grey)),
          const SizedBox(height: 32),

          // ── Scan výsledky ─────────────────────────────────────────
          Expanded(
            child: StreamBuilder<List<ScanResult>>(
              stream: context.read<BleService>().scanResults,
              builder: (ctx, snap) {
                final results = snap.data ?? [];
                if (results.isEmpty) {
                  return Center(
                    child: Column(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        if (state.isScanning)
                          const CircularProgressIndicator(color: Colors.blueAccent),
                        const SizedBox(height: 16),
                        Text(
                          state.isScanning
                              ? 'Skenujem BLE zariadenia…'
                              : 'Žiadne zariadenie nenájdené',
                          style: const TextStyle(color: Colors.grey),
                        ),
                      ],
                    ),
                  );
                }
                return ListView.builder(
                  padding: const EdgeInsets.symmetric(horizontal: 16),
                  itemCount: results.length,
                  itemBuilder: (ctx, i) {
                    final r    = results[i];
                    final name = r.device.platformName;
                    return Card(
                      color: const Color(0xFF1A1A2E),
                      margin: const EdgeInsets.only(bottom: 10),
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(14),
                        side: BorderSide(color: Colors.blue.shade900),
                      ),
                      child: ListTile(
                        leading: const Icon(Icons.bluetooth, color: Colors.blueAccent),
                        title: Text(name.isEmpty ? 'Neznáme' : name,
                          style: const TextStyle(fontWeight: FontWeight.w600)),
                        subtitle: Text('RSSI: ${r.rssi} dBm',
                          style: const TextStyle(fontSize: 12, color: Colors.grey)),
                        trailing: ElevatedButton(
                          onPressed: () => _connect(r.device),
                          style: ElevatedButton.styleFrom(
                            backgroundColor: Colors.blueAccent,
                            shape: RoundedRectangleBorder(
                              borderRadius: BorderRadius.circular(10)),
                          ),
                          child: const Text('Pripojiť'),
                        ),
                      ),
                    );
                  },
                );
              },
            ),
          ),

          // ── Znovu skenovať ───────────────────────────────────────
          Padding(
            padding: const EdgeInsets.fromLTRB(16, 8, 16, 32),
            child: SizedBox(
              width: double.infinity,
              child: OutlinedButton.icon(
                icon: state.isScanning
                    ? const SizedBox(width: 16, height: 16,
                        child: CircularProgressIndicator(strokeWidth: 2))
                    : const Icon(Icons.search),
                label: Text(state.isScanning ? 'Skenujem…' : 'Skenovať znovu'),
                onPressed: state.isScanning
                    ? null
                    : () => context.read<BleService>().startScan(),
              ),
            ),
          ),
        ],
      ),
    );
  }
}
