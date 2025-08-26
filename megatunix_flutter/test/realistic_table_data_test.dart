import 'package:flutter_test/flutter_test.dart';
import 'package:megatunix_flutter/services/realistic_table_data.dart';

void main() {
  group('Realistic Table Data Generation', () {
    test('VE table generates proper gradient', () {
      final veTable = RealisticTableData.generateVETable(rows: 12, cols: 16);
      
      // Check dimensions
      expect(veTable.length, equals(12));
      expect(veTable[0].length, equals(16));
      
      // Check value range
      for (final row in veTable) {
        for (final value in row) {
          expect(value, greaterThanOrEqualTo(20.0));
          expect(value, lessThanOrEqualTo(110.0));
        }
      }
      
      // Check for value variation (not all cells the same)
      final firstValue = veTable[0][0];
      final hasVariation = veTable.any((row) => 
        row.any((value) => (value - firstValue).abs() > 1.0));
      expect(hasVariation, isTrue, reason: 'VE table should have value variation for heatmap');
      
      print('✅ VE Table Range: ${_getTableRange(veTable)}');
    });

    test('AFR table generates proper gradient', () {
      final afrTable = RealisticTableData.generateAFRTable(rows: 12, cols: 16);
      
      // Check dimensions
      expect(afrTable.length, equals(12));
      expect(afrTable[0].length, equals(16));
      
      // Check value range
      for (final row in afrTable) {
        for (final value in row) {
          expect(value, greaterThanOrEqualTo(10.5));
          expect(value, lessThanOrEqualTo(18.0));
        }
      }
      
      // Check for value variation
      final firstValue = afrTable[0][0];
      final hasVariation = afrTable.any((row) => 
        row.any((value) => (value - firstValue).abs() > 0.5));
      expect(hasVariation, isTrue, reason: 'AFR table should have value variation for heatmap');
      
      print('✅ AFR Table Range: ${_getTableRange(afrTable)}');
    });

    test('Timing table generates proper gradient', () {
      final timingTable = RealisticTableData.generateTimingTable(rows: 12, cols: 16);
      
      // Check dimensions
      expect(timingTable.length, equals(12));
      expect(timingTable[0].length, equals(16));
      
      // Check value range
      for (final row in timingTable) {
        for (final value in row) {
          expect(value, greaterThanOrEqualTo(5.0));
          expect(value, lessThanOrEqualTo(40.0));
        }
      }
      
      // Check for value variation
      final firstValue = timingTable[0][0];
      final hasVariation = timingTable.any((row) => 
        row.any((value) => (value - firstValue).abs() > 2.0));
      expect(hasVariation, isTrue, reason: 'Timing table should have value variation for heatmap');
      
      print('✅ Timing Table Range: ${_getTableRange(timingTable)}');
    });

    test('Handles various table dimensions', () {
      // Test different table sizes
      final sizes = [
        (8, 8),   // Small table
        (12, 16), // Standard table
        (20, 24), // Large table
        (4, 6),   // Very small table
      ];
      
      for (final (rows, cols) in sizes) {
        final table = RealisticTableData.generateVETable(rows: rows, cols: cols);
        expect(table.length, equals(rows));
        expect(table[0].length, equals(cols));
        
        // Ensure all values are valid
        for (final row in table) {
          for (final value in row) {
            expect(value.isFinite, isTrue);
            expect(value, greaterThan(0));
          }
        }
      }
      
      print('✅ All table dimensions handled correctly');
    });

    test('Different table types generate different patterns', () {
      final veTable = RealisticTableData.generateVETable();
      final afrTable = RealisticTableData.generateAFRTable();
      final timingTable = RealisticTableData.generateTimingTable();
      
      // Tables should have different value ranges and patterns
      final veAvg = _getTableAverage(veTable);
      final afrAvg = _getTableAverage(afrTable);
      final timingAvg = _getTableAverage(timingTable);
      
      // VE should be higher values (around 80%)
      expect(veAvg, greaterThan(60));
      
      // AFR should be around stoich (14.7)
      expect(afrAvg, greaterThan(12));
      expect(afrAvg, lessThan(16));
      
      // Timing should be moderate (15-25 degrees)
      expect(timingAvg, greaterThan(10));
      expect(timingAvg, lessThan(35));
      
      print('✅ Table Types:');
      print('   VE Average: ${veAvg.toStringAsFixed(1)}%');
      print('   AFR Average: ${afrAvg.toStringAsFixed(1)}');
      print('   Timing Average: ${timingAvg.toStringAsFixed(1)}°');
    });
  });
}

/// Get min and max values from table
String _getTableRange(List<List<double>> table) {
  double min = table[0][0];
  double max = table[0][0];
  
  for (final row in table) {
    for (final value in row) {
      if (value < min) min = value;
      if (value > max) max = value;
    }
  }
  
  return '${min.toStringAsFixed(1)} - ${max.toStringAsFixed(1)}';
}

/// Get average value from table
double _getTableAverage(List<List<double>> table) {
  double sum = 0;
  int count = 0;
  
  for (final row in table) {
    for (final value in row) {
      sum += value;
      count++;
    }
  }
  
  return sum / count;
}