import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/ecu_service.dart';
import '../models/ecu_data.dart';
import '../views/dashboard_view.dart';
import '../views/tables_view.dart';
import 'tools_tab.dart';
import 'settings_tab.dart';

class MainScreen extends StatefulWidget {
  const MainScreen({super.key});

  @override
  State<MainScreen> createState() => _MainScreenState();
}

class _MainScreenState extends State<MainScreen> with SingleTickerProviderStateMixin {
  late TabController _tabController;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 4, vsync: this);
  }

  @override
  void dispose() {
    _tabController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Row(
          children: [
            Icon(Icons.speed, color: Theme.of(context).colorScheme.primary),
            const SizedBox(width: 8),
            const Text('MegaTunix Redux'),
          ],
        ),
        backgroundColor: Theme.of(context).colorScheme.surface,
        elevation: 1,
        bottom: TabBar(
          controller: _tabController,
          tabs: const [
            Tab(icon: Icon(Icons.dashboard), text: 'Dashboard'),
            Tab(icon: Icon(Icons.table_chart), text: 'Tables'),
            Tab(icon: Icon(Icons.build), text: 'Tools'),
            Tab(icon: Icon(Icons.settings), text: 'Settings'),
          ],
        ),
        actions: [
          Consumer<ECUService>(
            builder: (context, ecuService, child) {
              return Container(
                padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                child: Row(
                  children: [
                    Icon(
                      ecuService.connectionState == ECUConnectionState.connected
                          ? Icons.link
                          : Icons.link_off,
                      color: ecuService.connectionState == ECUConnectionState.connected
                          ? Colors.green
                          : Colors.red,
                      size: 20,
                    ),
                    const SizedBox(width: 4),
                    Text(
                      ecuService.connectionState == ECUConnectionState.connected
                          ? 'Connected (Demo)'
                          : 'Disconnected',
                      style: Theme.of(context).textTheme.bodySmall,
                    ),
                  ],
                ),
              );
            },
          ),
        ],
      ),
      body: TabBarView(
        controller: _tabController,
        children: const [
          DashboardView(),
          TablesView(),
          ToolsTab(),
          SettingsTab(),
        ],
      ),
    );
  }
}