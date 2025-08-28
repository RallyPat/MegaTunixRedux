/// Data Logging Dashboard
/// Professional interface for ECU data logging with real-time charts,
/// configuration controls, and session management

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import '../services/data_logging_service.dart';
import '../services/ecu_service.dart';
import '../widgets/realtime_data_chart.dart';

/// Professional data logging dashboard
class DataLoggingDashboard extends StatefulWidget {
  const DataLoggingDashboard({super.key});

  @override
  State<DataLoggingDashboard> createState() => _DataLoggingDashboardState();
}

class _DataLoggingDashboardState extends State<DataLoggingDashboard> {
  final TextEditingController _customFileNameController = TextEditingController();
  final TextEditingController _markerDescriptionController = TextEditingController();
  
  List<String> _selectedColumns = ['rpm', 'map', 'tps', 'afr'];
  List<String> _selectedChartParameters = ['rpm', 'map', 'afr'];
  int _selectedSampleRate = 10;
  String _selectedPreset = 'standard';
  bool _showAdvancedOptions = false;

  @override
  void initState() {
    super.initState();
    _initializeKeyboardShortcuts();
  }

  void _initializeKeyboardShortcuts() {
    // Keyboard shortcuts are handled by GlobalShortcutsService
    // This ensures F5/F6/F7 work from this screen
  }

