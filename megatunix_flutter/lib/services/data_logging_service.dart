/// Data Logging Service - ENHANCED
/// Professional ECU data acquisition with configurable sample rates,
/// comprehensive CSV export, real-time analysis, and advanced features
/// Provides F5/F6/F7 keyboard shortcuts for professional workflow

import 'package:flutter/material.dart';
import 'dart:async';
import 'dart:io';
import 'dart:convert';
import 'package:path/path.dart' as path;
import '../models/ecu_data.dart';
import 'ecu_service.dart';

/// Enhanced data logging columns configuration
class LoggingConfig {
  final Set<String> enabledColumns;
  final int sampleRateHz;
  final bool includeMetadata;
  final bool enableRealTimeAnalysis;
  final String customFilePrefix;
  final bool autoSaveInterval;
  final int autoSaveMinutes;

  LoggingConfig({
    Set<String>? enabledColumns,
    this.sampleRateHz = 10,
    this.includeMetadata = true,
    this.enableRealTimeAnalysis = true,
    this.customFilePrefix = 'megatunix_log',
    this.autoSaveInterval = false,
    this.autoSaveMinutes = 5,
  }) : enabledColumns = enabledColumns ?? _defaultColumns;

  static const Set<String> _defaultColumns = {
    'timestamp', 'time_seconds', 'rpm', 'map', 'tps', 'coolant_temp',
    'intake_temp', 'battery_voltage', 'afr', 'timing', 'boost', 'engine_status'
  };

  static const Map<String, String> columnDescriptions = {
    'timestamp': 'ISO timestamp',
    'time_seconds': 'Seconds since logging start',
    'rpm': 'Engine RPM',
    'map': 'Manifold Absolute Pressure (kPa)',
    'tps': 'Throttle Position Sensor (%)',
    'coolant_temp': 'Coolant Temperature (°C)',
    'intake_temp': 'Intake Air Temperature (°C)',
    'battery_voltage': 'Battery Voltage (V)',
    'afr': 'Air-Fuel Ratio',
    'timing': 'Ignition Timing (°BTDC)',
    'boost': 'Boost Pressure (psi)',
    'engine_status': 'Engine Status Flags',
    'markers': 'User Markers and Events',
  };

  int get sampleIntervalMs => (1000 / sampleRateHz).round();
}

/// Enhanced service for managing professional data logging operations
class DataLoggingService extends ChangeNotifier {
  static final DataLoggingService _instance = DataLoggingService._internal();
  factory DataLoggingService() => _instance;
  DataLoggingService._internal();

  // Enhanced logging state
  bool _isLogging = false;
  DateTime? _logStartTime;
  int _logEntryCount = 0;
  File? _currentLogFile;
  StreamSubscription<SpeeduinoData>? _dataSubscription;
  Timer? _sampleTimer;
  
  // Enhanced configuration
  LoggingConfig _config = LoggingConfig();
  String _logDirectory = 'logs';
  
  // Log markers and statistics
  final List<LogMarker> _markers = [];
  final List<SpeeduinoData> _realtimeBuffer = [];
  LoggingStatistics _statistics = LoggingStatistics();
  
  // Real-time analysis
  double _minRpm = double.infinity;
  double _maxRpm = 0;
  double _avgRpm = 0;
  double _minAfr = double.infinity;
  double _maxAfr = 0;
  double _avgAfr = 0;
  
  // Getters
  bool get isLogging => _isLogging;
  DateTime? get logStartTime => _logStartTime;
  int get logEntryCount => _logEntryCount;
  List<LogMarker> get markers => List.unmodifiable(_markers);
  String get logDirectory => _logDirectory;
  LoggingConfig get config => _config;
  LoggingStatistics get statistics => _statistics;
  
  // Real-time analysis getters
  double get minRpm => _minRpm == double.infinity ? 0 : _minRpm;
  double get maxRpm => _maxRpm;
  double get avgRpm => _avgRpm;
  double get minAfr => _minAfr == double.infinity ? 0 : _minAfr;
  double get maxAfr => _maxAfr;
  double get avgAfr => _avgAfr;
  
  /// Configure logging settings
  void updateConfig(LoggingConfig newConfig) {
    _config = newConfig;
    notifyListeners();
  }
  
