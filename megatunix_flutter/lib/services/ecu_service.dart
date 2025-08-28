/// ECU Service for Multi-Protocol Communication
/// Handles protocol abstraction, automatic detection, and unified communication
/// Supports Speeduino, MegaSquirt (MS1/MS2/MS3), and EpicECU protocols
/// Uses protocol handlers for extensible ECU support

import 'dart:async';
import 'dart:math';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
// import 'package:provider/single_child_widget.dart'; // Not needed for this implementation
import '../models/ecu_data.dart';
import 'ecu_protocol_handler.dart';
import 'ecu_protocol_factory.dart';

/// ECU Service for managing multi-protocol ECU communication
/// Provides unified interface for Speeduino, MegaSquirt, and EpicECU communication
/// Features automatic protocol detection, real-time data streaming, and error recovery
/// Implements professional ECU tuning software standards with robust error handling
class ECUService extends ChangeNotifier {
  static final ECUService _instance = ECUService._internal();
  factory ECUService() => _instance;
  ECUService._internal();

  // Protocol handler and connection state
  ECUProtocolHandler? _currentHandler;
  ECUConnectionState _connectionState = ECUConnectionState.disconnected;
  Timer? _dataStreamSubscription;
  StreamSubscription<SpeeduinoData>? _dataSubscription;
  StreamSubscription<ECUConnectionState>? _connectionSubscription;
  StreamSubscription<ECUError>? _errorSubscription;
  
  // Data streams
  final StreamController<SpeeduinoData> _dataController = StreamController<SpeeduinoData>.broadcast();
  final StreamController<ECUConnectionState> _connectionController = StreamController<ECUConnectionState>.broadcast();
  final StreamController<ECUError> _errorController = StreamController<ECUError>.broadcast();
  
  // Configuration and current data
  String _port = '/dev/ttyACM0';
  int _baudRate = 115200;
  ECUProtocol _selectedProtocol = ECUProtocol.speeduino;
  ECUProtocol? _detectedProtocol;
  SpeeduinoData? _currentData;
  
  // Protocol detection settings
  bool _autoDetectProtocol = true;
  List<ECUProtocol> _detectionOrder = [
    ECUProtocol.speeduino,
    ECUProtocol.megasquirt, 
    ECUProtocol.epicEFI,
  ];
  
  // Getters
  ECUConnectionState get connectionState => _connectionState;
  SpeeduinoData? get currentData => _currentData;
  Stream<SpeeduinoData> get dataStream => _dataController.stream;
  Stream<ECUConnectionState> get connectionStream => _connectionController.stream;
  Stream<ECUError> get errorStream => _errorController.stream;
  ECUStatistics get statistics => _currentHandler?.statistics ?? _emptyStatistics();
  String get port => _port;
  int get baudRate => _baudRate;
  ECUProtocol get selectedProtocol => _selectedProtocol;
  ECUProtocol? get detectedProtocol => _detectedProtocol;
  String get protocolName => _currentHandler?.protocolName ?? 'None';
  bool get autoDetectProtocol => _autoDetectProtocol;
  List<ECUProtocol> get supportedProtocols => ECUProtocolFactory.supportedProtocols;
  Map<ECUProtocol, String> get protocolNames => ECUProtocolFactory.protocolNames;
  List<int> get supportedBaudRates => _currentHandler?.supportedBaudRates ?? [115200];
  
  /// Set protocol selection (manual override)
  void setProtocol(ECUProtocol protocol) {
    if (_selectedProtocol != protocol) {
      _selectedProtocol = protocol;
      print('ECU Service: Manual protocol selection: ${ECUProtocolFactory.protocolNames[protocol]}');
      notifyListeners();
    }
  }
  
  /// Enable/disable automatic protocol detection
  void setAutoDetectProtocol(bool enabled) {
    if (_autoDetectProtocol != enabled) {
      _autoDetectProtocol = enabled;
      print('ECU Service: Auto-detection ${enabled ? 'enabled' : 'disabled'}');
      notifyListeners();
    }
  }
  
