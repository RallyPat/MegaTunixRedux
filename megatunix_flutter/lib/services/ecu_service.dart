/// ECU Service for Speeduino Communication
/// Handles CRC protocol, packet building, and simulated communication
/// Based on the existing working C++ implementation
/// Note: Currently uses mock implementation for development, can be replaced with real serial communication

import 'dart:async';
import 'dart:math';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:provider/single_child_widget.dart';
import '../models/ecu_data.dart';

/// ECU Service for managing ECU communication
class ECUService extends ChangeNotifier {
  static final ECUService _instance = ECUService._internal();
  factory ECUService() => _instance;
  ECUService._internal();

  // Connection state
  ECUConnectionState _connectionState = ECUConnectionState.disconnected;
  Timer? _mockDataTimer;
  StreamSubscription<List<int>>? _dataSubscription;
  
  // Data streams
  final StreamController<SpeeduinoData> _dataController = StreamController<SpeeduinoData>.broadcast();
  final StreamController<ECUConnectionState> _connectionController = StreamController<ECUConnectionState>.broadcast();
  final StreamController<ECUError> _errorController = StreamController<ECUError>.broadcast();
  
  // Statistics
  ECUStatistics _statistics = ECUStatistics(
    bytesReceived: 0,
    bytesTransmitted: 0,
    packetsReceived: 0,
    packetsTransmitted: 0,
    errors: 0,
    timeouts: 0,
    lastActivity: DateTime.now(),
  );
  
  // Configuration
  String _port = '/dev/ttyACM0';
  int _baudRate = 115200;
  ECUProtocol _protocol = ECUProtocol.speeduino;
  
  // Current data storage
  SpeeduinoData? _currentData;
  
  // Getters
  ECUConnectionState get connectionState => _connectionState;
  SpeeduinoData? get currentData => _currentData;
  Stream<SpeeduinoData> get dataStream => _dataController.stream;
  Stream<ECUConnectionState> get connectionStream => _connectionController.stream;
  Stream<ECUError> get errorStream => _errorController.stream;
  ECUStatistics get statistics => _statistics;
  String get port => _port;
  int get baudRate => _baudRate;
  ECUProtocol get protocol => _protocol;
  
  /// Connect to ECU using Speeduino protocol (Mock implementation)
  Future<bool> connect({
    String? port,
    int? baudRate,
    ECUProtocol? protocol,
  }) async {
    print('ECU Service: Attempting to connect...');
    try {
      // Update configuration if provided
      if (port != null) _port = port;
      if (baudRate != null) _baudRate = baudRate;
      if (protocol != null) _protocol = protocol;
      notifyListeners();

      // Update connection state
      _updateConnectionState(ECUConnectionState.connecting);
      print('ECU Service: Connection state set to connecting');

      // Simulate connection delay
      await Future.delayed(const Duration(seconds: 2));

      // Mock successful connection
      _updateConnectionState(ECUConnectionState.connected);
      print('ECU Service: Connection state set to connected');
      _startMockDataStreaming();
      print('ECU Service: Mock data streaming started');
      return true;

    } catch (e, stackTrace) {
      print('ECU Service: Connection error: $e');
      _updateConnectionState(ECUConnectionState.error);
      _addError('Connection error: $e', 'CONNECTION_ERROR', stackTrace);
      return false;
    }
  }
  
  /// Disconnect from ECU
  Future<void> disconnect() async {
    await _disconnect();
    _updateConnectionState(ECUConnectionState.disconnected);
  }
  
  /// Start mock data streaming for development
  void _startMockDataStreaming() {
    print('Starting mock data streaming at 60fps...');
    // Generate mock ECU data every 16.67ms for 60fps smooth animation
    _mockDataTimer = Timer.periodic(const Duration(milliseconds: 16), (timer) {
      if (_connectionState == ECUConnectionState.connected) {
        final mockData = _generateMockECUData();
        // Reduced logging for 60fps - only log every 100th update
        if (timer.tick % 100 == 0) {
          print('Generated mock data: RPM=${mockData.rpm}, MAP=${mockData.map}, TPS=${mockData.tps}');
        }
        _currentData = mockData; // Update current data
        _dataController.add(mockData);
        _updateStatistics(bytesReceived: 20, packetsReceived: 1);
        notifyListeners(); // Notify listeners of data change
      } else {
        print('Mock data timer: connection state is $_connectionState');
        timer.cancel();
      }
    });
  }
  
  /// Start mock data generation for development (without connection)
  void startMockDataGeneration() {
    print('Starting mock data generation without connection...');
    _updateConnectionState(ECUConnectionState.connected);
    _startMockDataStreaming();
  }
  
