/// Serial Communication Service
/// Handles low-level serial port communication for ECU connections
/// Provides platform abstraction for serial port operations

import 'dart:async';
import 'dart:typed_data';
import 'package:flutter/foundation.dart';
import 'package:flutter_libserialport/flutter_libserialport.dart';

/// Serial port configuration
class SerialConfig {
  final String port;
  final int baudRate;
  final int dataBits;
  final int stopBits;
  final String parity;
  final bool flowControl;

  const SerialConfig({
    required this.port,
    required this.baudRate,
    this.dataBits = 8,
    this.stopBits = 1,
    this.parity = 'none',
    this.flowControl = false,
  });

  Map<String, dynamic> toMap() {
    return {
      'port': port,
      'baudRate': baudRate,
      'dataBits': dataBits,
      'stopBits': stopBits,
      'parity': parity,
      'flowControl': flowControl,
    };
  }
}

/// Serial communication states
enum SerialConnectionState {
  disconnected,
  connecting,
  connected,
  error,
}

/// Serial communication service
class SerialService {
  // Serial port instance
  SerialPort? _serialPort;
  SerialPortReader? _reader;

  // Connection state
  SerialConnectionState _connectionState = SerialConnectionState.disconnected;
  SerialConfig? _currentConfig;

  // Data streams
  final StreamController<Uint8List> _dataController = StreamController<Uint8List>.broadcast();
  final StreamController<SerialConnectionState> _connectionController = StreamController<SerialConnectionState>.broadcast();
  final StreamController<String> _errorController = StreamController<String>.broadcast();

  // Getters
  SerialConnectionState get connectionState => _connectionState;
  SerialConfig? get currentConfig => _currentConfig;
  Stream<Uint8List> get dataStream => _dataController.stream;
  Stream<SerialConnectionState> get connectionStream => _connectionController.stream;
  Stream<String> get errorStream => _errorController.stream;

  /// Connect to serial port
  Future<bool> connect(SerialConfig config) async {
    try {
      print('SerialService: Attempting to connect to ${config.port} at ${config.baudRate} baud');
      _updateConnectionState(SerialConnectionState.connecting);

      // Create serial port instance
      _serialPort = SerialPort(config.port);
      print('SerialService: Created SerialPort instance for ${config.port}');

      if (!_serialPort!.openReadWrite()) {
        print('SerialService: Failed to open port, lastError: ${SerialPort.lastError}');
        _updateConnectionState(SerialConnectionState.error);
        _addError('Failed to open serial port: ${SerialPort.lastError}');
        return false;
      }
      print('SerialService: Successfully opened port for read/write');

       // Configure serial port
       final portConfig = SerialPortConfig()
         ..baudRate = config.baudRate
         ..bits = config.dataBits
         ..stopBits = config.stopBits == 1 ? 1 : 2
         ..parity = _mapParityString(config.parity)
         ..rts = config.flowControl ? 1 : 0  // RTS on/off as int
         ..cts = config.flowControl ? 1 : 0  // CTS on/off as int
         ..dtr = 1  // DTR on as int
         ..dsr = 0; // DSR off as int

       _serialPort!.config = portConfig;

      _currentConfig = config;

      // Set up data reading
      _setupDataReading();

      _updateConnectionState(SerialConnectionState.connected);
      print('SerialService: Connected to ${config.port} at ${config.baudRate} baud');
      return true;

    } catch (e) {
      _updateConnectionState(SerialConnectionState.error);
      _addError('Connection error: $e');
      _cleanup();
      return false;
    }
  }

  /// Disconnect from serial port
  Future<void> disconnect() async {
    try {
      _cleanup();
      _currentConfig = null;
      _updateConnectionState(SerialConnectionState.disconnected);
      print('SerialService: Disconnected');
    } catch (e) {
      _addError('Disconnect error: $e');
    }
  }

  /// Send data to serial port
  Future<bool> sendData(Uint8List data) async {
    if (_connectionState != SerialConnectionState.connected || _serialPort == null) {
      _addError('Cannot send data: not connected');
      return false;
    }

    try {
      final bytesWritten = _serialPort!.write(data);
      if (bytesWritten != data.length) {
        _addError('Failed to write all data: wrote $bytesWritten of ${data.length} bytes');
        return false;
      }
      return true;
    } catch (e) {
      _addError('Send data error: $e');
      return false;
    }
  }

  /// Send string data to serial port
  Future<bool> sendString(String data) async {
    return sendData(Uint8List.fromList(data.codeUnits));
  }

  /// Get available serial ports
  Future<List<String>> getAvailablePorts() async {
    try {
      return SerialPort.availablePorts;
    } catch (e) {
      _addError('Error getting available ports: $e');
      return [];
    }
  }

  /// Check if a port exists and is accessible
  Future<bool> isPortAvailable(String port) async {
    try {
      final ports = await getAvailablePorts();
      return ports.contains(port);
    } catch (e) {
      return false;
    }
  }

  /// Set up data reading from serial port
  void _setupDataReading() {
    if (_serialPort == null) return;

    _reader = SerialPortReader(_serialPort!);
    _reader!.stream.listen(
      (data) {
        if (data.isNotEmpty) {
          _dataController.add(Uint8List.fromList(data));
        }
      },
      onError: (error) {
        _addError('Serial read error: $error');
        _updateConnectionState(SerialConnectionState.error);
      },
      onDone: () {
        print('SerialService: Serial port reader closed');
        if (_connectionState == SerialConnectionState.connected) {
          _updateConnectionState(SerialConnectionState.error);
          _addError('Serial port connection lost');
        }
      },
    );
  }

  /// Map parity string to SerialPortParity enum
  int _mapParityString(String parity) {
    switch (parity.toLowerCase()) {
      case 'none':
        return 0; // SerialPortParity.none
      case 'even':
        return 1; // SerialPortParity.even
      case 'odd':
        return 2; // SerialPortParity.odd
      case 'mark':
        return 3; // SerialPortParity.mark
      case 'space':
        return 4; // SerialPortParity.space
      default:
        return 0; // SerialPortParity.none
    }
  }

  /// Clean up serial port resources
  void _cleanup() {
    try {
      _reader?.close();
      _reader = null;
    } catch (e) {
      print('SerialService: Error closing reader: $e');
    }

    try {
      _serialPort?.close();
      _serialPort = null;
    } catch (e) {
      print('SerialService: Error closing serial port: $e');
    }
  }

  /// Update connection state and notify listeners
  void _updateConnectionState(SerialConnectionState state) {
    if (_connectionState != state) {
      _connectionState = state;
      _connectionController.add(state);
    }
  }

  /// Add error and notify listeners
  void _addError(String error) {
    print('SerialService Error: $error');
    _errorController.add(error);
  }

  /// Dispose resources
  void dispose() {
    disconnect();
    _dataController.close();
    _connectionController.close();
    _errorController.close();
  }
}

/// Serial service singleton instance
final SerialService serialService = SerialService();