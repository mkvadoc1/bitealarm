import React from 'react';
import { Modal, View, Text, StyleSheet, Pressable, Switch, ScrollView } from 'react-native';
import Slider from '@react-native-community/slider';
import { useAlarmStore } from '../store/useAlarmStore';
import { bleService } from '../ble/BleService';
import { Colors } from '../constants';

export function SettingsModal({ visible, onClose }: { visible: boolean; onClose: () => void }) {
  const { volume, toneHz, vibro, sound } = useAlarmStore();

  return (
    <Modal visible={visible} animationType="slide" presentationStyle="pageSheet" onRequestClose={onClose}>
      <View style={styles.container}>
        <View style={styles.handle} />
        <View style={styles.header}>
          <Text style={styles.title}>Nastavenia receivera</Text>
          <Pressable onPress={onClose} hitSlop={12}>
            <Text style={styles.closeBtn}>✕</Text>
          </Pressable>
        </View>

        <ScrollView contentContainerStyle={styles.content}>
          {/* Hlasitosť */}
          <View style={styles.sliderBlock}>
            <View style={styles.sliderHeader}>
              <Text style={styles.label}>Hlasitosť</Text>
              <Text style={styles.value}>{volume}</Text>
            </View>
            <Slider
              style={styles.slider}
              value={volume}
              minimumValue={1} maximumValue={10} step={1}
              minimumTrackTintColor={Colors.primary}
              maximumTrackTintColor={Colors.border}
              thumbTintColor={Colors.primary}
              onValueChange={v => useAlarmStore.setState({ volume: Math.round(v) })}
              onSlidingComplete={v => bleService.send(`VOL:${Math.round(v)}`)}
            />
          </View>

          {/* Tón */}
          <View style={styles.sliderBlock}>
            <View style={styles.sliderHeader}>
              <Text style={styles.label}>Tón</Text>
              <Text style={styles.value}>{toneHz} Hz</Text>
            </View>
            <Slider
              style={styles.slider}
              value={toneHz}
              minimumValue={200} maximumValue={4000} step={100}
              minimumTrackTintColor={Colors.primary}
              maximumTrackTintColor={Colors.border}
              thumbTintColor={Colors.primary}
              onValueChange={v => useAlarmStore.setState({ toneHz: Math.round(v) })}
              onSlidingComplete={v => bleService.send(`TONE:${Math.round(v)}`)}
            />
          </View>

          {/* Prepínače */}
          <View style={styles.toggleRow}>
            <ToggleTile label="Vibrácia" icon="📳" value={vibro}
              onChange={v => { useAlarmStore.setState({ vibro: v }); bleService.send(`VIBRO:${v ? 'ON' : 'OFF'}`); }} />
            <ToggleTile label="Zvuk" icon="🔔" value={sound}
              onChange={v => { useAlarmStore.setState({ sound: v }); bleService.send(`SOUND:${v ? 'ON' : 'OFF'}`); }} />
          </View>

          <Pressable style={styles.refreshBtn}
            onPress={() => { bleService.send('STATUS'); bleService.send('NODES'); }}>
            <Text style={styles.refreshText}>↻  Obnoviť stav</Text>
          </Pressable>
        </ScrollView>
      </View>
    </Modal>
  );
}

function ToggleTile({ label, icon, value, onChange }: {
  label: string; icon: string; value: boolean; onChange: (v: boolean) => void;
}) {
  return (
    <View style={[styles.toggle,
      { borderColor: value ? `${Colors.primary}66` : Colors.border },
      value && { backgroundColor: `${Colors.primary}15` },
    ]}>
      <Text style={styles.toggleIcon}>{icon}</Text>
      <Text style={[styles.toggleLabel, { color: value ? Colors.text : Colors.textMuted }]}>{label}</Text>
      <Switch value={value} onValueChange={onChange}
        trackColor={{ false: Colors.border, true: `${Colors.primary}88` }}
        thumbColor={value ? Colors.primary : '#555'} />
    </View>
  );
}

const styles = StyleSheet.create({
  container:    { flex: 1, backgroundColor: Colors.bg, paddingTop: 12 },
  handle:       { width: 40, height: 4, backgroundColor: '#444', borderRadius: 2, alignSelf: 'center', marginBottom: 16 },
  header:       { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', paddingHorizontal: 20, marginBottom: 24 },
  title:        { color: Colors.text, fontSize: 18, fontWeight: '700' },
  closeBtn:     { color: Colors.textMuted, fontSize: 18 },
  content:      { paddingHorizontal: 20, gap: 20, paddingBottom: 40 },
  sliderBlock:  { gap: 4 },
  sliderHeader: { flexDirection: 'row', justifyContent: 'space-between' },
  label:        { color: Colors.text, fontSize: 14 },
  value:        { color: Colors.primary, fontSize: 14, fontWeight: '600' },
  slider:       { width: '100%', height: 40 },
  toggleRow:    { flexDirection: 'row', gap: 12 },
  toggle:       { flex: 1, flexDirection: 'row', alignItems: 'center', gap: 8, padding: 12, borderRadius: 12, borderWidth: 1, backgroundColor: Colors.surface },
  toggleIcon:   { fontSize: 18 },
  toggleLabel:  { flex: 1, fontSize: 13, fontWeight: '500' },
  refreshBtn:   { borderWidth: 1, borderColor: Colors.border, borderRadius: 12, paddingVertical: 14, alignItems: 'center' },
  refreshText:  { color: Colors.textMuted, fontSize: 14 },
});