  /// Enhanced data logging start with professional configuration (F5)
  Future<void> startLogging({String? customFileName}) async {
    if (_isLogging) {
      print('Data logging already in progress');
      return;
    }
    
    try {
      // Create log directory if it doesn't exist
      final logDir = Directory(_logDirectory);
      if (!await logDir.exists()) {
        await logDir.create(recursive: true);
      }
      
      // Generate professional log filename with metadata
      final timestamp = DateTime.now().toIso8601String().replaceAll(':', '-').split('.')[0];
      final fileName = customFileName ?? '${_config.customFilePrefix}_${timestamp}_${_config.sampleRateHz}Hz.csv';
      _currentLogFile = File(path.join(_logDirectory, fileName));
      
      // Initialize logging state
      _isLogging = true;
      _logStartTime = DateTime.now();
      _logEntryCount = 0;
      _markers.clear();
      _realtimeBuffer.clear();
      _resetStatistics();
      
      // Write professional CSV with metadata
      await _writeEnhancedCSVHeader();
      
      // Start ECU data subscription with configured sample rate
      final ecuService = ECUService();
      if (_config.sampleRateHz <= 10) {
        // For low sample rates, use timer-based sampling
        _sampleTimer = Timer.periodic(
          Duration(milliseconds: _config.sampleIntervalMs),
          (_) => _sampleCurrentData(),
        );
      } else {
        // For high sample rates, use direct stream subscription
        _dataSubscription = ecuService.dataStream.listen(_logDataPoint);
      }
      
      print('Enhanced data logging started: ${_currentLogFile!.path}');
      print('Sample rate: ${_config.sampleRateHz}Hz (${_config.sampleIntervalMs}ms interval)');
      print('Enabled columns: ${_config.enabledColumns.join(", ")}');
      
      notifyListeners();
      
    } catch (e) {
      print('Failed to start enhanced data logging: $e');
      _isLogging = false;
      _currentLogFile = null;
      rethrow;
    }
  }
  
  /// Sample current data for timer-based logging
  void _sampleCurrentData() {
    final ecuService = ECUService();
    final currentData = ecuService.currentData;
    if (currentData != null) {
      _logDataPoint(currentData);
    }
  }
  
  /// Enhanced data logging stop with statistics (F6)
  Future<void> stopLogging() async {
    if (!_isLogging) {
      print('No active data logging session');
      return;
    }
    
    try {
      // Cancel subscriptions and timers
      await _dataSubscription?.cancel();
      _dataSubscription = null;
      _sampleTimer?.cancel();
      _sampleTimer = null;
      
      // Calculate final statistics
      final duration = DateTime.now().difference(_logStartTime!);
      _statistics.totalDurationMs = duration.inMilliseconds;
      _statistics.averageSampleRate = _logEntryCount / (duration.inSeconds);
      
      // Write enhanced metadata footer
      if (_currentLogFile != null && _config.includeMetadata) {
        await _writeEnhancedFooter();
      }
      
      // Write final markers to separate file
      if (_markers.isNotEmpty && _currentLogFile != null) {
        await _writeEnhancedMarkersFile();
      }
      
      _isLogging = false;
      
      print('Enhanced data logging stopped:');
      print('  Duration: ${duration.toString()}');
      print('  Entries: $_logEntryCount'); 
      print('  Average sample rate: ${_statistics.averageSampleRate.toStringAsFixed(1)}Hz');
      print('  RPM range: ${minRpm.toStringAsFixed(0)} - ${maxRpm.toStringAsFixed(0)}');
      print('  AFR range: ${minAfr.toStringAsFixed(2)} - ${maxAfr.toStringAsFixed(2)}');
      print('  Log saved to: ${_currentLogFile?.path}');
      
      notifyListeners();
      
    } catch (e) {
      print('Error stopping enhanced data logging: $e');
      rethrow;
    }
  }
  
  /// Mark data log (F7)
  void markLog({String? description}) {
    if (!_isLogging) {
      print('Cannot mark log - no active logging session');
      return;
    }
    
    final marker = LogMarker(
      timestamp: DateTime.now(),
      logEntryIndex: _logEntryCount,
      description: description ?? 'Marker ${_markers.length + 1}',
    );
    
    _markers.add(marker);
    print('Log marker added: ${marker.description} at entry $_logEntryCount');
    notifyListeners();
  }
  
  /// Enhanced data point logging with real-time analysis
  void _logDataPoint(SpeeduinoData data) {
    if (!_isLogging || _currentLogFile == null) return;
    
    try {
      // Generate enhanced CSV line
      final csvLine = _generateEnhancedCSVLine(data);
      _currentLogFile!.writeAsString(csvLine + '\n', mode: FileMode.append);
      _logEntryCount++;
      
      // Update real-time analysis
      if (_config.enableRealTimeAnalysis) {
        _updateRealtimeAnalysis(data);
      }
      
      // Add to real-time buffer (keep last 1000 points for analysis)
      _realtimeBuffer.add(data);
      if (_realtimeBuffer.length > 1000) {
        _realtimeBuffer.removeAt(0);
      }
      
      // Update statistics
      _statistics.totalDataPoints = _logEntryCount;
      
      // Notify listeners periodically to avoid excessive updates
      if (_logEntryCount % 50 == 0) {
        notifyListeners();
      }
      
    } catch (e) {
      print('Error logging enhanced data point: $e');
    }
  }
  
