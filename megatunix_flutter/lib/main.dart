import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:logging/logging.dart';
import 'screens/main_screen.dart';
import 'theme/ecu_theme.dart';
import 'services/ecu_service.dart';
import 'services/ini_parser.dart';
import 'services/ini_msq_service.dart';

void main() {
  // Initialize logging
  Logger.root.level = Level.INFO;
  Logger.root.onRecord.listen((record) {
    print('${record.level.name}: ${record.time}: ${record.message}');
  });

  runApp(const MegaTunixApp());
}

class MegaTunixApp extends StatelessWidget {
  const MegaTunixApp({super.key});

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
      ],
      child: MaterialApp(
        title: 'MegaTunix Redux',
        theme: ECUTheme.lightTheme,
        darkTheme: ECUTheme.darkTheme,
        themeMode: ThemeMode.system,
        debugShowCheckedModeBanner: false,
        home: const MainScreen(),
      ),
    );
  }
}