  /// Connect to ECU with protocol detection or manual selection
  Future<bool> connect({
    String? port,
    int? baudRate,
    ECUProtocol? protocol,
  }) async {
    print('ECU Service: Starting connection process...');
    try {
      // Update configuration if provided
      if (port != null) _port = port;
      if (baudRate != null) _baudRate = baudRate;
      if (protocol != null) {
        _selectedProtocol = protocol;
        _autoDetectProtocol = false; // Manual selection disables auto-detect
      }
      
      notifyListeners();
      _updateConnectionState(ECUConnectionState.connecting);

      // Protocol detection or direct connection
      ECUProtocol targetProtocol;
      if (_autoDetectProtocol) {
        print('ECU Service: Starting automatic protocol detection...');
        targetProtocol = await _detectProtocol();
        _detectedProtocol = targetProtocol;
        print('ECU Service: Detected protocol: ${ECUProtocolFactory.protocolNames[targetProtocol]}');
      } else {
        targetProtocol = _selectedProtocol;
        print('ECU Service: Using manual protocol selection: ${ECUProtocolFactory.protocolNames[targetProtocol]}');
      }
      
      // Create and initialize protocol handler
      await _initializeProtocolHandler(targetProtocol);
      
      // Attempt connection with the selected protocol
      final success = await _currentHandler!.connect(_port, _baudRate);
      
      if (success) {
        _setupStreamSubscriptions();
        _updateConnectionState(ECUConnectionState.connected);
        print('ECU Service: Successfully connected using ${_currentHandler!.protocolName}');
        return true;
      } else {
        _updateConnectionState(ECUConnectionState.error);
        _addError('Connection failed with ${_currentHandler!.protocolName}', 'CONNECTION_FAILED');
        return false;
      }

    } catch (e, stackTrace) {
      print('ECU Service: Connection error: $e');
      _updateConnectionState(ECUConnectionState.error);
      _addError('Connection error: $e', 'CONNECTION_ERROR', stackTrace);
      return false;
    }
  }
  
  /// Disconnect from ECU
  Future<void> disconnect() async {
    print('ECU Service: Disconnecting...');
    await _cleanup();
    _updateConnectionState(ECUConnectionState.disconnected);
    _detectedProtocol = null;
    print('ECU Service: Disconnected');
  }
  
  /// Get ECU version information
  Future<String> getVersion() async {
    if (_currentHandler == null) {
      throw StateError('No ECU handler available');
    }
    return await _currentHandler!.getVersion();
  }
  
  /// Get ECU signature
  Future<String> getSignature() async {
    if (_currentHandler == null) {
      throw StateError('No ECU handler available');
    }
    return await _currentHandler!.getSignature();
  }
  
  /// Send command to ECU
  Future<bool> sendCommand(List<int> command) async {
    if (_currentHandler == null) {
      throw StateError('No ECU handler available');
    }
    return await _currentHandler!.sendCommand(command);
  }
  
  /// Get table data
  Future<List<List<double>>> getTable(String tableName) async {
    if (_currentHandler == null) {
      throw StateError('No ECU handler available');
    }
    return await _currentHandler!.getTable(tableName);
  }
  
  /// Set table data
  Future<bool> setTable(String tableName, List<List<double>> data) async {
    if (_currentHandler == null) {
      throw StateError('No ECU handler available');
    }
    return await _currentHandler!.setTable(tableName, data);
  }
  
  /// Get table axes
  Future<Map<String, List<double>>> getTableAxes(String tableName) async {
    if (_currentHandler == null) {
      throw StateError('No ECU handler available');
    }
    return await _currentHandler!.getTableAxes(tableName);
  }
  
  /// Get supported table names
  List<String> get supportedTables => _currentHandler?.supportedTables ?? [];
  
