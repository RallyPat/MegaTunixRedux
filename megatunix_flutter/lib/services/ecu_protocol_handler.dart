/// Base ECU Protocol Handler Interface
/// Defines the contract that all ECU protocol handlers must implement
/// This ensures consistent behavior across different ECU types

import 'dart:async';
import '../models/ecu_data.dart';

/// Abstract base class for ECU protocol handlers
abstract class ECUProtocolHandler {
  /// Get the protocol type this handler supports
  ECUProtocol get protocol;
  
  /// Get the display name for this protocol
  String get protocolName;
  
  /// Get supported baud rates for this protocol
  List<int> get supportedBaudRates;
  
  /// Get default baud rate for this protocol
  int get defaultBaudRate;
  
  /// Connect to ECU using this protocol
  Future<bool> connect(String port, int baudRate);
  
  /// Disconnect from ECU
  Future<void> disconnect();
  
  /// Get ECU version information
  Future<String> getVersion();
  
  /// Get ECU signature/identifier
  Future<String> getSignature();
  
  /// Get real-time data stream
  Stream<SpeeduinoData> get realTimeDataStream;
  
  /// Get connection state stream
  Stream<ECUConnectionState> get connectionStateStream;
  
  /// Get error stream
  Stream<ECUError> get errorStream;
  
  /// Get current connection state
  ECUConnectionState get connectionState;
  
  /// Get connection statistics
  ECUStatistics get statistics;
  
  /// Send command to ECU
  Future<bool> sendCommand(List<int> command);
  
  /// Get table data (VE, ignition, etc.)
  Future<List<List<double>>> getTable(String tableName);
  
  /// Set table data
  Future<bool> setTable(String tableName, List<List<double>> data);
  
  /// Get table axis values (RPM, MAP, etc.)
  Future<Map<String, List<double>>> getTableAxes(String tableName);
  
  /// Validate table data for this ECU type
  bool validateTableData(String tableName, List<List<double>> data);
  
  /// Get ECU-specific configuration options
  Map<String, dynamic> getConfigurationOptions();
  
  /// Set ECU-specific configuration
  Future<bool> setConfiguration(String key, dynamic value);
  
  /// Get supported table names for this ECU
  List<String> get supportedTables;
  
  /// Get table metadata (dimensions, ranges, etc.)
  Map<String, dynamic> getTableMetadata(String tableName);
}
