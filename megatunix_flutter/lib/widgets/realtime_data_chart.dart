/// Real-time Data Chart Widget
/// Professional real-time graphing for data logging sessions
/// Shows live ECU parameters with smooth scrolling and professional styling

import 'package:flutter/material.dart';
import 'dart:async';
import 'dart:math';
import '../models/ecu_data.dart';
import '../services/data_logging_service.dart';
import '../services/ecu_service.dart';

/// Real-time data chart for logging sessions
class RealtimeDataChart extends StatefulWidget {
  final List<String> selectedParameters;
  final Duration historyDuration;
  final bool showGrid;
  final bool showLegend;
  final double height;

  const RealtimeDataChart({
    super.key,
    this.selectedParameters = const ['rpm', 'map', 'afr'],
    this.historyDuration = const Duration(minutes: 2),
    this.showGrid = true,
    this.showLegend = true,
    this.height = 300,
  });

  @override
  State<RealtimeDataChart> createState() => _RealtimeDataChartState();
}

class _RealtimeDataChartState extends State<RealtimeDataChart>
    with TickerProviderStateMixin {
  late StreamSubscription<SpeeduinoData> _dataSubscription;
  late AnimationController _animationController;
  late Animation<double> _animation;

  // Data storage for real-time plotting
  final Map<String, List<DataPoint>> _dataHistory = {};
  final Map<String, Color> _parameterColors = {
    'rpm': Colors.red,
    'map': Colors.blue,
    'tps': Colors.green,
    'afr': Colors.orange,
    'coolant_temp': Colors.purple,
    'intake_temp': Colors.cyan,
    'battery_voltage': Colors.yellow.shade700,
    'timing': Colors.pink,
    'boost': Colors.indigo,
  };

  // Chart configuration
  double _maxHistoryPoints = 240; // 2 minutes at 10Hz
  DateTime? _chartStartTime;

  @override
  void initState() {
    super.initState();
    _initializeChart();
    _startDataSubscription();
  }

  void _initializeChart() {
    // Initialize animation for smooth scrolling
    _animationController = AnimationController(
      duration: const Duration(milliseconds: 500),
      vsync: this,
    );
    _animation = Tween<double>(begin: 0.0, end: 1.0).animate(
      CurvedAnimation(parent: _animationController, curve: Curves.easeInOut),
    );

    // Initialize data history for each parameter
    for (String param in widget.selectedParameters) {
      _dataHistory[param] = [];
    }

    // Calculate max history points based on duration
    _maxHistoryPoints = widget.historyDuration.inSeconds * 2.0; // Assume ~2Hz effective rate
  }

  void _startDataSubscription() {
    final ecuService = ECUService();
    _dataSubscription = ecuService.dataStream.listen(_addDataPoint);
    _chartStartTime = DateTime.now();
    _animationController.repeat();
  }

  void _addDataPoint(SpeeduinoData data) {
    if (!mounted) return;

    final now = DateTime.now();
    final timeOffset = _chartStartTime != null
        ? now.difference(_chartStartTime!).inMilliseconds / 1000.0
        : 0.0;

    // Add data points for each selected parameter
    for (String param in widget.selectedParameters) {
      final value = _getParameterValue(data, param);
      if (value != null) {
        _dataHistory[param]!.add(DataPoint(timeOffset, value));

        // Trim history to maintain performance
        while (_dataHistory[param]!.length > _maxHistoryPoints) {
          _dataHistory[param]!.removeAt(0);
        }
      }
    }

    // Update UI periodically (not every frame)
    if (_dataHistory.values.any((list) => list.length % 5 == 0)) {
      setState(() {});
    }
  }

  double? _getParameterValue(SpeeduinoData data, String parameter) {
    switch (parameter) {
      case 'rpm':
        return data.rpm.toDouble();
      case 'map':
        return data.map.toDouble();
      case 'tps':
        return data.tps.toDouble();
      case 'afr':
        return data.afr;
      case 'coolant_temp':
        return data.coolantTemp.toDouble();
      case 'intake_temp':
        return data.intakeTemp.toDouble();
      case 'battery_voltage':
        return data.batteryVoltage;
      case 'timing':
        return data.timing.toDouble();
      case 'boost':
        return data.boost.toDouble();
      default:
        return null;
    }
  }

  @override
  Widget build(BuildContext context) {
    return Card(
      elevation: 4,
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            _buildHeader(),
            const SizedBox(height: 8),
            SizedBox(
              height: widget.height,
              child: _buildChart(),
            ),
            if (widget.showLegend) ...[
              const SizedBox(height: 8),
              _buildLegend(),
            ],
          ],
        ),
      ),
    );
  }

  Widget _buildHeader() {
    final loggingService = DataLoggingService();
    final isLogging = loggingService.isLogging;

    return Row(
      children: [
        Icon(
          isLogging ? Icons.fiber_manual_record : Icons.show_chart,
          color: isLogging ? Colors.red : Colors.grey,
          size: 20,
        ),
        const SizedBox(width: 8),
        Text(
          'Real-time Data Chart',
          style: Theme.of(context).textTheme.titleMedium?.copyWith(
                fontWeight: FontWeight.bold,
              ),
        ),
        const Spacer(),
        if (isLogging) ...[
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
            decoration: BoxDecoration(
              color: Colors.red.withOpacity(0.1),
              borderRadius: BorderRadius.circular(4),
            ),
            child: Text(
              'LOGGING',
              style: TextStyle(
                color: Colors.red,
                fontSize: 12,
                fontWeight: FontWeight.bold,
              ),
            ),
          ),
        ],
        const SizedBox(width: 8),
        Text(
          '${widget.historyDuration.inSeconds}s history',
          style: Theme.of(context).textTheme.bodySmall,
        ),
      ],
    );
  }

  Widget _buildChart() {
    if (_dataHistory.isEmpty || _dataHistory.values.every((list) => list.isEmpty)) {
      return Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              Icons.timeline,
              size: 48,
              color: Colors.grey,
            ),
            const SizedBox(height: 8),
            Text(
              'No data available',
              style: Theme.of(context).textTheme.bodyLarge?.copyWith(
                    color: Colors.grey,
                  ),
            ),
            Text(
              'Start ECU connection to see real-time data',
              style: Theme.of(context).textTheme.bodySmall?.copyWith(
                    color: Colors.grey,
                  ),
            ),
          ],
        ),
      );
    }

    return AnimatedBuilder(
      animation: _animation,
      builder: (context, child) => CustomPaint(
        painter: RealtimeChartPainter(
          dataHistory: _dataHistory,
          parameterColors: _parameterColors,
          selectedParameters: widget.selectedParameters,
          showGrid: widget.showGrid,
          theme: Theme.of(context),
          animationValue: _animation.value,
        ),
        size: Size.infinite,
      ),
    );
  }

  Widget _buildLegend() {
    return Wrap(
      spacing: 16,
      runSpacing: 8,
      children: widget.selectedParameters.map((param) {
        final color = _parameterColors[param] ?? Colors.grey;
        final hasData = _dataHistory[param]?.isNotEmpty ?? false;
        final currentValue = hasData ? _dataHistory[param]!.last.value : null;

        return Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Container(
              width: 12,
              height: 12,
              decoration: BoxDecoration(
                color: color,
                shape: BoxShape.circle,
              ),
            ),
            const SizedBox(width: 4),
            Text(
              param.toUpperCase(),
              style: Theme.of(context).textTheme.labelSmall?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
            ),
            if (currentValue != null) ...[
              const SizedBox(width: 4),
              Text(
                _formatValue(param, currentValue),
                style: Theme.of(context).textTheme.labelSmall,
              ),
            ],
          ],
        );
      }).toList(),
    );
  }

  String _formatValue(String parameter, double value) {
    switch (parameter) {
      case 'rpm':
        return '${value.round()}';
      case 'afr':
      case 'battery_voltage':
        return value.toStringAsFixed(1);
      case 'map':
      case 'tps':
      case 'coolant_temp':
      case 'intake_temp':
      case 'timing':
      case 'boost':
        return '${value.round()}';
      default:
        return value.toStringAsFixed(1);
    }
  }

  @override
  void dispose() {
    _dataSubscription.cancel();
    _animationController.dispose();
    super.dispose();
  }
}

