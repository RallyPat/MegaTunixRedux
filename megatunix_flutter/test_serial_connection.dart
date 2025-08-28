/// Test script to verify serial port connection from Flutter
import 'dart:async';
import 'dart:io';
import 'package:flutter_libserialport/flutter_libserialport.dart';

void main() async {
  print('🧪 Testing Serial Port Connection...');

  const targetPort = '/dev/ttyACM0';

  // Test 1: Check if port exists using File API
  print('\n1. Testing port existence with File API...');
  final portFile = File(targetPort);
  if (await portFile.exists()) {
    print('✅ Port $targetPort exists');
  } else {
    print('❌ Port $targetPort does not exist');
    return;
  }

  // Test 2: Try to open with RandomAccessFile
  print('\n2. Testing RandomAccessFile access...');
  try {
    final raf = await portFile.open(mode: FileMode.read);
    print('✅ Successfully opened port for reading');
    await raf.close();
  } catch (e) {
    print('❌ Failed to open with RandomAccessFile: $e');
  }

  // Test 3: Check available ports with flutter_libserialport
  print('\n3. Available Serial Ports (flutter_libserialport):');
  try {
    final ports = SerialPort.availablePorts;
    print('Found ${ports.length} ports:');
    for (final port in ports) {
      print('  - $port');
    }
  } catch (e) {
    print('❌ Error getting available ports: $e');
  }

  // Test 4: Try flutter_libserialport approach
  print('\n4. Testing flutter_libserialport approach...');
  try {
    final serialPort = SerialPort(targetPort);
    print('✅ Created SerialPort instance');

    if (serialPort.openRead()) {
      print('✅ Successfully opened port for reading');

      // Try to read some data
      final data = serialPort.read(10, timeout: 1000);
      print('Read ${data.length} bytes: ${data.map((b) => '0x${b.toRadixString(16)}').join(' ')}');

      serialPort.close();
      print('✅ Port closed successfully');
    } else {
      print('❌ Failed to open port for reading');
      print('Last error: ${SerialPort.lastError}');
    }
  } catch (e) {
    print('❌ flutter_libserialport error: $e');
  }

  print('\n🎉 Serial connection test completed!');
}

  // Test 2: Check if our target port exists
  const targetPort = '/dev/ttyACM0';
  print('\n2. Checking target port: $targetPort');
  if (ports.contains(targetPort)) {
    print('✅ Port $targetPort is available');
  } else {
    print('❌ Port $targetPort not found');
    return;
  }

  // Test 3: Try to open the port
  print('\n3. Attempting to open port...');
  final serialPort = SerialPort(targetPort);

  // Test 4: Configure the port BEFORE opening
  print('\n4. Configuring port before opening...');
  final config = SerialPortConfig()
    ..baudRate = 115200
    ..bits = 8
    ..stopBits = 1
    ..parity = 0; // None

  serialPort.config = config;
  print('✅ Port configured successfully');

  if (serialPort.openReadWrite()) {
    print('✅ Successfully opened port for read/write');

    // Test 5: Try to read for a short time
    print('\n5. Listening for data (5 seconds)...');
    final completer = Completer<void>();
    Timer(const Duration(seconds: 5), () {
      completer.complete();
    });

    // Listen for data
    serialPort.read(10, timeout: 1000); // Try to read 10 bytes with 1s timeout

    await completer.future;
    print('✅ Listening completed');

    // Clean up
    serialPort.close();
    print('✅ Port closed successfully');

  } else {
    print('❌ Failed to open port');
    print('Last error: ${SerialPort.lastError}');
  }

  print('\n🎉 Serial connection test completed!');
}