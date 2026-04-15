import React, { useRef, useEffect } from 'react';
import {
  View, Text, ScrollView, Pressable,
  StyleSheet, Animated, FlatList,
} from 'react-native';
import { router } from 'expo-router';
import { useKeepAwake } from 'expo-keep-awake';
import { useAlarmStore, BiteEvent } from '../src/store/useAlarmStore';
import { bleService } from '../src/ble/BleService';
import { NodeCard } from '../src/components/NodeCard';
import { SettingsModal } from '../src/components/SettingsModal';
import { Colors, NODE_COLORS } from '../src/constants';

export default function DashboardScreen() {
  useKeepAwake();

  const {
    isConnected, deviceName,
    alarmActive, alarmNodeId,
    nodes, biteLog,
    clearAlarm, resetBiteFlags,
  } = useAlarmStore();

  const [settingsOpen, setSettingsOpen] = React.useState(false);

  // Pulzovacia animácia pre alarm overlay
  const pulseAnim = useRef(new Animated.Value(0.85)).current;

  useEffect(() => {
    if (!alarmActive) return;
    const anim = Animated.loop(
      Animated.sequence([
        Animated.timing(pulseAnim, { toValue: 1,    duration: 500, useNativeDriver: true }),
        Animated.timing(pulseAnim, { toValue: 0.85, duration: 500, useNativeDriver: true }),
      ])
    );
    anim.start();
    return () => anim.stop();
  }, [alarmActive]);

  const handleDisconnect = async () => {
    await bleService.disconnect();
    router.replace('/');
  };

  const handleStop = () => {
    bleService.send('STOP');
    clearAlarm();
  };

  // Len 2 aktívne nody
  const activeNodes = nodes.slice(0, 2);
  const alarmColor  = alarmActive
    ? NODE_COLORS[(alarmNodeId - 1)] ?? Colors.danger
    : Colors.danger;

  return (
    <View style={styles.container}>
      {/* ── Header ─────────────────────────────────────────────────── */}
      <View style={styles.header}>
        <View style={styles.headerLeft}>
          <Text style={styles.appName}>🎣 BiteAlarm</Text>
          <Text style={styles.deviceName}>{deviceName}</Text>
        </View>
        <View style={styles.headerRight}>
          <View style={[styles.connBadge,
            { backgroundColor: isConnected ? '#16532220' : '#7F1D1D20' }]}>
            <View style={[styles.connDot,
              { backgroundColor: isConnected ? Colors.success : Colors.danger }]} />
            <Text style={{ color: isConnected ? Colors.success : Colors.danger, fontSize: 12 }}>
              {isConnected ? 'Connected' : 'Lost'}
            </Text>
          </View>
          <Pressable onPress={() => setSettingsOpen(true)} hitSlop={10}>
            <Text style={styles.settingsIcon}>⚙️</Text>
          </Pressable>
        </View>
      </View>

      <ScrollView
        contentContainerStyle={styles.scroll}
        showsVerticalScrollIndicator={false}
      >
        {/* ── Node karty ─────────────────────────────────────────── */}
        <Text style={styles.sectionLabel}>PRÚTY</Text>
        <View style={styles.nodeGrid}>
          {activeNodes.map(node => (
            <View key={node.id} style={styles.nodeItem}>
              <NodeCard node={node} />
            </View>
          ))}
        </View>

        {nodes.some(n => n.hadBite) && (
          <Pressable style={styles.resetBtn} onPress={resetBiteFlags}>
            <Text style={styles.resetText}>↺  Resetovať záberov</Text>
          </Pressable>
        )}

        {/* ── Historia záberov ────────────────────────────────────── */}
        <Text style={[styles.sectionLabel, { marginTop: 24 }]}>HISTÓRIA</Text>
        {biteLog.length === 0
          ? (
            <View style={styles.emptyLog}>
              <Text style={styles.emptyLogText}>💧 Zatiaľ žiadny záber</Text>
            </View>
          )
          : biteLog.slice(0, 10).map((e, i) => (
              <BiteLogItem key={`${e.nodeId}-${e.time.toISOString()}`} event={e} />
            ))
        }
      </ScrollView>

      {/* ── Bottom bar ─────────────────────────────────────────────── */}
      <View style={styles.bottomBar}>
        <Pressable style={styles.disconnectBtn} onPress={handleDisconnect}>
          <Text style={styles.disconnectText}>⊗  Odpojiť</Text>
        </Pressable>
        <Pressable
          style={[styles.stopBtn, !alarmActive && styles.stopBtnDisabled]}
          onPress={handleStop}
          disabled={!alarmActive}
        >
          <Text style={styles.stopText}>■  STOP Alarm</Text>
        </Pressable>
      </View>

      {/* ── Alarm Overlay ──────────────────────────────────────────── */}
      {alarmActive && (
        <View style={[styles.overlay, { backgroundColor: `${alarmColor}22` }]}>
          <Animated.View style={[
            styles.alarmCircle,
            {
              borderColor: alarmColor,
              shadowColor: alarmColor,
              transform: [{ scale: pulseAnim }],
            },
          ]}>
            <Text style={styles.alarmEmoji}>💧</Text>
            <Text style={[styles.alarmLabel, { color: alarmColor }]}>ZÁBER!</Text>
            <Text style={styles.alarmNode}>
              {nodes.find(n => n.id === alarmNodeId)?.name ?? `Prút ${alarmNodeId}`}
            </Text>
          </Animated.View>

          <Pressable
            style={[styles.overlayStop, { backgroundColor: alarmColor }]}
            onPress={handleStop}
          >
            <Text style={styles.overlayStopText}>ZASTAVIŤ ALARM</Text>
          </Pressable>
        </View>
      )}

      {/* ── Settings Modal ─────────────────────────────────────────── */}
      <SettingsModal
        visible={settingsOpen}
        onClose={() => setSettingsOpen(false)}
      />
    </View>
  );
}

