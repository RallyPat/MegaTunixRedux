/// ECU Data Models for Speeduino Communication
/// Based on the existing C++ implementation and Speeduino protocol

import 'dart:typed_data';

/// ECU Connection State
enum ECUConnectionState {
  disconnected,
  connecting,
  connected,
  error,
}

/// ECU Protocol Type
enum ECUProtocol {
  speeduino,
  epicEFI,
  megasquirt,
}

/// Speeduino Protocol Constants
class SpeeduinoConstants {
  // Protocol bytes
  static const int startByte = 0x72;    // 'r' - Start of packet
  static const int stopByte = 0x03;     // ETX - End of packet
  static const int escapeByte = 0x2D;   // '-' - Escape character
  
  // Commands
  static const int cmdQuery = 0x51;     // 'Q' - Query command
  static const int cmdGetData = 0x41;   // 'A' - Get real-time data
  static const int cmdGetVersion = 0x53; // 'S' - Get version info
  static const int cmdGetSignature = 0x56; // 'V' - Get signature
  
  // Status bits
  static const int statusEngineRunning = 0x01;
  static const int statusEngineCranking = 0x02;
  static const int statusBoostControl = 0x04;
  static const int statusKnockDetected = 0x08;
  static const int statusCheckEngine = 0x10;
}

/// Speeduino Packet Structure
class SpeeduinoPacket {
  final int startByte;
  final int command;
  final int dataLength;
  final Uint8List data;
  final int crcHigh;
  final int crcLow;
  final int stopByte;
  
  SpeeduinoPacket({
    required this.startByte,
    required this.command,
    required this.dataLength,
    required this.data,
    required this.crcHigh,
    required this.crcLow,
    required this.stopByte,
  });
  
  /// Create packet from raw bytes
  factory SpeeduinoPacket.fromBytes(Uint8List bytes) {
    if (bytes.length < 6) {
      throw ArgumentError('Packet too short');
    }
    
    return SpeeduinoPacket(
      startByte: bytes[0],
      command: bytes[1],
      dataLength: bytes[2],
      data: bytes.sublist(3, 3 + bytes[2]),
      crcHigh: bytes[3 + bytes[2]],
      crcLow: bytes[4 + bytes[2]],
      stopByte: bytes[5 + bytes[2]],
    );
  }
  
  /// Convert packet to bytes
  Uint8List toBytes() {
    final bytes = Uint8List(6 + dataLength);
    bytes[0] = startByte;
    bytes[1] = command;
    bytes[2] = dataLength;
    bytes.setRange(3, 3 + dataLength, data);
    bytes[3 + dataLength] = crcHigh;
    bytes[4 + dataLength] = crcLow;
    bytes[5 + dataLength] = stopByte;
    return bytes;
  }
  
  /// Validate packet structure
  bool isValid() {
    return startByte == SpeeduinoConstants.startByte &&
           stopByte == SpeeduinoConstants.stopByte &&
           dataLength <= 256;
  }
}

/// Speeduino Real-Time Data
class SpeeduinoData {
  final int rpm;
  final int map;
  final int tps;
  final int coolantTemp;
  final int intakeTemp;
  final double batteryVoltage;
  final double afr;
  final int timing;
  final int boost;
  final int engineStatus;
  final DateTime timestamp;
  
  SpeeduinoData({
    required this.rpm,
    required this.map,
    required this.tps,
    required this.coolantTemp,
    required this.intakeTemp,
    required this.batteryVoltage,
    required this.afr,
    required this.timing,
    required this.boost,
    required this.engineStatus,
    required this.timestamp,
  });
  
  /// Create from raw data bytes
  factory SpeeduinoData.fromBytes(Uint8List bytes) {
    if (bytes.length < 20) {
      throw ArgumentError('Data too short');
    }
    
    return SpeeduinoData(
      rpm: (bytes[0] << 8) | bytes[1],
      map: bytes[2],
      tps: bytes[3],
      coolantTemp: bytes[4],
      intakeTemp: bytes[5],
      batteryVoltage: bytes[6] / 10.0,
      afr: bytes[7] / 10.0,
      timing: bytes[8],
      boost: bytes[9],
      engineStatus: bytes[10],
      timestamp: DateTime.now(),
    );
  }
  
  /// Create default values for initialization
  factory SpeeduinoData.defaultValues() {
    return SpeeduinoData(
      rpm: 0,
      map: 0,
      tps: 0,
      coolantTemp: 20,
      intakeTemp: 20,
      batteryVoltage: 12.6,
      afr: 14.7,
      timing: 15,
      boost: 0,
      engineStatus: 0,
      timestamp: DateTime.now(),
    );
  }
  
  /// Check if engine is running
  bool get isEngineRunning => (engineStatus & SpeeduinoConstants.statusEngineRunning) != 0;
  
  /// Check if engine is cranking
  bool get isEngineCranking => (engineStatus & SpeeduinoConstants.statusEngineCranking) != 0;
  
  /// Check if boost control is active
  bool get isBoostControlActive => (engineStatus & SpeeduinoConstants.statusBoostControl) != 0;
  
  /// Check if knock is detected
  bool get isKnockDetected => (engineStatus & SpeeduinoConstants.statusKnockDetected) != 0;
  
  /// Check if check engine light is on
  bool get isCheckEngineOn => (engineStatus & SpeeduinoConstants.statusCheckEngine) != 0;
}

/// ECU Connection Information
class ECUConnectionInfo {
  final String port;
  final int baudRate;
  final ECUProtocol protocol;
  final String ecuType;
  final String version;
  final DateTime connectedAt;
  
  ECUConnectionInfo({
    required this.port,
    required this.baudRate,
    required this.protocol,
    required this.ecuType,
    required this.version,
    required this.connectedAt,
  });
}

/// ECU Error Information
class ECUError {
  final String message;
  final String code;
  final DateTime timestamp;
  final StackTrace? stackTrace;
  
  ECUError({
    required this.message,
    required this.code,
    required this.timestamp,
    this.stackTrace,
  });
  
  @override
  String toString() => 'ECUError($code): $message at $timestamp';
}

/// ECU Statistics
class ECUStatistics {
  final int bytesReceived;
  final int bytesTransmitted;
  final int packetsReceived;
  final int packetsTransmitted;
  final int errors;
  final int timeouts;
  final DateTime lastActivity;
  
  ECUStatistics({
    required this.bytesReceived,
    required this.bytesTransmitted,
    required this.packetsReceived,
    required this.packetsTransmitted,
    required this.errors,
    required this.timeouts,
    required this.lastActivity,
  });
  
  /// Calculate receive rate (bytes per second)
  double get receiveRate {
    final now = DateTime.now();
    final duration = now.difference(lastActivity).inSeconds;
    return duration > 0 ? bytesReceived / duration : 0.0;
  }
  
  /// Calculate transmit rate (bytes per second)
  double get transmitRate {
    final now = DateTime.now();
    final duration = now.difference(lastActivity).inSeconds;
    return duration > 0 ? bytesTransmitted / duration : 0.0;
  }
  
  /// Calculate packet success rate
  double get successRate {
    final total = packetsReceived + packetsTransmitted;
    return total > 0 ? (total - errors - timeouts) / total : 0.0;
  }
  
  /// Calculate packets per second
  double get packetsPerSecond {
    final now = DateTime.now();
    final duration = now.difference(lastActivity).inSeconds;
    return duration > 0 ? packetsReceived / duration : 0.0;
  }
}
