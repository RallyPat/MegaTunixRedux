/// Table Cursor Widget for Real-time ECU Position Display
/// SIMPLIFIED - Always show trail without complex filtering

import 'dart:math' as math;
import 'package:flutter/material.dart';
import '../models/table_cursor.dart';

/// Widget that renders cursor overlay on table cells
class TableCursorOverlay extends StatefulWidget {
  final TableCursor cursor;
  final int rows;
  final int cols;
  final double cellWidth;
  final double cellHeight;
  final bool enabled;
  
  const TableCursorOverlay({
    super.key,
    required this.cursor,
    required this.rows,
    required this.cols,
    required this.cellWidth,
    required this.cellHeight,
    this.enabled = true,
  });

  @override
  State<TableCursorOverlay> createState() => _TableCursorOverlayState();
}

class _TableCursorOverlayState extends State<TableCursorOverlay> {
  
  @override
  Widget build(BuildContext context) {
    if (!widget.enabled) {
      return const SizedBox.shrink();
    }
    
    // Debug info
    print('TableCursorOverlay: ${widget.cursor.debugInfo}');
    print('AllPoints count: ${widget.cursor.allPoints.length}');
    
    return CustomPaint(
      size: Size(
        widget.cols * widget.cellWidth,
        widget.rows * widget.cellHeight,
      ),
      painter: _CursorPainter(
        cursor: widget.cursor,
        cellWidth: widget.cellWidth,
        cellHeight: widget.cellHeight,
      ),
    );
  }
}

/// Custom painter for cursor and trail rendering - SIMPLIFIED
class _CursorPainter extends CustomPainter {
  final TableCursor cursor;
  final double cellWidth;
  final double cellHeight;
  
  _CursorPainter({
    required this.cursor,
    required this.cellWidth,
    required this.cellHeight,
  });

  @override
  void paint(Canvas canvas, Size size) {
    final points = cursor.allPoints;
    
    print('_CursorPainter.paint: ${points.length} points to draw');
    
    if (points.isEmpty) return;
    
    // Draw ALL trail lines - no complex filtering
    _drawSimpleTrail(canvas, points);
    
    // Draw current position cursor on top
    if (points.isNotEmpty) {
      _drawCurrentCursor(canvas, points.last);
    }
  }
  
  void _drawSimpleTrail(Canvas canvas, List<CursorTrailPoint> points) {
    if (points.length < 2) return;
    
    print('Drawing simple trail with ${points.length} points');
    
    // Draw lines between consecutive points, but skip large jumps
    for (int i = 0; i < points.length - 1; i++) {
      final currentPoint = points[i];
      final nextPoint = points[i + 1];
      
      // Calculate distance between points
      final dx = nextPoint.position.x - currentPoint.position.x;
      final dy = nextPoint.position.y - currentPoint.position.y;
      final distance = math.sqrt(dx * dx + dy * dy);
      
      // More strict distance check to prevent any unwanted lines
      if (distance <= 1.5) { // Reduced from 2.0 to 1.5 cells max distance
        final startPos = Offset(
          (currentPoint.position.x + 0.5) * cellWidth,
          (currentPoint.position.y + 0.5) * cellHeight,
        );
        final endPos = Offset(
          (nextPoint.position.x + 0.5) * cellWidth,
          (nextPoint.position.y + 0.5) * cellHeight,
        );
        
        // Also check time difference - don't connect points too far apart in time
        final timeDiff = nextPoint.position.timestamp.difference(currentPoint.position.timestamp);
        if (timeDiff.inMilliseconds <= 200) { // Max 200ms between connected points
          
          // Calculate average opacity for the line segment
          final avgOpacity = (currentPoint.opacity + nextPoint.opacity) / 2;
          
          // Draw the line segment
          final paint = Paint()
            ..color = const Color(0xFFFF1493).withOpacity(avgOpacity * 0.6)
            ..style = PaintingStyle.stroke
            ..strokeWidth = 3.0
            ..strokeCap = StrokeCap.round;
          
          canvas.drawLine(startPos, endPos, paint);
          
          // Add subtle glow
          final glowPaint = Paint()
            ..color = const Color(0xFFFF1493).withOpacity(avgOpacity * 0.3)
            ..style = PaintingStyle.stroke
            ..strokeWidth = 5.0
            ..strokeCap = StrokeCap.round
            ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 1.5);
          
          canvas.drawLine(startPos, endPos, glowPaint);
        }
      }
      // If distance > 1.5 or time > 200ms, don't draw connecting line
    }
  }
  
  void _drawCurrentCursor(Canvas canvas, CursorTrailPoint currentPoint) {
    final position = currentPoint.position;
    
    // Calculate pixel position
    final centerX = (position.x + 0.5) * cellWidth;
    final centerY = (position.y + 0.5) * cellHeight;
    final center = Offset(centerX, centerY);
    
    final baseSize = (cellWidth.clamp(20, 60) + cellHeight.clamp(20, 35)) / 4;
    
    _drawCrosshairs(canvas, center, baseSize);
    _drawCenterDot(canvas, center, baseSize * 0.3);
    _drawBorder(canvas, center, baseSize * 1.5);
  }
  
  void _drawCrosshairs(Canvas canvas, Offset center, double size) {
    final paint = Paint()
      ..color = const Color(0xFFFF1493) // Bright neon pink
      ..strokeWidth = 2.0
      ..style = PaintingStyle.stroke
      ..strokeCap = StrokeCap.round;
    
    // Horizontal line
    canvas.drawLine(
      Offset(center.dx - size, center.dy),
      Offset(center.dx + size, center.dy),
      paint,
    );
    
    // Vertical line
    canvas.drawLine(
      Offset(center.dx, center.dy - size),
      Offset(center.dx, center.dy + size),
      paint,
    );
  }
  
  void _drawCenterDot(Canvas canvas, Offset center, double radius) {
    final paint = Paint()
      ..color = const Color(0xFFFF1493) // Bright neon pink
      ..style = PaintingStyle.fill;
    
    canvas.drawCircle(center, radius, paint);
    
    // White inner dot for contrast
    final innerPaint = Paint()
      ..color = Colors.white
      ..style = PaintingStyle.fill;
    
    canvas.drawCircle(center, radius * 0.5, innerPaint);
  }
  
  void _drawBorder(Canvas canvas, Offset center, double radius) {
    final paint = Paint()
      ..color = const Color(0xFFFF1493).withOpacity(0.6)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 2.0;
    
    canvas.drawCircle(center, radius, paint);
  }

  @override
  bool shouldRepaint(covariant _CursorPainter oldDelegate) {
    return cursor != oldDelegate.cursor ||
           cellWidth != oldDelegate.cellWidth ||
           cellHeight != oldDelegate.cellHeight;
  }
}