// ── Bite log položka ──────────────────────────────────────────────────
function BiteLogItem({ event }: { event: BiteEvent }) {
  const t = event.time;
  const time = [t.getHours(), t.getMinutes(), t.getSeconds()]
    .map(n => String(n).padStart(2, '0')).join(':');

  return (
    <View style={[styles.logItem, { borderColor: `${event.color}33` }]}>
      <View style={[styles.logDot, { backgroundColor: event.color }]} />
      <Text style={styles.logName}>{event.name}</Text>
      <Text style={styles.logTime}>{time}  💧</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: Colors.bg,
    paddingTop: 56,
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingHorizontal: 16,
    paddingBottom: 12,
  },
  headerLeft: { gap: 2 },
  appName: { color: Colors.text, fontSize: 20, fontWeight: '800' },
  deviceName: { color: Colors.textMuted, fontSize: 12 },
  headerRight: { flexDirection: 'row', alignItems: 'center', gap: 10 },
  connBadge: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 5,
    paddingHorizontal: 10,
    paddingVertical: 5,
    borderRadius: 20,
  },
  connDot: { width: 7, height: 7, borderRadius: 4 },
  settingsIcon: { fontSize: 22 },
  scroll: {
    paddingHorizontal: 16,
    paddingBottom: 100,
  },
  sectionLabel: {
    color: Colors.textMuted,
    fontSize: 12,
    fontWeight: '700',
    letterSpacing: 1,
    marginBottom: 10,
  },
  nodeGrid: {
    flexDirection: 'row',
    gap: 10,
  },
  nodeItem: { flex: 1 },
  resetBtn: {
    alignSelf: 'flex-end',
    marginTop: 8,
    padding: 6,
  },
  resetText: { color: Colors.textMuted, fontSize: 12 },
  emptyLog: {
    alignItems: 'center',
    paddingVertical: 32,
  },
  emptyLogText: { color: Colors.textMuted, fontSize: 15 },
  logItem: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 10,
    borderWidth: 1,
    borderRadius: 10,
    paddingHorizontal: 14,
    paddingVertical: 10,
    marginBottom: 6,
    backgroundColor: Colors.surface,
  },
  logDot: { width: 10, height: 10, borderRadius: 5 },
  logName: { color: Colors.text, fontWeight: '600', flex: 1 },
  logTime: { color: Colors.textMuted, fontSize: 12 },
  bottomBar: {
    position: 'absolute',
    bottom: 0, left: 0, right: 0,
    flexDirection: 'row',
    gap: 10,
    padding: 16,
    paddingBottom: 32,
    backgroundColor: Colors.bg,
    borderTopWidth: 1,
    borderColor: Colors.border,
  },
  disconnectBtn: {
    flex: 1,
    borderWidth: 1,
    borderColor: Colors.border,
    borderRadius: 12,
    paddingVertical: 14,
    alignItems: 'center',
  },
  disconnectText: { color: Colors.textMuted, fontWeight: '600' },
  stopBtn: {
    flex: 1,
    backgroundColor: '#7F1D1D',
    borderRadius: 12,
    paddingVertical: 14,
    alignItems: 'center',
  },
  stopBtnDisabled: { opacity: 0.35 },
  stopText: { color: '#fff', fontWeight: '700' },

  // Alarm overlay
  overlay: {
    ...StyleSheet.absoluteFillObject,
    alignItems: 'center',
    justifyContent: 'center',
    gap: 32,
  },
  alarmCircle: {
    width: 180, height: 180,
    borderRadius: 90,
    borderWidth: 3,
    alignItems: 'center',
    justifyContent: 'center',
    gap: 4,
    shadowOpacity: 0.6,
    shadowRadius: 30,
    shadowOffset: { width: 0, height: 0 },
    elevation: 20,
    backgroundColor: Colors.surface,
  },
  alarmEmoji: { fontSize: 44 },
  alarmLabel: { fontSize: 22, fontWeight: '800' },
  alarmNode:  { color: Colors.textMuted, fontSize: 14 },
  overlayStop: {
    paddingHorizontal: 36,
    paddingVertical: 18,
    borderRadius: 30,
  },
  overlayStopText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '800',
    letterSpacing: 0.5,
  },
});
