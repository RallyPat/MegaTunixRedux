/// EpicECU Protocol Handler
/// Implements EpicECU-specific communication protocol
/// Based on EpicECU documentation and INI file specifications

import 'dart:async';
import 'dart:math';
import '../models/ecu_data.dart';
import 'ecu_protocol_handler.dart';

/// EpicECU Protocol Handler Implementation
class EpicECUProtocolHandler implements ECUProtocolHandler {
  // EpicECU-specific constants
  static const int _startByte = 0xAA;
  static const int _stopByte = 0x55;
  static const int _escapeByte = 0x7D;
  
  // EpicECU commands
  static const int _cmdGetVersion = 0x01;
  static const int _cmdGetSignature = 0x02;
  static const int _cmdGetRealTimeData = 0x03;
  static const int _cmdGetTable = 0x04;
  static const int _cmdSetTable = 0x05;
  static const int _cmdGetConfig = 0x06;
  static const int _cmdSetConfig = 0x07;
  
  // Connection state
  ECUConnectionState _connectionState = ECUConnectionState.disconnected;
  Timer? _mockDataTimer;
  
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
  
  @override
  ECUProtocol get protocol => ECUProtocol.epicEFI;
  
  @override
  String get protocolName => 'EpicECU';
  
  @override
  List<int> get supportedBaudRates => [115200, 230400, 460800, 921600];
  
  @override
  int get defaultBaudRate => 115200;
  
  @override
  ECUConnectionState get connectionState => _connectionState;
  
  @override
  ECUStatistics get statistics => _statistics;
  
  @override
  Stream<SpeeduinoData> get realTimeDataStream => _dataController.stream;
  
  @override
  Stream<ECUConnectionState> get connectionStateStream => _connectionController.stream;
  
  @override
  Stream<ECUError> get errorStream => _errorController.stream;
  
  @override
  List<String> get supportedTables => [
    'VE_Table',
    'Ignition_Table',
    'AFR_Table',
    'Boost_Table',
    'Launch_Table',
    'Flat_Shift_Table',
  ];
  
  @override
  Future<bool> connect(String port, int baudRate) async {
    try {
      _updateConnectionState(ECUConnectionState.connecting);
      
      // Simulate EpicECU connection delay
      await Future.delayed(const Duration(seconds: 1));
      
      // Mock successful connection
      _updateConnectionState(ECUConnectionState.connected);
      _startMockDataStreaming();
      return true;
      
    } catch (e, stackTrace) {
      _updateConnectionState(ECUConnectionState.error);
      _addError('EpicECU connection error: $e', 'EPIC_CONNECTION_ERROR', stackTrace);
      return false;
    }
  }
  
  @override
  Future<void> disconnect() async {
    _mockDataTimer?.cancel();
    _updateConnectionState(ECUConnectionState.disconnected);
  }
  
  @override
  Future<String> getVersion() async {
    // Mock EpicECU version
    return 'EpicECU v2.1.0';
  }
  
  @override
  Future<String> getSignature() async {
    // Mock EpicECU signature
    return 'EpicECU_2.1.0_2024';
  }
  
  @override
  Future<bool> sendCommand(List<int> command) async {
    try {
      // Mock command sending
      await Future.delayed(const Duration(milliseconds: 50));
      _updateStatistics(bytesTransmitted: command.length, packetsTransmitted: 1);
      return true;
    } catch (e) {
      _addError('Command send error: $e', 'COMMAND_ERROR');
      return false;
    }
  }
  
  @override
  Future<List<List<double>>> getTable(String tableName) async {
    // Mock table data for EpicECU
    switch (tableName) {
      case 'VE_Table':
        return _generateMockVETable();
      case 'Ignition_Table':
        return _generateMockIgnitionTable();
      default:
        return List.generate(16, (i) => List.generate(16, (j) => 50.0 + (i + j) * 2.0));
    }
  }
  
  @override
  Future<bool> setTable(String tableName, List<List<double>> data) async {
    try {
      if (!validateTableData(tableName, data)) {
        throw ArgumentError('Invalid table data for $tableName');
      }
      
      // Mock table update
      await Future.delayed(const Duration(milliseconds: 100));
      return true;
    } catch (e) {
      _addError('Table set error: $e', 'TABLE_SET_ERROR');
      return false;
    }
  }
  
