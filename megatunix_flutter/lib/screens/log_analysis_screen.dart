/// Log Playback and Analysis System
/// Professional ECU data log analysis with playback controls,
/// statistical analysis, and advanced visualization

import 'package:flutter/material.dart';
import 'dart:async';
import 'dart:io';
import 'dart:convert';
import '../services/data_logging_service.dart';
import '../models/ecu_data.dart';

/// Log playback and analysis screen
class LogAnalysisScreen extends StatefulWidget {
  final LogFileInfo logFileInfo;

  const LogAnalysisScreen({
    super.key,
    required this.logFileInfo,
  });

  @override
  State<LogAnalysisScreen> createState() => _LogAnalysisScreenState();
}

class _LogAnalysisScreenState extends State<LogAnalysisScreen>
    with TickerProviderStateMixin {
  
  // Playback state
  List<LogDataPoint> _logData = [];
  List<LogMarkerData> _markers = [];
  int _currentIndex = 0;
  bool _isPlaying = false;
  Timer? _playbackTimer;
  double _playbackSpeed = 1.0;
  
  // Analysis state
  LogStatistics? _statistics;
  String _selectedParameter = 'rpm';
  bool _isLoading = true;
  
  // Animation controllers
  late AnimationController _playbackController;
  late AnimationController _chartController;

  @override
  void initState() {
    super.initState();
    _initializeControllers();
    _loadLogData();
  }

  void _initializeControllers() {
    _playbackController = AnimationController(
      duration: const Duration(milliseconds: 300),
      vsync: this,
    );
    
    _chartController = AnimationController(
      duration: const Duration(seconds: 2),
      vsync: this,
    );
  }

  Future<void> _loadLogData() async {
    setState(() => _isLoading = true);
    
    try {
      // Load main CSV data
      await _loadCSVData();
      
      // Load markers if available
      await _loadMarkersData();
      
      // Calculate statistics
      _calculateStatistics();
      
      _chartController.forward();
      
    } catch (e) {
      _showError('Failed to load log data: $e');
    } finally {
      setState(() => _isLoading = false);
    }
  }

  Future<void> _loadCSVData() async {
    final lines = await widget.logFileInfo.file.readAsLines();
    final dataLines = lines.where((line) => 
        !line.startsWith('#') && line.contains(',')).toList();
    
    if (dataLines.isEmpty) return;
    
    final headers = dataLines.first.split(',');
    _logData = [];
    
    for (int i = 1; i < dataLines.length; i++) {
      final values = dataLines[i].split(',');
      if (values.length >= headers.length) {
        try {
          final dataPoint = LogDataPoint.fromCSV(headers, values);
          _logData.add(dataPoint);
        } catch (e) {
          print('Error parsing line $i: $e');
        }
      }
    }
  }

  Future<void> _loadMarkersData() async {
    final markersPath = widget.logFileInfo.file.path.replaceAll('.csv', '_markers.json');
    final markersFile = File(markersPath);
    
    if (await markersFile.exists()) {
      try {
        final content = await markersFile.readAsString();
        final data = jsonDecode(content) as Map<String, dynamic>;
        final markersList = data['markers'] as List<dynamic>;
        
        _markers = markersList.map((m) => LogMarkerData.fromJson(m)).toList();
      } catch (e) {
        print('Error loading markers: $e');
      }
    }
  }

  void _calculateStatistics() {
    if (_logData.isEmpty) return;
    
    final stats = LogStatistics();
    
    // Calculate for each parameter
    final parameters = _logData.first.data.keys.toList();
    for (final param in parameters) {
      final values = _logData
          .map((point) => point.data[param])
          .where((value) => value != null)
          .cast<double>()
          .toList();
          
      if (values.isNotEmpty) {
        values.sort();
        stats.parameterStats[param] = ParameterStatistics(
          min: values.first,
          max: values.last,
          average: values.reduce((a, b) => a + b) / values.length,
          median: values[values.length ~/ 2],
          standardDeviation: _calculateStandardDeviation(values),
        );
      }
    }
    }
    
    // Calculate session info
    stats.totalDataPoints = _logData.length;
    stats.sessionDuration = _logData.last.timeSeconds - _logData.first.timeSeconds;
    stats.averageSampleRate = stats.totalDataPoints / stats.sessionDuration;
    stats.totalMarkers = _markers.length;
    
    _statistics = stats;
  }

  double _calculateStandardDeviation(List<double> values) {
    final mean = values.reduce((a, b) => a + b) / values.length;
    final variance = values
        .map((value) => (value - mean) * (value - mean))
        .reduce((a, b) => a + b) / values.length;
    return sqrt(variance);
  }

  @override
  Widget build(BuildContext context) {
    if (_isLoading) {
      return Scaffold(
        appBar: AppBar(title: const Text('Loading Log...')),
        body: const Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              CircularProgressIndicator(),
              SizedBox(height: 16),
              Text('Analyzing log data...'),
            ],
          ),
        ),
      );
    }

    if (_logData.isEmpty) {
      return Scaffold(
        appBar: AppBar(title: const Text('Log Analysis')),
        body: const Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(Icons.error, size: 64, color: Colors.red),
              SizedBox(height: 16),
              Text('No data found in log file'),
            ],
          ),
        ),
      );
    }

    return Scaffold(
      appBar: AppBar(
        title: Text('Analyze: ${widget.logFileInfo.fileName}'),
        actions: [
          IconButton(
            icon: const Icon(Icons.info),
            onPressed: _showLogInfo,
          ),
        ],
      ),
      body: Column(
        children: [
          // Playback Controls
          _buildPlaybackControls(),
          
          // Main content
          Expanded(
            child: Row(
              children: [
                // Chart area
                Expanded(
                  flex: 3,
                  child: _buildChartArea(),
                ),
                
                // Analysis panel
                SizedBox(
                  width: 300,
                  child: _buildAnalysisPanel(),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildPlaybackControls() {
    final progress = _logData.isNotEmpty ? _currentIndex / _logData.length : 0.0;
    final currentTime = _logData.isNotEmpty ? _logData[_currentIndex].timeSeconds : 0.0;
    
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surfaceVariant,
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.1),
            blurRadius: 4,
            offset: const Offset(0, 2),
          ),
        ],
      ),
      child: Column(
        children: [
          // Progress bar
          Row(
            children: [
              Text(_formatTime(currentTime)),
              Expanded(
                child: Slider(
                  value: progress,
                  onChanged: (value) {
                    setState(() {
                      _currentIndex = (value * _logData.length).round().clamp(0, _logData.length - 1);
                    });
                  },
                ),
              ),
              Text(_formatTime(_statistics?.sessionDuration ?? 0)),
            ],
          ),
          
          // Control buttons
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              IconButton(
                onPressed: _goToStart,
                icon: const Icon(Icons.skip_previous),
              ),
              IconButton(
                onPressed: _stepBackward,
                icon: const Icon(Icons.fast_rewind),
              ),
              IconButton(
                onPressed: _togglePlayback,
                icon: Icon(_isPlaying ? Icons.pause : Icons.play_arrow),
              ),
              IconButton(
                onPressed: _stepForward,
                icon: const Icon(Icons.fast_forward),
              ),
              IconButton(
                onPressed: _goToEnd,
                icon: const Icon(Icons.skip_next),
              ),
              const SizedBox(width: 16),
              
              // Speed control
              DropdownButton<double>(
                value: _playbackSpeed,
                items: [0.25, 0.5, 1.0, 2.0, 4.0, 8.0].map((speed) {
                  return DropdownMenuItem(
                    value: speed,
                    child: Text('${speed}x'),
                  );
                }).toList(),
                onChanged: (speed) {
                  setState(() {
                    _playbackSpeed = speed!;
                  });
                },
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildChartArea() {
    return Card(
      margin: const EdgeInsets.all(8),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Text(
                  'Parameter Analysis',
                  style: Theme.of(context).textTheme.titleLarge,
                ),
                const Spacer(),
                DropdownButton<String>(
                  value: _selectedParameter,
                  items: _getAvailableParameters().map((param) {
                    return DropdownMenuItem(
                      value: param,
                      child: Text(param.toUpperCase()),
                    );
                  }).toList(),
                  onChanged: (param) {
                    setState(() {
                      _selectedParameter = param!;
                    });
                  },
                ),
              ],
            ),
            const SizedBox(height: 16),
            
            Expanded(
              child: AnimatedBuilder(
                animation: _chartController,
                builder: (context, child) => CustomPaint(
                  painter: LogAnalysisChartPainter(
                    logData: _logData,
                    markers: _markers,
                    selectedParameter: _selectedParameter,
                    currentIndex: _currentIndex,
                    theme: Theme.of(context),
                    animationValue: _chartController.value,
                  ),
                  size: Size.infinite,
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildAnalysisPanel() {
    return Card(
      margin: const EdgeInsets.all(8),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Statistics',
              style: Theme.of(context).textTheme.titleLarge,
            ),
            const SizedBox(height: 16),
            
            if (_statistics != null) ...[
              _buildStatGroup('Session Info', [
                ('Duration', _formatTime(_statistics!.sessionDuration)),
                ('Data Points', _statistics!.totalDataPoints.toString()),
                ('Sample Rate', '${_statistics!.averageSampleRate.toStringAsFixed(1)}Hz'),
                ('Markers', _statistics!.totalMarkers.toString()),
              ]),
              
              const SizedBox(height: 16),
              
              if (_statistics!.parameterStats.containsKey(_selectedParameter)) ...[
                _buildStatGroup(_selectedParameter.toUpperCase(), [
                  ('Min', _formatParameterValue(_selectedParameter, _statistics!.parameterStats[_selectedParameter]!.min)),
                  ('Max', _formatParameterValue(_selectedParameter, _statistics!.parameterStats[_selectedParameter]!.max)),
                  ('Average', _formatParameterValue(_selectedParameter, _statistics!.parameterStats[_selectedParameter]!.average)),
                  ('Median', _formatParameterValue(_selectedParameter, _statistics!.parameterStats[_selectedParameter]!.median)),
                  ('Std Dev', _formatParameterValue(_selectedParameter, _statistics!.parameterStats[_selectedParameter]!.standardDeviation)),
                ]),
              ],
              
              const SizedBox(height: 16),
              
              // Current value
              if (_currentIndex < _logData.length) ...[
                Container(
                  padding: const EdgeInsets.all(12),
                  decoration: BoxDecoration(
                    color: Theme.of(context).colorScheme.primaryContainer,
                    borderRadius: BorderRadius.circular(8),
                  ),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'Current Values',
                        style: Theme.of(context).textTheme.titleMedium?.copyWith(
                          color: Theme.of(context).colorScheme.onPrimaryContainer,
                        ),
                      ),
                      const SizedBox(height: 8),
                      ..._logData[_currentIndex].data.entries.map((entry) {
                        return Padding(
                          padding: const EdgeInsets.symmetric(vertical: 2),
                          child: Row(
                            mainAxisAlignment: MainAxisAlignment.spaceBetween,
                            children: [
                              Text(
                                entry.key.toUpperCase(),
                                style: Theme.of(context).textTheme.bodySmall?.copyWith(
                                  color: Theme.of(context).colorScheme.onPrimaryContainer,
                                ),
                              ),
                              Text(
                                _formatParameterValue(entry.key, entry.value?.toDouble() ?? 0),
                                style: Theme.of(context).textTheme.bodySmall?.copyWith(
                                  color: Theme.of(context).colorScheme.onPrimaryContainer,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                            ],
                          ),
                        );
                      }).toList(),
                    ],
                  ),
                ),
              ],
            ],
            
            const Spacer(),
            
            // Export options
            Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                ElevatedButton.icon(
                  onPressed: _exportAnalysis,
                  icon: const Icon(Icons.download),
                  label: const Text('Export Analysis'),
                ),
                const SizedBox(height: 8),
                OutlinedButton.icon(
                  onPressed: _showMarkersDialog,
                  icon: const Icon(Icons.flag),
                  label: Text('View Markers (${_markers.length})'),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildStatGroup(String title, List<(String, String)> stats) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(
          title,
          style: Theme.of(context).textTheme.titleSmall?.copyWith(
            fontWeight: FontWeight.bold,
          ),
        ),
        const SizedBox(height: 8),
        ...stats.map((stat) => Padding(
          padding: const EdgeInsets.symmetric(vertical: 2),
          child: Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(stat.$1),
              Text(stat.$2, style: const TextStyle(fontWeight: FontWeight.bold)),
            ],
          ),
        )),
      ],
    );
  }

  List<String> _getAvailableParameters() {
    if (_logData.isEmpty) return [];
    return _logData.first.data.keys.toList()..sort();
  }

  String _formatTime(double seconds) {
    final minutes = (seconds / 60).floor();
    final remainingSeconds = (seconds % 60).floor();
    return '${minutes.toString().padLeft(2, '0')}:${remainingSeconds.toString().padLeft(2, '0')}';
  }

  String _formatParameterValue(String parameter, double value) {
    switch (parameter) {
      case 'rpm':
        return value.round().toString();
      case 'afr':
      case 'battery_voltage':
        return value.toStringAsFixed(2);
      case 'map':
      case 'tps':
      case 'coolant_temp':
      case 'intake_temp':
      case 'timing':
      case 'boost':
        return value.round().toString();
      default:
        return value.toStringAsFixed(1);
    }
  }

  void _togglePlayback() {
    setState(() {
      _isPlaying = !_isPlaying;
    });
    
    if (_isPlaying) {
      _startPlayback();
      _playbackController.forward();
    } else {
      _stopPlayback();
      _playbackController.reverse();
    }
  }

  void _startPlayback() {
    _playbackTimer = Timer.periodic(
      Duration(milliseconds: (100 / _playbackSpeed).round()),
      (timer) {
        if (_currentIndex < _logData.length - 1) {
          setState(() {
            _currentIndex++;
          });
        } else {
          _stopPlayback();
        }
      },
    );
  }

  void _stopPlayback() {
    _playbackTimer?.cancel();
    setState(() {
      _isPlaying = false;
    });
  }

  void _goToStart() {
    setState(() {
      _currentIndex = 0;
    });
  }

  void _goToEnd() {
    setState(() {
      _currentIndex = _logData.length - 1;
    });
  }

  void _stepForward() {
    if (_currentIndex < _logData.length - 1) {
      setState(() {
        _currentIndex++;
      });
    }
  }

  void _stepBackward() {
    if (_currentIndex > 0) {
      setState(() {
        _currentIndex--;
      });
    }
  }

  void _showLogInfo() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text('Log Information'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('File: ${widget.logFileInfo.fileName}'),
            Text('Size: ${widget.logFileInfo.fileSizeFormatted}'),
            Text('Created: ${widget.logFileInfo.createdAt}'),
            if (_statistics != null) ...[
              const SizedBox(height: 16),
              Text('Duration: ${_formatTime(_statistics!.sessionDuration)}'),
              Text('Data Points: ${_statistics!.totalDataPoints}'),
              Text('Sample Rate: ${_statistics!.averageSampleRate.toStringAsFixed(1)}Hz'),
            ],
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('Close'),
          ),
        ],
      ),
    );
  }

  void _showMarkersDialog() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text('Markers (${_markers.length})'),
        content: SizedBox(
          width: double.maxFinite,
          height: 400,
          child: _markers.isEmpty
            ? const Center(child: Text('No markers found'))
            : ListView.builder(
                itemCount: _markers.length,
                itemBuilder: (context, index) {
                  final marker = _markers[index];
                  return ListTile(
                    leading: const Icon(Icons.flag),
                    title: Text(marker.description),
                    subtitle: Text('${_formatTime(marker.timeSeconds)} (Entry ${marker.entryIndex})'),
                    onTap: () {
                      Navigator.of(context).pop();
                      setState(() {
                        _currentIndex = marker.entryIndex.clamp(0, _logData.length - 1);
                      });
                    },
                  );
                },
              ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('Close'),
          ),
        ],
      ),
    );
  }

  void _exportAnalysis() async {
    // Export analysis report
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
        content: Text('📊 Analysis export feature coming soon'),
      ),
    );
  }

  void _showError(String message) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),
        backgroundColor: Colors.red,
      ),
    );
  }

  @override
  void dispose() {
    _playbackTimer?.cancel();
    _playbackController.dispose();
    _chartController.dispose();
    super.dispose();
  }
}

