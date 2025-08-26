import 'package:flutter/material.dart';
import 'dart:math';
import '../theme/ecu_theme.dart';

/// VE Table 3D View Widget
/// Provides interactive 3D visualization of VE table data with proper math and viewport handling
class VETable3DView extends StatefulWidget {
  final List<List<double>> veTable;
  final List<double> rpmAxis;
  final List<double> mapAxis;
  final bool showGrid;
  final bool showValues;
  final bool showHeatmap;
  final int selectedRow;
  final int selectedCol;
  final Function(int, int) onCellSelected;
  final Function(int, int, double) onCellEdited;

  const VETable3DView({
    super.key,
    required this.veTable,
    required this.rpmAxis,
    required this.mapAxis,
    required this.showGrid,
    required this.showValues,
    required this.showHeatmap,
    required this.selectedRow,
    required this.selectedCol,
    required this.onCellSelected,
    required this.onCellEdited,
  });

  @override
  State<VETable3DView> createState() => _VETable3DViewState();
}

class _VETable3DViewState extends State<VETable3DView>
    with TickerProviderStateMixin {
  // 3D View State
  double _rotationX = -25.0; // Pitch (looking down)
  double _rotationY = 35.0;  // Yaw (rotation around Y axis)
  double _zoom = 1.0;        // Zoom level
  Offset _panOffset = Offset.zero; // Pan offset
  
  // Animation Controllers
  late AnimationController _rotationController;
  late Animation<double> _rotationAnimation;
  
  // Gesture State
  Offset? _lastPanPoint;
  double? _lastZoomDistance;

  @override
  void initState() {
    super.initState();
    
    // Initialize rotation animation
    _rotationController = AnimationController(
      duration: const Duration(seconds: 20),
      vsync: this,
    );

    _rotationAnimation = Tween<double>(
      begin: 0.0,
      end: 1.0,
    ).animate(CurvedAnimation(
      parent: _rotationController,
      curve: Curves.linear,
    ));

    // Start auto-rotation
    _rotationController.repeat();

    // Listen to rotation animation
    _rotationAnimation.addListener(() {
      setState(() {
        _rotationY = 35.0 + _rotationAnimation.value * 360.0;
      });
    });
  }

  void _resetView() {
    setState(() {
      _rotationX = -25.0;
      _rotationY = 35.0;
      _zoom = 1.0;
      _panOffset = Offset.zero;
      _rotationController.stop();
    });
  }

  void _zoomIn() {
    setState(() {
      _zoom = (_zoom * 1.2).clamp(0.3, 5.0);
    });
  }

  void _zoomOut() {
    setState(() {
      _zoom = (_zoom / 1.2).clamp(0.3, 5.0);
    });
  }

  @override
  void dispose() {
    _rotationController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Container(
      color: Theme.of(context).colorScheme.surface,
      child: Column(
        children: [
          // 3D View Controls
          Container(
            padding: const EdgeInsets.all(16),
            child: Row(
              children: [
                // Auto-rotation toggle
                ElevatedButton.icon(
                  onPressed: () {
                    if (_rotationController.isAnimating) {
                      _rotationController.stop();
                    } else {
                      _rotationController.repeat();
                    }
                  },
                  icon: Icon(
                    _rotationController.isAnimating ? Icons.pause : Icons.play_arrow,
                  ),
                  label: Text(
                    _rotationController.isAnimating ? 'Pause' : 'Auto-Rotate',
                  ),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: ECUTheme.getAccentColor('primary').withOpacity(0.1),
                    foregroundColor: ECUTheme.getAccentColor('primary'),
                  ),
                ),
                
                const SizedBox(width: 16),
                
                // Reset view button
                ElevatedButton.icon(
                  onPressed: _resetView,
                  icon: const Icon(Icons.refresh),
                  label: const Text('Reset View'),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: ECUTheme.getAccentColor('secondary').withOpacity(0.1),
                    foregroundColor: ECUTheme.getAccentColor('secondary'),
                  ),
                ),
                
                const SizedBox(width: 16),
                
                // Zoom controls
                Row(
                  children: [
                    IconButton(
                      onPressed: _zoomIn,
                      icon: const Icon(Icons.zoom_in),
                      tooltip: 'Zoom In',
                    ),
                    IconButton(
                      onPressed: _zoomOut,
                      icon: const Icon(Icons.zoom_out),
                      tooltip: 'Zoom Out',
                    ),
                  ],
                ),
                
                const Spacer(),
                
                // View info
                Container(
                  padding: const EdgeInsets.all(8),
                  decoration: BoxDecoration(
                    color: ECUTheme.getAccentColor('info').withOpacity(0.1),
                    borderRadius: BorderRadius.circular(8),
                    border: Border.all(
                      color: ECUTheme.getAccentColor('info').withOpacity(0.3),
                      width: 1,
                    ),
                  ),
                  child: Text(
                    'Drag to rotate & pan • Pinch to zoom • Auto-rotation stops on interaction',
                    style: TextStyle(
                      fontSize: 12,
                      color: ECUTheme.getAccentColor('info'),
                    ),
                  ),
                ),
              ],
            ),
          ),
          
          // 3D Visualization Area
          Expanded(
            child: Container(
              margin: const EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: Theme.of(context).colorScheme.surface,
                borderRadius: BorderRadius.circular(12),
                border: Border.all(
                  color: Theme.of(context).colorScheme.outline.withOpacity(0.2),
                  width: 1,
                ),
              ),
              child: ClipRRect(
                borderRadius: BorderRadius.circular(12),
                child: GestureDetector(
                  onScaleStart: (details) {
                    // Stop auto-rotation when user interacts
                    if (_rotationController.isAnimating) {
                      _rotationController.stop();
                    }
                    _lastPanPoint = details.focalPoint;
                    _lastZoomDistance = 1.0; // Initialize zoom distance
                  },
                  onScaleUpdate: (details) {
                    setState(() {
                      // Stop auto-rotation when user interacts
                      if (_rotationController.isAnimating) {
                        _rotationController.stop();
                      }

                      if (details.pointerCount == 1) {
                        // Single finger - rotate and pan
                        if (_lastPanPoint != null) {
                          final delta = details.focalPoint - _lastPanPoint!;
                          _rotationY += delta.dx * 0.5;
                          _rotationX += delta.dy * 0.5;
                          _rotationX = _rotationX.clamp(-90.0, 90.0);
                          _lastPanPoint = details.focalPoint;
                        }
                      } else if (details.pointerCount >= 2) {
                        // Multi-finger - zoom
                        if (_lastZoomDistance != null) {
                          final scaleFactor = details.scale / _lastZoomDistance!;
                          _zoom *= scaleFactor;
                          _zoom = _zoom.clamp(0.3, 5.0);
                          _lastZoomDistance = details.scale;
                        }
                      }
                    });
                  },
                  onScaleEnd: (details) {
                    _lastPanPoint = null;
                    _lastZoomDistance = null;
                  },
                  child: CustomPaint(
                    painter: VETable3DPainter(
                      veTable: widget.veTable,
                      rpmAxis: widget.rpmAxis,
                      mapAxis: widget.mapAxis,
                      rotationX: _rotationX,
                      rotationY: _rotationY,
                      zoom: _zoom,
                      panOffset: _panOffset,
                      showGrid: widget.showGrid,
                      showHeatmap: widget.showHeatmap,
                      selectedRow: widget.selectedRow,
                      selectedCol: widget.selectedCol,
                    ),
                    size: Size.infinite,
                  ),
                ),
              ),
            ),
          ),
          
          // 3D View Legend
          Container(
            padding: const EdgeInsets.all(16),
            child: Column(
              children: [
                // VE Values Legend
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Text(
                      'VE Values: ',
                      style: TextStyle(
                        fontSize: 12,
                        fontWeight: FontWeight.w600,
                        color: ECUTheme.getAccentColor('info'),
                      ),
                    ),
                    const SizedBox(width: 8),
                    // Low
                    Container(
                      width: 16,
                      height: 16,
                      decoration: BoxDecoration(
                        color: ECUTheme.getAccentColor('low'),
                        borderRadius: BorderRadius.circular(2),
                      ),
                    ),
                    const SizedBox(width: 4),
                    Text(
                      'Low',
                      style: TextStyle(
                        fontSize: 11,
                        color: ECUTheme.getAccentColor('info'),
                      ),
                    ),
                    const SizedBox(width: 12),
                    // Medium
                    Container(
                      width: 16,
                      height: 16,
                      decoration: BoxDecoration(
                        color: ECUTheme.getAccentColor('medium'),
                        borderRadius: BorderRadius.circular(2),
                      ),
                    ),
                    const SizedBox(width: 4),
                    Text(
                      'Medium',
                      style: TextStyle(
                        fontSize: 11,
                        color: ECUTheme.getAccentColor('info'),
                      ),
                    ),
                    const SizedBox(width: 12),
                    // High
                    Container(
                      width: 16,
                      height: 16,
                      decoration: BoxDecoration(
                        color: ECUTheme.getAccentColor('high'),
                        borderRadius: BorderRadius.circular(2),
                      ),
                    ),
                    const SizedBox(width: 4),
                    Text(
                      'High',
                      style: TextStyle(
                        fontSize: 11,
                        color: ECUTheme.getAccentColor('info'),
                      ),
                    ),
                  ],
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

/// Custom painter for 3D VE table visualization with proper math
class VETable3DPainter extends CustomPainter {
  final List<List<double>> veTable;
  final List<double> rpmAxis;
  final List<double> mapAxis;
  final double rotationX;
  final double rotationY;
  final double zoom;
  final Offset panOffset;
  final bool showGrid;
  final bool showHeatmap;
  final int selectedRow;
  final int selectedCol;
  
  VETable3DPainter({
    required this.veTable,
    required this.rpmAxis,
    required this.mapAxis,
    required this.rotationX,
    required this.rotationY,
    required this.zoom,
    required this.panOffset,
    required this.showGrid,
    required this.showHeatmap,
    required this.selectedRow,
    required this.selectedCol,
  });
  
  @override
  void paint(Canvas canvas, Size size) {
    if (veTable.isEmpty || rpmAxis.isEmpty || mapAxis.isEmpty) return;
    
    final center = Offset(size.width / 2, size.height / 2);
    
    // Calculate optimal cell size based on viewport
    final maxCellSize = min(size.width, size.height) / max(rpmAxis.length, mapAxis.length) * 0.8;
    final cellSize = maxCellSize * zoom;
    
    // Calculate table dimensions
    final tableWidth = mapAxis.length * cellSize;
    final tableHeight = rpmAxis.length * cellSize;
    
    // Pre-calculate rotation matrices
    final cosX = cos(rotationX * pi / 180);
    final sinX = sin(rotationX * pi / 180);
    final cosY = cos(rotationY * pi / 180);
    final sinY = sin(rotationY * pi / 180);
    
    // Apply transformations
    canvas.save();
    canvas.translate(center.dx + panOffset.dx, center.dy + panOffset.dy);
    
    // Draw 3D surface
    _draw3DSurface(canvas, size, cellSize, cosX, sinX, cosY, sinY);
    
    // Draw 3D axis labels that move with the model
    _draw3DAxisLabels(canvas, size, cellSize, cosX, sinX, cosY, sinY);
    
    // Draw selection indicator
    if (selectedRow >= 0 && selectedCol >= 0) {
      _drawSelectionIndicator(canvas, size, cellSize, cosX, sinX, cosY, sinY);
    }
    
    canvas.restore();
  }
  
  /// Draw 3D surface with proper projection
  void _draw3DSurface(Canvas canvas, Size size, double cellSize, double cosX, double sinX, double cosY, double sinY) {
    final maxHeight = cellSize * 0.8; // Height scale factor
    
    // Draw each cell as a 3D cube with proper faces
    for (int row = 0; row < veTable.length; row++) {
      for (int col = 0; col < veTable[row].length; col++) {
        final value = veTable[row][col];
        final height = (value / 100.0).clamp(0.0, 1.0) * maxHeight;
        
        // Calculate 3D position
        final x = (col - mapAxis.length / 2) * cellSize;
        final y = (row - rpmAxis.length / 2) * cellSize;
        final z = height;
        
        // Draw 3D cell with proper faces
        _draw3DCell(canvas, x, y, z, cellSize, value, cosX, sinX, cosY, sinY);
      }
    }
    
    // Draw grid lines if enabled
    if (showGrid) {
      _drawGridLines(canvas, size, cellSize, cosX, sinX, cosY, sinY);
    }
  }
  
  /// Transform 3D point to 2D screen coordinates
  Offset _transform3DPoint(double x, double y, double z, double cosX, double sinX, double cosY, double sinY) {
    // Apply Y rotation first
    final x1 = x * cosY - z * sinY;
    final z1 = x * sinY + z * cosY;
    
    // Apply X rotation
    final y1 = y * cosX - z1 * sinX;
    final z2 = y * sinX + z1 * cosX;
    
    // Project to 2D (simple orthographic projection)
    return Offset(x1, y1);
  }
  
  /// Draw individual 3D cell with proper faces
  void _draw3DCell(Canvas canvas, double x, double y, double z, double cellSize, double value, 
                   double cosX, double sinX, double cosY, double sinY) {
    // Define the 8 corners of the 3D cell
    final corners = [
      [x - cellSize/2, y - cellSize/2, 0],      // Bottom corners
      [x + cellSize/2, y - cellSize/2, 0],
      [x + cellSize/2, y + cellSize/2, 0],
      [x - cellSize/2, y + cellSize/2, 0],
      [x - cellSize/2, y - cellSize/2, z],      // Top corners
      [x + cellSize/2, y - cellSize/2, z],
      [x + cellSize/2, y + cellSize/2, z],
      [x - cellSize/2, y + cellSize/2, z],
    ];
    
    // Transform all corners to 2D
    final transformedCorners = corners.map((corner) {
      return _transform3DPoint(corner[0].toDouble(), corner[1].toDouble(), corner[2].toDouble(), cosX, sinX, cosY, sinY);
    }).toList();
    
    // Draw faces (quads) instead of individual rectangles
    if (showHeatmap) {
      final paint = Paint()
        ..color = _getCellColor(value).withOpacity(0.8)
        ..style = PaintingStyle.fill;
      
      // Top face (the main surface)
      final topFace = Path()
        ..moveTo(transformedCorners[4].dx, transformedCorners[4].dy)
        ..lineTo(transformedCorners[5].dx, transformedCorners[5].dy)
        ..lineTo(transformedCorners[6].dx, transformedCorners[6].dy)
        ..lineTo(transformedCorners[7].dx, transformedCorners[7].dy)
        ..close();
      canvas.drawPath(topFace, paint);
      
      // Side faces for 3D effect
      final sidePaint = Paint()
        ..color = _getCellColor(value).withOpacity(0.6)
        ..style = PaintingStyle.fill;
      
      // Front face
      final frontFace = Path()
        ..moveTo(transformedCorners[0].dx, transformedCorners[0].dy)
        ..lineTo(transformedCorners[1].dx, transformedCorners[1].dy)
        ..lineTo(transformedCorners[5].dx, transformedCorners[5].dy)
        ..lineTo(transformedCorners[4].dx, transformedCorners[4].dy)
        ..close();
      canvas.drawPath(frontFace, sidePaint);
      
      // Right face
      final rightFace = Path()
        ..moveTo(transformedCorners[1].dx, transformedCorners[1].dy)
        ..lineTo(transformedCorners[2].dx, transformedCorners[2].dy)
        ..lineTo(transformedCorners[6].dx, transformedCorners[6].dy)
        ..lineTo(transformedCorners[5].dx, transformedCorners[5].dy)
        ..close();
      canvas.drawPath(rightFace, sidePaint);
    }
    
    // Draw cell border
    final borderPaint = Paint()
      ..color = ECUTheme.getAccentColor('primary').withOpacity(0.3)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1;
    
    // Draw wireframe edges
    final edges = [
      [0, 1], [1, 2], [2, 3], [3, 0], // Bottom face
      [4, 5], [5, 6], [6, 7], [7, 4], // Top face
      [0, 4], [1, 5], [2, 6], [3, 7], // Vertical edges
    ];
    
    for (final edge in edges) {
      canvas.drawLine(
        transformedCorners[edge[0]], 
        transformedCorners[edge[1]], 
        borderPaint
      );
    }
  }
  
  /// Draw grid lines
  void _drawGridLines(Canvas canvas, Size size, double cellSize, double cosX, double sinX, double cosY, double sinY) {
    final gridPaint = Paint()
      ..color = ECUTheme.getAccentColor('primary').withOpacity(0.2)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 0.5;
    
    // Draw vertical lines (MAP axis)
    for (int col = 0; col <= mapAxis.length; col++) {
      final x = (col - mapAxis.length / 2) * cellSize;
      final start = _transform3DPoint(x, -rpmAxis.length / 2 * cellSize, 0, cosX, sinX, cosY, sinY);
      final end = _transform3DPoint(x, rpmAxis.length / 2 * cellSize, 0, cosX, sinX, cosY, sinY);
      canvas.drawLine(start, end, gridPaint);
    }
    
    // Draw horizontal lines (RPM axis)
    for (int row = 0; row <= rpmAxis.length; row++) {
      final y = (row - rpmAxis.length / 2) * cellSize;
      final start = _transform3DPoint(-mapAxis.length / 2 * cellSize, y, 0, cosX, sinX, cosY, sinY);
      final end = _transform3DPoint(mapAxis.length / 2 * cellSize, y, 0, cosX, sinX, cosY, sinY);
      canvas.drawLine(start, end, gridPaint);
    }
  }
  
  /// Draw 3D axis labels that move with the model
  void _draw3DAxisLabels(Canvas canvas, Size size, double cellSize, double cosX, double sinX, double cosY, double sinY) {
    final labelPaint = Paint()
      ..color = ECUTheme.getAccentColor('primary').withOpacity(0.8)
      ..style = PaintingStyle.fill;
    
    final textPainter = TextPainter(
      textDirection: TextDirection.ltr,
      textAlign: TextAlign.center,
    );
    
    // Draw MAP axis labels (X-axis) along the front edge
    for (int i = 0; i < mapAxis.length; i++) {
      final x = (i - mapAxis.length / 2) * cellSize;
      final y = -rpmAxis.length / 2 * cellSize; // Front edge
      final z = 0.0; // Ground level
      
      final position = _transform3DPoint(x, y, z, cosX, sinX, cosY, sinY);
      
      // Draw label background
      final labelRect = Rect.fromCenter(
        center: position,
        width: 40,
        height: 20,
      );
      
      final backgroundPaint = Paint()
        ..color = ECUTheme.getAccentColor('primary').withOpacity(0.2)
        ..style = PaintingStyle.fill;
      canvas.drawRect(labelRect, backgroundPaint);
      
      // Draw label text
      textPainter.text = TextSpan(
        text: '${mapAxis[i].toInt()}',
        style: TextStyle(
          color: ECUTheme.getAccentColor('primary'),
          fontSize: 10,
          fontWeight: FontWeight.bold,
        ),
      );
      
      textPainter.layout();
      textPainter.paint(
        canvas,
        position - Offset(textPainter.width / 2, textPainter.height / 2),
      );
    }
    
    // Draw RPM axis labels (Y-axis) along the left edge
    for (int i = 0; i < rpmAxis.length; i++) {
      final x = -mapAxis.length / 2 * cellSize; // Left edge
      final y = (i - rpmAxis.length / 2) * cellSize;
      final z = 0.0; // Ground level
      
      final position = _transform3DPoint(x, y, z, cosX, sinX, cosY, sinY);
      
      // Draw label background
      final labelRect = Rect.fromCenter(
        center: position,
        width: 50,
        height: 20,
      );
      
      final backgroundPaint = Paint()
        ..color = ECUTheme.getAccentColor('secondary').withOpacity(0.2)
        ..style = PaintingStyle.fill;
      canvas.drawRect(labelRect, backgroundPaint);
      
      // Draw label text
      textPainter.text = TextSpan(
        text: '${rpmAxis[i].toInt()}',
        style: TextStyle(
          color: ECUTheme.getAccentColor('secondary'),
          fontSize: 10,
          fontWeight: FontWeight.bold,
        ),
      );
      
      textPainter.layout();
      textPainter.paint(
        canvas,
        position - Offset(textPainter.width / 2, textPainter.height / 2),
      );
    }
    
    // Draw Z-axis label (VE values) at the top center
    final topCenter = _transform3DPoint(0, 0, cellSize * 0.8, cosX, sinX, cosY, sinY);
    
    final zLabelRect = Rect.fromCenter(
      center: topCenter,
      width: 60,
      height: 25,
    );
    
    final zBackgroundPaint = Paint()
      ..color = ECUTheme.getAccentColor('info').withOpacity(0.2)
      ..style = PaintingStyle.fill;
    canvas.drawRect(zLabelRect, zBackgroundPaint);
    
    textPainter.text = TextSpan(
      text: 'VE %',
      style: TextStyle(
        color: ECUTheme.getAccentColor('info'),
        fontSize: 12,
        fontWeight: FontWeight.bold,
      ),
    );
    
    textPainter.layout();
    textPainter.paint(
      canvas,
      topCenter - Offset(textPainter.width / 2, textPainter.height / 2),
    );
  }
  
  /// Draw selection indicator
  void _drawSelectionIndicator(Canvas canvas, Size size, double cellSize, double cosX, double sinX, double cosY, double sinY) {
    if (selectedRow < 0 || selectedCol < 0) return;
    
    final maxHeight = cellSize * 0.8;
    final value = veTable[selectedRow][selectedCol];
    final height = (value / 100.0).clamp(0.0, 1.0) * maxHeight;
    
    // Calculate 3D position
    final x = (selectedCol - mapAxis.length / 2) * cellSize;
    final y = (selectedRow - rpmAxis.length / 2) * cellSize;
    final z = height;
    
    // Apply transformations
    final position = _transform3DPoint(x, y, z, cosX, sinX, cosY, sinY);
    
    // Draw selection highlight
    final selectionPaint = Paint()
      ..color = ECUTheme.getAccentColor('selection')
      ..style = PaintingStyle.stroke
      ..strokeWidth = 3;
    
    final selectionRect = Rect.fromCenter(
      center: position,
      width: cellSize * 1.2,
      height: cellSize * 1.2,
    );
    
    canvas.drawRect(selectionRect, selectionPaint);
  }
  
  /// Get cell color based on value
  Color _getCellColor(double value) {
    if (!showHeatmap) {
      return ECUTheme.getAccentColor('primary');
    }
    
    final normalizedValue = ((value - 50.0) / 70.0).clamp(0.0, 1.0);
    
    if (normalizedValue < 0.5) {
      return Color.lerp(
        ECUTheme.getAccentColor('low'),
        ECUTheme.getAccentColor('medium'),
        normalizedValue * 2,
      )!;
    } else {
      return Color.lerp(
        ECUTheme.getAccentColor('medium'),
        ECUTheme.getAccentColor('high'),
        (normalizedValue - 0.5) * 2,
      )!;
    }
  }
  
  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => true;
}
