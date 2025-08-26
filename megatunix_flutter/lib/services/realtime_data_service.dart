/// Real-time ECU Data Service for Live Table Cursor Updates
/// Provides streaming ECU data for cursor positioning and dashboard updates

import 'dart:async';
import 'dart:math';
import 'package:flutter/foundation.dart';
import '../models/ecu_data.dart';
import '../models/table_cursor.dart';

/// Engine operating states for realistic simulation
enum EngineState {
  idle,          // ~800 RPM, low MAP, closed throttle
  cruising,      // 2000-3000 RPM, moderate MAP, steady throttle
  accelerating,  // Rising RPM/MAP, increasing throttle
  decelerating,  // Falling RPM, high vacuum, closed throttle
  wot,          // Wide open throttle, high RPM/MAP
}

/// Real-time ECU data provider with cursor positioning
class RealtimeDataService extends ChangeNotifier {
  SpeeduinoData? _currentData;
  ECUConnectionState _connectionState = ECUConnectionState.disconnected;
  Timer? _dataTimer;
  Timer? _connectionTimer;
  
  // Cursor positioning data
  final Map<String, TableCursor> _tableCursors = {};
  
  // Stream controllers for data updates
  final StreamController<SpeeduinoData> _dataStreamController = StreamController<SpeeduinoData>.broadcast();
  final StreamController<ECUConnectionState> _connectionStreamController = StreamController<ECUConnectionState>.broadcast();
  
  // Engine simulation state
  EngineState _engineState = EngineState.idle;
  double _lastRpm = 800;
  double _lastMap = 40;
  double _lastTps = 0;
  double _throttleTarget = 0;
  DateTime _lastStateChange = DateTime.now();
  
  // Configuration
  static const Duration _updateInterval = Duration(milliseconds: 100); // 10Hz updates for visible trail
  static const Duration _connectionTimeout = Duration(seconds: 5);
  
  /// Current ECU data (null if not connected)
  SpeeduinoData? get currentData => _currentData;
  
  /// Current connection state
  ECUConnectionState get connectionState => _connectionState;
  
  /// Data stream for real-time updates
  Stream<SpeeduinoData> get dataStream => _dataStreamController.stream;
  
  /// Connection state stream
  Stream<ECUConnectionState> get connectionStream => _connectionStreamController.stream;
  
  /// Get cursor for specific table
  TableCursor getCursor(String tableName) {
    return _tableCursors[tableName] ?? const TableCursor.hidden();
  }
  
  /// Check if service is connected and receiving data
  bool get isConnected => _connectionState == ECUConnectionState.connected;
  
  /// Check if service has live data
  bool get hasLiveData => _currentData != null && isConnected;
  
  @override
  void dispose() {
    _dataTimer?.cancel();
    _connectionTimer?.cancel();
    _dataStreamController.close();
    _connectionStreamController.close();
    super.dispose();
  }
  
  /// Start real-time data service (simulated for now)
  Future<void> startService() async {
    if (_dataTimer != null) return;
    
    _setConnectionState(ECUConnectionState.connecting);
    
    // Simulate connection delay
    await Future.delayed(const Duration(milliseconds: 500));
    
    _setConnectionState(ECUConnectionState.connected);
    
    // Start data simulation timer
    _dataTimer = Timer.periodic(_updateInterval, (_) => _generateSimulatedData());
    
    // Start connection monitoring
    _connectionTimer = Timer.periodic(const Duration(seconds: 1), (_) => _monitorConnection());
  }
  
  /// Stop real-time data service
  Future<void> stopService() async {
    _dataTimer?.cancel();
    _connectionTimer?.cancel();
    _dataTimer = null;
    _connectionTimer = null;
    
    _setConnectionState(ECUConnectionState.disconnected);
    _currentData = null;
    _tableCursors.clear();
    
    notifyListeners();
  }
  
  /// Update cursor position for a specific table
  void updateTableCursor(String tableName, List<double> xAxis, List<double> yAxis, String xAxisType, String yAxisType) {
    if (_currentData == null) {
      _tableCursors[tableName] = const TableCursor.hidden();
      return;
    }
    
    // Map axis types to ECU data
    final xValue = _getAxisValue(xAxisType, _currentData!);
    final yValue = _getAxisValue(yAxisType, _currentData!);
    
    if (xValue == null || yValue == null) {
      _tableCursors[tableName] = const TableCursor.hidden();
      return;
    }
    
    // Calculate cursor position
    final position = TableCursorPosition.fromValues(xValue, yValue, xAxis, yAxis);
    
    // Update cursor with new position
    final currentCursor = _tableCursors[tableName] ?? const TableCursor.hidden();
    _tableCursors[tableName] = currentCursor.updatePosition(position);
  }
  