// Data classes for log analysis
class LogDataPoint {
  final double timeSeconds;
  final Map<String, dynamic> data;

  LogDataPoint({required this.timeSeconds, required this.data});

  factory LogDataPoint.fromCSV(List<String> headers, List<String> values) {
    final data = <String, dynamic>{};
    
    for (int i = 0; i < headers.length && i < values.length; i++) {
      final header = headers[i].toLowerCase().trim();
      final value = values[i].trim().replaceAll('"', '');
      
      if (header == 'time_seconds') {
        continue; // Skip, we'll handle this separately
      }
      
      if (header == 'timestamp') {
        continue; // Skip timestamp for now
      }
      
      // Try to parse as number
      final numValue = double.tryParse(value);
      if (numValue != null) {
        data[header] = numValue;
      } else {
        data[header] = value;
      }
    }
    
    final timeSeconds = double.tryParse(values[headers.indexOf('time_seconds')]) ?? 0.0;
    
    return LogDataPoint(timeSeconds: timeSeconds, data: data);
  }
}

class LogMarkerData {
  final int entryIndex;
  final double timeSeconds;
  final String description;

  LogMarkerData({
    required this.entryIndex,
    required this.timeSeconds,
    required this.description,
  });

  factory LogMarkerData.fromJson(Map<String, dynamic> json) {
    return LogMarkerData(
      entryIndex: json['entry_index'] as int,
      timeSeconds: (json['time_seconds'] as num).toDouble(),
      description: json['description'] as String,
    );
  }
}

