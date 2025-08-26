import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/ecu_service.dart';
import '../models/ecu_data.dart';

class DashboardTab extends StatefulWidget {
  const DashboardTab({super.key});

  @override
  State<DashboardTab> createState() => _DashboardTabState();
}

class _DashboardTabState extends State<DashboardTab> {
  @override
  void initState() {
    super.initState();
    // Start mock data for demonstration
    WidgetsBinding.instance.addPostFrameCallback((_) {
      final ecuService = Provider.of<ECUService>(context, listen: false);
      if (ecuService.connectionState == ECUConnectionState.disconnected) {
        ecuService.startMockDataGeneration();
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(16),
      child: Column(
        children: [
          // Connection Control
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Row(
                children: [
                  Consumer<ECUService>(
                    builder: (context, ecuService, child) {
                      return ElevatedButton.icon(
                        onPressed: ecuService.connectionState == ECUConnectionState.connected
                            ? () => ecuService.disconnect()
                            : () => ecuService.startMockDataGeneration(),
                        icon: Icon(
                          ecuService.connectionState == ECUConnectionState.connected
                              ? Icons.stop
                              : Icons.play_arrow,
                        ),
                        label: Text(
                          ecuService.connectionState == ECUConnectionState.connected
                              ? 'Disconnect'
                              : 'Connect Demo',
                        ),
                      );
                    },
                  ),
                  const SizedBox(width: 16),
                  Expanded(
                    child: Consumer<ECUService>(
                      builder: (context, ecuService, child) {
                        final status = ecuService.connectionState == ECUConnectionState.connected
                            ? 'Connected - Real-time data streaming'
                            : 'Disconnected - Click Connect Demo to see sample data';
                        return Text(
                          status,
                          style: Theme.of(context).textTheme.bodyMedium,
                        );
                      },
                    ),
                  ),
                ],
              ),
            ),
          ),
          
          const SizedBox(height: 16),
          
          // Real-time Gauges
          Expanded(
            child: Consumer<ECUService>(
              builder: (context, ecuService, child) {
                final data = ecuService.currentData ?? SpeeduinoData.defaultValues();
                
                return GridView.count(
                  crossAxisCount: 3,
                  childAspectRatio: 1.2,
                  crossAxisSpacing: 16,
                  mainAxisSpacing: 16,
                  children: [
                    _buildGauge('RPM', data.rpm.toStringAsFixed(0), 'rpm', 0, 8000),
                    _buildGauge('MAP', data.map.toStringAsFixed(1), 'kPa', 0, 300),
                    _buildGauge('TPS', data.tps.toStringAsFixed(1), '%', 0, 100),
                    _buildGauge('IAT', data.intakeTemp.toStringAsFixed(1), '°C', -20, 100),
                    _buildGauge('CLT', data.coolantTemp.toStringAsFixed(1), '°C', -20, 120),
                    _buildGauge('AFR', data.afr.toStringAsFixed(2), ':1', 10, 20),
                  ],
                );
              },
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildGauge(String label, String value, String unit, double min, double max) {
    final numValue = double.tryParse(value) ?? 0;
    final percentage = (numValue - min) / (max - min);
    
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Text(
              label,
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                fontWeight: FontWeight.bold,
              ),
            ),
            
            Expanded(
              child: Center(
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Text(
                      value,
                      style: Theme.of(context).textTheme.headlineMedium?.copyWith(
                        fontWeight: FontWeight.bold,
                        color: _getValueColor(percentage),
                      ),
                    ),
                    Text(
                      unit,
                      style: Theme.of(context).textTheme.bodySmall,
                    ),
                  ],
                ),
              ),
            ),
            
            // Simple bar indicator
            Container(
              height: 4,
              decoration: BoxDecoration(
                borderRadius: BorderRadius.circular(2),
                color: Colors.grey.shade300,
              ),
              child: FractionallySizedBox(
                alignment: Alignment.centerLeft,
                widthFactor: percentage.clamp(0.0, 1.0),
                child: Container(
                  decoration: BoxDecoration(
                    borderRadius: BorderRadius.circular(2),
                    color: _getValueColor(percentage),
                  ),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Color _getValueColor(double percentage) {
    if (percentage < 0.3) return Colors.green;
    if (percentage < 0.7) return Colors.orange;
    return Colors.red;
  }
}