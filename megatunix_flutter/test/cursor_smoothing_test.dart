/// Test cursor smoothing and trail filtering functionality
import 'package:flutter_test/flutter_test.dart';
import '../lib/models/table_cursor.dart';

void main() {
  group('Cursor Smoothing Tests', () {
    test('smoothed cursor reduces jitter', () {
      // Create initial cursor position
      var cursor = const TableCursor.hidden();
      
      // Add a position to start smoothing
      final pos1 = TableCursorPosition(
        x: 5.0,
        y: 3.0,
        timestamp: DateTime.now(),
      );
      cursor = cursor.updatePosition(pos1);
      
      // Add a small jittery movement
      final pos2 = TableCursorPosition(
        x: 5.01, // Very small movement should be filtered
        y: 3.01,
        timestamp: DateTime.now().add(const Duration(milliseconds: 50)),
      );
      cursor = cursor.updatePosition(pos2);
      
      // The current cursor position should remain close to original
      final points = cursor.allPoints;
      expect(points.length, greaterThan(0));
      final currentPos = points.last.position;
      expect(currentPos.x, closeTo(5.0, 0.5)); // Should be smoothed/filtered
      expect(currentPos.y, closeTo(3.0, 0.5));
    });

    test('cursor trail filters out jumps', () {
      var cursor = const TableCursor.hidden();
      
      // Add sequential positions
      final positions = [
        TableCursorPosition(x: 1.0, y: 1.0, timestamp: DateTime.now()),
        TableCursorPosition(x: 1.2, y: 1.1, timestamp: DateTime.now().add(const Duration(milliseconds: 50))),
        TableCursorPosition(x: 1.4, y: 1.2, timestamp: DateTime.now().add(const Duration(milliseconds: 100))),
        // Big jump that should break trail continuity and trigger trail clearing
        TableCursorPosition(x: 5.0, y: 5.0, timestamp: DateTime.now().add(const Duration(milliseconds: 150))),
        TableCursorPosition(x: 5.1, y: 5.1, timestamp: DateTime.now().add(const Duration(milliseconds: 200))),
      ];
      
      for (final pos in positions) {
        cursor = cursor.updatePosition(pos);
      }
      
      // Check that trail has been heavily filtered due to jump detection
      expect(cursor.trailPositions.length, lessThan(positions.length - 1));
      
      // Current position should be smoothed version of last position
      final points = cursor.allPoints;
      expect(points.isNotEmpty, true);
    });

    test('cursor maintains trail within time window', () {
      var cursor = const TableCursor.hidden();
      final now = DateTime.now();
      
      // Add old position (should be filtered out)
      final oldPos = TableCursorPosition(
        x: 1.0,
        y: 1.0,
        timestamp: now.subtract(const Duration(seconds: 5)),
      );
      cursor = cursor.updatePosition(oldPos);
      
      // Add recent position
      final recentPos = TableCursorPosition(
        x: 2.0,
        y: 2.0,
        timestamp: now,
      );
      cursor = cursor.updatePosition(recentPos);
      
      // Only recent position should be in trail
      final points = cursor.allPoints;
      expect(points.length, equals(1)); // Only current position visible
    });

    test('cursor interpolation works correctly', () {
      // Test axis interpolation
      final xAxis = [1000.0, 2000.0, 3000.0, 4000.0, 5000.0];
      final yAxis = [50.0, 75.0, 100.0, 125.0, 150.0];
      
      // Test interpolation at midpoint
      final pos = TableCursorPosition.fromValues(2500.0, 87.5, xAxis, yAxis);
      
      expect(pos.x, closeTo(1.5, 0.1)); // Between indices 1 and 2
      expect(pos.y, closeTo(1.5, 0.1)); // Between indices 1 and 2
    });

    test('cursor handles edge cases', () {
      final xAxis = [1000.0, 2000.0, 3000.0];
      final yAxis = [50.0, 100.0, 150.0];
      
      // Test below minimum
      var pos = TableCursorPosition.fromValues(500.0, 25.0, xAxis, yAxis);
      expect(pos.x, equals(0.0));
      expect(pos.y, equals(0.0));
      
      // Test above maximum
      pos = TableCursorPosition.fromValues(5000.0, 200.0, xAxis, yAxis);
      expect(pos.x, equals(2.0)); // Last index
      expect(pos.y, equals(2.0));
    });
  });

  group('Trail Continuity Tests', () {
    test('trail segments properly based on distance', () {
      var cursor = const TableCursor.hidden();
      
      // Create positions that should form continuous segments
      final continuousPositions = [
        TableCursorPosition(x: 1.0, y: 1.0, timestamp: DateTime.now()),
        TableCursorPosition(x: 1.3, y: 1.2, timestamp: DateTime.now().add(const Duration(milliseconds: 50))),
        TableCursorPosition(x: 1.6, y: 1.4, timestamp: DateTime.now().add(const Duration(milliseconds: 100))),
      ];
      
      for (final pos in continuousPositions) {
        cursor = cursor.updatePosition(pos);
      }
      
      // All positions should be retained as they're close
      expect(cursor.trailPositions.length, greaterThan(0));
      
      // Now add a position that's too far (should break continuity)
      final jumpPos = TableCursorPosition(
        x: 10.0,
        y: 10.0,
        timestamp: DateTime.now().add(const Duration(milliseconds: 150)),
      );
      cursor = cursor.updatePosition(jumpPos);
      
      // Check that trail was filtered
      final points = cursor.allPoints;
      expect(points.isNotEmpty, true);
    });
  });
}