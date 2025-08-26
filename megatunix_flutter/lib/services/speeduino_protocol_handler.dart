/// Speeduino Protocol Handler
/// Implements Speeduino-specific communication protocol
/// Based on the existing C++ implementation and Speeduino protocol

import 'dart:async';
import 'dart:math';
import '../models/ecu_data.dart';
import 'ecu_protocol_handler.dart';

/// Speeduino Protocol Handler Implementation
class SpeeduinoProtocolHandler implements ECUProtocolHandler {
  // Speeduino-specific constants (from existing implementation)
  static const int _startByte = 0x72;    // 'r' - Start of packet
  static const int _stopByte = 0x03;     // ETX - End of packet
  static const int _escapeByte = 0x2D;   // '-' - Escape character
  
  // Speeduino commands
  static const int _cmdQuery = 0x51;     // 'Q' - Query command
  static const int _cmdGetData = 0x41;   // 'A' - Get real-time data
  static const int _cmdGetVersion = 0x53; // 'S' - Get version info
  static const int _cmdGetSignature = 0x56; // 'V' - Get signature
  
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
  ECUProtocol get protocol => ECUProtocol.speeduino;
  
  @override
  String get protocolName => 'Speeduino';
  
  @override
  List<int> get supportedBaudRates => [115200, 230400, 460800];
  
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
  ];
  
  @override
  Future<bool> connect(String port, int baudRate) async {
    try {
      _updateConnectionState(ECUConnectionState.connecting);
      
      // Simulate Speeduino connection delay
      await Future.delayed(const Duration(seconds: 2));
      
      // Mock successful connection
      _updateConnectionState(ECUConnectionState.connected);
      _startMockDataStreaming();
      return true;
      
    } catch (e, stackTrace) {
      _updateConnectionState(ECUConnectionState.error);
      _addError('Speeduino connection error: $e', 'SPEEDUINO_CONNECTION_ERROR', stackTrace);
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
    // Mock Speeduino version
    return 'Speeduino v202501.4';
  }
  
  @override
  Future<String> getSignature() async {
    // Mock Speeduino signature
    return 'Speeduino_202501.4_2024';
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
    // Mock table data for Speeduino
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
    // Speeduino-specific axis ranges
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
    
    // Speeduino-specific validation
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
      'Fuel_Type': ['Gasoline', 'E85', 'Methanol'],
      'Engine_Displacement': '2.0L',
      'Cylinder_Count': 4,
      'Max_RPM': 8000,
      'Max_Boost': 25.0,
      'Launch_Control': true,
      'Flat_Shift': false,
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
        final mockData = _generateMockSpeeduinoData();
        _dataController.add(mockData);
        _updateStatistics(bytesReceived: 20, packetsReceived: 1);
      } else {
        timer.cancel();
      }
    });
  }
  
  SpeeduinoData _generateMockSpeeduinoData() {
    // Generate realistic running vehicle data
    final now = DateTime.now();
    final timeFactor = now.millisecondsSinceEpoch / 3000.0; // Slower, more realistic changes
    
    // Simulate realistic engine behavior
    final baseRPM = 800.0; // Idle RPM
    final rpmVariation = sin(timeFactor * 0.2) * 0.3; // Gentle RPM variation
    final rpm = (baseRPM + (rpmVariation * 2000)).clamp(800.0, 3000.0);
    
    // MAP varies with RPM and load
    final mapBase = 30.0 + (rpm - 800) * 0.02; // MAP increases with RPM
    final mapVariation = sin(timeFactor * 0.3) * 10.0;
    final map = (mapBase + mapVariation).clamp(25.0, 80.0);
    
    // TPS varies with driving simulation
    final tpsBase = 15.0 + (rpm - 800) * 0.01; // Slight TPS increase with RPM
    final tpsVariation = sin(timeFactor * 0.4) * 8.0;
    final tps = (tpsBase + tpsVariation).clamp(10.0, 35.0);
    
    // Coolant temp - realistic warm-up and operation
    final tempBase = 85.0 + (rpm - 800) * 0.01; // Temp increases slightly with RPM
    final tempVariation = sin(timeFactor * 0.1) * 5.0;
    final coolantTemp = (tempBase + tempVariation).clamp(80.0, 95.0);
    
    // Intake temp - varies with ambient and engine load
    final intakeTemp = 25.0 + (map - 30) * 0.2 + sin(timeFactor * 0.15) * 3.0;
    
    // Battery voltage - realistic charging system
    final batteryBase = 13.8 + (rpm - 800) * 0.0005; // Slightly higher at higher RPM
    final batteryVariation = sin(timeFactor * 0.05) * 0.2;
    final batteryVoltage = (batteryBase + batteryVariation).clamp(12.8, 14.2);
    
    // AFR - realistic stoichiometric operation
    final afrBase = 14.7; // Stoichiometric
    final afrVariation = sin(timeFactor * 0.6) * 0.8;
    final afr = (afrBase + afrVariation).clamp(13.5, 15.5);
    
    // Timing - realistic ignition advance
    final timingBase = 12.0 + (rpm - 800) * 0.008; // Advance with RPM
    final timingVariation = sin(timeFactor * 0.25) * 3.0;
    final timing = (timingBase + timingVariation).clamp(8.0, 25.0);
    
    // Boost - realistic turbo behavior (if applicable)
    final boostBase = (rpm > 2000) ? (rpm - 2000) * 0.01 : 0.0;
    final boostVariation = sin(timeFactor * 0.35) * 2.0;
    final boost = (boostBase + boostVariation).clamp(0.0, 8.0);

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
