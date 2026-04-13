import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/alarm_state.dart';
import '../services/ble_service.dart';

class SettingsPanel extends StatelessWidget {
  const SettingsPanel({super.key});

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AlarmState>();
    final ble   = context.read<BleService>();

    return Container(
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surface,
        borderRadius: const BorderRadius.vertical(top: Radius.circular(24)),
      ),
      padding: const EdgeInsets.fromLTRB(20, 12, 20, 32),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // Handle
          Center(
            child: Container(
              width: 40, height: 4,
              margin: const EdgeInsets.only(bottom: 20),
              decoration: BoxDecoration(
                color: Colors.grey.shade700,
                borderRadius: BorderRadius.circular(2),
              ),
            ),
          ),

          Text('Nastavenia receivera',
            style: Theme.of(context).textTheme.titleMedium?.copyWith(
              fontWeight: FontWeight.bold,
            )),
          const SizedBox(height: 20),

          // ── Hlasitosť ──────────────────────────────────────────
          _SliderRow(
            icon: Icons.volume_up,
            label: 'Hlasitosť',
            value: state.volume.toDouble(),
            min: 1, max: 10, divisions: 9,
            displayValue: '${state.volume}',
            onChangeEnd: (v) {
              state.volume = v.round();
              ble.send('VOL:${state.volume}');
            },
          ),
          const SizedBox(height: 12),

          // ── Tón ────────────────────────────────────────────────
          _SliderRow(
            icon: Icons.music_note,
            label: 'Tón',
            value: state.toneHz.toDouble(),
            min: 200, max: 4000, divisions: 38,
            displayValue: '${state.toneHz} Hz',
            onChangeEnd: (v) {
              state.toneHz = v.round();
              ble.send('TONE:${state.toneHz}');
            },
          ),
          const SizedBox(height: 20),

          // ── Prepínače ──────────────────────────────────────────
          Row(
            children: [
              Expanded(
                child: _ToggleTile(
                  icon: Icons.vibration,
                  label: 'Vibrácia',
                  value: state.vibro,
                  onChanged: (v) {
                    state.vibro = v;
                    ble.send('VIBRO:${v ? "ON" : "OFF"}');
                    state.notifyListeners(); // priamy call
                  },
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: _ToggleTile(
                  icon: Icons.notifications_active,
                  label: 'Zvuk',
                  value: state.sound,
                  onChanged: (v) {
                    state.sound = v;
                    ble.send('SOUND:${v ? "ON" : "OFF"}');
                    state.notifyListeners();
                  },
                ),
              ),
            ],
          ),
          const SizedBox(height: 20),

          // ── Refresh stavu ───────────────────────────────────────
          SizedBox(
            width: double.infinity,
            child: OutlinedButton.icon(
              icon: const Icon(Icons.refresh, size: 18),
              label: const Text('Obnoviť stav'),
              onPressed: () {
                ble.send('STATUS');
                ble.send('NODES');
              },
            ),
          ),
        ],
      ),
    );
  }
}

// ── Slider riadok ─────────────────────────────────────────────────────
class _SliderRow extends StatefulWidget {
  final IconData icon;
  final String   label;
  final double   value;
  final double   min, max;
  final int      divisions;
  final String   displayValue;
  final void Function(double) onChangeEnd;

  const _SliderRow({
    required this.icon, required this.label,
    required this.value, required this.min, required this.max,
    required this.divisions, required this.displayValue,
    required this.onChangeEnd,
  });

  @override
  State<_SliderRow> createState() => _SliderRowState();
}

class _SliderRowState extends State<_SliderRow> {
  late double _val;
  @override
  void initState() { super.initState(); _val = widget.value; }
  @override
  void didUpdateWidget(_SliderRow old) { super.didUpdateWidget(old); _val = widget.value; }

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            Icon(widget.icon, size: 18, color: Colors.grey.shade400),
            const SizedBox(width: 8),
            Text(widget.label, style: const TextStyle(fontSize: 14)),
            const Spacer(),
            Text(widget.displayValue,
              style: TextStyle(
                fontSize: 13, fontWeight: FontWeight.bold,
                color: Theme.of(context).colorScheme.primary,
              )),
          ],
        ),
        Slider(
          value: _val,
          min: widget.min, max: widget.max,
          divisions: widget.divisions,
          onChanged: (v) => setState(() => _val = v),
          onChangeEnd: widget.onChangeEnd,
        ),
      ],
    );
  }
}

// ── Toggle dlaždica ───────────────────────────────────────────────────
class _ToggleTile extends StatelessWidget {
  final IconData icon;
  final String   label;
  final bool     value;
  final void Function(bool) onChanged;

  const _ToggleTile({
    required this.icon, required this.label,
    required this.value, required this.onChanged,
  });

  @override
  Widget build(BuildContext context) {
    final color = value
        ? Theme.of(context).colorScheme.primary
        : Colors.grey.shade700;

    return GestureDetector(
      onTap: () => onChanged(!value),
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 200),
        padding: const EdgeInsets.symmetric(vertical: 14, horizontal: 12),
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(12),
          color: value
              ? Theme.of(context).colorScheme.primary.withOpacity(0.12)
              : Colors.grey.shade900,
          border: Border.all(color: color.withOpacity(0.5)),
        ),
        child: Row(
          children: [
            Icon(icon, size: 20, color: color),
            const SizedBox(width: 8),
            Text(label, style: TextStyle(fontSize: 13, color: color)),
            const Spacer(),
            Icon(
              value ? Icons.toggle_on : Icons.toggle_off,
              color: color, size: 28,
            ),
          ],
        ),
      ),
    );
  }
}