class LogStatistics {
  int totalDataPoints = 0;
  double sessionDuration = 0.0;
  double averageSampleRate = 0.0;
  int totalMarkers = 0;
  Map<String, ParameterStatistics> parameterStats = {};
}

class ParameterStatistics {
  final double min;
  final double max;
  final double average;
  final double median;
  final double standardDeviation;

  ParameterStatistics({
    required this.min,
    required this.max,
    required this.average,
    required this.median,
    required this.standardDeviation,
  });
}

// Custom painter for log analysis chart
class LogAnalysisChartPainter extends CustomPainter {
  final List<LogDataPoint> logData;
  final List<LogMarkerData> markers;
  final String selectedParameter;
  final int currentIndex;
  final ThemeData theme;
  final double animationValue;

  LogAnalysisChartPainter({
    required this.logData,
    required this.markers,
    required this.selectedParameter,
    required this.currentIndex,
    required this.theme,
    required this.animationValue,
  });

  @override
  void paint(Canvas canvas, Size size) {
    if (logData.isEmpty) return;

    final rect = Rect.fromLTWH(0, 0, size.width, size.height);
    
    // Draw background
    final backgroundPaint = Paint()
      ..color = theme.colorScheme.surface
      ..style = PaintingStyle.fill;
    canvas.drawRect(rect, backgroundPaint);

    // Get parameter values
    final values = logData
        .map((point) => point.data[selectedParameter])
        .where((value) => value != null)
        .map((value) => value!.toDouble())
        .toList();
        
    if (values.isEmpty) return;

    // Calculate bounds
    final minValue = values.reduce((a, b) => a < b ? a : b);
    final maxValue = values.reduce((a, b) => a > b ? a : b);
    final valueRange = maxValue - minValue;
    final timeRange = logData.last.timeSeconds - logData.first.timeSeconds;

    // Draw grid
    _drawGrid(canvas, size);

    // Draw data line
    _drawDataLine(canvas, size, minValue, valueRange, timeRange);

    // Draw markers
    _drawMarkers(canvas, size, timeRange);

    // Draw current position indicator
    _drawCurrentPosition(canvas, size, timeRange);

    // Draw border
    final borderPaint = Paint()
      ..color = theme.colorScheme.outline.withOpacity(0.3)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1;
    canvas.drawRect(rect, borderPaint);
  }