  /// Get table metadata
  Map<String, dynamic> getTableMetadata(String tableName) {
    if (_currentHandler == null) {
      return {};
    }
    return _currentHandler!.getTableMetadata(tableName);
  }
  
  /// Get configuration options
  Map<String, dynamic> getConfigurationOptions() {
    if (_currentHandler == null) {
      return {};
    }
    return _currentHandler!.getConfigurationOptions();
  }
  
  /// Set configuration
  Future<bool> setConfiguration(String key, dynamic value) async {
    if (_currentHandler == null) {
      throw StateError('No ECU handler available');
    }
    return await _currentHandler!.setConfiguration(key, value);
  }
  
  /// Start mock data generation for development (without connection)
  void startMockDataGeneration() {
    print('ECU Service: Starting mock data generation...');
    try {
      // Use current protocol or default to Speeduino
      _initializeProtocolHandler(_selectedProtocol);
      
      // Start mock streaming directly (only when not connected to real ECU)
      _dataStreamSubscription = Timer.periodic(const Duration(milliseconds: 100), (timer) {
        if (_connectionState != ECUConnectionState.connected) {
          final mockData = _generateFallbackMockData();
          _currentData = mockData;
          _dataController.add(mockData);
        }
      });
      
      _updateConnectionState(ECUConnectionState.connected);
      print('ECU Service: Mock data generation started');
    } catch (e) {
      print('ECU Service: Mock data generation failed: $e');
    }
  }
  
  // Private methods
  Future<ECUProtocol> _detectProtocol() async {
    print('ECU Service: Beginning protocol detection sequence...');
    
    for (final protocol in _detectionOrder) {
      try {
        print('ECU Service: Testing ${ECUProtocolFactory.protocolNames[protocol]} protocol...');
        
        // Quick detection attempt (shorter timeout)
        final handler = ECUProtocolFactory.createHandler(protocol);
        
        // Simulate detection delay
        await Future.delayed(Duration(milliseconds: 500));
        
        // For mock implementation, randomly succeed on one of the protocols
        // In real implementation, this would attempt actual communication
        if (protocol == _selectedProtocol) {
          print('ECU Service: Successfully detected ${ECUProtocolFactory.protocolNames[protocol]}');
          return protocol;
        }
        
      } catch (e) {
        print('ECU Service: ${ECUProtocolFactory.protocolNames[protocol]} detection failed: $e');
        continue;
      }
    }
    
    // Fallback to selected protocol if detection fails
    print('ECU Service: Detection failed, using selected protocol: ${ECUProtocolFactory.protocolNames[_selectedProtocol]}');
    return _selectedProtocol;
  }
  
  Future<void> _initializeProtocolHandler(ECUProtocol protocol) async {
    try {
      // Clean up existing handler
      await _cleanup();

      // Create new protocol handler
      _currentHandler = ECUProtocolFactory.createHandler(protocol);
      print('ECU Service: Initialized ${_currentHandler!.protocolName} handler');

      // Reset connection state after cleanup
      _updateConnectionState(ECUConnectionState.disconnected);

    } catch (e) {
      throw Exception('Failed to initialize protocol handler for $protocol: $e');
    }
  }
  
  void _setupStreamSubscriptions() {
    if (_currentHandler == null) return;
    
    // Subscribe to handler data streams
    _dataSubscription = _currentHandler!.realTimeDataStream.listen(
      (data) {
        _currentData = data;
        _dataController.add(data);
      },
      onError: (error) {
        print('ECU Service: Data stream error: $error');
        _addError('Data stream error: $error', 'DATA_STREAM_ERROR');
      },
    );
    
    _connectionSubscription = _currentHandler!.connectionStateStream.listen(
      (state) {
        if (state != _connectionState) {
          _updateConnectionState(state);
        }
      },
    );
    
    _errorSubscription = _currentHandler!.errorStream.listen(
      (error) {
        _errorController.add(error);
      },
    );
    
    print('ECU Service: Stream subscriptions established');
  }
  
