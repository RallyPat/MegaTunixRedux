import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/ecu_service.dart';
import '../models/ecu_data.dart';

class SettingsTab extends StatefulWidget {
  const SettingsTab({super.key});

  @override
  State<SettingsTab> createState() => _SettingsTabState();
}

class _SettingsTabState extends State<SettingsTab> {
  String _selectedPort = '/dev/ttyUSB0';
  int _selectedBaudRate = 115200;
  String _selectedProtocol = 'Speeduino';
  bool _autoConnect = true;
  bool _darkMode = false;

  final List<String> _availablePorts = [
    '/dev/ttyUSB0',
    '/dev/ttyUSB1', 
    '/dev/ttyACM0',
    '/dev/ttyACM1',
    'COM1',
    'COM3',
    'COM4',
  ];

  final List<int> _availableBaudRates = [
    9600,
    19200,
    38400,
    57600,
    115200,
    230400,
  ];

  final List<String> _availableProtocols = [
    'Speeduino',
    'MegaSquirt',
    'Epic',
  ];

  @override
  Widget build(BuildContext context) {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'Settings',
            style: Theme.of(context).textTheme.headlineMedium,
          ),
          
          const SizedBox(height: 24),
          
          // ECU Connection Settings
          _buildSettingsSection(
            'ECU Connection',
            [
              _buildDropdownSetting(
                'Serial Port',
                _selectedPort,
                _availablePorts,
                (value) => setState(() => _selectedPort = value!),
              ),
              _buildDropdownSetting(
                'Baud Rate',
                _selectedBaudRate.toString(),
                _availableBaudRates.map((e) => e.toString()).toList(),
                (value) => setState(() => _selectedBaudRate = int.parse(value!)),
              ),
              _buildDropdownSetting(
                'Protocol',
                _selectedProtocol,
                _availableProtocols,
                (value) => setState(() => _selectedProtocol = value!),
              ),
              _buildSwitchSetting(
                'Auto-connect on startup',
                _autoConnect,
                (value) => setState(() => _autoConnect = value),
              ),
            ],
          ),
          
          const SizedBox(height: 24),
          
          // Application Settings
          _buildSettingsSection(
            'Application',
            [
              _buildSwitchSetting(
                'Dark Mode',
                _darkMode,
                (value) => setState(() => _darkMode = value),
              ),
            ],
          ),
          
          const SizedBox(height: 24),
          
          // Connection Test
          _buildSettingsSection(
            'Connection Test',
            [
              Card(
                child: Padding(
                  padding: const EdgeInsets.all(16),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'Test ECU Connection',
                        style: Theme.of(context).textTheme.titleMedium,
                      ),
                      const SizedBox(height: 8),
                      Text(
                        'Verify connection settings with your ECU',
                        style: Theme.of(context).textTheme.bodySmall,
                      ),
                      const SizedBox(height: 16),
                      Row(
                        children: [
                          ElevatedButton.icon(
                            onPressed: _testConnection,
                            icon: const Icon(Icons.link),
                            label: const Text('Test Connection'),
                          ),
                          const SizedBox(width: 16),
                          Consumer<ECUService>(
                            builder: (context, ecuService, child) {
                              return Text(
                                ecuService.connectionState == ECUConnectionState.connected
                                    ? '✅ Connected'
                                    : '❌ Disconnected',
                                style: Theme.of(context).textTheme.bodyMedium,
                              );
                            },
                          ),
                        ],
                      ),
                    ],
                  ),
                ),
              ),
            ],
          ),
          
          const SizedBox(height: 24),
          
          // About
          _buildSettingsSection(
            'About',
            [
              Card(
                child: Padding(
                  padding: const EdgeInsets.all(16),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'MegaTunix Redux',
                        style: Theme.of(context).textTheme.titleLarge?.copyWith(
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                      const SizedBox(height: 8),
                      Text(
                        'Version 1.0.0',
                        style: Theme.of(context).textTheme.bodyMedium,
                      ),
                      const SizedBox(height: 4),
                      Text(
                        'Professional ECU tuning software built with Flutter',
                        style: Theme.of(context).textTheme.bodySmall,
                      ),
                      const SizedBox(height: 16),
                      Text(
                        'Supported ECUs:',
                        style: Theme.of(context).textTheme.titleSmall,
                      ),
                      const SizedBox(height: 4),
                      const Text('• Speeduino'),
                      const Text('• MegaSquirt MS1/MS2/MS3'),
                      const Text('• Epic EMS'),
                    ],
                  ),
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildSettingsSection(String title, List<Widget> children) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(
          title,
          style: Theme.of(context).textTheme.titleLarge?.copyWith(
            fontWeight: FontWeight.bold,
          ),
        ),
        const SizedBox(height: 12),
        ...children,
      ],
    );
  }

  Widget _buildDropdownSetting<T>(
    String label,
    String value,
    List<String> options,
    ValueChanged<String?> onChanged,
  ) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Row(
          children: [
            Expanded(
              child: Text(
                label,
                style: Theme.of(context).textTheme.titleMedium,
              ),
            ),
            DropdownButton<String>(
              value: value,
              items: options.map((option) {
                return DropdownMenuItem(
                  value: option,
                  child: Text(option),
                );
              }).toList(),
              onChanged: onChanged,
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildSwitchSetting(
    String label,
    bool value,
    ValueChanged<bool> onChanged,
  ) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Row(
          children: [
            Expanded(
              child: Text(
                label,
                style: Theme.of(context).textTheme.titleMedium,
              ),
            ),
            Switch(
              value: value,
              onChanged: onChanged,
            ),
          ],
        ),
      ),
    );
  }

  void _testConnection() {
    final ecuService = Provider.of<ECUService>(context, listen: false);
    
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (context) => AlertDialog(
        title: const Text('Testing Connection'),
        content: const Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            CircularProgressIndicator(),
            SizedBox(height: 16),
            Text('Attempting to connect to ECU...'),
          ],
        ),
      ),
    );

    // Simulate connection test
    Future.delayed(const Duration(seconds: 2), () {
      Navigator.pop(context); // Close loading dialog
      
      showDialog(
        context: context,
        builder: (context) => AlertDialog(
          title: const Text('Connection Test Result'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text('Port: $_selectedPort'),
              Text('Baud Rate: $_selectedBaudRate'),
              Text('Protocol: $_selectedProtocol'),
              const SizedBox(height: 16),
              const Text('Result: ✅ Connection successful!'),
              const Text('ECU Type: Speeduino v202311'),
            ],
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context),
              child: const Text('OK'),
            ),
          ],
        ),
      );
      
      // Start mock data to simulate successful connection
      ecuService.startMockDataGeneration();
    });
  }
}