import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import 'models/alarm_state.dart';
import 'services/ble_service.dart';
import 'screens/scan_screen.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  // Portrétnny mód
  await SystemChrome.setPreferredOrientations([DeviceOrientation.portraitUp]);

  // Tmavá status bar
  SystemChrome.setSystemUIOverlayStyle(const SystemUiOverlayStyle(
    statusBarColor: Colors.transparent,
    statusBarIconBrightness: Brightness.light,
  ));

  final alarmState = AlarmState();
  final bleService = BleService(alarmState);
  await bleService.init();

  runApp(
    MultiProvider(
      providers: [
        ChangeNotifierProvider.value(value: alarmState),
        Provider.value(value: bleService),
      ],
      child: const FishAlarmApp(),
    ),
  );
}

class FishAlarmApp extends StatelessWidget {
  const FishAlarmApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'FishAlarm',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        useMaterial3: true,
        brightness: Brightness.dark,
        colorScheme: ColorScheme.dark(
          primary:    Colors.blueAccent,
          secondary:  Colors.cyanAccent,
          surface:    const Color(0xFF141420),
          background: const Color(0xFF0A0A0F),
        ),
        scaffoldBackgroundColor: const Color(0xFF0A0A0F),
        cardTheme: CardTheme(
          color: const Color(0xFF141420),
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
        ),
        sliderTheme: const SliderThemeData(
          trackHeight: 4,
        ),
        elevatedButtonTheme: ElevatedButtonThemeData(
          style: ElevatedButton.styleFrom(
            shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
            padding: const EdgeInsets.symmetric(vertical: 14),
          ),
        ),
        outlinedButtonTheme: OutlinedButtonThemeData(
          style: OutlinedButton.styleFrom(
            shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
            side: const BorderSide(color: Colors.grey),
            padding: const EdgeInsets.symmetric(vertical: 14),
          ),
        ),
        appBarTheme: const AppBarTheme(
          backgroundColor: Colors.transparent,
          elevation: 0,
          titleTextStyle: TextStyle(
            fontSize: 20, fontWeight: FontWeight.bold, color: Colors.white,
          ),
        ),
      ),
      home: const ScanScreen(),
    );
  }
}
