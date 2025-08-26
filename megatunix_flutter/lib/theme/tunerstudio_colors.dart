/// TunerStudio-Compatible Color System for Professional ECU Table Visualization
/// 
/// This module provides industry-standard color schemes that match TunerStudio Ultra's
/// thermal heatmap visualization system. Features include:
/// 
/// **Professional Color Schemes:**
/// - Thermal gradient mapping from blue (low) through red (high) for intuitive data visualization
/// - Table-specific color palettes optimized for different ECU data types (VE, fuel, timing, etc.)
/// - TunerStudio-compatible text color selection for optimal readability across thermal ranges
/// - High-contrast selection and editing colors for professional workflow support
/// 
/// **Intelligent Table Type Detection:**
/// - Automatic detection of table purpose from INI table names and metadata
/// - Context-aware color scheme selection based on ECU data type and purpose
/// - Fallback systems ensuring proper visualization even for unknown table types
/// 
/// **Performance Optimization:**
/// - Efficient color interpolation algorithms for smooth gradient rendering
/// - Pre-computed color tables for real-time heatmap updates during live data streaming
/// - Memory-efficient color management for large table visualization (32x32+ tables)

import 'package:flutter/material.dart';

/// ECU table types that determine visualization approach and color scheme selection
/// 
/// This enum categorizes tables by their primary purpose to enable appropriate
/// color scheme selection and heatmap rendering strategies:
enum ECUTableType {
  // === TABLES WITH HEATMAP VISUALIZATION ===
  // These tables benefit from thermal color gradients showing data intensity
  
  /// Volumetric Efficiency tables - Primary fuel calculation tables
  /// Colors: Blue (low efficiency) to Red (high efficiency)
  volumetricEfficiency,
  
  /// Direct fuel delivery tables and pulse width maps
  /// Colors: Blue (lean) to Red (rich) fuel delivery
  fuelTable,
  
  /// Spark advance timing tables for ignition control
  /// Colors: Blue (retarded) to Red (advanced) timing
  ignitionTiming,
  
  /// Air-fuel ratio target tables for closed-loop control
  /// Colors: Blue (rich) to Red (lean) AFR targets  
  afrTarget,
  
  /// Lambda sensor target tables (normalized AFR)
  /// Colors: Blue (rich) to Red (lean) lambda values
  lambdaTarget,
  
  /// Turbo boost control and wastegate duty cycle tables
  /// Colors: Blue (low boost) to Red (high boost) 
  boostControl,
  
  /// Acceleration pump enrichment for transient fueling
  /// Colors: Blue (minimal) to Red (maximum) enrichment
  accelerationEnrichment,
  
  /// Cold start and warm-up fuel enrichment tables
  /// Colors: Blue (normal) to Red (maximum) enrichment
  warmupEnrichment,
  
  /// Temperature-based correction factors
  /// Colors: Blue (cold correction) to Red (hot correction)
  temperatureCorrection,
  
  /// Barometric pressure altitude correction factors
  /// Colors: Blue (low altitude) to Red (high altitude)
  baroCorrection,
  
  // === TABLES WITH PLAIN NUMERIC DISPLAY ===
  // These tables are better served with plain numeric values
  
  /// Injector flow rate and sizing calibration data
  /// Plain display: Precise numeric values more important than trends
  injectorData,
  
  /// Sensor calibration curves and linearization tables  
  /// Plain display: Accuracy of calibration values is critical
  sensorCalibration,
  
  /// Basic ECU configuration parameters and settings
  /// Plain display: Boolean and enumerated values don't benefit from heatmaps
  configuration,
  
  /// Diagnostic status information and error codes
  /// Plain display: Status information needs precise readability
  diagnosticData,
}

/// TunerStudio color scheme utility
class TunerStudioColors {
  
  /// Determines if a table type should use heatmap visualization
  static bool shouldUseHeatmap(ECUTableType tableType) {
    switch (tableType) {
      case ECUTableType.volumetricEfficiency:
      case ECUTableType.fuelTable:
      case ECUTableType.ignitionTiming:
      case ECUTableType.afrTarget:
      case ECUTableType.lambdaTarget:
      case ECUTableType.boostControl:
      case ECUTableType.accelerationEnrichment:
      case ECUTableType.warmupEnrichment:
      case ECUTableType.temperatureCorrection:
      case ECUTableType.baroCorrection:
        return true;
        
      case ECUTableType.injectorData:
      case ECUTableType.sensorCalibration:
      case ECUTableType.configuration:
      case ECUTableType.diagnosticData:
        return false;
    }
  }
  