  /// Write professional CSV header with metadata
  Future<void> _writeEnhancedCSVHeader() async {
    if (_currentLogFile == null) return;
    
    final buffer = StringBuffer();
    
    // Professional metadata header
    if (_config.includeMetadata) {
      buffer.writeln('# MegaTunix Redux Data Log');
      buffer.writeln('# Generated: ${DateTime.now().toIso8601String()}');
      buffer.writeln('# Software Version: 2.0.0');
      buffer.writeln('# Sample Rate: ${_config.sampleRateHz}Hz');
      buffer.writeln('# Sample Interval: ${_config.sampleIntervalMs}ms');
      buffer.writeln('# ECU Protocol: Speeduino');
      buffer.writeln('# Enabled Columns: ${_config.enabledColumns.length}');
      buffer.writeln('#');
      
      // Column descriptions
      for (String column in _config.enabledColumns) {
        final description = LoggingConfig.columnDescriptions[column] ?? 'Unknown';
        buffer.writeln('# $column: $description');
      }
      buffer.writeln('#');
    }
    
    // CSV column headers
    final headers = _config.enabledColumns.toList()..sort();
    if (_markers.isNotEmpty || _isLogging) {
      headers.add('markers');
    }
    buffer.writeln(headers.join(','));
    
    await _currentLogFile!.writeAsString(buffer.toString());
  }
  
  /// Generate enhanced CSV line with configurable columns
  String _generateEnhancedCSVLine(SpeeduinoData data) {
    final values = <String>[];
    
    for (String column in _config.enabledColumns.toList()..sort()) {
      switch (column) {
        case 'timestamp':
          values.add(data.timestamp.toIso8601String());
          break;
        case 'time_seconds':
          final timeSeconds = _logStartTime != null 
              ? data.timestamp.difference(_logStartTime!).inMilliseconds / 1000.0
              : 0.0;
          values.add(timeSeconds.toStringAsFixed(3));
          break;
        case 'rpm':
          values.add(data.rpm.toString());
          break;
        case 'map':
          values.add(data.map.toString());
          break;
        case 'tps':
          values.add(data.tps.toString());
          break;
        case 'coolant_temp':
          values.add(data.coolantTemp.toString());
          break;
        case 'intake_temp':
          values.add(data.intakeTemp.toString());
          break;
        case 'battery_voltage':
          values.add(data.batteryVoltage.toStringAsFixed(2));
          break;
        case 'afr':
          values.add(data.afr.toStringAsFixed(2));
          break;
        case 'timing':
          values.add(data.timing.toString());
          break;
        case 'boost':
          values.add(data.boost.toString());
          break;
        case 'engine_status':
          values.add(data.engineStatus.toString());
          break;
      }
    }
    
    // Add marker if present
    final markersList = _markers.where((m) => m.logEntryIndex == _logEntryCount).toList();
    if (markersList.isNotEmpty || _config.enabledColumns.contains('markers')) {
      final markerText = markersList.isNotEmpty ? markersList.first.description : '';
      values.add('"$markerText"');
    }
    
    return values.join(',');
  }
  
  /// Update real-time analysis statistics
  void _updateRealtimeAnalysis(SpeeduinoData data) {
    // RPM analysis
    if (data.rpm > 0) {
      _minRpm = _minRpm == double.infinity ? data.rpm.toDouble() : 
               (_minRpm > data.rpm ? data.rpm.toDouble() : _minRpm);
      _maxRpm = _maxRpm < data.rpm ? data.rpm.toDouble() : _maxRpm;
      _avgRpm = (_avgRpm * (_logEntryCount - 1) + data.rpm) / _logEntryCount;
    }
    
    // AFR analysis
    if (data.afr > 0) {
      _minAfr = _minAfr == double.infinity ? data.afr : 
               (_minAfr > data.afr ? data.afr : _minAfr);
      _maxAfr = _maxAfr < data.afr ? data.afr : _maxAfr;
      _avgAfr = (_avgAfr * (_logEntryCount - 1) + data.afr) / _logEntryCount;
    }
  }
  
