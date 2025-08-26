import 'package:flutter/material.dart';
import 'dart:math';
import '../theme/ecu_theme.dart';
import '../services/ecu_service.dart';
import '../models/ecu_data.dart';

/// ECU Gauge Cluster Widget
/// Displays real-time ECU parameters with properly calculated gauges and tick marks
class ECUGaugeCluster extends StatefulWidget {
  final SpeeduinoData ecuData;
  final bool showLabels;
  final bool showValues;
  final bool showUnits;
  final double gaugeSize;
  final bool enableAnimations;

  const ECUGaugeCluster({
    super.key,
    required this.ecuData,
    this.showLabels = true,
    this.showValues = true,
    this.showUnits = true,
    this.gaugeSize = 120.0, // Increased from 80.0 to make gauges larger
    this.enableAnimations = true,
  });

  @override
  State<ECUGaugeCluster> createState() => _ECUGaugeClusterState();
}

class _ECUGaugeClusterState extends State<ECUGaugeCluster>
    with TickerProviderStateMixin {
  // Animation controllers for smooth gauge transitions
  late final AnimationController _rpmController;
  late final AnimationController _mapController;
  late final AnimationController _tpsController;
  late final AnimationController _coolantController;
  late final AnimationController _intakeController;
  late final AnimationController _batteryController;

  // Current gauge values for animation
  double _currentRpm = 0.0;      // RPM starts at 0
  double _currentMap = 0.0;      // MAP starts at 0
  double _currentTps = 0.0;      // TPS starts at 0
  double _currentCoolant = 0.0;  // Coolant starts at 0°C
  double _currentIntake = 0.0;   // Intake starts at 0°C
  double _currentBattery = 10.0; // Battery starts at 10V

  @override
  void initState() {
    super.initState();
    
    // Initialize animation controllers
    _rpmController = AnimationController(
      duration: const Duration(milliseconds: 300),
      vsync: this,
    );
    _mapController = AnimationController(
      duration: const Duration(milliseconds: 300),
      vsync: this,
    );
    _tpsController = AnimationController(
      duration: const Duration(milliseconds: 300),
      vsync: this,
    );
    _coolantController = AnimationController(
      duration: const Duration(milliseconds: 300),
      vsync: this,
    );
    _intakeController = AnimationController(
      duration: const Duration(milliseconds: 300),
      vsync: this,
    );
    _batteryController = AnimationController(
      duration: const Duration(milliseconds: 300),
      vsync: this,
    );

    // Set initial values
    // Don't call _updateGaugeValues here as it will use default (zero) values
    // It will be called when real data arrives via didUpdateWidget
  }

  @override
  void didUpdateWidget(ECUGaugeCluster oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.ecuData != widget.ecuData) {
      print('Gauge cluster updating with new data: RPM=${widget.ecuData.rpm}, MAP=${widget.ecuData.map}');
      _updateGaugeValues();
    }
  }

  void _updateGaugeValues() {
    print('Updating gauge values: RPM=${widget.ecuData.rpm}, MAP=${widget.ecuData.map}');
    
    if (!widget.enableAnimations) {
      setState(() {
        _currentRpm = widget.ecuData.rpm.toDouble();
        _currentMap = widget.ecuData.map.toDouble();
        _currentTps = widget.ecuData.tps.toDouble();
        _currentCoolant = widget.ecuData.coolantTemp.toDouble();
        _currentIntake = widget.ecuData.intakeTemp.toDouble();
        _currentBattery = widget.ecuData.batteryVoltage;
      });
      return;
    }

    // Animate to new values using proper animation controllers
    _animateGauge(_rpmController, _currentRpm, widget.ecuData.rpm.toDouble(), (value) {
      if (mounted) {
        setState(() => _currentRpm = value);
      }
    });
    _animateGauge(_mapController, _currentMap, widget.ecuData.map.toDouble(), (value) {
      if (mounted) {
        setState(() => _currentMap = value);
      }
    });
    _animateGauge(_tpsController, _currentTps, widget.ecuData.tps.toDouble(), (value) {
      if (mounted) {
        setState(() => _currentTps = value);
      }
    });
    _animateGauge(_coolantController, _currentCoolant, widget.ecuData.coolantTemp.toDouble(), (value) {
      if (mounted) {
        setState(() => _currentCoolant = value);
      }
    });
    _animateGauge(_intakeController, _currentIntake, widget.ecuData.intakeTemp.toDouble(), (value) {
      if (mounted) {
        setState(() => _currentIntake = value);
      }
    });
    _animateGauge(_batteryController, _currentBattery, widget.ecuData.batteryVoltage, (value) {
      if (mounted) {
        setState(() => _currentBattery = value);
      }
    });
  }

  void _animateGauge(AnimationController controller, double from, double to, Function(double) onUpdate) {
    if (controller.isAnimating) {
      controller.stop();
    }
    
    // Create a new animation from current value to target value
    final animation = Tween<double>(
      begin: from,
      end: to,
    ).animate(CurvedAnimation(
      parent: controller,
      curve: Curves.easeInOut,
    ));
    
    // Listen to animation updates
    animation.addListener(() {
      onUpdate(animation.value);
    });
    
    // Start the animation
    controller.forward(from: 0.0);
  }

  @override
  void dispose() {
    _rpmController.dispose();
    _mapController.dispose();
    _tpsController.dispose();
    _coolantController.dispose();
    _intakeController.dispose();
    _batteryController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(8),
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surface,
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: Theme.of(context).colorScheme.outline.withOpacity(0.2),
          width: 1,
        ),
      ),
      child: Column(
        children: [
          // Top row - 3 gauges
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _buildGauge(
                context,
                'RPM',
                '',
                _currentRpm,
                0.0,
                8000.0,
                ECUTheme.getAccentColor('primary'),
                GaugeType.rpm,
              ),
              _buildGauge(
                context,
                'MAP',
                'kPa',
                _currentMap,
                0.0,
                255.0,
                ECUTheme.getAccentColor('secondary'),
                GaugeType.map,
              ),
              _buildGauge(
                context,
                'TPS',
                '%',
                _currentTps,
                0.0,
                100.0,
                ECUTheme.getAccentColor('success'),
                GaugeType.tps,
              ),
            ],
          ),
          
          const SizedBox(height: 8),
          
          // Bottom row - 3 gauges
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _buildGauge(
                context,
                'Coolant',
                '°C',
                _currentCoolant,
                0.0,
                150.0,
                ECUTheme.getAccentColor('warning'),
                GaugeType.temperature,
              ),
              _buildGauge(
                context,
                'Intake',
                '°C',
                _currentIntake,
                0.0,
                150.0,
                ECUTheme.getAccentColor('info'),
                GaugeType.temperature,
              ),
              _buildGauge(
                context,
                'Battery',
                'V',
                _currentBattery,
                10.0,
                15.0,
                ECUTheme.getAccentColor('success'),
                GaugeType.voltage,
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildGauge(
    BuildContext context,
    String label,
    String unit,
    double value,
    double minValue,
    double maxValue,
    Color color,
    GaugeType gaugeType,
  ) {
    // Remove individual container - just return the gauge content
    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        // Gauge with label
        SizedBox(
          width: widget.gaugeSize,
          height: widget.gaugeSize,
          child: CustomPaint(
            painter: GaugePainter(
              value: value,
              minValue: minValue,
              maxValue: maxValue,
              color: color,
              gaugeType: gaugeType,
              showTickMarks: true,
              showValue: widget.showValues,
            ),
          ),
        ),
        
        // Label and value below gauge
        if (widget.showLabels || widget.showValues) ...[
          const SizedBox(height: 8), // Increased spacing
          if (widget.showLabels)
            Text(
              label,
              style: TextStyle(
                fontSize: 14, // Increased from 10
                fontWeight: FontWeight.w600, // Increased weight
                color: color,
              ),
            ),
          if (widget.showValues)
            Text(
              _formatValue(value, gaugeType), // Don't add unit again since it's already in _formatValue
              style: TextStyle(
                fontSize: 12, // Increased from 9
                fontWeight: FontWeight.bold,
                color: color,
              ),
            ),
        ],
      ],
    );
  }

  String _formatValue(double value, GaugeType gaugeType) {
    switch (gaugeType) {
      case GaugeType.rpm:
        return value.round().toString();
      case GaugeType.map:
        return value.round().toString();
      case GaugeType.tps:
        return value.round().toString();
      case GaugeType.temperature:
        return '${value.round()}°C'; // Fixed: was adding °C twice
      case GaugeType.voltage:
        return '${value.toStringAsFixed(1)}V'; // Fixed: was adding V twice
      case GaugeType.afr:
        return value.toStringAsFixed(1);
      case GaugeType.timing:
        return '${value.round()}°';
      case GaugeType.boost:
        return '${value.round()}';
    }
  }
}

