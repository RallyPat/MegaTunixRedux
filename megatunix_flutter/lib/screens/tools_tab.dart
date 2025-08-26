import 'package:flutter/material.dart';

class ToolsTab extends StatelessWidget {
  const ToolsTab({super.key});

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'Tuning Tools',
            style: Theme.of(context).textTheme.headlineMedium,
          ),
          
          const SizedBox(height: 24),
          
          // Tools grid
          Expanded(
            child: GridView.count(
              crossAxisCount: 2,
              childAspectRatio: 1.5,
              crossAxisSpacing: 16,
              mainAxisSpacing: 16,
              children: [
                _buildToolCard(
                  context,
                  'Data Logging',
                  'Record and analyze ECU data',
                  Icons.analytics,
                  () => _showDataLogging(context),
                ),
                _buildToolCard(
                  context,
                  'Diagnostics',
                  'Check ECU status and errors',
                  Icons.bug_report,
                  () => _showDiagnostics(context),
                ),
                _buildToolCard(
                  context,
                  'Calibration',
                  'Calibrate sensors and actuators',
                  Icons.tune,
                  () => _showCalibration(context),
                ),
                _buildToolCard(
                  context,
                  'Real-time Display',
                  'Live parameter monitoring',
                  Icons.monitor,
                  () => _showRealTimeDisplay(context),
                ),
                _buildToolCard(
                  context,
                  'Firmware Update',
                  'Update ECU firmware',
                  Icons.system_update,
                  () => _showFirmwareUpdate(context),
                ),
                _buildToolCard(
                  context,
                  'Backup & Restore',
                  'Save and restore ECU settings',
                  Icons.backup,
                  () => _showBackupRestore(context),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildToolCard(
    BuildContext context,
    String title,
    String description,
    IconData icon,
    VoidCallback onTap,
  ) {
    return Card(
      child: InkWell(
        onTap: onTap,
        borderRadius: BorderRadius.circular(12),
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(
                icon,
                size: 48,
                color: Theme.of(context).colorScheme.primary,
              ),
              const SizedBox(height: 12),
              Text(
                title,
                style: Theme.of(context).textTheme.titleMedium?.copyWith(
                  fontWeight: FontWeight.bold,
                ),
                textAlign: TextAlign.center,
              ),
              const SizedBox(height: 4),
              Text(
                description,
                style: Theme.of(context).textTheme.bodySmall,
                textAlign: TextAlign.center,
                maxLines: 2,
                overflow: TextOverflow.ellipsis,
              ),
            ],
          ),
        ),
      ),
    );
  }

  void _showDataLogging(BuildContext context) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Data Logging'),
        content: const Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Configure data logging parameters:'),
            SizedBox(height: 16),
            Text('• Sample Rate: 10 Hz'),
            Text('• Log File: /logs/session_001.csv'),
            Text('• Parameters: RPM, MAP, TPS, AFR, CLT'),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Close'),
          ),
          ElevatedButton(
            onPressed: () {
              Navigator.pop(context);
              // TODO: Start logging
            },
            child: const Text('Start Logging'),
          ),
        ],
      ),
    );
  }

  void _showDiagnostics(BuildContext context) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('ECU Diagnostics'),
        content: const Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('ECU Status: OK'),
            SizedBox(height: 8),
            Text('Firmware: Speeduino 202311'),
            SizedBox(height: 8),
            Text('Uptime: 02:34:17'),
            SizedBox(height: 8),
            Text('Free RAM: 1024 bytes'),
            SizedBox(height: 16),
            Text('Active Errors: None'),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Close'),
          ),
          ElevatedButton(
            onPressed: () {
              Navigator.pop(context);
              // TODO: Run full diagnostic
            },
            child: const Text('Run Full Diagnostic'),
          ),
        ],
      ),
    );
  }

  void _showCalibration(BuildContext context) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Sensor Calibration'),
        content: const Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Available calibrations:'),
            SizedBox(height: 16),
            Text('• TPS Calibration'),
            Text('• MAP Sensor Calibration'),
            Text('• IAT Sensor Calibration'),
            Text('• CLT Sensor Calibration'),
            Text('• O2 Sensor Calibration'),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Close'),
          ),
          ElevatedButton(
            onPressed: () {
              Navigator.pop(context);
              // TODO: Open calibration wizard
            },
            child: const Text('Start Calibration'),
          ),
        ],
      ),
    );
  }

  void _showRealTimeDisplay(BuildContext context) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Real-time Display'),
        content: const Text('Real-time parameter display is already active on the Dashboard tab.'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('OK'),
          ),
        ],
      ),
    );
  }

  void _showFirmwareUpdate(BuildContext context) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Firmware Update'),
        content: const Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Current Firmware: Speeduino 202311'),
            SizedBox(height: 8),
            Text('Latest Available: Speeduino 202401'),
            SizedBox(height: 16),
            Text('⚠️ Warning: Do not disconnect during update!'),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () {
              Navigator.pop(context);
              // TODO: Start firmware update
            },
            child: const Text('Update Firmware'),
          ),
        ],
      ),
    );
  }

  void _showBackupRestore(BuildContext context) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Backup & Restore'),
        content: const Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Backup Options:'),
            SizedBox(height: 8),
            Text('• Full ECU Configuration'),
            Text('• Tables Only'),
            Text('• Settings Only'),
            SizedBox(height: 16),
            Text('Recent Backups:'),
            Text('• backup_20240125_143022.mtx'),
            Text('• backup_20240124_091455.mtx'),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Close'),
          ),
          TextButton(
            onPressed: () {
              Navigator.pop(context);
              // TODO: Restore backup
            },
            child: const Text('Restore'),
          ),
          ElevatedButton(
            onPressed: () {
              Navigator.pop(context);
              // TODO: Create backup
            },
            child: const Text('Backup Now'),
          ),
        ],
      ),
    );
  }
}