  /// Reset analysis statistics
  void _resetStatistics() {
    _minRpm = double.infinity;
    _maxRpm = 0;
    _avgRpm = 0;
    _minAfr = double.infinity;
    _maxAfr = 0;
    _avgAfr = 0;
    _statistics = LoggingStatistics();
  }
  
  /// Write enhanced metadata footer
  Future<void> _writeEnhancedFooter() async {
    if (_currentLogFile == null || !_config.includeMetadata) return;
    
    final buffer = StringBuffer();
    buffer.writeln('#');
    buffer.writeln('# Session Statistics:');
    buffer.writeln('# Total Data Points: $_logEntryCount');
    buffer.writeln('# Average Sample Rate: ${_statistics.averageSampleRate.toStringAsFixed(1)}Hz');
    buffer.writeln('# RPM Range: ${minRpm.toStringAsFixed(0)} - ${maxRpm.toStringAsFixed(0)} (avg: ${avgRpm.toStringAsFixed(0)})');
    buffer.writeln('# AFR Range: ${minAfr.toStringAsFixed(2)} - ${maxAfr.toStringAsFixed(2)} (avg: ${avgAfr.toStringAsFixed(2)})');
    buffer.writeln('# Total Markers: ${_markers.length}');
    buffer.writeln('# Session Ended: ${DateTime.now().toIso8601String()}');
    
    await _currentLogFile!.writeAsString(buffer.toString(), mode: FileMode.append);
  }
  
  /// Write enhanced markers file with analysis
  Future<void> _writeEnhancedMarkersFile() async {
    if (_markers.isEmpty || _currentLogFile == null) return;
    
    try {
      final markersFileName = _currentLogFile!.path.replaceAll('.csv', '_markers.json');
      final markersFile = File(markersFileName);
      
      final markersData = {
        'log_file': path.basename(_currentLogFile!.path),
        'generated': DateTime.now().toIso8601String(),
        'total_entries': _logEntryCount,
        'session_duration_ms': _statistics.totalDurationMs,
        'sample_rate_hz': _config.sampleRateHz,
        'statistics': {
          'rpm': {'min': minRpm, 'max': maxRpm, 'avg': avgRpm},
          'afr': {'min': minAfr, 'max': maxAfr, 'avg': avgAfr},
        },
        'markers': _markers.map((marker) {
          final timeSeconds = _logStartTime != null
              ? marker.timestamp.difference(_logStartTime!).inMilliseconds / 1000.0
              : 0.0;
          return {
            'entry_index': marker.logEntryIndex,
            'timestamp': marker.timestamp.toIso8601String(),
            'time_seconds': timeSeconds,
            'description': marker.description,
          };
        }).toList(),
      };
      
      await markersFile.writeAsString(jsonEncode(markersData));
      print('Enhanced markers saved to: $markersFileName');
      
    } catch (e) {
      print('Error writing enhanced markers file: $e');
    }
  }
  
  /// Enhanced log file management with metadata
  Future<List<LogFileInfo>> getRecentLogFiles({int limit = 10}) async {
    try {
      final logDir = Directory(_logDirectory);
      if (!await logDir.exists()) return [];
      
      final files = await logDir
          .list()
          .where((entity) => entity is File && entity.path.endsWith('.csv'))
          .cast<File>()
          .toList();
      
      // Sort by modification time, most recent first
      files.sort((a, b) => b.lastModifiedSync().compareTo(a.lastModifiedSync()));
      
      final logFiles = <LogFileInfo>[];
      for (File file in files.take(limit)) {
        final info = await _analyzeLogFile(file);
        if (info != null) logFiles.add(info);
      }
      
      return logFiles;
      
    } catch (e) {
      print('Error getting enhanced log files: $e');
      return [];
    }
  }
  
  /// Analyze log file for metadata
  Future<LogFileInfo?> _analyzeLogFile(File file) async {
    try {
      final lines = await file.readAsLines();
      final stats = file.statSync();
      
      String? sampleRate;
      int? totalEntries;
      DateTime? sessionStart;
      
      // Parse metadata from header
      for (String line in lines.take(20)) {
        if (line.startsWith('# Sample Rate:')) {
          sampleRate = line.split(':')[1].trim();
        } else if (line.startsWith('# Generated:')) {
          sessionStart = DateTime.tryParse(line.split('Generated: ')[1].trim());
        } else if (!line.startsWith('#')) {
          break;
        }
      }
      
      // Count data lines (non-comment, non-header)
      totalEntries = lines.where((line) => !line.startsWith('#') && line.contains(',')).length - 1;
      
      return LogFileInfo(
        file: file,
        fileName: path.basename(file.path),
        fileSize: stats.size,
        createdAt: stats.modified,
        sessionStart: sessionStart,
        sampleRate: sampleRate,
        totalEntries: totalEntries,
        duration: totalEntries != null && sampleRate != null 
            ? Duration(seconds: (totalEntries / int.tryParse(sampleRate.replaceAll('Hz', ''))! ).round())
            : null,
      );
    } catch (e) {
      print('Error analyzing log file ${file.path}: $e');
      return null;
    }
  }
  
