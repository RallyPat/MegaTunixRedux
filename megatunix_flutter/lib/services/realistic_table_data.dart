/// Realistic TunerStudio-Style Table Data Generator
/// Creates believable default table values for VE, Fuel, Timing tables

import 'dart:math';

/// Generates realistic automotive tuning table data
class RealisticTableData {
  static final Random _random = Random();
  
  /// Standard RPM bins (16 columns) - typical TunerStudio layout
  static const List<double> standardRpmBins = [
    500, 750, 1000, 1500, 2000, 2500, 3000, 3500,
    4000, 4500, 5000, 5500, 6000, 6500, 7000, 7500
  ];
  
  /// Standard MAP bins (12 rows) - kPa values for naturally aspirated engine
  static const List<double> standardMapBins = [
    20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130
  ];
  
  /// Helper method to ensure bins match table dimensions
  static (List<double>, List<double>) _prepareBins({
    required int rows,
    required int cols,
    List<double>? mapBins,
    List<double>? rpmBins,
  }) {
    // Use provided bins or create default ones that match the table dimensions
    final map = mapBins?.take(rows).toList() ?? 
                standardMapBins.take(rows).toList();
    final rpm = rpmBins?.take(cols).toList() ?? 
                standardRpmBins.take(cols).toList();
    
    // Pad with default values if we don't have enough bins
    while (map.length < rows) {
      final lastValue = map.isNotEmpty ? map.last : 20.0;
      map.add(lastValue + 10.0);
    }
    while (rpm.length < cols) {
      final lastValue = rpm.isNotEmpty ? rpm.last : 500.0;
      rpm.add(lastValue + 400.0);
    }
    
    return (map, rpm);
  }
  
  /// Generate realistic VE (Volumetric Efficiency) table
  /// VE values typically range from 20-110% with peak around 3000-4000 RPM
  static List<List<double>> generateVETable({
    int rows = 12,
    int cols = 16,
    List<double>? mapBins,
    List<double>? rpmBins,
  }) {
    final (map, rpm) = _prepareBins(
      rows: rows, cols: cols, mapBins: mapBins, rpmBins: rpmBins);
    
    return List.generate(rows, (mapIdx) {
      final mapValue = map[mapIdx];
      
      return List.generate(cols, (rpmIdx) {
        final rpmValue = rpm[rpmIdx];
        
        // Base VE curve - peaks around 3000-4000 RPM
        double baseVE;
        if (rpmValue < 1000) {
          baseVE = 35 + (rpmValue - 500) * 0.04; // Low RPM efficiency
        } else if (rpmValue < 3500) {
          baseVE = 75 + (rpmValue - 1000) * 0.012; // Rising to peak
        } else if (rpmValue < 5000) {
          baseVE = 105 - (rpmValue - 3500) * 0.008; // Peak efficiency zone
        } else {
          baseVE = 93 - (rpmValue - 5000) * 0.015; // Falling off at high RPM
        }
        
        // Load adjustment - higher MAP = higher VE potential
        final loadFactor = 1.0 + (mapValue - 60) * 0.002;
        baseVE *= loadFactor;
        
        // Add realistic variation
        final variation = (_random.nextDouble() - 0.5) * 8.0;
        
        return (baseVE + variation).clamp(20.0, 110.0);
      });
    });
  }
  
  /// Generate realistic AFR (Air/Fuel Ratio) table
  /// AFR values: 14.7 stoich, 12.0-13.5 rich for power, 15.0-16.0 lean for economy
  static List<List<double>> generateAFRTable({
    int rows = 12,
    int cols = 16,
    List<double>? mapBins,
    List<double>? rpmBins,
  }) {
    final (map, rpm) = _prepareBins(
      rows: rows, cols: cols, mapBins: mapBins, rpmBins: rpmBins);
    
    return List.generate(rows, (mapIdx) {
      final mapValue = map[mapIdx];
      
      return List.generate(cols, (rpmIdx) {
        final rpmValue = rpm[rpmIdx];
        
        // Base AFR strategy
        double targetAFR;
        
        if (mapValue < 50) {
          // Light load - lean for economy
          targetAFR = 15.5 - (rpmValue / 1000) * 0.3;
        } else if (mapValue < 80) {
          // Medium load - closer to stoich
          targetAFR = 14.7 - (mapValue - 50) * 0.01;
        } else {
          // High load - rich for power and cooling
          targetAFR = 13.2 - (mapValue - 80) * 0.02;
          
          // Extra rich at high RPM/load for power
          if (rpmValue > 4000) {
            targetAFR -= 0.3;
          }
        }
        
        // Idle enrichment
        if (rpmValue < 1200 && mapValue < 40) {
          targetAFR = 13.8;
        }
        
        // Add small random variation
        final variation = (_random.nextDouble() - 0.5) * 0.3;
        
        return (targetAFR + variation).clamp(10.5, 18.0);
      });
    });
  }
  