/// Gauge types for different ECU parameters
enum GaugeType {
  rpm,
  map,
  tps,
  temperature,
  voltage,
  afr,
  timing,
  boost,
}

/// Custom painter for drawing gauges with proper math
class GaugePainter extends CustomPainter {
  final double value;
  final double minValue;
  final double maxValue;
  final Color color;
  final GaugeType gaugeType;
  final bool showTickMarks;
  final bool showValue;

  GaugePainter({
    required this.value,
    required this.minValue,
    required this.maxValue,
    required this.color,
    required this.gaugeType,
    this.showTickMarks = true,
    this.showValue = true,
  });

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = size.width / 2 * 0.8;
    
    // Draw tick mark labels first (on unrotated canvas for proper orientation)
    if (showTickMarks) {
      _drawTickMarkLabels(canvas, center, radius);
    }
    
    // Rotate canvas 90° to the left (270° clockwise) for automotive gauge look
    canvas.save();
    canvas.translate(center.dx, center.dy);
    canvas.rotate(-90 * pi / 180); // -90 degrees = 90° to the left
    canvas.translate(-center.dx, -center.dy);
    
    // Draw gauge background
    _drawGaugeBackground(canvas, center, radius);
    
    // Draw tick marks (without labels)
    if (showTickMarks) {
      _drawTickMarks(canvas, center, radius);
    }
    