  @override
  Future<Map<String, List<double>>> getTableAxes(String tableName) async {
    // EpicECU-specific axis ranges
    switch (tableName) {
      case 'VE_Table':
      case 'Ignition_Table':
        return {
          'RPM': List.generate(16, (i) => 500.0 + i * 500.0), // 500-8000 RPM
          'MAP': List.generate(16, (i) => 20.0 + i * 15.0),   // 20-245 kPa
        };
      default:
        return {
          'X': List.generate(16, (i) => i * 10.0),
          'Y': List.generate(16, (i) => i * 10.0),
        };
    }
  }
  
  @override
  bool validateTableData(String tableName, List<List<double>> data) {
    if (data.isEmpty || data[0].isEmpty) return false;
    
    // EpicECU-specific validation
    switch (tableName) {
      case 'VE_Table':
        return data.length == 16 && data[0].length == 16 && 
               data.every((row) => row.every((value) => value >= 0.0 && value <= 255.0));
      case 'Ignition_Table':
        return data.length == 16 && data[0].length == 16 && 
               data.every((row) => row.every((value) => value >= -20.0 && value <= 60.0));
      default:
        return data.length > 0 && data[0].length > 0;
    }
  }
  
  @override
  Map<String, dynamic> getConfigurationOptions() {
    return {
      'Fuel_Type': ['Gasoline', 'E85', 'Methanol', 'Diesel'],
      'Engine_Displacement': '2.0L',
      'Cylinder_Count': 4,
      'Max_RPM': 8000,
      'Max_Boost': 30.0,
      'Launch_Control': true,
      'Flat_Shift': true,
      'Anti_Lag': false,
    };
  }
  
  @override
  Future<bool> setConfiguration(String key, dynamic value) async {
    try {
      // Mock configuration update
      await Future.delayed(const Duration(milliseconds: 50));
      return true;
    } catch (e) {
      _addError('Configuration set error: $e', 'CONFIG_SET_ERROR');
      return false;
    }
  }
  
  @override
  Map<String, dynamic> getTableMetadata(String tableName) {
    switch (tableName) {
      case 'VE_Table':
        return {
          'dimensions': [16, 16],
          'x_axis': 'RPM',
          'y_axis': 'MAP',
          'unit': 'VE %',
          'min_value': 0.0,
          'max_value': 255.0,
          'description': 'Volumetric Efficiency Table',
        };
      case 'Ignition_Table':
        return {
          'dimensions': [16, 16],
          'x_axis': 'RPM',
          'y_axis': 'MAP',
          'unit': 'Degrees BTDC',
          'min_value': -20.0,
          'max_value': 60.0,
          'description': 'Ignition Timing Table',
        };
      default:
        return {
          'dimensions': [16, 16],
          'description': 'Generic Table',
        };
    }
  }
  
  // Private helper methods
  void _updateConnectionState(ECUConnectionState state) {
    _connectionState = state;
    _connectionController.add(state);
  }
  
  void _addError(String message, String code, [StackTrace? stackTrace]) {
    final error = ECUError(
      message: message,
      code: code,
      timestamp: DateTime.now(),
      stackTrace: stackTrace,
    );
    _errorController.add(error);
    _statistics = ECUStatistics(
      bytesReceived: _statistics.bytesReceived,
      bytesTransmitted: _statistics.bytesTransmitted,
      packetsReceived: _statistics.packetsReceived,
      packetsTransmitted: _statistics.packetsTransmitted,
      errors: _statistics.errors + 1,
      timeouts: _statistics.timeouts,
      lastActivity: DateTime.now(),
    );
  }
  
  void _updateStatistics({int? bytesReceived, int? bytesTransmitted, int? packetsReceived, int? packetsTransmitted}) {
    _statistics = ECUStatistics(
      bytesReceived: _statistics.bytesReceived + (bytesReceived ?? 0),
      bytesTransmitted: _statistics.bytesTransmitted + (bytesTransmitted ?? 0),
      packetsReceived: _statistics.packetsReceived + (packetsReceived ?? 0),
      packetsTransmitted: _statistics.packetsTransmitted + (packetsTransmitted ?? 0),
      errors: _statistics.errors,
      timeouts: _statistics.timeouts,
      lastActivity: DateTime.now(),
    );
  }
  
