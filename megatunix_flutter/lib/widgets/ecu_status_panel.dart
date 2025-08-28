import 'package:flutter/material.dart';
import '../models/ecu_data.dart';
import '../services/ecu_service.dart';
import '../theme/ecu_theme.dart';

/// ECU Status Panel Widget
/// Shows ECU status and system information
class ECUStatusPanel extends StatefulWidget {
  const ECUStatusPanel({super.key});

  @override
  State<ECUStatusPanel> createState() => _ECUStatusPanelState();
}

class _ECUStatusPanelState extends State<ECUStatusPanel> {
  late final ECUService _ecuService;
  ECUStatistics? _statistics;

  @override
  void initState() {
    super.initState();
    _ecuService = ECUService();
    
    // Listen to statistics updates
    _ecuService.dataStream.listen((_) {
      if (mounted) {
        setState(() {
          _statistics = _ecuService.statistics;
        });
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'ECU Status',
              style: Theme.of(context).textTheme.headlineSmall,
            ),
            const SizedBox(height: 16),
            Row(
              children: [
                Expanded(
                  child: _buildStatusItem(
                    icon: Icons.check_circle,
                    label: 'System',
                    value: _ecuService.connectionState.toString().split('.').last.toUpperCase(),
                    color: _getSystemStatusColor(),
                  ),
                ),
                Expanded(
                  child: _buildStatusItem(
                    icon: Icons.speed,
                    label: 'Baud Rate',
                    value: '${_ecuService.baudRate}',
                    color: ECUTheme.getAccentColor('speed'),
                  ),
                ),
                Expanded(
                  child: _buildStatusItem(
                    icon: Icons.usb,
                    label: 'Port',
                    value: _ecuService.port.split('/').last,
                    color: ECUTheme.getAccentColor('port'),
                  ),
                ),
              ],
            ),
            if (_statistics != null) ...[
              const SizedBox(height: 16),
              const Divider(),
              const SizedBox(height: 16),
              Row(
                children: [
                  Expanded(
                    child: _buildStatusItem(
                      icon: Icons.download,
                      label: 'RX Rate',
                      value: '${_statistics!.receiveRate.toStringAsFixed(1)} B/s',
                      color: ECUTheme.getAccentColor('rx'),
                    ),
                  ),
                  Expanded(
                    child: _buildStatusItem(
                      icon: Icons.upload,
                      label: 'TX Rate',
                      value: '${_statistics!.transmitRate.toStringAsFixed(1)} B/s',
                      color: ECUTheme.getAccentColor('tx'),
                    ),
                  ),
                  Expanded(
                    child: _buildStatusItem(
                      icon: Icons.check_circle_outline,
                      label: 'Success Rate',
                      value: '${(_statistics!.successRate * 100).toStringAsFixed(1)}%',
                      color: ECUTheme.getAccentColor('success'),
                    ),
                  ),
                ],
              ),
            ],
          ],
        ),
      ),
    );
  }

  Widget _buildStatusItem({
    required IconData icon,
    required String label,
    required String value,
    required Color color,
  }) {
    return Column(
      children: [
        Icon(
          icon,
          color: color,
          size: 24,
        ),
        const SizedBox(height: 8),
        Text(
          label,
          style: const TextStyle(
            fontSize: 12,
            color: Colors.grey,
          ),
        ),
        Text(
          value,
          style: const TextStyle(
            fontSize: 16,
            fontWeight: FontWeight.bold,
          ),
        ),
      ],
    );
  }
  
  /// Get system status color based on connection state
  Color _getSystemStatusColor() {
    switch (_ecuService.connectionState) {
      case ECUConnectionState.connected:
        return Colors.green;
      case ECUConnectionState.connecting:
        return Colors.orange;
      case ECUConnectionState.disconnected:
        return Colors.grey;
      case ECUConnectionState.error:
        return Colors.red;
    }
  }
}

