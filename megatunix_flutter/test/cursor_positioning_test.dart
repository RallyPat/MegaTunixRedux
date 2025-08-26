import 'package:flutter_test/flutter_test.dart';
import 'package:megatunix_flutter/models/table_cursor.dart';

void main() {
  group('TableCursor Positioning Tests', () {
    test('should interpolate position correctly between axis points', () {
      // Test data: RPM axis from 800 to 6000
      final rpmAxis = [800.0, 1200.0, 2000.0, 3000.0, 4500.0, 6000.0];
      // Test data: MAP axis from 20 to 200 kPa
      final mapAxis = [20.0, 40.0, 60.0, 100.0, 150.0, 200.0];
      
      // Test case 1: Exact axis point
      var position = TableCursorPosition.fromValues(2000.0, 100.0, rpmAxis, mapAxis);
      expect(position.x, equals(2.0)); // Exactly on index 2
      expect(position.y, equals(3.0)); // Exactly on index 3
      
      // Test case 2: Interpolated position
      position = TableCursorPosition.fromValues(1600.0, 80.0, rpmAxis, mapAxis);
      expect(position.x, closeTo(1.5, 0.01)); // Halfway between index 1 and 2
      expect(position.y, closeTo(2.5, 0.01)); // Halfway between index 2 and 3
      
      // Test case 3: Below minimum value
      position = TableCursorPosition.fromValues(500.0, 10.0, rpmAxis, mapAxis);
      expect(position.x, equals(0.0));
      expect(position.y, equals(0.0));
      
      // Test case 4: Above maximum value
      position = TableCursorPosition.fromValues(7000.0, 250.0, rpmAxis, mapAxis);
      expect(position.x, equals(5.0)); // Last index
      expect(position.y, equals(5.0)); // Last index
    });

    test('should calculate cell coordinates and fractions correctly', () {
      final rpmAxis = [800.0, 1200.0, 2000.0, 3000.0, 4500.0, 6000.0];
      final mapAxis = [20.0, 40.0, 60.0, 100.0, 150.0, 200.0];
      
      // Position at 1600 RPM, 80 kPa (interpolated)
      final position = TableCursorPosition.fromValues(1600.0, 80.0, rpmAxis, mapAxis);
      
      final (row, col) = position.cellCoordinates;
      expect(col, equals(2)); // Rounds to nearest cell
      expect(row, equals(3)); // Rounds to nearest cell
      
      final (rowFraction, colFraction) = position.fractions;
      expect(colFraction, closeTo(0.5, 0.01)); // Half way into cell
      expect(rowFraction, closeTo(0.5, 0.01)); // Half way into cell
    });

    test('should validate cursor position bounds', () {
      final rpmAxis = [800.0, 1200.0, 2000.0];
      final mapAxis = [20.0, 40.0];
      
      // Valid position
      var position = TableCursorPosition.fromValues(1000.0, 30.0, rpmAxis, mapAxis);
      expect(position.isValidFor(2, 3), isTrue);
      
      // Invalid position (would be outside table bounds)
      position = TableCursorPosition.fromValues(7000.0, 250.0, rpmAxis, mapAxis);
      expect(position.isValidFor(1, 2), isFalse); // Position would be at (1, 2) but table is only 1x2
    });

    test('should update cursor trail correctly', () {
      final rpmAxis = [800.0, 1200.0, 2000.0];
      final mapAxis = [20.0, 40.0];
      
      // Start with empty cursor
      var cursor = const TableCursor.hidden();
      expect(cursor.hasVisibleElements, isFalse);
      
      // Add first position
      final pos1 = TableCursorPosition.fromValues(1000.0, 30.0, rpmAxis, mapAxis);
      cursor = cursor.updatePosition(pos1);
      expect(cursor.hasVisibleElements, isTrue);
      expect(cursor.currentPosition?.x, equals(pos1.x));
      expect(cursor.currentPosition?.y, equals(pos1.y));
      expect(cursor.trailPositions.length, equals(0)); // No trail yet
      
      // Add second position (this should create trail)
      final pos2 = TableCursorPosition.fromValues(1100.0, 35.0, rpmAxis, mapAxis);
      cursor = cursor.updatePosition(pos2);
      expect(cursor.currentPosition?.x, equals(pos2.x));
      expect(cursor.currentPosition?.y, equals(pos2.y));
      expect(cursor.trailPositions.length, equals(1)); // Previous position in trail
      
      // Clear cursor
      cursor = cursor.clear();
      expect(cursor.hasVisibleElements, isFalse);
      expect(cursor.currentPosition, isNull);
      expect(cursor.trailPositions.length, equals(0));
    });

    test('should handle edge cases gracefully', () {
      // Empty axis
      var position = TableCursorPosition.fromValues(1000.0, 30.0, [], [20.0, 40.0]);
      expect(position.x, equals(0.0));
      
      // Single point axis
      position = TableCursorPosition.fromValues(1000.0, 30.0, [1000.0], [30.0]);
      expect(position.x, equals(0.0));
      expect(position.y, equals(0.0));
      
      // Identical values in axis
      position = TableCursorPosition.fromValues(1000.0, 30.0, [1000.0, 1000.0, 1000.0], [30.0, 30.0]);
      expect(position.x, isA<double>()); // Should not crash
      expect(position.y, isA<double>()); // Should not crash
    });
  });

  group('CursorTrailPoint Fade Tests', () {
    test('should calculate opacity based on age', () {
      final oldPosition = TableCursorPosition(
        x: 1.0, 
        y: 1.0, 
        timestamp: DateTime.now().subtract(const Duration(seconds: 1))
      );
      
      final trailPoint = CursorTrailPoint.withFade(
        oldPosition,
        maxAge: const Duration(seconds: 3),
      );
      
      expect(trailPoint.opacity, lessThan(1.0));
      expect(trailPoint.opacity, greaterThan(0.0));
      expect(trailPoint.isVisible, isTrue);
    });
    
    test('should be invisible when too old', () {
      final veryOldPosition = TableCursorPosition(
        x: 1.0, 
        y: 1.0, 
        timestamp: DateTime.now().subtract(const Duration(seconds: 10))
      );
      
      final trailPoint = CursorTrailPoint.withFade(
        veryOldPosition,
        maxAge: const Duration(seconds: 3),
      );
      
      expect(trailPoint.opacity, lessThanOrEqualTo(0.05));
      expect(trailPoint.isVisible, isFalse);
    });
  });
}