  @override
  Widget build(BuildContext context) {
    return Consumer2<DataLoggingService, ECUService>(
      builder: (context, loggingService, ecuService, child) {
        return Scaffold(
          appBar: AppBar(
            title: const Text('Data Logging'),
            actions: [
              _buildConnectionStatus(ecuService),
            ],
          ),
          body: SingleChildScrollView(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                // Quick Actions Card
                _buildQuickActionsCard(loggingService, ecuService),
                const SizedBox(height: 16),
                
                // Real-time Chart
                RealtimeDataChart(
                  selectedParameters: _selectedChartParameters,
                  historyDuration: const Duration(minutes: 2),
                  height: 300,
                ),
                const SizedBox(height: 16),

                Row(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    // Configuration Panel
                    Expanded(
                      flex: 1,
                      child: _buildConfigurationPanel(loggingService),
                    ),
                    const SizedBox(width: 16),
                    
                    // Session Info & Statistics
                    Expanded(
                      flex: 1,
                      child: _buildSessionPanel(loggingService),
                    ),
                  ],
                ),
                const SizedBox(height: 16),

                // Recent Logs
                _buildRecentLogsPanel(loggingService),
              ],
            ),
          ),
        );
      },
    );
  }

  Widget _buildConnectionStatus(ECUService ecuService) {
    final isConnected = ecuService.connectionState == ECUConnectionState.connected;
    return Container(
      margin: const EdgeInsets.only(right: 16),
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      decoration: BoxDecoration(
        color: isConnected ? Colors.green.withOpacity(0.2) : Colors.red.withOpacity(0.2),
        borderRadius: BorderRadius.circular(12),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(
            isConnected ? Icons.wifi : Icons.wifi_off,
            size: 16,
            color: isConnected ? Colors.green : Colors.red,
          ),
          const SizedBox(width: 4),
          Text(
            isConnected ? 'Connected' : 'Disconnected',
            style: TextStyle(
              color: isConnected ? Colors.green : Colors.red,
              fontSize: 12,
              fontWeight: FontWeight.bold,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildQuickActionsCard(DataLoggingService loggingService, ECUService ecuService) {
    final isLogging = loggingService.isLogging;
    final isConnected = ecuService.connectionState == ECUConnectionState.connected;

    return Card(
      elevation: 4,
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Quick Actions',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(height: 16),

            Row(
              children: [
                // Start/Stop Logging
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: isConnected
                        ? (isLogging ? _stopLogging : _startLogging)
                        : null,
                    icon: Icon(isLogging ? Icons.stop : Icons.play_arrow),
                    label: Text(isLogging ? 'Stop Logging (F6)' : 'Start Logging (F5)'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: isLogging ? Colors.red : Colors.green,
                      foregroundColor: Colors.white,
                      padding: const EdgeInsets.symmetric(vertical: 12),
                    ),
                  ),
                ),
                const SizedBox(width: 8),

                // Add Marker
                ElevatedButton.icon(
                  onPressed: isLogging ? _addMarker : null,
                  icon: const Icon(Icons.flag),
                  label: const Text('Mark (F7)'),
                  style: ElevatedButton.styleFrom(
                    padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
                  ),
                ),
              ],
            ),

            if (isLogging) ...[
              const SizedBox(height: 16),
              _buildLoggingStatus(loggingService),
            ],

            if (!isConnected) ...[
              const SizedBox(height: 16),
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Colors.orange.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(8),
                  border: Border.all(color: Colors.orange.withOpacity(0.3)),
                ),
                child: Row(
                  children: [
                    Icon(Icons.warning, color: Colors.orange),
                    const SizedBox(width: 8),
                    Expanded(
                      child: Text(
                        'ECU not connected. Connect to ECU first to start logging.',
                        style: TextStyle(color: Colors.orange.shade700),
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

  Widget _buildLoggingStatus(DataLoggingService loggingService) {
    final duration = loggingService.logStartTime != null
        ? DateTime.now().difference(loggingService.logStartTime!)
        : Duration.zero;

    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Colors.green.withOpacity(0.1),
        borderRadius: BorderRadius.circular(8),
      ),
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                'Recording',
                style: TextStyle(
                  color: Colors.green.shade700,
                  fontWeight: FontWeight.bold,
                ),
              ),
              Row(
                children: [
                  Icon(
                    Icons.fiber_manual_record,
                    color: Colors.red,
                    size: 16,
                  ),
                  const SizedBox(width: 4),
                  Text(
                    _formatDuration(duration),
                    style: TextStyle(
                      color: Colors.green.shade700,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ],
              ),
            ],
          ),
          const SizedBox(height: 8),
          LinearProgressIndicator(
            backgroundColor: Colors.green.withOpacity(0.2),
            valueColor: AlwaysStoppedAnimation<Color>(Colors.green),
          ),
          const SizedBox(height: 8),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                'Data Points: ${loggingService.logEntryCount}',
                style: Theme.of(context).textTheme.bodySmall,
              ),
              Text(
                'Markers: ${loggingService.markers.length}',
                style: Theme.of(context).textTheme.bodySmall,
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildConfigurationPanel(DataLoggingService loggingService) {
    return Card(
      elevation: 2,
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Configuration',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(height: 16),

            // Preset Selection
            DropdownButtonFormField<String>(
              value: _selectedPreset,
              decoration: const InputDecoration(
                labelText: 'Preset Configuration',
                border: OutlineInputBorder(),
              ),
              items: const [
                DropdownMenuItem(value: 'basic', child: Text('Basic (5Hz)')),
                DropdownMenuItem(value: 'standard', child: Text('Standard (10Hz)')),
                DropdownMenuItem(value: 'professional', child: Text('Professional (20Hz)')),
                DropdownMenuItem(value: 'high_speed', child: Text('High Speed (50Hz)')),
                DropdownMenuItem(value: 'custom', child: Text('Custom')),
              ],
              onChanged: loggingService.isLogging ? null : (value) {
                setState(() {
                  _selectedPreset = value!;
                  _applyPreset(value);
                });
              },
            ),
            const SizedBox(height: 16),

            // Sample Rate
            Row(
              children: [
                Expanded(
                  child: Text('Sample Rate: ${_selectedSampleRate}Hz'),
                ),
                Expanded(
                  flex: 2,
                  child: Slider(
                    value: _selectedSampleRate.toDouble(),
                    min: 1,
                    max: 50,
                    divisions: 49,
                    label: '${_selectedSampleRate}Hz',
                    onChanged: loggingService.isLogging ? null : (value) {
                      setState(() {
                        _selectedSampleRate = value.round();
                        _selectedPreset = 'custom';
                      });
                    },
                  ),
                ),
              ],
            ),

            // Chart Parameters
            const SizedBox(height: 16),
            Text(
              'Chart Parameters:',
              style: Theme.of(context).textTheme.titleSmall,
            ),
            const SizedBox(height: 8),
            Wrap(
              spacing: 8,
              runSpacing: 8,
              children: ['rpm', 'map', 'tps', 'afr', 'coolant_temp', 'timing'].map((param) {
                return FilterChip(
                  label: Text(param.toUpperCase()),
                  selected: _selectedChartParameters.contains(param),
                  onSelected: (selected) {
                    setState(() {
                      if (selected) {
                        _selectedChartParameters.add(param);
                      } else {
                        _selectedChartParameters.remove(param);
                      }
                    });
                  },
                );
              }).toList(),
            ),

            // Advanced Options Toggle
            const SizedBox(height: 16),
            TextButton.icon(
              onPressed: () {
                setState(() {
                  _showAdvancedOptions = !_showAdvancedOptions;
                });
              },
              icon: Icon(_showAdvancedOptions ? Icons.expand_less : Icons.expand_more),
              label: const Text('Advanced Options'),
            ),

            if (_showAdvancedOptions) ...[
              const Divider(),
              _buildAdvancedOptions(),
            ],
          ],
        ),
      ),
    );
  }

  Widget _buildAdvancedOptions() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        // Custom filename
        TextField(
          controller: _customFileNameController,
          decoration: const InputDecoration(
            labelText: 'Custom Filename (optional)',
            hintText: 'Leave empty for auto-generated name',
            border: OutlineInputBorder(),
          ),
        ),
        const SizedBox(height: 12),

        // Logging Columns Selection
        Text(
          'Logging Columns:',
          style: Theme.of(context).textTheme.titleSmall,
        ),
        const SizedBox(height: 8),
        Wrap(
          spacing: 8,
          runSpacing: 8,
          children: DataLoggingService.getAvailableColumns().map((column) {
            return FilterChip(
              label: Text(column.replaceAll('_', ' ').toUpperCase()),
              selected: _selectedColumns.contains(column),
              onSelected: (selected) {
                setState(() {
                  if (selected) {
                    _selectedColumns.add(column);
                  } else {
                    _selectedColumns.remove(column);
                  }
                });
              },
            );
          }).toList(),
        ),
      ],
    );
  }

  Widget _buildSessionPanel(DataLoggingService loggingService) {
    return Card(
      elevation: 2,
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Session Statistics',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(height: 16),

            if (loggingService.isLogging) ...[
              _buildStatRow('RPM Range', '${loggingService.minRpm.toStringAsFixed(0)} - ${loggingService.maxRpm.toStringAsFixed(0)}'),
              _buildStatRow('AFR Range', '${loggingService.minAfr.toStringAsFixed(2)} - ${loggingService.maxAfr.toStringAsFixed(2)}'),
              _buildStatRow('Avg RPM', loggingService.avgRpm.toStringAsFixed(0)),
              _buildStatRow('Avg AFR', loggingService.avgAfr.toStringAsFixed(2)),
              _buildStatRow('Data Points', loggingService.logEntryCount.toString()),
              _buildStatRow('Markers', loggingService.markers.length.toString()),
            ] else ...[
              Center(
                child: Column(
                  children: [
                    Icon(Icons.analytics, size: 48, color: Colors.grey),
                    const SizedBox(height: 8),
                    Text(
                      'No active session',
                      style: Theme.of(context).textTheme.bodyLarge?.copyWith(
                        color: Colors.grey,
                      ),
                    ),
                    Text(
                      'Start logging to see statistics',
                      style: Theme.of(context).textTheme.bodySmall?.copyWith(
                        color: Colors.grey,
                      ),
                    ),
                  ],
                ),
              ),
            ],

            if (loggingService.isLogging) ...[
              const SizedBox(height: 16),
              const Divider(),
              const SizedBox(height: 16),

              // Quick Marker Entry
              TextField(
                controller: _markerDescriptionController,
                decoration: InputDecoration(
                  labelText: 'Quick Marker',
                  hintText: 'Enter marker description',
                  border: const OutlineInputBorder(),
                  suffixIcon: IconButton(
                    icon: const Icon(Icons.send),
                    onPressed: () {
                      _addCustomMarker(_markerDescriptionController.text);
                      _markerDescriptionController.clear();
                    },
                  ),
                ),
                onSubmitted: (value) {
                  _addCustomMarker(value);
                  _markerDescriptionController.clear();
                },
              ),
            ],
          ],
        ),
      ),
    );
  }

  Widget _buildStatRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(
            label,
            style: Theme.of(context).textTheme.bodyMedium,
          ),
          Text(
            value,
            style: Theme.of(context).textTheme.bodyMedium?.copyWith(
              fontWeight: FontWeight.bold,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildRecentLogsPanel(DataLoggingService loggingService) {
    return Card(
      elevation: 2,
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text(
                  'Recent Logs',
                  style: Theme.of(context).textTheme.titleMedium?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
                ),
                TextButton.icon(
                  onPressed: () => _openLogsDirectory(loggingService),
                  icon: const Icon(Icons.folder_open),
                  label: const Text('Open Folder'),
                ),
              ],
            ),
            const SizedBox(height: 16),

            FutureBuilder<List<LogFileInfo>>(
              future: loggingService.getRecentLogFiles(limit: 5),
              builder: (context, snapshot) {
                if (snapshot.connectionState == ConnectionState.waiting) {
                  return const Center(child: CircularProgressIndicator());
                }

                if (!snapshot.hasData || snapshot.data!.isEmpty) {
                  return Center(
                    child: Column(
                      children: [
                        Icon(Icons.description, size: 48, color: Colors.grey),
                        const SizedBox(height: 8),
                        Text(
                          'No log files found',
                          style: Theme.of(context).textTheme.bodyLarge?.copyWith(
                            color: Colors.grey,
                          ),
                        ),
                      ],
                    ),
                  );
                }

                return Column(
                  children: snapshot.data!.map((logInfo) {
                    return ListTile(
                      leading: const Icon(Icons.description),
                      title: Text(logInfo.fileName),
                      subtitle: Text(
                        '${logInfo.fileSizeFormatted} • ${logInfo.durationFormatted} • ${logInfo.sampleRate ?? \"Unknown rate\"}',
                      ),
                      trailing: PopupMenuButton<String>(
                        onSelected: (action) => _handleLogFileAction(action, logInfo),
                        itemBuilder: (context) => [
                          const PopupMenuItem(
                            value: 'open',
                            child: Row(
                              children: [
                                Icon(Icons.open_in_new),
                                SizedBox(width: 8),
                                Text('Open'),
                              ],
                            ),
                          ),
                          const PopupMenuItem(
                            value: 'analyze',
                            child: Row(
                              children: [
                                Icon(Icons.analytics),
                                SizedBox(width: 8),
                                Text('Analyze'),
                              ],
                            ),
                          ),
                          const PopupMenuItem(
                            value: 'delete',
                            child: Row(
                              children: [
                                Icon(Icons.delete, color: Colors.red),
                                SizedBox(width: 8),
                                Text('Delete', style: TextStyle(color: Colors.red)),
                              ],
                            ),
                          ),
                        ],
                      ),
                    );
                  }).toList(),
                );
              },
            ),
          ],
        ),
      ),
    );
  }

  void _applyPreset(String preset) {
    final config = DataLoggingService.createPresetConfig(preset);
    setState(() {
      _selectedSampleRate = config.sampleRateHz;
      _selectedColumns = config.enabledColumns.toList();
      
      // Update chart parameters based on preset
      switch (preset) {
        case 'basic':
          _selectedChartParameters = ['rpm', 'map'];
          break;
        case 'standard':
          _selectedChartParameters = ['rpm', 'map', 'afr'];
          break;
        case 'professional':
          _selectedChartParameters = ['rpm', 'map', 'afr', 'timing'];
          break;
        case 'high_speed':
          _selectedChartParameters = ['rpm', 'map', 'timing'];
          break;
      }
    });
  }

  Future<void> _startLogging() async {
    final loggingService = context.read<DataLoggingService>();
    
    // Apply current configuration
    final config = LoggingConfig(
      enabledColumns: _selectedColumns.toSet(),
      sampleRateHz: _selectedSampleRate,
      includeMetadata: true,
      enableRealTimeAnalysis: true,
      customFilePrefix: _selectedPreset == 'custom' ? 'custom_log' : _selectedPreset,
    );
    
    loggingService.updateConfig(config);
    
    final customFileName = _customFileNameController.text.trim();
    await loggingService.startLogging(
      customFileName: customFileName.isNotEmpty ? customFileName : null,
    );
    
    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('🎯 Data logging started at ${_selectedSampleRate}Hz'),
          backgroundColor: Colors.green,
        ),
      );
    }
  }

  Future<void> _stopLogging() async {
    final loggingService = context.read<DataLoggingService>();
    await loggingService.stopLogging();
    
    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('⏹️ Data logging stopped and saved'),
          backgroundColor: Colors.blue,
        ),
      );
    }
  }

  void _addMarker() {
    final loggingService = context.read<DataLoggingService>();
    loggingService.markLog();
    
    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('📌 Marker ${loggingService.markers.length} added'),
          duration: const Duration(seconds: 1),
        ),
      );
    }
  }

  void _addCustomMarker(String description) {
    if (description.isEmpty) return;
    
    final loggingService = context.read<DataLoggingService>();
    loggingService.markLog(description: description);
    
    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('📌 Marker added: $description'),
          duration: const Duration(seconds: 1),
        ),
      );
    }
  }

  void _openLogsDirectory(DataLoggingService loggingService) async {
    // This would open the logs directory in the system file manager
    // Implementation depends on platform
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text('Logs saved to: ${loggingService.logDirectory}'),
      ),
    );
  }

  void _handleLogFileAction(String action, LogFileInfo logInfo) {
    switch (action) {
      case 'open':
        // Open log file in system default application
        break;
      case 'analyze':
        // Open log analysis view
        break;
      case 'delete':
        // Delete log file
        _showDeleteConfirmation(logInfo);
        break;
    }
  }

  void _showDeleteConfirmation(LogFileInfo logInfo) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Delete Log File'),
        content: Text('Are you sure you want to delete ${logInfo.fileName}?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('Cancel'),
          ),
          TextButton(
            onPressed: () {
              Navigator.of(context).pop();
              _deleteLogFile(logInfo);
            },
            child: const Text('Delete', style: TextStyle(color: Colors.red)),
          ),
        ],
      ),
    );
  }

  Future<void> _deleteLogFile(LogFileInfo logInfo) async {
    try {
      await logInfo.file.delete();
      setState(() {}); // Refresh the list
      
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('🗑️ Deleted ${logInfo.fileName}'),
          ),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('Error deleting file: $e'),
            backgroundColor: Colors.red,
          ),
        );
      }
    }
  }

  String _formatDuration(Duration duration) {
    final minutes = duration.inMinutes;
    final seconds = duration.inSeconds % 60;
    return '${minutes.toString().padLeft(2, '0')}:${seconds.toString().padLeft(2, '0')}';
  }

  @override
  void dispose() {
    _customFileNameController.dispose();
    _markerDescriptionController.dispose();
    super.dispose();
  }
}