  /// Generate mock ECU data for development
  SpeeduinoData _generateMockECUData() {
    // Simulate realistic engine data with smooth, non-seizure-inducing values
    final now = DateTime.now();
    final timeSeconds = now.millisecondsSinceEpoch / 1000.0;
    
    // Create smooth, realistic engine simulation with subtle variations for 60fps
    // RPM: Simulate engine running with smooth variations
    final baseRpm = 1200.0;
    final rpmVariation = 200.0 * sin(timeSeconds * 0.3) + 100.0 * sin(timeSeconds * 1.2) + 50.0 * sin(timeSeconds * 0.8);
    final rpm = (baseRpm + rpmVariation).clamp(800.0, 3000.0).round();
    
    // MAP: Simulate manifold pressure with smooth changes
    final mapBase = 40.0 + (rpm - 800) / 50.0; // Higher RPM = higher MAP
    final mapVariation = 15.0 * sin(timeSeconds * 0.4) + 8.0 * sin(timeSeconds * 1.5);
    final map = (mapBase + mapVariation).clamp(30.0, 100.0).round();
    
    // TPS: Simulate throttle position with smooth changes
    final tpsBase = 25.0 + 20.0 * sin(timeSeconds * 0.2) + 10.0 * sin(timeSeconds * 0.9);
    final tps = tpsBase.clamp(0.0, 80.0).round();
    
    // Coolant temp: Simulate warm engine with smooth variation
    final coolantTemp = (90.0 + 5.0 * sin(timeSeconds * 0.1) + 2.0 * sin(timeSeconds * 0.6)).clamp(85.0, 95.0).round();
    
    // Intake temp: Simulate air temperature with smooth variation
    final intakeTemp = (30.0 + 5.0 * sin(timeSeconds * 0.15) + 3.0 * sin(timeSeconds * 0.8)).clamp(28.0, 38.0).round();
    
    // Battery voltage: Simulate stable battery with very subtle variations
    final batteryVoltage = 12.6 + 0.1 * sin(timeSeconds * 0.05) + 0.05 * sin(timeSeconds * 0.3);
    
    // AFR: Simulate air-fuel ratio with smooth variation
    final afr = 14.7 + 0.4 * sin(timeSeconds * 0.25) + 0.2 * sin(timeSeconds * 1.1);
    
    // Timing: Simulate ignition timing with smooth variation
    final timing = (15.0 + 3.0 * sin(timeSeconds * 0.3) + 1.5 * sin(timeSeconds * 0.9)).clamp(12.0, 18.0).round();
    
    // Boost: Simulate turbo boost with smooth variation
    final boost = (5.0 + 2.0 * sin(timeSeconds * 0.4) + 1.0 * sin(timeSeconds * 1.2)).clamp(2.0, 8.0).round();
    
    final engineStatus = SpeeduinoConstants.statusEngineRunning;
    
    return SpeeduinoData(
      rpm: rpm,
      map: map,
      tps: tps,
      coolantTemp: coolantTemp,
      intakeTemp: intakeTemp,
      batteryVoltage: batteryVoltage,
      afr: afr,
      timing: timing,
      boost: boost,
      engineStatus: engineStatus,
      timestamp: now,
    );
  }
  

  

  
  /// Update connection state
  void _updateConnectionState(ECUConnectionState newState) {
    if (_connectionState != newState) {
      _connectionState = newState;
      _connectionController.add(newState);
    }
  }
  
  /// Update statistics
  void _updateStatistics({
    int? bytesReceived,
    int? bytesTransmitted,
    int? packetsReceived,
    int? packetsTransmitted,
    int? errors,
    int? timeouts,
  }) {
    _statistics = ECUStatistics(
      bytesReceived: _statistics.bytesReceived + (bytesReceived ?? 0),
      bytesTransmitted: _statistics.bytesTransmitted + (bytesTransmitted ?? 0),
      packetsReceived: _statistics.packetsReceived + (packetsReceived ?? 0),
      packetsTransmitted: _statistics.packetsTransmitted + (packetsTransmitted ?? 0),
      errors: _statistics.errors + (errors ?? 0),
      timeouts: _statistics.timeouts + (timeouts ?? 0),
      lastActivity: DateTime.now(),
    );
  }
  
  /// Add error
  void _addError(String message, String code, [StackTrace? stackTrace]) {
    final error = ECUError(
      message: message,
      code: code,
      timestamp: DateTime.now(),
      stackTrace: stackTrace,
    );
    _errorController.add(error);
    _updateStatistics(errors: 1);
  }
  
  /// Internal disconnect method
  Future<void> _disconnect() async {
    try {
      // Cancel data subscription
      await _dataSubscription?.cancel();
      _dataSubscription = null;
      
      // Cancel mock data timer
      _mockDataTimer?.cancel();
      _mockDataTimer = null;
    } catch (e) {
      _addError('Disconnect error: $e', 'DISCONNECT_ERROR');
    }
  }
  
  /// Dispose resources
  void dispose() {
    _disconnect();
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
