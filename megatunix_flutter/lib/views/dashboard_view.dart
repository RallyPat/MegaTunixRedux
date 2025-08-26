import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/ecu_service.dart';
import '../models/ecu_data.dart';

/// Modern dashboard view with responsive gauge layout
class DashboardView extends StatelessWidget {
  const DashboardView({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Consumer<ECUService>(
        builder: (context, ecuService, child) {
          return CustomScrollView(
            slivers: [
              SliverAppBar(
                title: const Text('Dashboard'),
                floating: true,
                automaticallyImplyLeading: false,
                actions: [
                  _buildDemoButton(context, ecuService),
                  const SizedBox(width: 8),
                ],
              ),
              SliverPadding(
                padding: const EdgeInsets.all(16),
                sliver: SliverGrid(
                  gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                    crossAxisCount: 3,
                    childAspectRatio: 1.2,
                    crossAxisSpacing: 16,
                    mainAxisSpacing: 16,
                  ),
                  delegate: SliverChildListDelegate([
                    _buildGaugeCard(
                      context,
                      title: 'RPM',
                      value: '${ecuService.currentData?.rpm ?? 0}',
                      unit: 'rpm',
                      icon: Icons.speed,
                      color: Theme.of(context).colorScheme.primary,
                    ),
                    _buildGaugeCard(
                      context,
                      title: 'MAP',
                      value: '${ecuService.currentData?.map ?? 0}',
                      unit: 'kPa',
                      icon: Icons.air,
                      color: const Color(0xFFFF6B35),
                    ),
                    _buildGaugeCard(
                      context,
                      title: 'TPS',
                      value: '${ecuService.currentData?.tps ?? 0}',
                      unit: '%',
                      icon: Icons.touch_app,
                      color: const Color(0xFFFFC107),
                    ),
                    _buildGaugeCard(
                      context,
                      title: 'AFR',
                      value: '${ecuService.currentData?.afr?.toStringAsFixed(1) ?? '0.0'}',
                      unit: ':1',
                      icon: Icons.local_gas_station,
                      color: const Color(0xFF4CAF50),
                    ),
                    _buildGaugeCard(
                      context,
                      title: 'Timing',
                      value: '${ecuService.currentData?.timing ?? 0}',
                      unit: '°',
                      icon: Icons.access_time,
                      color: Theme.of(context).colorScheme.primary,
                    ),
                    _buildGaugeCard(
                      context,
                      title: 'Coolant',
                      value: '${ecuService.currentData?.coolantTemp ?? 0}',
                      unit: '°C',
                      icon: Icons.thermostat,
                      color: const Color(0xFF2196F3),
                    ),
                  ]),
                ),
              ),
              SliverToBoxAdapter(
                child: Padding(
                  padding: const EdgeInsets.all(16),
                  child: _buildStatusPanel(context, ecuService),
                ),
              ),
            ],
          );
        },
      ),
    );
  }

  Widget _buildDemoButton(BuildContext context, ECUService ecuService) {
    final isConnected = ecuService.connectionState == ECUConnectionState.connected;
    
    return FilledButton.icon(
      onPressed: () {
        if (isConnected) {
          ecuService.disconnect();
        } else {
          ecuService.startMockDataGeneration();
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('🚗 Demo mode started with simulated car data!'),
              duration: Duration(seconds: 2),
            ),
          );
        }
      },
      icon: Icon(
        isConnected ? Icons.stop : Icons.play_arrow,
        size: 18,
      ),
      label: Text(isConnected ? 'Stop Demo' : 'Start Demo'),
      style: FilledButton.styleFrom(
        backgroundColor: isConnected ? Colors.red : Colors.green,
        foregroundColor: Colors.white,
      ),
    );
  }

  Widget _buildGaugeCard(
    BuildContext context, {
    required String title,
    required String value,
    required String unit,
    required IconData icon,
    required Color color,
  }) {
    final theme = Theme.of(context);
    
    return Card(
      elevation: 0,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(12),
        side: BorderSide(
          color: theme.colorScheme.outlineVariant,
          width: 1,
        ),
      ),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Container(
              padding: const EdgeInsets.all(8),
              decoration: BoxDecoration(
                color: color.withOpacity(0.1),
                borderRadius: BorderRadius.circular(8),
              ),
              child: Icon(
                icon,
                color: color,
                size: 24,
              ),
            ),
            const SizedBox(height: 12),
            Text(
              title,
              style: theme.textTheme.bodySmall?.copyWith(
                color: theme.colorScheme.onSurfaceVariant,
              ),
            ),
            const SizedBox(height: 4),
            RichText(
              text: TextSpan(
                text: value,
                style: theme.textTheme.headlineSmall?.copyWith(
                  color: theme.colorScheme.onSurface,
                  fontWeight: FontWeight.w600,
                ),
                children: [
                  TextSpan(
                    text: ' $unit',
                    style: theme.textTheme.bodyMedium?.copyWith(
                      color: theme.colorScheme.onSurfaceVariant,
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildStatusPanel(BuildContext context, ECUService ecuService) {
    final theme = Theme.of(context);
    final isConnected = ecuService.connectionState == ECUConnectionState.connected;
    
    return Card(
      elevation: 0,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(12),
        side: BorderSide(
          color: theme.colorScheme.outlineVariant,
          width: 1,
        ),
      ),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(
                  Icons.info_outline,
                  color: theme.colorScheme.primary,
                  size: 20,
                ),
                const SizedBox(width: 8),
                Text(
                  'System Status',
                  style: theme.textTheme.titleMedium?.copyWith(
                    fontWeight: FontWeight.w500,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            _buildStatusItem(
              context,
              'ECU Connection',
              isConnected ? 'Connected (Demo)' : 'Disconnected',
              isConnected ? Icons.link : Icons.link_off,
              isConnected ? Colors.green : Colors.red,
            ),
            _buildStatusItem(
              context,
              'Data Stream',
              isConnected ? 'Active (${ecuService.statistics.packetsPerSecond}/s)' : 'No Data',
              Icons.stream,
              isConnected ? Colors.green : Colors.grey,
            ),
            _buildStatusItem(
              context,
              'Engine State',
              isConnected ? 'Running' : 'Stopped',
              Icons.power_settings_new,
              isConnected ? Colors.green : Colors.grey,
            ),
            if (isConnected) ...[
              const SizedBox(height: 12),
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Colors.blue.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Row(
                  children: [
                    const Icon(Icons.info, color: Colors.blue, size: 16),
                    const SizedBox(width: 8),
                    Expanded(
                      child: Text(
                        'Demo mode is active with simulated car data. Values update in real-time!',
                        style: theme.textTheme.bodySmall?.copyWith(
                          color: Colors.blue,
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ],
        ),
      ),
    );
  }

  Widget _buildStatusItem(
    BuildContext context,
    String label,
    String status,
    IconData icon,
    Color color,
  ) {
    final theme = Theme.of(context);
    
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8),
      child: Row(
        children: [
          Icon(icon, color: color, size: 18),
          const SizedBox(width: 12),
          Expanded(
            child: Text(
              label,
              style: theme.textTheme.bodyMedium,
            ),
          ),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
            decoration: BoxDecoration(
              color: color.withOpacity(0.1),
              borderRadius: BorderRadius.circular(12),
            ),
            child: Text(
              status,
              style: theme.textTheme.bodySmall?.copyWith(
                color: color,
                fontWeight: FontWeight.w500,
              ),
            ),
          ),
        ],
      ),
    );
  }
}