/// MegaSquirt Protocol Handler
/// Implements MegaSquirt MS1/MS2/MS3-specific communication protocol
/// Based on the C++ implementation and MegaSquirt protocol documentation

import 'dart:async';
import 'dart:math';
import '../models/ecu_data.dart';
import 'ecu_protocol_handler.dart';

/// MegaSquirt Protocol Constants
class MegaSquirtConstants {
  // MS1 Protocol Constants
  static const int ms1CmdWriteByteOffset = 0x77;     // 'w' - Write byte at offset
  static const int ms1CmdReadPageOffset = 0x72;      // 'r' - Read page at offset
  static const int ms1CmdBurnPageCommand = 0x62;     // 'b' - Burn flash page
  static const int ms1CmdGetPageActivate = 0x70;     // 'p' - Set page number
  static const int ms1CmdGetRevision = 0x51;         // 'Q' - Get firmware revision
  static const int ms1CmdGetPageData = 0x72;         // 'r' - Get page data
  static const int ms1CmdGetRTVars = 0x41;           // 'A' - Get real time vars
  
  // MS2/MS3 Protocol Constants (CAN-based)
  static const int ms2DefaultCanId = 0x00;
  static const int ms2TableIndexRTVars = 0x06;
  static const int ms2TableIndexVeTable = 0x04;
  static const int ms2TableIndexIgnTable = 0x05;
  static const int ms2CmdGetCanId = 0x72;            // 'r' - Read CAN data
  static const int ms2CmdSetCanId = 0x77;            // 'w' - Write CAN data
  static const int ms2CmdGetVersion = 0x51;          // 'Q' - Get version
  
  // MegaSquirt Protocol Versions
  static const String ms1Protocol = 'MS1';
  static const String ms2Protocol = 'MS2';
  static const String ms3Protocol = 'MS3';
}

/// MegaSquirt Protocol Handler Implementation
class MegaSquirtProtocolHandler implements ECUProtocolHandler {
  // MegaSquirt-specific properties
  String _msVersion = 'MS1';  // Default to MS1, will be detected
  int _canId = MegaSquirtConstants.ms2DefaultCanId;
  Timer? _mockDataTimer;
  
  // Connection state
  ECUConnectionState _connectionState = ECUConnectionState.disconnected;
  
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
  ECUProtocol get protocol => ECUProtocol.megasquirt;
  
  @override
  String get protocolName => 'MegaSquirt $_msVersion';
  