/// Cursor information display widget
class CursorInfoWidget extends StatelessWidget {
  final TableCursor cursor;
  final String tableName;
  final String currentRpm;
  final String currentLoad;
  
  const CursorInfoWidget({
    super.key,
    required this.cursor,
    required this.tableName,
    required this.currentRpm,
    required this.currentLoad,
  });

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    
    if (!cursor.hasVisibleElements) {
      return const SizedBox.shrink();
    }
    
    final position = cursor.currentPosition;
    if (position == null) return const SizedBox.shrink();
    
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
      decoration: BoxDecoration(
        color: const Color(0xFFFF6B35).withOpacity(0.1),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: const Color(0xFFFF6B35).withOpacity(0.3),
        ),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(
            Icons.my_location,
            color: const Color(0xFFFF1493), // Neon pink
            size: 16,
          ),
          const SizedBox(width: 6),
          Text(
            'Live: $currentRpm RPM, $currentLoad kPa',
            style: theme.textTheme.bodySmall?.copyWith(
              color: const Color(0xFFFF1493), // Neon pink
              fontWeight: FontWeight.w500,
            ),
          ),
          const SizedBox(width: 8),
      Container(
        padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 2),
        decoration: BoxDecoration(
          color: const Color(0xFFFF1493).withOpacity(0.1), // Neon pink background
          borderRadius: BorderRadius.circular(16),
          border: Border.all(
            color: const Color(0xFFFF1493).withOpacity(0.3), // Neon pink border
          ),
        ),
            child: Text(
              '[${position.cellCoordinates.$2 + 1}, ${position.cellCoordinates.$1 + 1}] (${cursor.trailPositions.length} trail)',
              style: theme.textTheme.bodySmall?.copyWith(
                color: const Color(0xFFFF1493), // Neon pink
                fontWeight: FontWeight.w600,
                fontSize: 10,
              ),
            ),
          ),
        ],
      ),
    );
  }
}

/// Toggle button for cursor display
class CursorToggleButton extends StatelessWidget {
  final bool enabled;
  final VoidCallback onToggle;
  
  const CursorToggleButton({
    super.key,
    required this.enabled,
    required this.onToggle,
  });

  @override
  Widget build(BuildContext context) {
    return IconButton(
      onPressed: onToggle,
      tooltip: enabled ? 'Hide Live Cursor' : 'Show Live Cursor',
      icon: Icon(
        enabled ? Icons.my_location : Icons.location_disabled,
        color: enabled ? const Color(0xFFFF1493) : null, // Neon pink when enabled
      ),
      style: IconButton.styleFrom(
        backgroundColor: enabled 
            ? const Color(0xFFFF1493).withOpacity(0.1) // Neon pink background
            : Theme.of(context).colorScheme.surfaceContainerHighest,
      ),
    );
  }
}