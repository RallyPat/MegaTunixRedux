/// Serial Communication Service using Dart File API
/// Alternative implementation that doesn't rely on flutter_libserialport
/// Provides reliable serial port communication for ECU connections

import 'dart:async';
import 'dart:io';
import 'dart:convert';
import 'dart:typed_data';

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

/// Serial communication service using File API
class SerialServiceFile {
  // File handles
  RandomAccessFile? _portFile;
  StreamController<Uint8List>? _dataController;
  StreamController<SerialConnectionState>? _connectionController;
  StreamController<String>? _errorController;

  // Connection state
  SerialConnectionState _connectionState = SerialConnectionState.disconnected;
  SerialConfig? _currentConfig;
  Timer? _readTimer;

  // Getters
  SerialConnectionState get connectionState => _connectionState;
  SerialConfig? get currentConfig => _currentConfig;
  Stream<Uint8List> get dataStream => _dataController?.stream ?? Stream.empty();
  Stream<SerialConnectionState> get connectionStream => _connectionController?.stream ?? Stream.empty();
  Stream<String> get errorStream => _errorController?.stream ?? Stream.empty();

  /// Connect to serial port
  Future<bool> connect(SerialConfig config) async {
    try {
      print('SerialServiceFile: Attempting to connect to ${config.port} at ${config.baudRate} baud');
      _updateConnectionState(SerialConnectionState.connecting);

      // Check if port exists
      final portFile = File(config.port);
      if (!await portFile.exists()) {
        _updateConnectionState(SerialConnectionState.error);
        _addError('Serial port ${config.port} does not exist');
        return false;
      }

      // Configure port using stty
      final sttyResult = await _configurePortWithStty(config);
      if (!sttyResult) {
        _updateConnectionState(SerialConnectionState.error);
        _addError('Failed to configure serial port with stty');
        return false;
      }

      // Store config for later use
      _currentConfig = config;

      // Initialize stream controllers
      _dataController = StreamController<Uint8List>.broadcast();
      _connectionController = StreamController<SerialConnectionState>.broadcast();
      _errorController = StreamController<String>.broadcast();

      // Start reading data
      _startReading();

      _updateConnectionState(SerialConnectionState.connected);
      print('SerialServiceFile: Successfully connected to ${config.port}');
      return true;

    } catch (e) {
      print('SerialServiceFile: Connection error: $e');
      _updateConnectionState(SerialConnectionState.error);
      _addError('Connection error: $e');
      await _cleanup();
      return false;
    }
  }

  /// Configure serial port using stty command
  Future<bool> _configurePortWithStty(SerialConfig config) async {
    try {
      // Build stty command with more complete configuration
      final sttyCmd = 'stty -F ${config.port} ${config.baudRate} cs8 raw -echo -echoe -echok -ixon -ixoff';

      // Execute stty command
      final result = await Process.run('bash', ['-c', sttyCmd]);

      if (result.exitCode == 0) {
        print('SerialServiceFile: Port configured successfully with stty at ${config.baudRate} baud');
        return true;
      } else {
        print('SerialServiceFile: stty failed: ${result.stderr}');
        return false;
      }
    } catch (e) {
      print('SerialServiceFile: stty configuration error: $e');
      return false;
    }
  }

  /// Start reading data from serial port
  void _startReading() {
    _readTimer = Timer.periodic(const Duration(milliseconds: 200), (timer) async {
      if (_connectionState != SerialConnectionState.connected || _currentConfig == null) {
        return;
      }

      try {
        // Use dd command to read from serial port
        final result = await Process.run('bash', ['-c', 'timeout 0.1 dd if=${_currentConfig!.port} bs=1 count=1024 2>/dev/null || true']);

        if (result.exitCode == 0 && result.stdout.isNotEmpty) {
          final data = Uint8List.fromList(result.stdout);
          if (data.isNotEmpty) {
            print('SerialServiceFile: 📥 Received ${data.length} bytes from ECU: ${data.map((b) => '0x${b.toRadixString(16).padLeft(2, '0')}').join(' ')}');
            _dataController?.add(data);
          }
        } else if (result.exitCode != 0) {
          // Only log errors occasionally to avoid spam
          print('SerialServiceFile: Read error (exit code: ${result.exitCode})');
        }
      } catch (e) {
        // Ignore read errors (port might not have data)
      }
    });
  }

  /// Send data to serial port
  Future<bool> sendData(Uint8List data) async {
    if (_connectionState != SerialConnectionState.connected || _currentConfig == null) {
      _addError('Cannot send data: not connected');
      return false;
    }

    try {
      // Use echo command to send data to serial port
      final hexData = data.map((b) => '\\x${b.toRadixString(16).padLeft(2, '0')}').join('');
      final result = await Process.run('bash', ['-c', 'echo -e -n "$hexData" > ${_currentConfig!.port}']);

      if (result.exitCode == 0) {
        return true;
      } else {
        _addError('Failed to send data: ${result.stderr}');
        return false;
      }
    } catch (e) {
      _addError('Send data error: $e');
      return false;
    }
  }

  /// Send string data to serial port
  Future<bool> sendString(String data) async {
    if (_connectionState != SerialConnectionState.connected || _currentConfig == null) {
      _addError('Cannot send data: not connected');
      return false;
    }

    try {
      // Use echo command to send string to serial port
      final result = await Process.run('bash', ['-c', 'echo -n "$data" > ${_currentConfig!.port}']);

      if (result.exitCode == 0) {
        return true;
      } else {
        _addError('Failed to send string: ${result.stderr}');
        return false;
      }
    } catch (e) {
      _addError('Send string error: $e');
      return false;
    }
  }

  /// Get available serial ports
  Future<List<String>> getAvailablePorts() async {
    try {
      // Use ls to find serial ports
      final result = await Process.run('bash', ['-c', 'ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || true']);
      if (result.exitCode == 0) {
        final ports = result.stdout.toString().trim().split('\n').where((p) => p.isNotEmpty).toList();
        return ports;
      }
      return [];
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

  /// Disconnect from serial port
  Future<void> disconnect() async {
    print('SerialServiceFile: Disconnecting...');
    await _cleanup();
    _updateConnectionState(SerialConnectionState.disconnected);
    print('SerialServiceFile: Disconnected');
  }

  /// Clean up resources
  Future<void> _cleanup() async {
    _readTimer?.cancel();
    _readTimer = null;

    if (_portFile != null) {
      try {
        await _portFile!.close();
      } catch (e) {
        print('SerialServiceFile: Error closing port file: $e');
      }
      _portFile = null;
    }

    await _dataController?.close();
    await _connectionController?.close();
    await _errorController?.close();

    _dataController = null;
    _connectionController = null;
    _errorController = null;
    _currentConfig = null;
  }

  /// Update connection state
  void _updateConnectionState(SerialConnectionState state) {
    if (_connectionState != state) {
      _connectionState = state;
      _connectionController?.add(state);
    }
  }

  /// Add error
  void _addError(String error) {
    // Only log errors that aren't repetitive
    if (!error.contains('async operation is currently pending')) {
      print('SerialServiceFile Error: $error');
    }
    _errorController?.add(error);
  }

  /// Dispose resources
  void dispose() {
    disconnect();
  }
}

/// Serial service singleton instance
final SerialServiceFile serialServiceFile = SerialServiceFile();