  /// Get ECU value for specific axis type
  double? _getAxisValue(String axisType, SpeeduinoData data) {
    switch (axisType.toLowerCase()) {
      case 'rpm':
        return data.rpm.toDouble();
      case 'map':
      case 'load':
      case 'pressure':
        return data.map.toDouble();
      case 'tps':
      case 'throttle':
        return data.tps.toDouble();
      case 'coolant':
      case 'clt':
        return data.coolantTemp.toDouble();
      case 'intake':
      case 'iat':
        return data.intakeTemp.toDouble();
      case 'battery':
      case 'volts':
        return data.batteryVoltage;
      case 'afr':
      case 'lambda':
        return data.afr;
      case 'timing':
      case 'advance':
        return data.timing.toDouble();
      case 'boost':
        return data.boost.toDouble();
      default:
        return null;
    }
  }
  
  /// Generate realistic simulated ECU data for development/testing
  void _generateSimulatedData() {
    final now = DateTime.now();
    final deltaTime = _updateInterval.inMilliseconds / 1000.0; // Time step in seconds
    
    // Update engine state based on time and current conditions
    _updateEngineState();
    
    // Generate smooth, realistic engine data based on current state
    final newData = _simulateEngineData(deltaTime);
    
    // Apply smooth transitions to prevent cursor jumping
    _lastRpm = _smoothTransition(_lastRpm, newData['rpm']!, deltaTime, 1000); // RPM changes quickly
    _lastMap = _smoothTransition(_lastMap, newData['map']!, deltaTime, 200);  // MAP changes fast
    _lastTps = _smoothTransition(_lastTps, newData['tps']!, deltaTime, 100);  // TPS changes very fast
    
    _currentData = SpeeduinoData(
      rpm: _lastRpm.round(),
      map: _lastMap.round(),
      tps: _lastTps.round(),
      coolantTemp: newData['coolantTemp']!.round(),
      intakeTemp: newData['intakeTemp']!.round(),
      batteryVoltage: newData['batteryVoltage']!,
      afr: newData['afr']!,
      timing: newData['timing']!.round(),
      boost: newData['boost']!.round(),
      engineStatus: newData['status']!.round(),
      timestamp: now,
    );
    
    // Broadcast data update
    _dataStreamController.add(_currentData!);
    notifyListeners();
  }
  
  void _updateEngineState() {
    final timeSinceChange = DateTime.now().difference(_lastStateChange);
    final random = Random();
    
    // Change states periodically with realistic transitions
    if (timeSinceChange.inSeconds > 3 + random.nextInt(5)) {
      switch (_engineState) {
        case EngineState.idle:
          _engineState = random.nextBool() ? EngineState.accelerating : EngineState.cruising;
          _throttleTarget = _engineState == EngineState.accelerating ? 0.6 : 0.3;
          break;
        case EngineState.cruising:
          if (random.nextDouble() < 0.4) {
            _engineState = EngineState.accelerating;
            _throttleTarget = 0.8;
          } else if (random.nextDouble() < 0.3) {
            _engineState = EngineState.decelerating;
            _throttleTarget = 0.0;
          }
          break;
        case EngineState.accelerating:
          if (random.nextDouble() < 0.3) {
            _engineState = EngineState.wot;
            _throttleTarget = 1.0;
          } else if (random.nextDouble() < 0.4) {
            _engineState = EngineState.cruising;
            _throttleTarget = 0.3;
          }
          break;
        case EngineState.decelerating:
          _engineState = random.nextBool() ? EngineState.idle : EngineState.cruising;
          _throttleTarget = _engineState == EngineState.idle ? 0.0 : 0.25;
          break;
        case EngineState.wot:
          _engineState = EngineState.decelerating;
          _throttleTarget = 0.0;
          break;
      }
      _lastStateChange = DateTime.now();
    }
  }
  