  /// Configure logging with enhanced options
  void configureLogging({
    String? logDirectory,
    LoggingConfig? config,
  }) {
    if (logDirectory != null) _logDirectory = logDirectory;
    if (config != null) _config = config;
    notifyListeners();
  }
  
  /// Get available columns for configuration
  static List<String> getAvailableColumns() {
    return LoggingConfig.columnDescriptions.keys.toList()..sort();
  }
  
  /// Create preset configurations
  static LoggingConfig createPresetConfig(String preset) {
    switch (preset.toLowerCase()) {
      case 'basic':
        return LoggingConfig(
          enabledColumns: {'timestamp', 'time_seconds', 'rpm', 'map', 'tps'},
          sampleRateHz: 5,
          customFilePrefix: 'basic_log',
        );
      case 'standard':
        return LoggingConfig(
          enabledColumns: {'timestamp', 'time_seconds', 'rpm', 'map', 'tps', 
                          'coolant_temp', 'intake_temp', 'afr'},
          sampleRateHz: 10,
          customFilePrefix: 'standard_log',
        );
      case 'professional':
        return LoggingConfig(
          enabledColumns: LoggingConfig.columnDescriptions.keys.toSet(),
          sampleRateHz: 20,
          customFilePrefix: 'pro_log',
          enableRealTimeAnalysis: true,
        );
      case 'high_speed':
        return LoggingConfig(
          enabledColumns: {'timestamp', 'time_seconds', 'rpm', 'map', 'tps', 'timing'},
          sampleRateHz: 50,
          customFilePrefix: 'highspeed_log',
          includeMetadata: false,
        );
      default:
        return LoggingConfig();
    }
  }
  
  /// Emergency stop logging (in case of errors)
  void emergencyStop() {
    _isLogging = false;
    _dataSubscription?.cancel();
    _dataSubscription = null;
    print('Emergency stop of data logging');
    notifyListeners();
  }
  
  @override
  void dispose() {
    emergencyStop();
    super.dispose();
  }
}

/// Enhanced log marker with additional metadata
class LogMarker {
  final DateTime timestamp;
  final int logEntryIndex;
  final String description;
  final String? category;
  final Map<String, dynamic>? metadata;
  
  LogMarker({
    required this.timestamp,
    required this.logEntryIndex,
    required this.description,
    this.category,
    this.metadata,
  });
  
  @override
  String toString() => 'LogMarker($logEntryIndex: $description)';
}

/// Logging statistics for analysis
class LoggingStatistics {
  int totalDataPoints = 0;
  int totalDurationMs = 0;
  double averageSampleRate = 0.0;
  DateTime? sessionStart;
  DateTime? sessionEnd;
  
  LoggingStatistics({
    this.totalDataPoints = 0,
    this.totalDurationMs = 0,
    this.averageSampleRate = 0.0,
    this.sessionStart,
    this.sessionEnd,
  });
  
  Duration get duration => Duration(milliseconds: totalDurationMs);
  
  double get actualSampleRate => totalDataPoints > 0 && totalDurationMs > 0
      ? totalDataPoints / (totalDurationMs / 1000.0)
      : 0.0;
}

/// Enhanced log file information
class LogFileInfo {
  final File file;
  final String fileName;
  final int fileSize;
  final DateTime createdAt;
  final DateTime? sessionStart;
  final String? sampleRate;
  final int? totalEntries;
  final Duration? duration;
  
  LogFileInfo({
    required this.file,
    required this.fileName,
    required this.fileSize,
    required this.createdAt,
    this.sessionStart,
    this.sampleRate,
    this.totalEntries,
    this.duration,
  });
  
  String get fileSizeFormatted {
    if (fileSize < 1024) return '${fileSize}B';
    if (fileSize < 1024 * 1024) return '${(fileSize / 1024).toStringAsFixed(1)}KB';
    return '${(fileSize / (1024 * 1024)).toStringAsFixed(1)}MB';
  }
  
  String get durationFormatted {
    if (duration == null) return 'Unknown';
    final minutes = duration!.inMinutes;
    final seconds = duration!.inSeconds % 60;
    return '${minutes}m ${seconds}s';
  }
}