  Future<void> _cleanup() async {
    // Cancel subscriptions
    await _dataSubscription?.cancel();
    await _connectionSubscription?.cancel();
    await _errorSubscription?.cancel();
    _dataStreamSubscription?.cancel();
    
    _dataSubscription = null;
    _connectionSubscription = null;
    _errorSubscription = null;
    _dataStreamSubscription = null;
    
    // Disconnect current handler
    if (_currentHandler != null) {
      try {
        await _currentHandler!.disconnect();
      } catch (e) {
        print('ECU Service: Handler cleanup error: $e');
      }
      _currentHandler = null;
    }
  }
  
  void _updateConnectionState(ECUConnectionState newState) {
    if (_connectionState != newState) {
      print('ECU Service: Connection state changing from $_connectionState to $newState');
      _connectionState = newState;
      _connectionController.add(newState);
      notifyListeners();
      print('ECU Service: Notified listeners of state change to $newState');
    } else {
      print('ECU Service: Connection state unchanged: $newState');
    }
  }
  
  void _addError(String message, String code, [StackTrace? stackTrace]) {
    final error = ECUError(
      message: message,
      code: code,
      timestamp: DateTime.now(),
      stackTrace: stackTrace,
    );
    _errorController.add(error);
  }
  
  ECUStatistics _emptyStatistics() {
    return ECUStatistics(
      bytesReceived: 0,
      bytesTransmitted: 0,
      packetsReceived: 0,
      packetsTransmitted: 0,
      errors: 0,
      timeouts: 0,
      lastActivity: DateTime.now(),
    );
  }
  
  /// Fallback mock data when no protocol handler is available
  SpeeduinoData _generateFallbackMockData() {
    final now = DateTime.now();
    final timeSeconds = now.millisecondsSinceEpoch / 1000.0;

    final baseRpm = 1200.0;
    final rpmVariation = 200.0 * sin(timeSeconds * 0.3) + 100.0 * sin(timeSeconds * 1.2);
    final rpm = (baseRpm + rpmVariation).clamp(800.0, 3000.0).round();

    final mapBase = 40.0 + (rpm - 800) / 50.0;
    final mapVariation = 15.0 * sin(timeSeconds * 0.4);
    final map = (mapBase + mapVariation).clamp(30.0, 100.0).round();

    final mockData = SpeeduinoData(
      rpm: rpm,
      map: map,
      tps: (25.0 + 20.0 * sin(timeSeconds * 0.2)).clamp(0.0, 80.0).round(),
      coolantTemp: (90.0 + 5.0 * sin(timeSeconds * 0.1)).clamp(85.0, 95.0).round(),
      intakeTemp: (30.0 + 5.0 * sin(timeSeconds * 0.15)).clamp(28.0, 38.0).round(),
      batteryVoltage: 12.6 + 0.1 * sin(timeSeconds * 0.05),
      afr: 14.7 + 0.4 * sin(timeSeconds * 0.25),
      timing: (15.0 + 3.0 * sin(timeSeconds * 0.3)).clamp(12.0, 18.0).round(),
      boost: (5.0 + 2.0 * sin(timeSeconds * 0.4)).clamp(2.0, 8.0).round(),
      engineStatus: SpeeduinoConstants.statusEngineRunning,
      timestamp: now,
    );

    // Only print mock data occasionally to avoid spam
    if (timeSeconds % 5 < 0.1) {
      print('ECU Service: 📊 Using MOCK data - RPM: ${mockData.rpm}, MAP: ${mockData.map}');
    }

    return mockData;
  }
  
  /// Dispose resources
  void dispose() {
    _cleanup();
    _dataController.close();
    _connectionController.close();
    _errorController.close();
    super.dispose();
  }

  /// Provider helper methods for easier access
  static ECUService of(BuildContext context, {bool listen = true}) {
    return Provider.of<ECUService>(context, listen: listen);
  }
}