    // Draw gauge value
    if (showValue) {
      _drawGaugeValue(canvas, center, radius);
    }
    
    // Draw needle
    _drawNeedle(canvas, center, radius);
    
    // Draw center cap
    _drawCenterCap(canvas, center, radius * 0.1);
    
    // Restore canvas rotation
    canvas.restore();
  }

  /// Draw gauge background circle
  void _drawGaugeBackground(Canvas canvas, Offset center, double radius) {
    final backgroundPaint = Paint()
      ..color = color.withOpacity(0.1)
      ..style = PaintingStyle.fill;
    
    final borderPaint = Paint()
      ..color = color.withOpacity(0.3)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 2;
    
    // Draw background circle
    canvas.drawCircle(center, radius, backgroundPaint);
    
    // Draw border circle
    canvas.drawCircle(center, radius, borderPaint);
  }

  /// Draw tick marks around the gauge
  void _drawTickMarks(Canvas canvas, Offset center, double radius) {
    final tickValues = _getTickValues();
    final paint = Paint()
      ..color = color.withOpacity(0.7)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 2;
    
    for (final tickValue in tickValues) {
      final angle = _getStartAngle() + (tickValue - minValue) / (maxValue - minValue) * _getSweepAngle();
      
      // Draw tick mark from just inside the gauge to the edge
      final startPoint = Offset(
        center.dx + (radius - 20) * cos(angle),
        center.dy + (radius - 20) * sin(angle),
      );
      final endPoint = Offset(
        center.dx + radius * cos(angle),
        center.dy + radius * sin(angle),
      );
      
      canvas.drawLine(startPoint, endPoint, paint);
    }
  }
  
  /// Draw the gauge needle
  void _drawNeedle(Canvas canvas, Offset center, double radius) {
    final angle = _getStartAngle() + (value - minValue) / (maxValue - minValue) * _getSweepAngle();
    
    // Draw needle shadow
    final shadowPaint = Paint()
      ..color = Colors.black.withOpacity(0.3)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 4
      ..strokeCap = StrokeCap.round;
    
    final shadowOffset = 2.0;
    final shadowStart = Offset(
      center.dx + shadowOffset,
      center.dy + shadowOffset,
    );
    final shadowEnd = Offset(
      shadowStart.dx + (radius - 20) * cos(angle),
      shadowStart.dy + (radius - 20) * sin(angle),
    );
    canvas.drawLine(shadowStart, shadowEnd, shadowPaint);
    
    // Draw main needle
    final needlePaint = Paint()
      ..color = color
      ..style = PaintingStyle.stroke
      ..strokeWidth = 3
      ..strokeCap = StrokeCap.round;
    
    final needleStart = center;
    final needleEnd = Offset(
      center.dx + (radius - 20) * cos(angle),
      center.dy + (radius - 20) * sin(angle),
    );
    canvas.drawLine(needleStart, needleEnd, needlePaint);
  }
  
  /// Get the starting angle for the gauge (rotated 90° left)
  double _getStartAngle() {
    return -pi / 2; // Start at 9 o'clock position (270°)
  }
  
  /// Get the sweep angle for the gauge
  double _getSweepAngle() {
    return pi; // 180° sweep from 9 o'clock to 3 o'clock
  }

  /// Draw gauge value text
  void _drawGaugeValue(Canvas canvas, Offset center, double radius) {
    final valuePaint = TextPainter(
      text: TextSpan(
        text: _formatValue(value),
        style: TextStyle(
          fontSize: 14,
          fontWeight: FontWeight.bold,
          color: color,
        ),
      ),
      textDirection: TextDirection.ltr,
      textAlign: TextAlign.center,
    );
    
    valuePaint.layout();
    valuePaint.paint(
      canvas,
      center - Offset(valuePaint.width / 2, valuePaint.height / 2),
    );
  }

  /// Draw center cap
  void _drawCenterCap(Canvas canvas, Offset center, double radius) {
    final capPaint = Paint()
      ..color = color
      ..style = PaintingStyle.fill;
    
    final borderPaint = Paint()
      ..color = Colors.white
      ..style = PaintingStyle.stroke
      ..strokeWidth = 2;
    
    canvas.drawCircle(center, radius, capPaint);
    canvas.drawCircle(center, radius, borderPaint);
  }

  /// Draw tick mark labels on unrotated canvas (for proper text orientation)
  void _drawTickMarkLabels(Canvas canvas, Offset center, double radius) {
    final tickValues = _getTickValues();
    
    for (final tickValue in tickValues) {
      // Calculate the angle for the tick mark position
      // Since the gauge face is rotated 90° left, we need to match the tick mark positioning exactly
      final progress = (tickValue - minValue) / (maxValue - minValue);
      final angle = -pi / 2 + progress * pi; // Start at 9 o'clock (-90°) and sweep to 3 o'clock (90°)
      
      // Calculate label position - place it exactly where the tick mark would be on the rotated canvas
      // but draw it on the unrotated canvas for proper text orientation
      final labelRadius = radius + 15; // Slightly closer to the gauge
      final labelPoint = Offset(
        center.dx + labelRadius * cos(angle),
        center.dy + labelRadius * sin(angle),
      );
      
      // Check if label is within reasonable bounds of the gauge area
      final gaugeBounds = radius * 2.0; // Tighter bounds
      if (labelPoint.dx >= center.dx - gaugeBounds && 
          labelPoint.dx <= center.dx + gaugeBounds &&
          labelPoint.dy >= center.dy - gaugeBounds && 
          labelPoint.dy <= center.dy + gaugeBounds) {
        
        final textPainter = TextPainter(
          text: TextSpan(
            text: _formatTickValue(tickValue),
            style: TextStyle(
              color: color.withOpacity(0.8),
              fontSize: 10,
              fontWeight: FontWeight.w500,
            ),
          ),
          textDirection: TextDirection.ltr,
          textAlign: TextAlign.center,
        );
        
        textPainter.layout();
        textPainter.paint(
          canvas,
          labelPoint - Offset(textPainter.width / 2, textPainter.height / 2),
        );
      }
    }
  }

  /// Get tick values for specific gauge type
  List<double> _getTickValues() {
    switch (gaugeType) {
      case GaugeType.rpm:
        return [0, 1000, 2000, 3000, 4000, 6000, 8000];
      case GaugeType.map:
        return [0, 50, 100, 150, 200, 225, 255];
      case GaugeType.tps:
        return [0, 20, 40, 60, 80, 90, 100];
      case GaugeType.temperature:
        return [0, 30, 60, 90, 110, 130, 150];
      case GaugeType.voltage:
        return [10.0, 11.0, 12.0, 12.5, 13.0, 14.0, 15.0];
      case GaugeType.afr:
        return [10.0, 11.5, 13.0, 14.7, 16.0, 18.0, 20.0];
      case GaugeType.timing:
        return [-20, -10, 0, 15, 25, 35, 50];
      case GaugeType.boost:
        return [0, 5, 10, 15, 20, 25, 30];
    }
  }

  /// Format tick value for display
  String _formatTickValue(double value) {
    if (value == value.toInt()) {
      return value.toInt().toString();
    }
    return value.toStringAsFixed(1);
  }

  /// Format current value for display
  String _formatValue(double value) {
    switch (gaugeType) {
      case GaugeType.rpm:
      case GaugeType.map:
      case GaugeType.tps:
      case GaugeType.temperature:
      case GaugeType.timing:
      case GaugeType.boost:
        return value.toInt().toString();
      case GaugeType.voltage:
      case GaugeType.afr:
        return value.toStringAsFixed(1);
    }
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => true;
}