/// Data point for real-time charting
class DataPoint {
  final double time;
  final double value;

  DataPoint(this.time, this.value);
}

/// Custom painter for real-time chart
class RealtimeChartPainter extends CustomPainter {
  final Map<String, List<DataPoint>> dataHistory;
  final Map<String, Color> parameterColors;
  final List<String> selectedParameters;
  final bool showGrid;
  final ThemeData theme;
  final double animationValue;

  RealtimeChartPainter({
    required this.dataHistory,
    required this.parameterColors,
    required this.selectedParameters,
    required this.showGrid,
    required this.theme,
    required this.animationValue,
  });

  @override
  void paint(Canvas canvas, Size size) {
    final rect = Rect.fromLTWH(0, 0, size.width, size.height);
    
    // Draw background
    final backgroundPaint = Paint()
      ..color = theme.colorScheme.surface
      ..style = PaintingStyle.fill;
    canvas.drawRect(rect, backgroundPaint);

    if (dataHistory.isEmpty || dataHistory.values.every((list) => list.isEmpty)) {
      return;
    }

    // Calculate data bounds
    final bounds = _calculateDataBounds();
    if (bounds == null) return;

    // Draw grid
    if (showGrid) {
      _drawGrid(canvas, size, bounds);
    }

    // Draw data lines for each parameter
    for (String param in selectedParameters) {
      final data = dataHistory[param];
      if (data != null && data.isNotEmpty) {
        _drawDataLine(canvas, size, data, param, bounds);
      }
    }

    // Draw border
    final borderPaint = Paint()
      ..color = theme.colorScheme.outline.withOpacity(0.3)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1;
    canvas.drawRect(rect, borderPaint);
  }

