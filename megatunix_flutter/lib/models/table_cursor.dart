/// Table Cursor Models for Real-time ECU Position Tracking System
/// 
/// This module provides a complete real-time cursor system for table visualization that:
/// - Tracks live ECU operating points with interpolated sub-cell positioning
/// - Renders smooth 3-second fade trails showing operational history
/// - Applies exponential smoothing and anti-jitter filtering for professional appearance
/// - Integrates seamlessly with TunerStudio-style table editing workflows
/// - Supports 10Hz update rates with optimized performance and memory management
/// 
/// The system is designed to match TunerStudio Ultra's live cursor functionality
/// while providing enhanced visual feedback and smoother operation.

import 'dart:ui';
import 'dart:math' as math;

/// Precise cursor position within a table with sub-cell interpolation
/// 
/// Represents the exact location of the cursor based on ECU data values,
/// providing fractional positioning for smooth cursor movement between cells.
/// Position is calculated by interpolating ECU values against table axis data.
class TableCursorPosition {
  /// Interpolated X position (column + fractional part for sub-cell precision)
  /// Range: 0.0 to (table_width - 1.0), where fractional part indicates 
  /// position within the cell (0.5 = center of cell)
  final double x;
  
  /// Interpolated Y position (row + fractional part for sub-cell precision)  
  /// Range: 0.0 to (table_height - 1.0), where fractional part indicates
  /// position within the cell (0.5 = center of cell)
  final double y;
  
  /// Timestamp when this position was calculated from ECU data
  /// Used for trail fade calculations and age-based filtering
  final DateTime timestamp;
  
  const TableCursorPosition({
    required this.x,
    required this.y,
    required this.timestamp,
  });

  /// Create position from live ECU values by interpolating against table axes
  /// 
  /// This is the primary factory for converting raw ECU data into cursor positions:
  /// - Takes current ECU readings (e.g., RPM=3500, MAP=85 kPa)
  /// - Interpolates against table axis values to find exact position
  /// - Handles edge cases (values outside table range, single-point axes)
  /// - Returns position with current timestamp for trail management
  /// 
  /// Example: RPM=3500 in axis [1000,2000,4000,6000] -> x position ≈ 2.25
  factory TableCursorPosition.fromValues(
    double xValue,    // Current ECU X-axis value (e.g., RPM reading)
    double yValue,    // Current ECU Y-axis value (e.g., MAP reading)
    List<double> xAxis, // Table X-axis breakpoints (e.g., RPM values)
    List<double> yAxis, // Table Y-axis breakpoints (e.g., MAP values)
  ) {
    final xPos = _interpolatePosition(xValue, xAxis);
    final yPos = _interpolatePosition(yValue, yAxis);
    
    return TableCursorPosition(
      x: xPos,
      y: yPos,
      timestamp: DateTime.now(),
    );
  }

  /// Interpolate ECU value position within table axis breakpoints
  /// 
  /// Converts a continuous ECU value into a fractional table position:
  /// - Linear interpolation between adjacent breakpoints
  /// - Clamps to valid table range (0.0 to axis.length-1.0)
  /// - Handles edge cases: empty axes, single values, duplicate breakpoints
  /// 
  /// Returns fractional position where integer part = cell index, 
  /// fractional part = position within cell (0.0=left/top, 1.0=right/bottom)
  static double _interpolatePosition(double value, List<double> axis) {
    if (axis.isEmpty) return 0.0;
    if (axis.length == 1) return 0.0;
    
    // Clamp values outside table range to edges
    if (value <= axis.first) return 0.0;
    if (value >= axis.last) return axis.length - 1.0;
    
    // Find position by linear interpolation between adjacent breakpoints
    for (int i = 0; i < axis.length - 1; i++) {
      if (value >= axis[i] && value <= axis[i + 1]) {
        final range = axis[i + 1] - axis[i];
        if (range == 0) return i.toDouble(); // Handle duplicate breakpoints
        
        final fraction = (value - axis[i]) / range;
        return i + fraction;
      }
    }
    
    return axis.length - 1.0;
  }

  /// Get discrete cell coordinates for cell-based operations
  /// Returns (row, col) as integers by rounding fractional positions
  (int row, int col) get cellCoordinates => (y.round(), x.round());
  
  /// Get sub-cell positioning fractions for smooth rendering
  /// Returns fractional parts (0.0-1.0) indicating position within cell
  (double rowFraction, double colFraction) get fractions => (y - y.floor(), x - x.floor());
  
  /// Calculate time elapsed since this position was recorded
  /// Used for trail fade calculations and stale data detection
  Duration get age => DateTime.now().difference(timestamp);
  
  /// Validate position is within table bounds
  /// Prevents rendering cursors outside valid table area
  bool isValidFor(int rows, int cols) {
    return x >= 0 && x < cols && y >= 0 && y < rows;
  }

  @override
  String toString() => 'TableCursorPosition(x: $x, y: $y, age: ${age.inMilliseconds}ms)';
}

/// Cursor trail point for fade effect
class CursorTrailPoint {
  final TableCursorPosition position;
  final double opacity;      // 0.0 to 1.0, calculated from age
  final Color color;         // Color with applied opacity
  
  const CursorTrailPoint({
    required this.position,
    required this.opacity,
    required this.color,
  });

