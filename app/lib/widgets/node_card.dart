import 'package:flutter/material.dart';
import '../models/alarm_state.dart';

class NodeCard extends StatelessWidget {
  final NodeState node;
  final bool      compact;

  const NodeCard({super.key, required this.node, this.compact = false});

  @override
  Widget build(BuildContext context) {
    final color   = node.color;
    final online  = node.online;
    final hasBite = node.hadBite;

    return AnimatedContainer(
      duration: const Duration(milliseconds: 300),
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(16),
        color: Theme.of(context).colorScheme.surface,
        border: Border.all(
          color: hasBite ? color : (online ? color.withOpacity(0.4) : Colors.grey.shade800),
          width: hasBite ? 2.5 : 1.5,
        ),
        boxShadow: hasBite
            ? [BoxShadow(color: color.withOpacity(0.4), blurRadius: 16, spreadRadius: 2)]
            : [],
      ),
      padding: const EdgeInsets.all(14),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          // ── Farba + meno ────────────────────────────────────────
          Row(
            children: [
              Container(
                width: 14, height: 14,
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  color: online ? color : Colors.grey.shade700,
                  boxShadow: online
                      ? [BoxShadow(color: color.withOpacity(0.6), blurRadius: 8)]
                      : [],
                ),
              ),
              const SizedBox(width: 8),
              Text(
                node.name,
                style: TextStyle(
                  fontSize: 15,
                  fontWeight: FontWeight.w600,
                  color: online ? Colors.white : Colors.grey.shade500,
                ),
              ),
              const Spacer(),
              // Stav
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
                decoration: BoxDecoration(
                  borderRadius: BorderRadius.circular(20),
                  color: online ? color.withOpacity(0.15) : Colors.grey.shade900,
                ),
                child: Text(
                  online ? 'ONLINE' : 'OFFLINE',
                  style: TextStyle(
                    fontSize: 10,
                    fontWeight: FontWeight.bold,
                    color: online ? color : Colors.grey.shade600,
                    letterSpacing: 0.8,
                  ),
                ),
              ),
            ],
          ),
          if (!compact) ...[
            const SizedBox(height: 12),
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                // Batéria
                _BatteryIndicator(battery: node.battery),
                // Záber indikátor
                if (hasBite)
                  Container(
                    padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                    decoration: BoxDecoration(
                      borderRadius: BorderRadius.circular(20),
                      color: color.withOpacity(0.2),
                    ),
                    child: Row(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        Icon(Icons.water_drop, size: 14, color: color),
                        const SizedBox(width: 4),
                        Text('ZÁBER', style: TextStyle(
                          fontSize: 11, fontWeight: FontWeight.bold, color: color,
                        )),
                      ],
                    ),
                  ),
              ],
            ),
          ],
        ],
      ),
    );
  }
}

class _BatteryIndicator extends StatelessWidget {
  final int battery;
  const _BatteryIndicator({required this.battery});

  @override
  Widget build(BuildContext context) {
    if (battery < 0) {
      return Text('Bat: –', style: TextStyle(fontSize: 12, color: Colors.grey.shade600));
    }
    final color = battery > 50 ? Colors.green
                : battery > 20 ? Colors.orange
                : Colors.red;
    return Row(
      children: [
        Icon(
          battery > 50 ? Icons.battery_full
          : battery > 20 ? Icons.battery_3_bar
          : Icons.battery_1_bar,
          size: 16, color: color,
        ),
        const SizedBox(width: 4),
        Text('$battery%', style: TextStyle(fontSize: 12, color: color)),
      ],
    );
  }
}