  void _drawGrid(Canvas canvas, Size size) {
    final gridPaint = Paint()
      ..color = theme.colorScheme.outline.withOpacity(0.1)
      ..strokeWidth = 0.5;

    // Vertical lines
    for (int i = 0; i <= 10; i++) {
      final x = size.width * i / 10;
      canvas.drawLine(Offset(x, 0), Offset(x, size.height), gridPaint);
    }

    // Horizontal lines
    for (int i = 0; i <= 5; i++) {
      final y = size.height * i / 5;
      canvas.drawLine(Offset(0, y), Offset(size.width, y), gridPaint);
    }
  }

  void _drawDataLine(Canvas canvas, Size size, double minValue, double valueRange, double timeRange) {
    final paint = Paint()
      ..color = theme.colorScheme.primary.withOpacity(0.8)
      ..strokeWidth = 2.0
      ..style = PaintingStyle.stroke
      ..strokeCap = StrokeCap.round;

    final path = Path();
    bool isFirstPoint = true;

    for (int i = 0; i < logData.length && i <= currentIndex * animationValue; i++) {
      final point = logData[i];
      final value = point.data[selectedParameter]?.toDouble();
      if (value == null) continue;

      final x = timeRange > 0 ? size.width * point.timeSeconds / timeRange : 0.0;
      final y = valueRange > 0
          ? size.height * (1 - (value - minValue) / valueRange)
          : size.height / 2;

      if (isFirstPoint) {
        path.moveTo(x, y);
        isFirstPoint = false;
      } else {
        path.lineTo(x, y);
      }
    }

    canvas.drawPath(path, paint);
  }