  Map<String, double> _simulateEngineData(double deltaTime) {
    final time = DateTime.now().millisecondsSinceEpoch / 1000.0;
    
    // Calculate target values based on engine state
    double targetRpm, targetMap, targetTps;
    
    switch (_engineState) {
      case EngineState.idle:
        targetRpm = 800 + sin(time * 2) * 50; // Idle fluctuation
        targetMap = 30 + sin(time * 1.5) * 8;  // Vacuum at idle
        targetTps = 0 + sin(time * 3) * 2;     // Closed throttle with slight variation
        break;
      case EngineState.cruising:
        targetRpm = 2200 + sin(time * 0.8) * 200;
        targetMap = 60 + sin(time * 1.2) * 15;
        targetTps = 15 + sin(time * 0.6) * 8;
        break;
      case EngineState.accelerating:
        final accelProgress = sin(time * 0.7) * 0.5 + 0.5; // 0-1
        targetRpm = 2000 + accelProgress * 3500;
        targetMap = 80 + accelProgress * 120;
        targetTps = 30 + accelProgress * 60;
        break;
      case EngineState.decelerating:
        final decelProgress = 1.0 - (sin(time * 1.2) * 0.5 + 0.5); // 1-0
        targetRpm = 1000 + decelProgress * 2500;
        targetMap = 25 + decelProgress * 40;  // High vacuum during decel
        targetTps = decelProgress * 10;       // Closing throttle
        break;
      case EngineState.wot:
        targetRpm = 4500 + sin(time * 1.5) * 1200;
        targetMap = 180 + sin(time * 2.2) * 30;
        targetTps = 85 + sin(time * 4) * 10;
        break;
    }
    
    // Add small realistic variations
    targetRpm += (Random().nextDouble() - 0.5) * 30;
    targetMap += (Random().nextDouble() - 0.5) * 5;
    targetTps += (Random().nextDouble() - 0.5) * 2;
    
    // Clamp to realistic ranges
    targetRpm = targetRpm.clamp(700, 7200);
    targetMap = targetMap.clamp(15, 250);
    targetTps = targetTps.clamp(0, 100);
    
    // Calculate dependent parameters
    final coolantTemp = (82 + sin(time * 0.05) * 8 + (targetRpm - 800) * 0.003).clamp(70, 105);
    final intakeTemp = (25 + sin(time * 0.08) * 12 + (targetTps * 0.3)).clamp(15, 65);
    final batteryVoltage = (13.8 + sin(time * 0.3) * 0.6 - (targetRpm > 3000 ? 0.2 : 0)).clamp(11.5, 14.8);
    
    // Realistic AFR based on load
    final baseAfr = targetTps > 80 ? 12.0 : 14.7; // Rich under heavy load
    final afr = baseAfr + sin(time * 2.5) * 0.4 + (Random().nextDouble() - 0.5) * 0.3;
    
    // Timing based on load and RPM
    final baseTiming = 18 - (targetMap - 50) * 0.08; // Retard under load
    final timing = (baseTiming + sin(time * 1.3) * 2).clamp(8, 35);
    
    // Boost simulation
    final boost = targetTps > 70 ? (targetTps - 70) * 3.0 : 0;
    
    // Engine status
    int status = 0;
    if (targetRpm > 400) status |= SpeeduinoConstants.statusEngineRunning;
    if (targetRpm > 300 && targetRpm < 500) status |= SpeeduinoConstants.statusEngineCranking;
    if (boost > 5) status |= SpeeduinoConstants.statusBoostControl;
    if (Random().nextDouble() < 0.002) status |= SpeeduinoConstants.statusKnockDetected;
    
    return {
      'rpm': targetRpm,
      'map': targetMap,
      'tps': targetTps,
      'coolantTemp': coolantTemp.toDouble(),
      'intakeTemp': intakeTemp.toDouble(),
      'batteryVoltage': batteryVoltage,
      'afr': afr,
      'timing': timing.toDouble(),
      'boost': boost.toDouble(),
      'status': status.toDouble(),
    };
  }
  
  /// Apply smooth transitions between values to prevent cursor jumping
  double _smoothTransition(double current, double target, double deltaTime, double maxChangeRate) {
    final maxChange = maxChangeRate * deltaTime;
    final difference = target - current;
    
    if (difference.abs() <= maxChange) {
      return target;
    } else {
      return current + (difference > 0 ? maxChange : -maxChange);
    }
  }
  
  /// Monitor connection health
  void _monitorConnection() {
    if (_currentData != null) {
      final age = DateTime.now().difference(_currentData!.timestamp);
      if (age > _connectionTimeout) {
        _setConnectionState(ECUConnectionState.error);
      }
    }
  }
  
  /// Update connection state and notify listeners
  void _setConnectionState(ECUConnectionState newState) {
    if (_connectionState != newState) {
      _connectionState = newState;
      _connectionStreamController.add(_connectionState);
      notifyListeners();
    }
  }
  
  /// Get current RPM for cursor positioning
  double? get currentRpm => _currentData?.rpm.toDouble();
  
  /// Get current MAP/Load for cursor positioning
  double? get currentLoad => _currentData?.map.toDouble();
  
  /// Get current TPS for cursor positioning
  double? get currentTps => _currentData?.tps.toDouble();
  
  /// Get display string for current operating point
  String get currentOperatingPoint {
    if (_currentData == null) return 'No Data';
    return '${_currentData!.rpm} RPM / ${_currentData!.map} kPa';
  }
  
  /// Get formatted display string for data value
  String getFormattedValue(String parameterName) {
    if (_currentData == null) return '--';
    
    switch (parameterName.toLowerCase()) {
      case 'rpm':
        return '${_currentData!.rpm}';
      case 'map':
      case 'load':
        return '${_currentData!.map} kPa';
      case 'tps':
        return '${_currentData!.tps}%';
      case 'coolant':
      case 'clt':
        return '${_currentData!.coolantTemp}°C';
      case 'intake':
      case 'iat':
        return '${_currentData!.intakeTemp}°C';
      case 'battery':
        return '${_currentData!.batteryVoltage.toStringAsFixed(1)}V';
      case 'afr':
        return _currentData!.afr.toStringAsFixed(1);
      case 'timing':
        return '${_currentData!.timing}°';
      case 'boost':
        return '${_currentData!.boost} kPa';
      default:
        return '--';
    }
  }
}