  /// Create trail point with fade calculation
  factory CursorTrailPoint.withFade(
    TableCursorPosition position, {
    Duration maxAge = const Duration(seconds: 3), // 3-second trail
    Color baseColor = const Color(0xFFFF1493), // Neon pink
  }) {
    final age = position.age;
    final ageRatio = age.inMilliseconds / maxAge.inMilliseconds;
    final opacity = (1.0 - ageRatio.clamp(0.0, 1.0));
    
    return CursorTrailPoint(
      position: position,
      opacity: opacity,
      color: baseColor.withOpacity(opacity),
    );
  }

  /// Check if trail point should be rendered
  bool get isVisible => opacity > 0.05;
}

/// Complete cursor state with trail - SIMPLIFIED VERSION WITH SMOOTHING
class TableCursor {
  final TableCursorPosition? currentPosition;
  final List<TableCursorPosition> trailPositions;
  final bool isVisible;
  final TableCursorPosition? _smoothedPosition; // Internal smoothed position for display
  
  const TableCursor({
    this.currentPosition,
    this.trailPositions = const [],
    this.isVisible = true,
    TableCursorPosition? smoothedPosition,
  }) : _smoothedPosition = smoothedPosition;

  /// Create cursor with no position (hidden)
  const TableCursor.hidden() : this(isVisible: false);

  /// Update cursor with new position and smoothing
  TableCursor updatePosition(TableCursorPosition newPosition) {
    // Apply smoothing to reduce jitter
    final smoothedPos = _applySmoothingFilter(newPosition);
    
    final updatedTrail = <TableCursorPosition>[];
    
    // Add smoothed previous position to trail (not raw position)
    if (_smoothedPosition != null) {
      updatedTrail.add(_smoothedPosition!);
    }
    
    // Add all existing trail positions
    updatedTrail.addAll(trailPositions);
    
    // Only remove positions older than 3 seconds - NO OTHER FILTERING
    final cutoffTime = DateTime.now().subtract(const Duration(seconds: 3));
    final filteredTrail = updatedTrail.where((pos) => pos.timestamp.isAfter(cutoffTime)).toList();
    
    // Limit to 200 points for performance
    while (filteredTrail.length > 200) {
      filteredTrail.removeAt(0);
    }
    
    return TableCursor(
      currentPosition: newPosition, // Keep raw position for reference
      trailPositions: filteredTrail,
      isVisible: true,
      smoothedPosition: smoothedPos, // Store smoothed for display
    );
  }

  /// Apply exponential smoothing filter to reduce jitter
  TableCursorPosition _applySmoothingFilter(TableCursorPosition newPosition) {
    final smoothedPos = _smoothedPosition;
    if (smoothedPos == null) {
      return newPosition; // First position, no smoothing needed
    }
    
    const double smoothingFactor = 0.4; // Higher = more responsive, lower = smoother
    const double minMovement = 0.03; // Minimum movement threshold to prevent micro-jitter
    
    final dx = newPosition.x - smoothedPos.x;
    final dy = newPosition.y - smoothedPos.y;
    final movement = math.sqrt(dx * dx + dy * dy);
    
    // If movement is too small, ignore it to prevent jitter
    if (movement < minMovement) {
      return TableCursorPosition(
        x: smoothedPos.x,
        y: smoothedPos.y,
        timestamp: newPosition.timestamp, // Keep current timestamp
      );
    }
    
    // Apply exponential smoothing
    final smoothedX = smoothedPos.x + (newPosition.x - smoothedPos.x) * smoothingFactor;
    final smoothedY = smoothedPos.y + (newPosition.y - smoothedPos.y) * smoothingFactor;
    
    return TableCursorPosition(
      x: smoothedX,
      y: smoothedY,
      timestamp: newPosition.timestamp,
    );
  }

  /// Clear cursor and trail
  TableCursor clear() => const TableCursor.hidden();
  
  /// Get all positions (trail + current) for line drawing
  List<CursorTrailPoint> get allPoints {
    final points = <CursorTrailPoint>[];
    final now = DateTime.now();
    const trailDuration = Duration(seconds: 3);
    
    // Add trail positions with calculated opacity based on age
    for (final position in trailPositions) {
      final age = now.difference(position.timestamp);
      final ageRatio = age.inMilliseconds / trailDuration.inMilliseconds;
      final opacity = (1.0 - ageRatio.clamp(0.0, 1.0)) * 0.7; // Max 0.7 for trail
      
      if (opacity > 0.05) {
        points.add(CursorTrailPoint(
          position: position,
          opacity: opacity,
          color: const Color(0xFFFF1493).withOpacity(opacity), // Neon pink
        ));
      }
    }
    
    // Add smoothed current position as brightest point (use smoothed for display)
    final displayPosition = _smoothedPosition ?? currentPosition;
    if (displayPosition != null && isVisible) {
      points.add(CursorTrailPoint(
        position: displayPosition,
        opacity: 1.0,
        color: const Color(0xFFFF1493), // Bright neon pink
      ));
    }
    
    return points;
  }

  /// Check if cursor has any visible elements
  bool get hasVisibleElements {
    if (currentPosition != null && isVisible) return true;
    
    // Check if any trail positions are still visible (within 3 seconds)
    final cutoffTime = DateTime.now().subtract(const Duration(seconds: 3));
    return trailPositions.any((pos) => pos.timestamp.isAfter(cutoffTime));
  }

  /// Get debug info about trail
  String get debugInfo {
    return 'TableCursor(current: $currentPosition, trail: ${trailPositions.length} points, visible: $isVisible)';
  }
}