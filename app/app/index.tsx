import React, { useEffect, useCallback, useState } from 'react';
import { View, Text, FlatList, Pressable, ActivityIndicator, StyleSheet } from 'react-native';
import { router } from 'expo-router';
import { Device } from 'react-native-ble-plx';
import { useAlarmStore } from '../src/store/useAlarmStore';
import { bleService } from '../src/ble/BleService';
import { Colors, DEVICE_NAME } from '../src/constants';

export default function ScanScreen() {
  const { isScanning } = useAlarmStore();
  const [found, setFound] = useState<Device[]>([]);

  const startScan = useCallback(() => {
    setFound([]);
    bleService.startScan(device =>
      setFound(prev => prev.find(d => d.id === device.id) ? prev : [...prev, device])
    );
  }, []);

  useEffect(() => { startScan(); return () => bleService.stopScan(); }, []);

  const connect = async (device: Device) => {
    try {
      await bleService.connect(device);
      router.replace('/dashboard');
    } catch (e) {
      console.error('[CONNECT]', e);
    }
  };

  return (
    <View style={styles.container}>
      <View style={styles.logoWrap}>
        <View style={styles.logoCircle}>
          <Text style={styles.logoIcon}>🎣</Text>
        </View>
        <Text style={styles.appName}>BiteAlarm</Text>
        <Text style={styles.subtitle}>Hľadám {DEVICE_NAME}…</Text>
      </View>

      <FlatList
        data={found}
        keyExtractor={d => d.id}
        contentContainerStyle={styles.list}
        ListEmptyComponent={
          <View style={styles.empty}>
            {isScanning
              ? <ActivityIndicator color={Colors.primary} size="large" />
              : <Text style={styles.emptyText}>Žiadne zariadenie nenájdené</Text>}
          </View>
        }
        renderItem={({ item }) => (
          <View style={styles.card}>
            <View style={{ flex: 1 }}>
              <Text style={styles.cardName}>{item.name ?? 'Neznáme'}</Text>
              <Text style={styles.cardSub}>RSSI: {item.rssi} dBm</Text>
            </View>
            <Pressable style={styles.connectBtn} onPress={() => connect(item)}>
              <Text style={styles.connectText}>Pripojiť</Text>
            </Pressable>
          </View>
        )}
      />

      <Pressable style={[styles.scanBtn, isScanning && { opacity: 0.5 }]}
        onPress={startScan} disabled={isScanning}>
        {isScanning
          ? <ActivityIndicator color={Colors.textMuted} size="small" />
          : <Text style={styles.scanText}>↻  Skenovať znovu</Text>}
      </Pressable>
    </View>
  );
}

const styles = StyleSheet.create({
  container:   { flex: 1, backgroundColor: Colors.bg, paddingTop: 60 },
  logoWrap:    { alignItems: 'center', marginBottom: 40, gap: 8 },
  logoCircle:  { width: 96, height: 96, borderRadius: 48, backgroundColor: Colors.surface, borderWidth: 2, borderColor: `${Colors.primary}55`, alignItems: 'center', justifyContent: 'center', marginBottom: 8 },
  logoIcon:    { fontSize: 44 },
  appName:     { color: Colors.text, fontSize: 28, fontWeight: '800', letterSpacing: 1 },
  subtitle:    { color: Colors.textMuted, fontSize: 15 },
  list:        { paddingHorizontal: 16, flexGrow: 1 },
  empty:       { flex: 1, alignItems: 'center', justifyContent: 'center', paddingTop: 60 },
  emptyText:   { color: Colors.textMuted, fontSize: 15 },
  card:        { flexDirection: 'row', alignItems: 'center', backgroundColor: Colors.surface, borderRadius: 14, borderWidth: 1, borderColor: Colors.border, padding: 16, marginBottom: 10 },
  cardName:    { color: Colors.text, fontSize: 16, fontWeight: '600' },
  cardSub:     { color: Colors.textMuted, fontSize: 12, marginTop: 2 },
  connectBtn:  { backgroundColor: Colors.primary, paddingHorizontal: 18, paddingVertical: 10, borderRadius: 10 },
  connectText: { color: '#fff', fontWeight: '600', fontSize: 14 },
  scanBtn:     { margin: 16, marginBottom: 40, borderWidth: 1, borderColor: Colors.border, borderRadius: 12, paddingVertical: 16, alignItems: 'center' },
  scanText:    { color: Colors.textMuted, fontSize: 15, fontWeight: '500' },
});