  void _drawMarkers(Canvas canvas, Size size, double timeRange) {
    final markerPaint = Paint()
      ..color = Colors.orange
      ..strokeWidth = 2.0;

    for (LogMarkerData marker in markers) {
      final x = timeRange > 0 ? size.width * marker.timeSeconds / timeRange : 0.0;
      
      // Draw vertical line
      canvas.drawLine(
        Offset(x, 0),
        Offset(x, size.height),
        markerPaint,
      );

      // Draw flag icon
      final flagPath = Path()
        ..moveTo(x, 10)
        ..lineTo(x + 15, 15)
        ..lineTo(x, 20)
        ..close();
      canvas.drawPath(flagPath, markerPaint..style = PaintingStyle.fill);
    }
  }

  void _drawCurrentPosition(Canvas canvas, Size size, double timeRange) {
    if (currentIndex >= logData.length) return;

    final currentTime = logData[currentIndex].timeSeconds;
    final x = timeRange > 0 ? size.width * currentTime / timeRange : 0.0;

    final paint = Paint()
      ..color = Colors.red
      ..strokeWidth = 3.0;

    canvas.drawLine(
      Offset(x, 0),
      Offset(x, size.height),
      paint,
    );

    // Draw position indicator circle
    final circlePaint = Paint()
      ..color = Colors.red
      ..style = PaintingStyle.fill;

    canvas.drawCircle(Offset(x, size.height - 10), 6, circlePaint);
  }

  @override
  bool shouldRepaint(covariant LogAnalysisChartPainter oldDelegate) {
    return oldDelegate.currentIndex != currentIndex ||
           oldDelegate.animationValue != animationValue ||
           oldDelegate.selectedParameter != selectedParameter;
  }
}

// Helper function for square root (since dart:math import might be missing)
double sqrt(double x) {
  if (x < 0) return double.nan;
  if (x == 0) return 0;
  
  double guess = x / 2;
  double prev;
  
  do {
    prev = guess;
    guess = (guess + x / guess) / 2;
  } while ((guess - prev).abs() > 0.0001);
  
  return guess;
}