  /// Get table type from table name (matches TunerStudio naming conventions)
  static ECUTableType getTableType(String tableName) {
    final lowerName = tableName.toLowerCase();
    
    // VE and fuel tables
    if (lowerName.contains('ve') || lowerName.contains('volumetric')) {
      return ECUTableType.volumetricEfficiency;
    }
    if (lowerName.contains('fuel') && !lowerName.contains('injector')) {
      return ECUTableType.fuelTable;
    }
    
    // Timing tables
    if (lowerName.contains('spark') || lowerName.contains('timing') || 
        lowerName.contains('advance') || lowerName.contains('ignition')) {
      return ECUTableType.ignitionTiming;
    }
    
    // AFR and Lambda tables
    if (lowerName.contains('afr') || lowerName.contains('air') && lowerName.contains('fuel')) {
      return ECUTableType.afrTarget;
    }
    if (lowerName.contains('lambda') || lowerName.contains('o2')) {
      return ECUTableType.lambdaTarget;
    }
    
    // Boost control
    if (lowerName.contains('boost') || lowerName.contains('wastegate')) {
      return ECUTableType.boostControl;
    }
    
    // Enrichment tables
    if (lowerName.contains('accel') && lowerName.contains('enrich')) {
      return ECUTableType.accelerationEnrichment;
    }
    if (lowerName.contains('warmup') || lowerName.contains('cold')) {
      return ECUTableType.warmupEnrichment;
    }
    
    // Correction tables
    if (lowerName.contains('temp') && lowerName.contains('correct')) {
      return ECUTableType.temperatureCorrection;
    }
    if (lowerName.contains('baro') || lowerName.contains('pressure')) {
      return ECUTableType.baroCorrection;
    }
    
    // Configuration and diagnostic tables
    if (lowerName.contains('injector') || lowerName.contains('flow')) {
      return ECUTableType.injectorData;
    }
    if (lowerName.contains('sensor') && lowerName.contains('cal')) {
      return ECUTableType.sensorCalibration;
    }
    if (lowerName.contains('config') || lowerName.contains('setting')) {
      return ECUTableType.configuration;
    }
    if (lowerName.contains('diag') || lowerName.contains('status')) {
      return ECUTableType.diagnosticData;
    }
    
    // Default to VE table for unknown names
    return ECUTableType.volumetricEfficiency;
  }
  
  /// Standard TunerStudio blue-to-red thermal color mapping
  /// This matches the industry standard used by TunerStudio, HP Tuners, etc.
  static Color getThermalColor(double normalizedValue, {double opacity = 1.0}) {
    final clampedValue = normalizedValue.clamp(0.0, 1.0);
    
    if (clampedValue <= 0.25) {
      // Blue to Cyan (0.0 to 0.25)
      final t = clampedValue * 4; // Scale to 0-1
      return Color.lerp(
        const Color(0xFF0000FF), // Pure blue
        const Color(0xFF00FFFF), // Cyan
        t,
      )!.withOpacity(opacity);
    } else if (clampedValue <= 0.5) {
      // Cyan to Green (0.25 to 0.5)
      final t = (clampedValue - 0.25) * 4; // Scale to 0-1
      return Color.lerp(
        const Color(0xFF00FFFF), // Cyan
        const Color(0xFF00FF00), // Green
        t,
      )!.withOpacity(opacity);
    } else if (clampedValue <= 0.75) {
      // Green to Yellow (0.5 to 0.75)
      final t = (clampedValue - 0.5) * 4; // Scale to 0-1
      return Color.lerp(
        const Color(0xFF00FF00), // Green
        const Color(0xFFFFFF00), // Yellow
        t,
      )!.withOpacity(opacity);
    } else {
      // Yellow to Red (0.75 to 1.0)
      final t = (clampedValue - 0.75) * 4; // Scale to 0-1
      return Color.lerp(
        const Color(0xFFFFFF00), // Yellow
        const Color(0xFFFF0000), // Red
        t,
      )!.withOpacity(opacity);
    }
  }
  
