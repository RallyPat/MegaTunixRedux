/// Test script to run Flutter app with mock ECU data
/// This allows testing the UI without requiring a real ECU connection

import 'dart:async';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'screens/main_screen.dart';
import 'theme/ecu_theme.dart';
import 'services/ecu_service.dart';
import 'services/ini_parser.dart';
import 'services/ini_msq_service.dart';
import 'services/global_shortcuts_service.dart';
import 'services/data_logging_service.dart';
import 'services/help_service.dart';
import 'services/tuning_view_service.dart';
import 'services/gauge_designer_service.dart';

void main() async {
  // Initialize logging
  // Logger.root.level = Level.INFO;
  // Logger.root.onRecord.listen((record) {
  //   print('${record.level.name}: ${record.time}: ${record.message}');
  // });

  // Initialize services
  final helpService = HelpService();
  helpService.initialize();

  final gaugeDesignerService = GaugeDesignerService();

  final dataLoggingService = DataLoggingService();

  final globalShortcuts = GlobalShortcutsService();
  globalShortcuts.initialize(
    dataLoggingService: dataLoggingService,
    helpService: helpService,
    tuningViewService: TuningViewService(),
    gaugeDesignerService: gaugeDesignerService,
  );

  runApp(const MegaTunixAppWithMock());
}

class MegaTunixAppWithMock extends StatelessWidget {
  const MegaTunixAppWithMock({super.key});

  @override
  Widget build(BuildContext context) {
    return MultiProvider(
      providers: [
        ChangeNotifierProvider<ECUService>(
          create: (_) => ECUService(),
        ),
        ChangeNotifierProvider<INIECUManager>(
          create: (_) => INIECUManager(),
        ),
        ChangeNotifierProxyProvider<INIECUManager, INIMSQFileService>(
          create: (context) => INIMSQFileService(
            Provider.of<INIECUManager>(context, listen: false),
          ),
          update: (context, ecuManager, previous) =>
            previous ?? INIMSQFileService(ecuManager),
        ),
        ChangeNotifierProvider<GlobalShortcutsService>(
          create: (_) => GlobalShortcutsService(),
        ),
        ChangeNotifierProvider<DataLoggingService>(
          create: (_) => DataLoggingService(),
        ),
        ChangeNotifierProvider<HelpService>(
          create: (_) => HelpService(),
        ),
        ChangeNotifierProvider<TuningViewService>(
          create: (_) => TuningViewService(),
        ),
        ChangeNotifierProvider<GaugeDesignerService>(
          create: (_) => GaugeDesignerService(),
        ),
      ],
      child: MaterialApp(
        title: 'MegaTunix Redux - Mock Data Test',
        theme: ECUTheme.lightTheme,
        darkTheme: ECUTheme.darkTheme,
        themeMode: ThemeMode.system,
        debugShowCheckedModeBanner: false,
        home: const MockDataTestScreen(),
      ),
    );
  }
}

class MockDataTestScreen extends StatefulWidget {
  const MockDataTestScreen({super.key});

  @override
  State<MockDataTestScreen> createState() => _MockDataTestScreenState();
}

class _MockDataTestScreenState extends State<MockDataTestScreen> {
  late final ECUService _ecuService;

  @override
  void initState() {
    super.initState();
    _ecuService = context.read<ECUService>();

    // Start mock data generation after a short delay
    Future.delayed(const Duration(seconds: 1), () {
      _ecuService.startMockDataGeneration();
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('MegaTunix Redux - Mock Data Test'),
        actions: [
          IconButton(
            icon: const Icon(Icons.play_arrow),
            tooltip: 'Start Mock Data',
            onPressed: () => _ecuService.startMockDataGeneration(),
          ),
          IconButton(
            icon: const Icon(Icons.stop),
            tooltip: 'Stop Mock Data',
            onPressed: () => _ecuService.disconnect(),
          ),
        ],
      ),
      body: const MainScreen(),
    );
  }
}