  /// Generate realistic Timing Advance table
  /// Timing values: 10-35 degrees, more advance at light load, retard under heavy load
  static List<List<double>> generateTimingTable({
    int rows = 12,
    int cols = 16,
    List<double>? mapBins,
    List<double>? rpmBins,
  }) {
    final (map, rpm) = _prepareBins(
      rows: rows, cols: cols, mapBins: mapBins, rpmBins: rpmBins);
    
    return List.generate(rows, (mapIdx) {
      final mapValue = map[mapIdx];
      
      return List.generate(cols, (rpmIdx) {
        final rpmValue = rpm[rpmIdx];
        
        // Base timing curve - more advance at higher RPM, less at high load
        double baseTiming;
        
        // RPM-based timing curve
        if (rpmValue < 2000) {
          baseTiming = 12 + (rpmValue / 100); // Start conservative, build advance
        } else if (rpmValue < 4000) {
          baseTiming = 30 + (rpmValue - 2000) * 0.002; // Peak timing zone
        } else {
          baseTiming = 34 - (rpmValue - 4000) * 0.003; // Slight retard at very high RPM
        }
        
        // Load-based retard - pull timing under load to prevent knock
        final loadRetard = (mapValue - 40) * 0.15;
        baseTiming -= loadRetard.clamp(0, 15);
        
        // Extra retard for very high load conditions
        if (mapValue > 100) {
          baseTiming -= 3;
        }
        
        // Add small random variation
        final variation = (_random.nextDouble() - 0.5) * 2.0;
        
        return (baseTiming + variation).clamp(5.0, 40.0);
      });
    });
  }
  
  /// Generate realistic Fuel VE table (similar to VE but with fuel perspective)
  static List<List<double>> generateFuelVETable({
    int rows = 12,
    int cols = 16,
    List<double>? mapBins,
    List<double>? rpmBins,
  }) {
    // Start with VE table and adjust for fuel delivery characteristics
    final veTable = generateVETable(rows: rows, cols: cols, mapBins: mapBins, rpmBins: rpmBins);
    
    return veTable.map((row) => 
      row.map((value) {
        // Fuel VE is typically 5-10% lower than air VE due to fuel dynamics
        final fuelVE = value * 0.92 + (_random.nextDouble() - 0.5) * 3;
        return fuelVE.clamp(15.0, 105.0);
      }).toList()
    ).toList();
  }
  
  /// Generate realistic Boost Control table (for turbocharged engines)
  static List<List<double>> generateBoostTable({
    int rows = 12,
    int cols = 16,
    List<double>? mapBins,
    List<double>? rpmBins,
  }) {
    final (map, rpm) = _prepareBins(
      rows: rows, cols: cols, mapBins: mapBins, rpmBins: rpmBins);
    
    return List.generate(rows, (mapIdx) {
      final mapValue = map[mapIdx];
      
      return List.generate(cols, (rpmIdx) {
        final rpmValue = rpm[rpmIdx];
        
        // Boost target - lower at low RPM, higher at optimal RPM range
        double boostTarget;
        
        if (rpmValue < 2000) {
          boostTarget = 5 + (rpmValue / 400); // Conservative low-end boost
        } else if (rpmValue < 5000) {
          boostTarget = 15 + (rpmValue - 2000) * 0.003; // Building boost
        } else {
          boostTarget = 24 - (rpmValue - 5000) * 0.002; // Taper at high RPM
        }
        
        // Load-based adjustment
        if (mapValue < 80) {
          boostTarget *= 0.6; // Less boost at part throttle
        }
        
        // Add variation
        final variation = (_random.nextDouble() - 0.5) * 1.5;
        
        return (boostTarget + variation).clamp(0.0, 30.0);
      });
    });
  }
  
