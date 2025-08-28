/// Simple test for serial port access
import 'dart:io';

void main() async {
  const targetPort = '/dev/ttyACM0';

  print('Testing serial port access...');

  // Check if port exists
  final portFile = File(targetPort);
  if (await portFile.exists()) {
    print('✅ Port $targetPort exists');

    // Try to open for reading
    try {
      final raf = await portFile.open(mode: FileMode.read);
      print('✅ Successfully opened port for reading');

      // Try to read a few bytes
      final buffer = List<int>.filled(10, 0);
      final bytesRead = await raf.readInto(buffer);
      print('Read $bytesRead bytes: ${buffer.sublist(0, bytesRead)}');

      await raf.close();
      print('✅ Port closed successfully');
    } catch (e) {
      print('❌ Error accessing port: $e');
    }
  } else {
    print('❌ Port $targetPort does not exist');
  }

  print('Test completed!');
}