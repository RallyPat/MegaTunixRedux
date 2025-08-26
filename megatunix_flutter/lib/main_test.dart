import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:logging/logging.dart';
import 'services/ecu_service.dart';

void main() {
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
      ],
      child: MaterialApp(
        title: 'MegaTunix Redux',
        theme: ThemeData(
          useMaterial3: true,
          colorScheme: ColorScheme.fromSeed(seedColor: const Color(0xFF42CEDB)),
        ),
        debugShowCheckedModeBanner: false,
        home: const SimpleTestScreen(),
      ),
    );
  }
}

class SimpleTestScreen extends StatelessWidget {
  const SimpleTestScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('MegaTunix Redux - New UI Test'),
        backgroundColor: Theme.of(context).colorScheme.primaryContainer,
      ),
      body: const Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              Icons.speed,
              size: 100,
              color: Colors.blue,
            ),
            SizedBox(height: 20),
            Text(
              'MegaTunix Redux',
              style: TextStyle(fontSize: 32, fontWeight: FontWeight.bold),
            ),
            SizedBox(height: 10),
            Text(
              'New Flutter UI Working!',
              style: TextStyle(fontSize: 18),
            ),
            SizedBox(height: 20),
            Text(
              'If you can see this, the Flutter UI redesign is successful!',
              textAlign: TextAlign.center,
            ),
          ],
        ),
      ),
    );
  }
}