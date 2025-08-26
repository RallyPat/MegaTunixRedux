import 'package:flutter/material.dart';
import '../theme/ecu_theme.dart';

/// Diagnostics Screen Widget
/// Provides ECU diagnostics and troubleshooting information
class DiagnosticsScreen extends StatefulWidget {
  const DiagnosticsScreen({super.key});

  @override
  State<DiagnosticsScreen> createState() => _DiagnosticsScreenState();
}

class _DiagnosticsScreenState extends State<DiagnosticsScreen> {
  bool _isRunningDiagnostics = false;
  List<Map<String, dynamic>> _diagnosticResults = [];

  @override
  void initState() {
    super.initState();
    _runInitialDiagnostics();
  }

  void _runInitialDiagnostics() {
    setState(() {
      _isRunningDiagnostics = true;
      _diagnosticResults = [];
    });

    // Simulate diagnostic checks
    Future.delayed(const Duration(seconds: 2), () {
      setState(() {
        _isRunningDiagnostics = false;
        _diagnosticResults = [
          {
            'category': 'ECU Communication',
            'status': 'Pass',
            'details': 'Serial connection established successfully',
            'color': Colors.green,
          },
          {
            'category': 'Sensor Readings',
            'status': 'Pass',
            'details': 'All sensors reporting valid data',
            'color': Colors.green,
          },
          {
            'category': 'Table Validation',
            'status': 'Warning',
            'details': 'VE table has 3 cells with extreme values',
            'color': Colors.orange,
          },
          {
            'category': 'Memory Usage',
            'status': 'Pass',
            'details': 'ECU memory usage: 67%',
            'color': Colors.green,
          },
          {
            'category': 'Error Codes',
            'status': 'Pass',
            'details': 'No active error codes',
            'color': Colors.green,
          },
        ];
      });
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Theme.of(context).colorScheme.surface,
      appBar: AppBar(
        title: const Text('ECU Diagnostics'),
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: _runInitialDiagnostics,
            tooltip: 'Run Diagnostics',
          ),
        ],
      ),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Status Overview
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'System Status',
                      style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                        color: ECUTheme.getAccentColor('primary'),
                      ),
                    ),
                    const SizedBox(height: 16),
                    Row(
                      children: [
                        Icon(
                          _isRunningDiagnostics ? Icons.sync : Icons.check_circle,
                          color: _isRunningDiagnostics 
                              ? ECUTheme.getAccentColor('primary')
                              : Colors.green,
                        ),
                        const SizedBox(width: 8),
                        Text(
                          _isRunningDiagnostics 
                              ? 'Running diagnostics...'
                              : 'System healthy',
                          style: Theme.of(context).textTheme.titleMedium,
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            ),
            
            const SizedBox(height: 16),
            
            // Diagnostic Results
            Expanded(
              child: _isRunningDiagnostics
                  ? const Center(
                      child: Column(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          CircularProgressIndicator(),
                          SizedBox(height: 16),
                          Text('Running diagnostic checks...'),
                        ],
                      ),
                    )
                  : ListView.builder(
                      itemCount: _diagnosticResults.length,
                      itemBuilder: (context, index) {
                        final result = _diagnosticResults[index];
                        return Card(
                          margin: const EdgeInsets.only(bottom: 8),
                          child: ListTile(
                            leading: Icon(
                              result['status'] == 'Pass' 
                                  ? Icons.check_circle
                                  : result['status'] == 'Warning'
                                      ? Icons.warning
                                      : Icons.error,
                              color: result['color'],
                            ),
                            title: Text(result['category']),
                            subtitle: Text(result['details']),
                            trailing: Container(
                              padding: const EdgeInsets.symmetric(
                                horizontal: 8,
                                vertical: 4,
                              ),
                              decoration: BoxDecoration(
                                color: result['color'].withOpacity(0.1),
                                borderRadius: BorderRadius.circular(12),
                                border: Border.all(
                                  color: result['color'].withOpacity(0.3),
                                ),
                              ),
                              child: Text(
                                result['status'],
                                style: TextStyle(
                                  color: result['color'],
                                  fontSize: 12,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                            ),
                          ),
                        );
                      },
                    ),
            ),
            
            // Quick Actions
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Quick Actions',
                      style: Theme.of(context).textTheme.titleMedium?.copyWith(
                        color: ECUTheme.getAccentColor('primary'),
                      ),
                    ),
                    const SizedBox(height: 16),
                    Row(
                      children: [
                        Expanded(
                          child: ElevatedButton.icon(
                            onPressed: _runInitialDiagnostics,
                            icon: const Icon(Icons.refresh),
                            label: const Text('Re-run Diagnostics'),
                            style: ElevatedButton.styleFrom(
                              backgroundColor: ECUTheme.getAccentColor('primary').withOpacity(0.1),
                              foregroundColor: ECUTheme.getAccentColor('primary'),
                            ),
                          ),
                        ),
                        const SizedBox(width: 16),
                        Expanded(
                          child: ElevatedButton.icon(
                            onPressed: () {
                              // TODO: Export diagnostic report
                            },
                            icon: const Icon(Icons.download),
                            label: const Text('Export Report'),
                            style: ElevatedButton.styleFrom(
                              backgroundColor: ECUTheme.getAccentColor('secondary').withOpacity(0.1),
                              foregroundColor: ECUTheme.getAccentColor('secondary'),
                            ),
                          ),
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