  DataBounds? _calculateDataBounds() {
    if (dataHistory.isEmpty) return null;

    double minTime = double.infinity;
    double maxTime = double.negativeInfinity;
    final Map<String, double> minValues = {};
    final Map<String, double> maxValues = {};

    for (String param in selectedParameters) {
      final data = dataHistory[param];
      if (data == null || data.isEmpty) continue;

      minValues[param] = double.infinity;
      maxValues[param] = double.negativeInfinity;

      for (DataPoint point in data) {
        minTime = min(minTime, point.time);
        maxTime = max(maxTime, point.time);
        minValues[param] = min(minValues[param]!, point.value);
        maxValues[param] = max(maxValues[param]!, point.value);
      }

      // Add padding to value ranges
      final range = maxValues[param]! - minValues[param]!;
      final padding = range * 0.1;
      minValues[param] = minValues[param]! - padding;
      maxValues[param] = maxValues[param]! + padding;
    }

    return DataBounds(
      minTime: minTime,
      maxTime: maxTime,
      minValues: minValues,
      maxValues: maxValues,
    );
  }

  void _drawGrid(Canvas canvas, Size size, DataBounds bounds) {
    final gridPaint = Paint()
      ..color = theme.colorScheme.outline.withOpacity(0.1)
      ..strokeWidth = 0.5;

    // Draw vertical grid lines (time)
    for (int i = 0; i <= 10; i++) {
      final x = size.width * i / 10;
      canvas.drawLine(
        Offset(x, 0),
        Offset(x, size.height),
        gridPaint,
      );
    }

    // Draw horizontal grid lines
    for (int i = 0; i <= 5; i++) {
      final y = size.height * i / 5;
      canvas.drawLine(
        Offset(0, y),
        Offset(size.width, y),
        gridPaint,
      );
    }
  }

  void _drawDataLine(Canvas canvas, Size size, List<DataPoint> data, 
                     String parameter, DataBounds bounds) {
    if (data.length < 2) return;

    final color = parameterColors[parameter] ?? Colors.grey;
    final paint = Paint()
      ..color = color.withOpacity(0.8)
      ..strokeWidth = 2.0
      ..style = PaintingStyle.stroke
      ..strokeCap = StrokeCap.round;

    final path = Path();
    bool isFirstPoint = true;

    final minValue = bounds.minValues[parameter] ?? 0;
    final maxValue = bounds.maxValues[parameter] ?? 1;
    final timeRange = bounds.maxTime - bounds.minTime;

    for (DataPoint point in data) {
      // Normalize coordinates
      final x = timeRange > 0 
          ? size.width * (point.time - bounds.minTime) / timeRange
          : 0.0;
      final y = maxValue > minValue
          ? size.height * (1 - (point.value - minValue) / (maxValue - minValue))
          : size.height / 2;

      if (isFirstPoint) {
        path.moveTo(x, y);
        isFirstPoint = false;
      } else {
        path.lineTo(x, y);
      }
    }

    // Apply animation effect
    final animatedPaint = Paint()
      ..color = color.withOpacity(0.6 + 0.4 * animationValue)
      ..strokeWidth = 2.0 + 0.5 * sin(animationValue * 2 * pi)
      ..style = PaintingStyle.stroke
      ..strokeCap = StrokeCap.round;

    canvas.drawPath(path, animatedPaint);
  }

  @override
  bool shouldRepaint(covariant RealtimeChartPainter oldDelegate) {
    return oldDelegate.dataHistory != dataHistory ||
           oldDelegate.animationValue != animationValue;
  }
}

/// Data bounds for chart scaling
class DataBounds {
  final double minTime;
  final double maxTime;
  final Map<String, double> minValues;
  final Map<String, double> maxValues;

  DataBounds({
    required this.minTime,
    required this.maxTime,
    required this.minValues,
    required this.maxValues,
  });
}