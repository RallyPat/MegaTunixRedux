import 'package:flutter/material.dart';
import '../theme/ecu_theme.dart';
import '../services/ecu_service.dart';
import '../models/ecu_data.dart';

/// ECU Connection Panel Widget
/// Shows connection status and provides connection management
class ECUConnectionPanel extends StatefulWidget {
  const ECUConnectionPanel({super.key});

  @override
  State<ECUConnectionPanel> createState() => _ECUConnectionPanelState();
}

class _ECUConnectionPanelState extends State<ECUConnectionPanel>
    with TickerProviderStateMixin {
  late final ECUService _ecuService;
  late AnimationController _pulseController;
  late Animation<double> _pulseAnimation;
  
  // Connection state
  ECUConnectionState _connectionState = ECUConnectionState.disconnected;
  String _ecuType = 'None';
  String _port = 'None';
  String _version = 'Unknown';
  
  // Statistics
  ECUStatistics? _statistics;

  @override
  void initState() {
    super.initState();
    
    // Initialize ECU service
    _ecuService = ECUService();
    
    // Initialize pulse animation for connection indicator
    _pulseController = AnimationController(
      duration: const Duration(seconds: 2),
      vsync: this,
    );
    
    _pulseAnimation = Tween<double>(
      begin: 0.8,
      end: 1.0,
    ).animate(CurvedAnimation(
      parent: _pulseController,
      curve: Curves.easeInOut,
    ));
    
    // Listen to connection state changes
    _ecuService.connectionStream.listen((state) {
      if (mounted) {
        setState(() {
          _connectionState = state;
          _updateConnectionInfo();
        });
      }
    });
    
    // Listen to error stream
    _ecuService.errorStream.listen((error) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('ECU Error: ${error.message}'),
            backgroundColor: Colors.red,
          ),
        );
      }
    });
    
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
  void dispose() {
    _pulseController.dispose();
    super.dispose();
  }

  void _toggleConnection() async {
    if (_connectionState == ECUConnectionState.connected) {
      await _ecuService.disconnect();
    } else {
      await _ecuService.connect();
    }
  }
  
  void _updateConnectionInfo() {
    switch (_connectionState) {
      case ECUConnectionState.connected:
        _ecuType = 'Speeduino';
        _port = _ecuService.port;
        _version = 'v1.0'; // TODO: Get actual version from ECU
        _pulseController.repeat(reverse: true);
        break;
      case ECUConnectionState.connecting:
        _ecuType = 'Connecting...';
        _port = _ecuService.port;
        _version = 'Unknown';
        break;
      case ECUConnectionState.disconnected:
        _ecuType = 'None';
        _port = 'None';
        _version = 'Unknown';
        _pulseController.stop();
        break;
      case ECUConnectionState.error:
        _ecuType = 'Error';
        _port = 'None';
        _version = 'Unknown';
        _pulseController.stop();
        break;
    }
  }

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                // Connection status indicator
                AnimatedBuilder(
                  animation: _pulseAnimation,
                  builder: (context, child) {
                    return Container(
                      width: 16,
                      height: 16,
                      decoration: BoxDecoration(
                        shape: BoxShape.circle,
                                                color: _connectionState == ECUConnectionState.connected
                            ? ECUTheme.getAccentColor('afr')
                            : Colors.grey,
                        boxShadow: _connectionState == ECUConnectionState.connected ? [
                          BoxShadow(
                            color: ECUTheme.getAccentColor('afr').withOpacity(0.3),
                            blurRadius: 8 * _pulseAnimation.value,
                            spreadRadius: 2 * _pulseAnimation.value,
                          ),
                        ] : null,
                      ),
                    );
                  },
                ),
                
                const SizedBox(width: 12),
                
                // Connection status text
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'ECU Connection',
                        style: Theme.of(context).textTheme.headlineSmall,
                      ),
                      Text(
                        _getConnectionStatusText(),
                        style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                          color: _connectionState == ECUConnectionState.connected
                              ? ECUTheme.getAccentColor('afr')
                              : Colors.grey,
                          fontWeight: FontWeight.w500,
                        ),
                      ),
                    ],
                  ),
                ),
                
                // Connection toggle button
                ElevatedButton(
                  onPressed: _connectionState == ECUConnectionState.connecting ? null : _toggleConnection,
                  style: ElevatedButton.styleFrom(
                    backgroundColor: _getConnectionButtonColor(),
                    foregroundColor: Colors.white,
                  ),
                  child: Text(_getConnectionButtonText()),
                ),
              ],
            ),
            
            if (_connectionState == ECUConnectionState.connected) ...[
              const SizedBox(height: 20),
              const Divider(),
              const SizedBox(height: 16),
              
              // Connection details
              Row(
                children: [
                  Expanded(
                    child: _buildConnectionDetail(
                      icon: Icons.memory,
                      label: 'ECU Type',
                      value: _ecuType,
                    ),
                  ),
                  Expanded(
                    child: _buildConnectionDetail(
                      icon: Icons.usb,
                      label: 'Port',
                      value: _port,
                    ),
                  ),
                  Expanded(
                    child: _buildConnectionDetail(
                      icon: Icons.info,
                      label: 'Version',
                      value: _version,
                    ),
                  ),
                ],
              ),
              
              const SizedBox(height: 16),
              
              // Connection quality indicator
              Row(
                children: [
                  Icon(
                    Icons.signal_cellular_alt,
                    color: ECUTheme.getAccentColor('afr'),
                    size: 20,
                  ),
                  const SizedBox(width: 8),
                  Text(
                    'Connection Quality: Excellent',
                    style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                      color: ECUTheme.getAccentColor('afr'),
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

  /// Build connection detail item
  Widget _buildConnectionDetail({
    required IconData icon,
    required String label,
    required String value,
  }) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            Icon(
              icon,
              color: Colors.grey[600],
              size: 18,
            ),
            const SizedBox(width: 8),
            Text(
              label,
              style: TextStyle(
                fontSize: 12,
                color: Colors.grey[600],
              ),
            ),
          ],
        ),
        const SizedBox(height: 4),
        Text(
          value,
          style: const TextStyle(
            fontSize: 16,
            fontWeight: FontWeight.w600,
          ),
        ),
      ],
    );
  }
  
  /// Get connection status text based on current state
  String _getConnectionStatusText() {
    switch (_connectionState) {
      case ECUConnectionState.connected:
        return 'Connected';
      case ECUConnectionState.connecting:
        return 'Connecting...';
      case ECUConnectionState.disconnected:
        return 'Disconnected';
      case ECUConnectionState.error:
        return 'Error';
    }
  }
  
  /// Get connection button text based on current state
  String _getConnectionButtonText() {
    switch (_connectionState) {
      case ECUConnectionState.connected:
        return 'Disconnect';
      case ECUConnectionState.connecting:
        return 'Connecting...';
      case ECUConnectionState.disconnected:
        return 'Connect';
      case ECUConnectionState.error:
        return 'Retry';
    }
  }
  
  /// Get connection button color based on current state
  Color _getConnectionButtonColor() {
    switch (_connectionState) {
      case ECUConnectionState.connected:
        return Colors.red;
      case ECUConnectionState.connecting:
        return Colors.orange;
      case ECUConnectionState.disconnected:
        return ECUTheme.getAccentColor('afr');
      case ECUConnectionState.error:
        return Colors.red;
    }
  }
}
