import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { NodeState } from '../store/useAlarmStore';
import { Colors } from '../constants';

export function NodeCard({ node }: { node: NodeState }) {
  const { online, battery, hadBite, color, name } = node;

  return (
    <View style={[
      styles.card,
      { borderColor: hadBite ? color : online ? `${color}55` : Colors.border },
      hadBite && { shadowColor: color, shadowOpacity: 0.5, shadowRadius: 12, elevation: 8 },
    ]}>
      <View style={styles.row}>
        <View style={[styles.dot, { backgroundColor: online ? color : Colors.textMuted }]} />
        <Text style={[styles.name, !online && { color: Colors.textMuted }]}>{name}</Text>
        <View style={[styles.badge, { backgroundColor: online ? `${color}22` : '#1A1A1A' }]}>
          <Text style={[styles.badgeText, { color: online ? color : Colors.textMuted }]}>
            {online ? 'ONLINE' : 'OFFLINE'}
          </Text>
        </View>
      </View>

      <View style={[styles.row, { marginTop: 10 }]}>
        <BatteryBadge battery={battery} />
        {hadBite && (
          <View style={[styles.biteBadge, { backgroundColor: `${color}22` }]}>
            <Text style={[styles.biteText, { color }]}>● ZÁBER</Text>
          </View>
        )}
      </View>
    </View>
  );
}

function BatteryBadge({ battery }: { battery: number }) {
  if (battery < 0) return <Text style={styles.batUnknown}>Bat: –</Text>;
  const color = battery > 50 ? Colors.success : battery > 20 ? '#F59E0B' : Colors.danger;
  return <Text style={[styles.battery, { color }]}>▰ {battery}%</Text>;
}

const styles = StyleSheet.create({
  card: {
    flex: 1,
    backgroundColor: Colors.surface,
    borderRadius: 16,
    borderWidth: 1.5,
    padding: 14,
  },
  row:       { flexDirection: 'row', alignItems: 'center', gap: 8 },
  dot:       { width: 12, height: 12, borderRadius: 6 },
  name:      { flex: 1, color: Colors.text, fontSize: 15, fontWeight: '600' },
  badge:     { paddingHorizontal: 8, paddingVertical: 3, borderRadius: 20 },
  badgeText: { fontSize: 10, fontWeight: '700', letterSpacing: 0.8 },
  battery:   { fontSize: 12, fontWeight: '500' },
  batUnknown:{ fontSize: 12, color: Colors.textMuted },
  biteBadge: { marginLeft: 'auto', paddingHorizontal: 10, paddingVertical: 3, borderRadius: 20 },
  biteText:  { fontSize: 11, fontWeight: '700' },
});