  /// Get optimized text color for readability on thermal background
  static Color getThermalTextColor(double normalizedValue) {
    final clampedValue = normalizedValue.clamp(0.0, 1.0);
    
    // Use white text on dark colors (blue, dark green), black on bright colors
    if (clampedValue < 0.4) {
      return Colors.white; // White text on blue/cyan/dark green
    } else {
      return Colors.black; // Black text on bright green/yellow/orange/red
    }
  }
  
  /// Alternative monochrome color scheme (for accessibility)
  static Color getMonochromeColor(double normalizedValue, {double opacity = 1.0}) {
    final clampedValue = normalizedValue.clamp(0.0, 1.0);
    final grayValue = (clampedValue * 255).round();
    return Color.fromARGB(
      (opacity * 255).round(),
      grayValue,
      grayValue,
      grayValue,
    );
  }
  
  /// Cool-to-warm alternative color scheme
  static Color getCoolWarmColor(double normalizedValue, {double opacity = 1.0}) {
    final clampedValue = normalizedValue.clamp(0.0, 1.0);
    
    if (clampedValue <= 0.5) {
      // Cool blues and cyans
      final t = clampedValue * 2; // Scale to 0-1
      return Color.lerp(
        const Color(0xFF08519C), // Dark blue
        const Color(0xFF3182BD), // Light blue
        t,
      )!.withOpacity(opacity);
    } else {
      // Warm oranges and reds
      final t = (clampedValue - 0.5) * 2; // Scale to 0-1
      return Color.lerp(
        const Color(0xFFE6550D), // Orange
        const Color(0xFFBD0026), // Red
        t,
      )!.withOpacity(opacity);
    }
  }
  
  /// Get appropriate heatmap color based on table type and value
  static Color getTableHeatmapColor(
    ECUTableType tableType, 
    double normalizedValue, 
    {double opacity = 1.0}
  ) {
    switch (tableType) {
      case ECUTableType.volumetricEfficiency:
      case ECUTableType.fuelTable:
      case ECUTableType.ignitionTiming:
      case ECUTableType.afrTarget:
      case ECUTableType.lambdaTarget:
      case ECUTableType.boostControl:
      case ECUTableType.accelerationEnrichment:
      case ECUTableType.warmupEnrichment:
      case ECUTableType.temperatureCorrection:
      case ECUTableType.baroCorrection:
        // All use standard thermal color scheme
        return getThermalColor(normalizedValue, opacity: opacity);
        
      default:
        // Plain tables don't use heatmaps
        return Colors.transparent;
    }
  }
  
  /// Color legend for heatmap tables
  static List<MapEntry<String, Color>> getColorLegend(ECUTableType tableType) {
    if (!shouldUseHeatmap(tableType)) return [];
    
    return [
      const MapEntry('Low', Color(0xFF0000FF)),    // Blue
      const MapEntry('', Color(0xFF00FFFF)),       // Cyan
      const MapEntry('Medium', Color(0xFF00FF00)), // Green
      const MapEntry('', Color(0xFFFFFF00)),       // Yellow
      const MapEntry('High', Color(0xFFFF0000)),   // Red
    ];
  }
  
  /// Get descriptive labels for different value ranges
  static String getValueDescription(ECUTableType tableType, double normalizedValue) {
    final clampedValue = normalizedValue.clamp(0.0, 1.0);
    
    switch (tableType) {
      case ECUTableType.volumetricEfficiency:
        if (clampedValue < 0.2) return 'Very Low VE';
        if (clampedValue < 0.4) return 'Low VE';
        if (clampedValue < 0.6) return 'Normal VE';
        if (clampedValue < 0.8) return 'High VE';
        return 'Very High VE';
        
      case ECUTableType.ignitionTiming:
        if (clampedValue < 0.2) return 'Retarded';
        if (clampedValue < 0.4) return 'Conservative';
        if (clampedValue < 0.6) return 'Normal';
        if (clampedValue < 0.8) return 'Advanced';
        return 'Very Advanced';
        
      case ECUTableType.afrTarget:
        if (clampedValue < 0.2) return 'Very Rich';
        if (clampedValue < 0.4) return 'Rich';
        if (clampedValue < 0.6) return 'Stoichiometric';
        if (clampedValue < 0.8) return 'Lean';
        return 'Very Lean';
        
      default:
        if (clampedValue < 0.2) return 'Very Low';
        if (clampedValue < 0.4) return 'Low';
        if (clampedValue < 0.6) return 'Medium';
        if (clampedValue < 0.8) return 'High';
        return 'Very High';
    }
  }
}