  void _startMockDataStreaming() {
    _mockDataTimer = Timer.periodic(const Duration(milliseconds: 500), (timer) {
      if (_connectionState == ECUConnectionState.connected) {
        final mockData = _generateMockEpicECUData();
        _dataController.add(mockData);
        _updateStatistics(bytesReceived: 24, packetsReceived: 1);
      } else {
        timer.cancel();
      }
    });
  }
  
  SpeeduinoData _generateMockEpicECUData() {
    // Generate realistic running vehicle data (EpicECU variant)
    final now = DateTime.now();
    final timeFactor = now.millisecondsSinceEpoch / 3000.0; // Slower, more realistic changes
    
    // Simulate realistic engine behavior (EpicECU typically higher performance)
    final baseRPM = 900.0; // Slightly higher idle for performance ECU
    final rpmVariation = sin(timeFactor * 0.2) * 0.4; // More RPM variation
    final rpm = (baseRPM + (rpmVariation * 2500)).clamp(900.0, 3500.0);
    
    // MAP varies with RPM and load (EpicECU often higher MAP)
    final mapBase = 35.0 + (rpm - 900) * 0.025; // MAP increases with RPM
    final mapVariation = sin(timeFactor * 0.3) * 12.0;
    final map = (mapBase + mapVariation).clamp(30.0, 90.0);
    
    // TPS varies with driving simulation
    final tpsBase = 18.0 + (rpm - 900) * 0.012; // Slight TPS increase with RPM
    final tpsVariation = sin(timeFactor * 0.4) * 10.0;
    final tps = (tpsBase + tpsVariation).clamp(12.0, 40.0);
    
    // Coolant temp - realistic warm-up and operation
    final tempBase = 88.0 + (rpm - 900) * 0.012; // Temp increases slightly with RPM
    final tempVariation = sin(timeFactor * 0.1) * 6.0;
    final coolantTemp = (tempBase + tempVariation).clamp(82.0, 98.0);
    
    // Intake temp - varies with ambient and engine load
    final intakeTemp = 28.0 + (map - 35) * 0.22 + sin(timeFactor * 0.15) * 4.0;
    
    // Battery voltage - realistic charging system
    final batteryBase = 13.9 + (rpm - 900) * 0.0006; // Slightly higher at higher RPM
    final batteryVariation = sin(timeFactor * 0.05) * 0.25;
    final batteryVoltage = (batteryBase + batteryVariation).clamp(12.9, 14.3);
    
    // AFR - realistic stoichiometric operation (EpicECU often richer)
    final afrBase = 13.8; // Slightly richer than stoichiometric
    final afrVariation = sin(timeFactor * 0.6) * 1.0;
    final afr = (afrBase + afrVariation).clamp(12.8, 15.0);
    
    // Timing - realistic ignition advance (EpicECU often more aggressive)
    final timingBase = 14.0 + (rpm - 900) * 0.01; // More aggressive advance
    final timingVariation = sin(timeFactor * 0.25) * 4.0;
    final timing = (timingBase + timingVariation).clamp(10.0, 28.0);
    
    // Boost - realistic turbo behavior (EpicECU often higher boost)
    final boostBase = (rpm > 2200) ? (rpm - 2200) * 0.015 : 0.0;
    final boostVariation = sin(timeFactor * 0.35) * 3.0;
    final boost = (boostBase + boostVariation).clamp(0.0, 12.0);

    return SpeeduinoData(
      rpm: rpm.round(),
      map: map.round(),
      tps: tps.round(),
      coolantTemp: coolantTemp.round(),
      intakeTemp: intakeTemp.round(),
      batteryVoltage: batteryVoltage,
      afr: afr,
      timing: timing.round(),
      boost: boost.round(),
      engineStatus: 0x01, // Engine running
      timestamp: now,
    );
  }
  
  List<List<double>> _generateMockVETable() {
    return List.generate(16, (i) => 
      List.generate(16, (j) => 80.0 + (i * 0.5) + (j * 0.3) + (DateTime.now().millisecondsSinceEpoch % 10))
    );
  }
  
  List<List<double>> _generateMockIgnitionTable() {
    return List.generate(16, (i) => 
      List.generate(16, (j) => 15.0 + (i * 0.2) + (j * 0.1) + (DateTime.now().millisecondsSinceEpoch % 5 - 2.5))
    );
  }
}
