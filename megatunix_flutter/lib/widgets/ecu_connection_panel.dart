import 'package:flutter/material.dart';
import '../theme/ecu_theme.dart';
import '../services/ecu_service.dart';
import '../models/ecu_data.dart';

/// ECU Connection Panel Widget
/// Shows connection status, protocol selection, and provides connection management
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
  
  // Protocol selection
  ECUProtocol _selectedProtocol = ECUProtocol.speeduino;
  bool _autoDetectProtocol = true;
  bool _showAdvanced = false;
  
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
    
    // Initialize state from service
    _selectedProtocol = _ecuService.selectedProtocol;
    _autoDetectProtocol = _ecuService.autoDetectProtocol;
    _port = _ecuService.port;
    
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
            duration: const Duration(seconds: 4),
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
      // Use current protocol selection for connection
      await _ecuService.connect(
        port: _port,
        protocol: _autoDetectProtocol ? null : _selectedProtocol,
      );
    }
  }
  
  void _updateConnectionInfo() async {
    switch (_connectionState) {
      case ECUConnectionState.connected:
        _ecuType = _ecuService.protocolName;
        _port = _ecuService.port;
        _pulseController.repeat(reverse: true);
        
        // Get version info from ECU
        try {
          _version = await _ecuService.getVersion();
        } catch (e) {
          _version = 'Unknown';
        }
        
        if (mounted) {
          setState(() {});
        }
        break;
      case ECUConnectionState.connecting:
        _ecuType = 'Detecting...';
        _port = _ecuService.port;
        _version = 'Unknown';
        break;
      case ECUConnectionState.disconnected:
        _ecuType = 'None';
        _port = _ecuService.port;
        _version = 'Unknown';
        _pulseController.stop();
        break;
      case ECUConnectionState.error:
        _ecuType = 'Error';
        _port = _ecuService.port;
        _version = 'Unknown';
        _pulseController.stop();
        break;
    }
  }

  void _onProtocolChanged(ECUProtocol? protocol) {
    if (protocol != null && protocol != _selectedProtocol) {
      setState(() {
        _selectedProtocol = protocol;
        _autoDetectProtocol = false; // Manual selection disables auto-detect
      });
      _ecuService.setProtocol(protocol);
      _ecuService.setAutoDetectProtocol(false);
    }
  }
  
  void _onAutoDetectChanged(bool? enabled) {
    if (enabled != null) {
      setState(() {
        _autoDetectProtocol = enabled;
      });
      _ecuService.setAutoDetectProtocol(enabled);
    }
  }
  
  void _onPortChanged(String port) {
    setState(() {
      _port = port;
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
            // Connection status header
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
                        color: _getConnectionIndicatorColor(),
                        boxShadow: _connectionState == ECUConnectionState.connected ? [
                          BoxShadow(
                            color: _getConnectionIndicatorColor().withOpacity(0.3),
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
                          color: _getConnectionIndicatorColor(),
                          fontWeight: FontWeight.w500,
                        ),
                      ),
                    ],
                  ),
                ),
                
                // Advanced settings toggle
                IconButton(
                  icon: Icon(_showAdvanced ? Icons.expand_less : Icons.expand_more),
                  tooltip: 'Advanced Settings',
                  onPressed: () {
                    setState(() {
                      _showAdvanced = !_showAdvanced;
                    });
                  },
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
            
            // Advanced configuration panel
            AnimatedCrossFade(
              firstChild: const SizedBox.shrink(),
              secondChild: Column(
                children: [
                  const SizedBox(height: 16),
                  const Divider(),
                  const SizedBox(height: 16),
                  
                  // Protocol selection
                  Row(
                    children: [
                      Expanded(
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Text(
                              'ECU Protocol',
                              style: Theme.of(context).textTheme.titleSmall,
                            ),
                            const SizedBox(height: 8),
                            
                            // Auto-detect checkbox
                            CheckboxListTile(
                              contentPadding: EdgeInsets.zero,
                              dense: true,
                              title: const Text('Auto-detect protocol'),
                              value: _autoDetectProtocol,
                              onChanged: _connectionState == ECUConnectionState.connected ? null : _onAutoDetectChanged,
                            ),
                            
                            // Manual protocol selection
                            if (!_autoDetectProtocol) ...[
                              const SizedBox(height: 8),
                              DropdownButtonFormField<ECUProtocol>(
                                value: _selectedProtocol,
                                decoration: const InputDecoration(
                                  labelText: 'Select Protocol',
                                  border: OutlineInputBorder(),
                                ),
                                items: _ecuService.supportedProtocols.map((protocol) {
                                  return DropdownMenuItem<ECUProtocol>(
                                    value: protocol,
                                    child: Row(
                                      children: [
                                        Icon(_getProtocolIcon(protocol), size: 20),
                                        const SizedBox(width: 8),
                                        Text(_ecuService.protocolNames[protocol] ?? 'Unknown'),
                                      ],
                                    ),
                                  );
                                }).toList(),
                                onChanged: _connectionState == ECUConnectionState.connected ? null : _onProtocolChanged,
                              ),
                            ],
                          ],
                        ),
                      ),
                      
                      const SizedBox(width: 20),
                      
                      // Connection settings
                      Expanded(
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Text(
                              'Connection Settings',
                              style: Theme.of(context).textTheme.titleSmall,
                            ),
                            const SizedBox(height: 12),
                            
                            // Port selection
                            TextFormField(
                              initialValue: _port,
                              decoration: const InputDecoration(
                                labelText: 'Port',
                                border: OutlineInputBorder(),
                                prefixIcon: Icon(Icons.usb),
                              ),
                              onChanged: _connectionState == ECUConnectionState.connected ? null : _onPortChanged,
                              enabled: _connectionState != ECUConnectionState.connected,
                            ),
                            
                            const SizedBox(height: 12),
                            
                            // Baud rate selection
                            DropdownButtonFormField<int>(
                              value: _ecuService.baudRate,
                              decoration: const InputDecoration(
                                labelText: 'Baud Rate',
                                border: OutlineInputBorder(),
                                prefixIcon: Icon(Icons.speed),
                              ),
                              items: _ecuService.supportedBaudRates.map((rate) {
                                return DropdownMenuItem<int>(
                                  value: rate,
                                  child: Text('$rate bps'),
                                );
                              }).toList(),
                              onChanged: _connectionState == ECUConnectionState.connected ? null : (int? rate) {
                                // TODO: Update baud rate in service
                              },
                            ),
                          ],
                        ),
                      ),
                    ],
                  ),
                ],
              ),
              crossFadeState: _showAdvanced ? CrossFadeState.showSecond : CrossFadeState.showFirst,
              duration: const Duration(milliseconds: 300),
            ),
            
            // Connected state details
            if (_connectionState == ECUConnectionState.connected) ...[
              const SizedBox(height: 20),
              const Divider(),
              const SizedBox(height: 16),
              
              // Connection details
              Row(
                children: [
                  Expanded(
                    child: _buildConnectionDetail(
                      icon: _getProtocolIcon(_ecuService.detectedProtocol ?? _selectedProtocol),
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
              
              // Protocol detection result
              if (_ecuService.detectedProtocol != null && _autoDetectProtocol) ...[
                Container(
                  padding: const EdgeInsets.all(12),
                  decoration: BoxDecoration(
                    color: Colors.green.withOpacity(0.1),
                    border: Border.all(color: Colors.green.withOpacity(0.3)),
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Row(
                    children: [
                      Icon(
                        Icons.check_circle,
                        color: Colors.green,
                        size: 20,
                      ),
                      const SizedBox(width: 8),
                      Text(
                        'Auto-detected: ${_ecuService.protocolNames[_ecuService.detectedProtocol!]}',
                        style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                          color: Colors.green,
                          fontWeight: FontWeight.w500,
                        ),
                      ),
                    ],
                  ),
                ),
                const SizedBox(height: 12),
              ],
              
              // Connection quality indicator
              Row(
                children: [
                  Icon(
                    Icons.signal_cellular_alt,
                    color: _getConnectionQualityColor(),
                    size: 20,
                  ),
                  const SizedBox(width: 8),
                  Text(
                    _getConnectionQualityText(),
                    style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                      color: _getConnectionQualityColor(),
                    ),
                  ),
                  const Spacer(),
                  if (_statistics != null) ...[
                    Text(
                      '${_statistics!.packetsPerSecond.toStringAsFixed(1)} pps',
                      style: Theme.of(context).textTheme.bodySmall?.copyWith(
                        color: Colors.grey[600],
                      ),
                    ),
                  ],
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
  
  /// Get protocol icon
  IconData _getProtocolIcon(ECUProtocol protocol) {
    switch (protocol) {
      case ECUProtocol.speeduino:
        return Icons.flash_on;
      case ECUProtocol.megasquirt:
        return Icons.memory;
      case ECUProtocol.epicEFI:
        return Icons.developer_board;
      default:
        return Icons.device_unknown;
    }
  }
  
  /// Get connection indicator color
  Color _getConnectionIndicatorColor() {
    switch (_connectionState) {
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
  
  /// Get connection status text based on current state
  String _getConnectionStatusText() {
    switch (_connectionState) {
      case ECUConnectionState.connected:
        return 'Connected to $_ecuType';
      case ECUConnectionState.connecting:
        return _autoDetectProtocol ? 'Auto-detecting protocol...' : 'Connecting to ${_ecuService.protocolNames[_selectedProtocol]}...';
      case ECUConnectionState.disconnected:
        return 'Disconnected';
      case ECUConnectionState.error:
        return 'Connection Error';
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
        return Colors.green;
      case ECUConnectionState.error:
        return Colors.red;
    }
  }
  
  /// Get connection quality color based on statistics
  Color _getConnectionQualityColor() {
    if (_statistics == null) return Colors.grey;
    
    final successRate = _statistics!.successRate;
    if (successRate > 0.9) return Colors.green;
    if (successRate > 0.7) return Colors.orange;
    return Colors.red;
  }
  
  /// Get connection quality text based on statistics
  String _getConnectionQualityText() {
    if (_statistics == null) return 'Unknown Quality';
    
    final successRate = _statistics!.successRate;
    if (successRate > 0.95) return 'Excellent Quality';
    if (successRate > 0.9) return 'Good Quality';
    if (successRate > 0.7) return 'Fair Quality';
    return 'Poor Quality';
  }
}