  /// Generate realistic Lambda table (alternative to AFR)
  static List<List<double>> generateLambdaTable({
    int rows = 12,
    int cols = 16,
    List<double>? mapBins,
    List<double>? rpmBins,
  }) {
    final afrTable = generateAFRTable(rows: rows, cols: cols, mapBins: mapBins, rpmBins: rpmBins);
    
    // Convert AFR to Lambda (Lambda = AFR / 14.7)
    return afrTable.map((row) => 
      row.map((afr) => (afr / 14.7).clamp(0.7, 1.2)).toList()
    ).toList();
  }
  
  /// Generate table data based on table name/type
  static List<List<double>> generateTableByName(
    String tableName, {
    int rows = 12,
    int cols = 16,
    List<double>? mapBins,
    List<double>? rpmBins,
  }) {
    final name = tableName.toLowerCase();
    
    if (name.contains('ve') || name.contains('volumetric')) {
      return generateVETable(rows: rows, cols: cols, mapBins: mapBins, rpmBins: rpmBins);
    } else if (name.contains('afr') || name.contains('air') && name.contains('fuel')) {
      return generateAFRTable(rows: rows, cols: cols, mapBins: mapBins, rpmBins: rpmBins);
    } else if (name.contains('timing') || name.contains('advance') || name.contains('ignition')) {
      return generateTimingTable(rows: rows, cols: cols, mapBins: mapBins, rpmBins: rpmBins);
    } else if (name.contains('fuel') && name.contains('ve')) {
      return generateFuelVETable(rows: rows, cols: cols, mapBins: mapBins, rpmBins: rpmBins);
    } else if (name.contains('boost') || name.contains('turbo')) {
      return generateBoostTable(rows: rows, cols: cols, mapBins: mapBins, rpmBins: rpmBins);
    } else if (name.contains('lambda')) {
      return generateLambdaTable(rows: rows, cols: cols, mapBins: mapBins, rpmBins: rpmBins);
    } else {
      // Default to VE table pattern for unknown types
      return generateVETable(rows: rows, cols: cols, mapBins: mapBins, rpmBins: rpmBins);
    }
  }
  
  /// Get realistic axis bins based on axis type
  static List<double> generateAxisBins(String axisType, int count) {
    switch (axisType.toLowerCase()) {
      case 'rpm':
        return _generateRPMBins(count);
      case 'map':
      case 'load':
      case 'pressure':
        return _generateMAPBins(count);
      case 'tps':
      case 'throttle':
        return _generateTPSBins(count);
      case 'boost':
        return _generateBoostBins(count);
      case 'coolant':
      case 'clt':
        return _generateCoolantBins(count);
      case 'intake':
      case 'iat':
        return _generateIntakeBins(count);
      default:
        // Default to linear 0-100 range
        return List.generate(count, (i) => i * (100.0 / (count - 1)));
    }
  }
  
  static List<double> _generateRPMBins(int count) {
    // Standard RPM range 500-7500
    const minRpm = 500.0;
    const maxRpm = 7500.0;
    
    return List.generate(count, (i) {
      final ratio = i / (count - 1);
      // Use slight curve to give more resolution at lower RPMs
      final curvedRatio = ratio * ratio * 0.3 + ratio * 0.7;
      return minRpm + (maxRpm - minRpm) * curvedRatio;
    });
  }
  
  static List<double> _generateMAPBins(int count) {
    // Standard MAP range 20-130 kPa (naturally aspirated)
    const minMap = 20.0;
    const maxMap = 130.0;
    
    return List.generate(count, (i) => 
      minMap + (maxMap - minMap) * i / (count - 1)
    );
  }
  
  static List<double> _generateTPSBins(int count) {
    // Standard TPS range 0-100%
    return List.generate(count, (i) => i * (100.0 / (count - 1)));
  }
  
  static List<double> _generateBoostBins(int count) {
    // Boost range 0-30 PSI
    return List.generate(count, (i) => i * (30.0 / (count - 1)));
  }
  
  static List<double> _generateCoolantBins(int count) {
    // Coolant temp range -20°C to 120°C
    const minTemp = -20.0;
    const maxTemp = 120.0;
    
    return List.generate(count, (i) => 
      minTemp + (maxTemp - minTemp) * i / (count - 1)
    );
  }
  
  static List<double> _generateIntakeBins(int count) {
    // Intake air temp range -10°C to 60°C
    const minTemp = -10.0;
    const maxTemp = 60.0;
    
    return List.generate(count, (i) => 
      minTemp + (maxTemp - minTemp) * i / (count - 1)
    );
  }
}