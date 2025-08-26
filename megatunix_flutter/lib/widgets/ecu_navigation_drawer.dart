import 'package:flutter/material.dart';
import '../screens/diagnostics_screen.dart';
import '../screens/ecu_dashboard_screen.dart';
import '../screens/ve_table_editor_screen.dart';

/// ECU Navigation Drawer Widget
/// Provides navigation between different ECU views
class ECUNavigationDrawer extends StatelessWidget {
  const ECUNavigationDrawer({super.key});

  @override
  Widget build(BuildContext context) {
    return Drawer(
      child: ListView(
        padding: EdgeInsets.zero,
        children: [
          DrawerHeader(
            decoration: BoxDecoration(
              color: Theme.of(context).colorScheme.primary,
            ),
            child: const Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  'MegaTunix Redux',
                  style: TextStyle(
                    color: Colors.white,
                    fontSize: 24,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                SizedBox(height: 8),
                Text(
                  'ECU Tuning Software',
                  style: TextStyle(
                    color: Colors.white70,
                    fontSize: 16,
                  ),
                ),
              ],
            ),
          ),
          ListTile(
            leading: const Icon(Icons.dashboard),
            title: const Text('Dashboard'),
            onTap: () {
              Navigator.pop(context);
              // Navigate back to dashboard after current frame
              WidgetsBinding.instance.addPostFrameCallback((_) {
                Navigator.pushReplacement(
                  context,
                  MaterialPageRoute(
                    builder: (context) => const ECUDashboardScreen(),
                  ),
                );
              });
            },
          ),
          ListTile(
            leading: const Icon(Icons.table_chart),
            title: const Text('VE Table Editor'),
            onTap: () {
              Navigator.pop(context);
              // Navigate to VE table editor after current frame
              WidgetsBinding.instance.addPostFrameCallback((_) {
                Navigator.push(
                  context,
                  MaterialPageRoute(
                    builder: (context) => const VETableEditorScreen(),
                  ),
                );
              });
            },
          ),
          ListTile(
            leading: const Icon(Icons.timeline),
            title: const Text('Ignition Table'),
            onTap: () {
              Navigator.pop(context);
              // TODO: Navigate to ignition table
              WidgetsBinding.instance.addPostFrameCallback((_) {
                // Add navigation logic here when screen is created
              });
            },
          ),
          ListTile(
            leading: const Icon(Icons.analytics),
            title: const Text('Data Logging'),
            onTap: () {
              Navigator.pop(context);
              // TODO: Navigate to data logging
              WidgetsBinding.instance.addPostFrameCallback((_) {
                // Add navigation logic here when screen is created
              });
            },
          ),
          ListTile(
            leading: const Icon(Icons.bug_report),
            title: const Text('Diagnostics'),
            onTap: () {
              Navigator.pop(context);
              // Navigate to diagnostics after current frame
              WidgetsBinding.instance.addPostFrameCallback((_) {
                Navigator.push(
                  context,
                  MaterialPageRoute(
                    builder: (context) => const DiagnosticsScreen(),
                  ),
                );
              });
            },
          ),
          const Divider(),
          ListTile(
            leading: const Icon(Icons.settings),
            title: const Text('Settings'),
            onTap: () {
              Navigator.pop(context);
              // TODO: Navigate to settings
              WidgetsBinding.instance.addPostFrameCallback((_) {
                // Add navigation logic here when screen is created
              });
            },
          ),
          ListTile(
            leading: const Icon(Icons.help),
            title: const Text('Help'),
            onTap: () {
              Navigator.pop(context);
              // TODO: Navigate to help
              WidgetsBinding.instance.addPostFrameCallback((_) {
                // Add navigation logic here when screen is created
              });
            },
          ),
        ],
      ),
    );
  }
}

