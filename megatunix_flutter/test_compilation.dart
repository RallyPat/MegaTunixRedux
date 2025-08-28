/// Simple compilation test without Flutter
/// Tests basic Dart syntax and imports

// Test basic imports
import 'dart:async';
import 'dart:convert';

// Test enum handling
enum TestConnectionState {
  disconnected,
  connecting,
  connected,
  error,
}

void main() {
  print('Testing basic compilation...');

  // Test enum conversion
  final state = TestConnectionState.connected;
  final stateString = state.toString().split('.').last;
  print('Enum conversion: $stateString');

  // Test basic async
  Future.delayed(Duration(seconds: 1), () {
    print('Async test passed');
  });

  print('Basic compilation test passed');
}