  @override
  List<int> get supportedBaudRates => [9600, 19200, 38400, 57600, 115200, 230400];
  
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
    'Warmup_Table',
    'Accel_Enrich_Table',
  ];
  
  @override
  Future<bool> connect(String port, int baudRate) async {
    try {
      _updateConnectionState(ECUConnectionState.connecting);
      
      // Simulate MegaSquirt connection sequence
      await Future.delayed(const Duration(milliseconds: 500));
      
      // Detect MegaSquirt version
      _msVersion = await _detectMSVersion();
      
      // Additional connection delay for MS detection
      await Future.delayed(const Duration(milliseconds: 1500));
      
      _updateConnectionState(ECUConnectionState.connected);
      _startMockDataStreaming();
      print('MegaSquirt $_msVersion connected successfully');
      return true;
      
    } catch (e, stackTrace) {
      _updateConnectionState(ECUConnectionState.error);
      _addError('MegaSquirt connection error: $e', 'MS_CONNECTION_ERROR', stackTrace);
      return false;
    }
  }
  
  @override
  Future<void> disconnect() async {
    _mockDataTimer?.cancel();
    _mockDataTimer = null;
    _updateConnectionState(ECUConnectionState.disconnected);
    print('MegaSquirt $_msVersion disconnected');
  }
  
  @override
  Future<String> getVersion() async {
    switch (_msVersion) {
      case 'MS1':
        return 'MegaSquirt I v2.905';
      case 'MS2':
        return 'MegaSquirt II v3.1.0';
      case 'MS3':
        return 'MegaSquirt III v1.4.2';
      default:
        return 'MegaSquirt v?.?.?';
    }
  }
  
  @override
  Future<String> getSignature() async {
    switch (_msVersion) {
      case 'MS1':
        return 'MS1_Extra_2.905';
      case 'MS2':  
        return 'MS2_Extra_3.1.0';
      case 'MS3':
        return 'MS3_1.4.2_Standard';
      default:
        return 'MegaSquirt_Unknown';
    }
  }
  
  @override
  Future<bool> sendCommand(List<int> command) async {
    try {
      // Mock command sending with MegaSquirt-specific delays
      await Future.delayed(const Duration(milliseconds: 50));
      
      // Different handling for MS1 vs MS2/MS3
      if (_msVersion == 'MS1') {
        // MS1 uses simple serial protocol
        _updateStatistics(bytesTransmitted: command.length, packetsTransmitted: 1);
      } else {
        // MS2/MS3 uses CAN protocol wrapped in serial
        _updateStatistics(bytesTransmitted: command.length + 8, packetsTransmitted: 1); // +8 for CAN header
      }
      
      return true;
    } catch (e) {
      _addError('MegaSquirt command send error: $e', 'MS_COMMAND_ERROR');
      return false;
    }
  }
  
  @override
  Future<List<List<double>>> getTable(String tableName) async {
    // MegaSquirt-specific table data generation
    switch (tableName) {
      case 'VE_Table':
        return _generateMSVETable();
      case 'Ignition_Table':
        return _generateMSIgnitionTable();
      case 'AFR_Table':
        return _generateMSAFRTable();
      case 'Boost_Table':
        return _generateMSBoostTable();
      default:
        // Default 12x12 table (common MS size)
        return List.generate(12, (i) => List.generate(12, (j) => 50.0 + (i + j) * 2.0));
    }
  }
  
  @override
  Future<bool> setTable(String tableName, List<List<double>> data) async {
    try {
      if (!validateTableData(tableName, data)) {
        throw ArgumentError('Invalid MegaSquirt table data for $tableName');
      }
      
      // MegaSquirt table update simulation
      int updateDelay = _msVersion == 'MS1' ? 200 : 150; // MS1 is slower
      await Future.delayed(Duration(milliseconds: updateDelay));
      
      print('MegaSquirt $_msVersion table $tableName updated');
      return true;
    } catch (e) {
      _addError('MegaSquirt table set error: $e', 'MS_TABLE_SET_ERROR');
      return false;
    }
  }
  
  @override
  Future<Map<String, List<double>>> getTableAxes(String tableName) async {
    // MegaSquirt-specific axis configurations
    switch (tableName) {
      case 'VE_Table':
      case 'Ignition_Table':
      case 'AFR_Table':
        if (_msVersion == 'MS1') {
          // MS1 uses 8x8 tables
          return {
            'RPM': List.generate(8, (i) => 400.0 + i * 750.0), // 400-5650 RPM
            'MAP': List.generate(8, (i) => 20.0 + i * 20.0),   // 20-160 kPa
          };
        } else {
          // MS2/MS3 uses 12x12 tables  
          return {
            'RPM': List.generate(12, (i) => 500.0 + i * 500.0), // 500-6000 RPM
            'MAP': List.generate(12, (i) => 20.0 + i * 15.0),   // 20-185 kPa
          };
        }
      case 'Boost_Table':
        return {
          'RPM': List.generate(8, (i) => 1000.0 + i * 750.0),  // 1000-6250 RPM
          'TPS': List.generate(8, (i) => 10.0 + i * 11.25),    // 10-88.75 TPS%
        };
      default:
        return {
          'X': List.generate(8, (i) => i * 10.0),
          'Y': List.generate(8, (i) => i * 10.0),
        };
    }
  }
  
  @override
  bool validateTableData(String tableName, List<List<double>> data) {
    if (data.isEmpty || data[0].isEmpty) return false;
    
    // MegaSquirt-specific validation
    switch (tableName) {
      case 'VE_Table':
        int expectedSize = _msVersion == 'MS1' ? 8 : 12;
        return data.length == expectedSize && data[0].length == expectedSize && 
               data.every((row) => row.every((value) => value >= 0.0 && value <= 255.0));
      case 'Ignition_Table':
        int expectedSize = _msVersion == 'MS1' ? 8 : 12;
        return data.length == expectedSize && data[0].length == expectedSize && 
               data.every((row) => row.every((value) => value >= -10.0 && value <= 50.0));
      case 'AFR_Table':
        int expectedSize = _msVersion == 'MS1' ? 8 : 12;
        return data.length == expectedSize && data[0].length == expectedSize && 
               data.every((row) => row.every((value) => value >= 10.0 && value <= 20.0));
      default:
        return data.length > 0 && data[0].length > 0;
    }
  }
  
  @override
  Map<String, dynamic> getConfigurationOptions() {
    // MegaSquirt-specific configuration options
    Map<String, dynamic> baseConfig = {
      'MS_Version': _msVersion,
      'Fuel_Type': ['Gasoline', 'E85', 'Methanol', 'Race Gas'],
      'Engine_Displacement': '2.0L',
      'Cylinder_Count': 4,
      'Max_RPM': _msVersion == 'MS1' ? 6400 : 8500,
      'Injection_Type': ['Port', 'Throttle Body', 'Sequential'],
      'Ignition_Type': ['Standard', 'Waste Spark', 'COP'],
    };
    
    // Add version-specific options
    if (_msVersion != 'MS1') {
      baseConfig.addAll({
        'CAN_ID': _canId,
        'Launch_Control': true,
        'Traction_Control': true,
        'Anti_Lag': false,
        'Boost_Control': true,
        'Water_Injection': false,
      });
    }
    
    return baseConfig;
  }
  
  @override
  Future<bool> setConfiguration(String key, dynamic value) async {
    try {
      // MegaSquirt configuration update simulation
      await Future.delayed(const Duration(milliseconds: 100));
      
      if (key == 'CAN_ID' && value is int) {
        _canId = value;
      }
      
      print('MegaSquirt $_msVersion configuration $key set to $value');
      return true;
    } catch (e) {
      _addError('MegaSquirt configuration set error: $e', 'MS_CONFIG_SET_ERROR');
      return false;
    }
  }
  
  @override
  Map<String, dynamic> getTableMetadata(String tableName) {
    int tableSize = _msVersion == 'MS1' ? 8 : 12;
    
    switch (tableName) {
      case 'VE_Table':
        return {
          'dimensions': [tableSize, tableSize],
          'x_axis': 'RPM',
          'y_axis': 'MAP',
          'unit': 'VE %',
          'min_value': 0.0,
          'max_value': 255.0,
          'description': 'MegaSquirt $_msVersion Volumetric Efficiency Table',
          'ms_version': _msVersion,
        };
      case 'Ignition_Table':
        return {
          'dimensions': [tableSize, tableSize],
          'x_axis': 'RPM', 
          'y_axis': 'MAP',
          'unit': 'Degrees BTDC',
          'min_value': -10.0,
          'max_value': 50.0,
          'description': 'MegaSquirt $_msVersion Ignition Timing Table',
          'ms_version': _msVersion,
        };
      case 'AFR_Table':
        return {
          'dimensions': [tableSize, tableSize],
          'x_axis': 'RPM',
          'y_axis': 'MAP', 
          'unit': 'AFR',
          'min_value': 10.0,
          'max_value': 20.0,
          'description': 'MegaSquirt $_msVersion Air/Fuel Ratio Table',
          'ms_version': _msVersion,
        };
      default:
        return {
          'dimensions': [tableSize, tableSize],
          'description': 'MegaSquirt $_msVersion Generic Table',
          'ms_version': _msVersion,
        };
    }
  }
  
  // Private helper methods
  Future<String> _detectMSVersion() async {
    // Mock version detection - in real implementation this would query the ECU
    final random = Random();
    final versions = ['MS1', 'MS2', 'MS3'];
    final detectedVersion = versions[random.nextInt(versions.length)];
    print('Detected MegaSquirt version: $detectedVersion');
    return detectedVersion;
  }
  
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
    // MegaSquirt typically streams data at 4-10 Hz
    int updateInterval = _msVersion == 'MS1' ? 250 : 200; // MS1 is slightly slower
    
    _mockDataTimer = Timer.periodic(Duration(milliseconds: updateInterval), (timer) {
      if (_connectionState == ECUConnectionState.connected) {
        final mockData = _generateMockMSData();
        _dataController.add(mockData);
        _updateStatistics(bytesReceived: _msVersion == 'MS1' ? 16 : 24, packetsReceived: 1);
      } else {
        timer.cancel();
      }
    });
  }
  
  SpeeduinoData _generateMockMSData() {
    // Generate MegaSquirt-specific realistic data
    final now = DateTime.now();
    final timeFactor = now.millisecondsSinceEpoch / 4000.0; // Slightly slower changes for MS
    
    // MegaSquirt tends to have slightly different operating characteristics
    final baseRPM = _msVersion == 'MS1' ? 750.0 : 850.0; // MS1 tends to idle lower
    final rpmVariation = sin(timeFactor * 0.25) * 0.4; 
    final rpm = (baseRPM + (rpmVariation * 1800)).clamp(600.0, 2800.0);
    
    // MegaSquirt MAP characteristics
    final mapBase = 35.0 + (rpm - 600) * 0.025;
    final mapVariation = sin(timeFactor * 0.35) * 12.0;
    final map = (mapBase + mapVariation).clamp(30.0, 85.0);
    
    // TPS with MegaSquirt response characteristics
    final tpsBase = 18.0 + (rpm - 600) * 0.008;
    final tpsVariation = sin(timeFactor * 0.45) * 10.0;
    final tps = (tpsBase + tpsVariation).clamp(12.0, 40.0);
    
    // MegaSquirt temperature characteristics
    final coolantTemp = (88.0 + (rpm - 600) * 0.008 + sin(timeFactor * 0.12) * 4.0).clamp(82.0, 98.0);
    final intakeTemp = 28.0 + (map - 30) * 0.15 + sin(timeFactor * 0.18) * 4.0;
    
    // MegaSquirt electrical characteristics
    final batteryBase = 13.9 + (rpm - 600) * 0.0003; 
    final batteryVariation = sin(timeFactor * 0.08) * 0.15;
    final batteryVoltage = (batteryBase + batteryVariation).clamp(13.2, 14.1);
    
    // AFR with MegaSquirt characteristics (slightly richer than Speeduino)
    final afrBase = 14.5; // Slightly rich default
    final afrVariation = sin(timeFactor * 0.55) * 0.9;
    final afr = (afrBase + afrVariation).clamp(13.2, 15.8);
    
    // MegaSquirt timing characteristics
    final timingBase = 14.0 + (rpm - 600) * 0.006;
    final timingVariation = sin(timeFactor * 0.3) * 2.5;
    final timing = (timingBase + timingVariation).clamp(10.0, 22.0);
    
    // Boost for MS2/MS3
    final boost = (_msVersion != 'MS1' && rpm > 1800) ? 
                  ((rpm - 1800) * 0.008 + sin(timeFactor * 0.4) * 1.5).clamp(0.0, 6.0) : 0.0;
    
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
  
  List<List<double>> _generateMSVETable() {
    int size = _msVersion == 'MS1' ? 8 : 12;
    return List.generate(size, (i) => 
      List.generate(size, (j) => 85.0 + (i * 0.8) + (j * 0.6) + (DateTime.now().millisecondsSinceEpoch % 8))
    );
  }
  
  List<List<double>> _generateMSIgnitionTable() {
    int size = _msVersion == 'MS1' ? 8 : 12;
    return List.generate(size, (i) => 
      List.generate(size, (j) => 18.0 + (i * 0.3) + (j * 0.2) + (DateTime.now().millisecondsSinceEpoch % 4 - 2.0))
    );
  }
  
  List<List<double>> _generateMSAFRTable() {
    int size = _msVersion == 'MS1' ? 8 : 12;
    return List.generate(size, (i) => 
      List.generate(size, (j) => 14.7 + (i * 0.05) + (j * -0.02) + (DateTime.now().millisecondsSinceEpoch % 3 - 1.5))
    );
  }
  
  List<List<double>> _generateMSBoostTable() {
    return List.generate(8, (i) => 
      List.generate(8, (j) => 5.0 + (i * 0.4) + (j * 0.3))
    );
  }
}
