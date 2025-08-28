/// Tuning View Service
/// Handles F10 Tuning & Dyno View Editor functionality
/// Provides advanced tuning visualization and analysis tools

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/ecu_data.dart';
import 'ecu_service.dart';
import 'data_logging_service.dart';

/// Service for managing advanced tuning views and dyno functionality
class TuningViewService extends ChangeNotifier {
  static final TuningViewService _instance = TuningViewService._internal();
  factory TuningViewService() => _instance;
  TuningViewService._internal();

  // View state
  bool _isViewOpen = false;
  TuningViewMode _currentMode = TuningViewMode.realTime;
  
  // Data analysis
  final List<SpeeduinoData> _analysisData = [];
  TuningAnalysis? _currentAnalysis;
  
  // Getters
  bool get isViewOpen => _isViewOpen;
  TuningViewMode get currentMode => _currentMode;
  TuningAnalysis? get currentAnalysis => _currentAnalysis;
  
  /// Open Tuning & Dyno View Editor (F10)
  void openTuningView(BuildContext context) {
    if (_isViewOpen) {
      // Focus existing view
      print('Tuning view already open, focusing...');
      return;
    }
    
    Navigator.of(context).push(
      MaterialPageRoute(
        builder: (context) => const TuningViewScreen(),
        settings: const RouteSettings(name: '/tuning_view'),
      ),
    ).then((_) {
      _isViewOpen = false;
      notifyListeners();
    });
    
    _isViewOpen = true;
    notifyListeners();
  }
  
  /// Switch tuning view mode
  void setMode(TuningViewMode mode) {
    _currentMode = mode;
    notifyListeners();
  }
  
  /// Perform tuning analysis
  Future<void> performAnalysis() async {
    final ecuService = ECUService();
    final dataLoggingService = DataLoggingService();
    
    // Get recent data for analysis
    _analysisData.clear();
    
    // If logging is active, use logged data
    if (dataLoggingService.isLogging) {
      // TODO: Get logged data for analysis
      print('Using logged data for analysis');
    }
    
    // Generate analysis
    _currentAnalysis = _generateAnalysis(_analysisData);
    notifyListeners();
  }
  
  /// Generate tuning analysis from data
  TuningAnalysis _generateAnalysis(List<SpeeduinoData> data) {
    if (data.isEmpty) {
      return TuningAnalysis(
        summary: 'No data available for analysis',
        recommendations: ['Start data logging to collect analysis data'],
        afRatio: TuningMetric(value: 0, status: TuningStatus.noData),
        veTable: TuningMetric(value: 0, status: TuningStatus.noData),
        timingMap: TuningMetric(value: 0, status: TuningStatus.noData),
        powerCurve: [],
      );
    }
    
    // Analyze AF ratio
    final afRatios = data.map((d) => d.afr).toList();
    final avgAFR = afRatios.fold(0.0, (a, b) => a + b) / afRatios.length;
    final afrStatus = _evaluateAFR(avgAFR);
    
    // Analyze VE table efficiency
    final veEfficiency = _calculateVEEfficiency(data);
    final veStatus = _evaluateVEEfficiency(veEfficiency);
    
    // Analyze timing
    final timings = data.map((d) => d.timing).toList();
    final avgTiming = timings.fold(0, (a, b) => a + b) / timings.length;
    final timingStatus = _evaluateTiming(avgTiming);
    
    // Generate power curve
    final powerCurve = _generatePowerCurve(data);
    
    // Generate recommendations
    final recommendations = <String>[];
    if (afrStatus != TuningStatus.optimal) {
      recommendations.add(_getAFRRecommendation(avgAFR));
    }
    if (veStatus != TuningStatus.optimal) {
      recommendations.add(_getVERecommendation(veEfficiency));
    }
    if (timingStatus != TuningStatus.optimal) {
      recommendations.add(_getTimingRecommendation(avgTiming));
    }
    
    if (recommendations.isEmpty) {
      recommendations.add('Current tune appears optimal for analyzed data');
    }
    
    return TuningAnalysis(
      summary: 'Analysis of ${data.length} data points',
      recommendations: recommendations,
      afRatio: TuningMetric(value: avgAFR, status: afrStatus),
      veTable: TuningMetric(value: veEfficiency, status: veStatus),
      timingMap: TuningMetric(value: avgTiming, status: timingStatus),
      powerCurve: powerCurve,
    );
  }
  
  // Analysis helper methods
  TuningStatus _evaluateAFR(double afr) {
    if (afr >= 14.2 && afr <= 15.2) return TuningStatus.optimal;
    if (afr >= 13.5 && afr <= 16.0) return TuningStatus.acceptable;
    return TuningStatus.needsAttention;
  }
  
  double _calculateVEEfficiency(List<SpeeduinoData> data) {
    // Simplified VE efficiency calculation
    final mapValues = data.map((d) => d.map).toList();
    final avgMAP = mapValues.fold(0, (a, b) => a + b) / mapValues.length;
    return (avgMAP / 100.0) * 85.0; // Simplified calculation
  }
  
  TuningStatus _evaluateVEEfficiency(double efficiency) {
    if (efficiency >= 80.0) return TuningStatus.optimal;
    if (efficiency >= 70.0) return TuningStatus.acceptable;
    return TuningStatus.needsAttention;
  }
  
  TuningStatus _evaluateTiming(double timing) {
    if (timing >= 10 && timing <= 20) return TuningStatus.optimal;
    if (timing >= 5 && timing <= 25) return TuningStatus.acceptable;
    return TuningStatus.needsAttention;
  }
  
  String _getAFRRecommendation(double afr) {
    if (afr < 13.5) return 'AFR too rich - consider leaning mixture';
    if (afr > 16.0) return 'AFR too lean - consider enriching mixture';
    return 'AFR within acceptable range';
  }
  
  String _getVERecommendation(double efficiency) {
    if (efficiency < 70.0) return 'VE table may need optimization';
    return 'VE efficiency looks good';
  }
  
  String _getTimingRecommendation(double timing) {
    if (timing < 5) return 'Timing advance may be too conservative';
    if (timing > 25) return 'Timing advance may be too aggressive';
    return 'Timing within safe range';
  }
  
  List<PowerPoint> _generatePowerCurve(List<SpeeduinoData> data) {
    // Generate simplified power curve from RPM and MAP data
    final Map<int, List<SpeeduinoData>> rpmGroups = {};
    
    for (final point in data) {
      final rpmBucket = (point.rpm / 500).round() * 500;
      rpmGroups[rpmBucket] ??= [];
      rpmGroups[rpmBucket]!.add(point);
    }
    
    final powerCurve = <PowerPoint>[];
    for (final entry in rpmGroups.entries) {
      final rpm = entry.key;
      final points = entry.value;
      final avgMAP = points.map((p) => p.map).fold(0, (a, b) => a + b) / points.length;
      
      // Simplified power calculation (not accurate, just for demonstration)
      final power = (rpm * avgMAP / 1000.0).round();
      powerCurve.add(PowerPoint(rpm: rpm, power: power));
    }
    
    powerCurve.sort((a, b) => a.rpm.compareTo(b.rpm));
    return powerCurve;
  }
}

/// Tuning view modes
enum TuningViewMode {
  realTime,
  dataAnalysis,
  powerCurve,
  afrTarget,
  timingMap,
}

/// Tuning status levels
enum TuningStatus {
  optimal,
  acceptable,
  needsAttention,
  critical,
  noData,
}

/// Tuning analysis result
class TuningAnalysis {
  final String summary;
  final List<String> recommendations;
  final TuningMetric afRatio;
  final TuningMetric veTable;
  final TuningMetric timingMap;
  final List<PowerPoint> powerCurve;
  
  TuningAnalysis({
    required this.summary,
    required this.recommendations,
    required this.afRatio,
    required this.veTable,
    required this.timingMap,
    required this.powerCurve,
  });
}

/// Tuning metric with status
class TuningMetric {
  final double value;
  final TuningStatus status;
  
  TuningMetric({
    required this.value,
    required this.status,
  });
}

/// Power curve point
class PowerPoint {
  final int rpm;
  final int power;
  
  PowerPoint({
    required this.rpm,
    required this.power,
  });
}

/// Tuning View Screen Widget
class TuningViewScreen extends StatefulWidget {
  const TuningViewScreen({super.key});
  
  @override
  State<TuningViewScreen> createState() => _TuningViewScreenState();
}

class _TuningViewScreenState extends State<TuningViewScreen> {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Row(
          children: [
            Icon(Icons.tune),
            SizedBox(width: 8),
            Text('Tuning & Dyno View'),
          ],
        ),
        actions: [
          IconButton(
            onPressed: () => _showHelpDialog(),
            icon: const Icon(Icons.help_outline),
            tooltip: 'Help (F1)',
          ),
        ],
      ),
      body: Consumer<TuningViewService>(
        builder: (context, service, child) {
          return Column(
            children: [
              // Mode selector
              Container(
                padding: const EdgeInsets.all(16),
                child: SegmentedButton<TuningViewMode>(
                  segments: const [
                    ButtonSegment(
                      value: TuningViewMode.realTime,
                      label: Text('Real-time'),
                      icon: Icon(Icons.speed),
                    ),
                    ButtonSegment(
                      value: TuningViewMode.dataAnalysis,
                      label: Text('Analysis'),
                      icon: Icon(Icons.analytics),
                    ),
                    ButtonSegment(
                      value: TuningViewMode.powerCurve,
                      label: Text('Power'),
                      icon: Icon(Icons.trending_up),
                    ),
                    ButtonSegment(
                      value: TuningViewMode.afrTarget,
                      label: Text('AFR'),
                      icon: Icon(Icons.local_fire_department),
                    ),
                  ],
                  selected: {service.currentMode},
                  onSelectionChanged: (Set<TuningViewMode> selection) {
                    service.setMode(selection.first);
                  },
                ),
              ),
              
              // Content area
              Expanded(
                child: _buildModeContent(service.currentMode, service.currentAnalysis),
              ),
              
              // Action buttons
              Container(
                padding: const EdgeInsets.all(16),
                child: Row(
                  children: [
                    FilledButton.icon(
                      onPressed: () => service.performAnalysis(),
                      icon: const Icon(Icons.refresh),
                      label: const Text('Analyze'),
                    ),
                    const SizedBox(width: 16),
                    OutlinedButton.icon(
                      onPressed: () => _exportAnalysis(),
                      icon: const Icon(Icons.download),
                      label: const Text('Export'),
                    ),
                    const Spacer(),
                    TextButton(
                      onPressed: () => Navigator.of(context).pop(),
                      child: const Text('Close'),
                    ),
                  ],
                ),
              ),
            ],
          );
        },
      ),
    );
  }
  
  Widget _buildModeContent(TuningViewMode mode, TuningAnalysis? analysis) {
    switch (mode) {
      case TuningViewMode.realTime:
        return _buildRealTimeView();
      case TuningViewMode.dataAnalysis:
        return _buildAnalysisView(analysis);
      case TuningViewMode.powerCurve:
        return _buildPowerCurveView(analysis?.powerCurve ?? []);
      case TuningViewMode.afrTarget:
        return _buildAFRView(analysis?.afRatio);
      case TuningViewMode.timingMap:
        return _buildTimingView(analysis?.timingMap);
    }
  }
  
  Widget _buildRealTimeView() {
    return const Center(
      child: Card(
        child: Padding(
          padding: EdgeInsets.all(24),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Icon(Icons.speed, size: 48),
              SizedBox(height: 16),
              Text('Real-time Tuning View', style: TextStyle(fontSize: 18)),
              SizedBox(height: 8),
              Text('Live ECU monitoring and analysis coming soon'),
            ],
          ),
        ),
      ),
    );
  }
  
  Widget _buildAnalysisView(TuningAnalysis? analysis) {
    if (analysis == null) {
      return Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Icon(Icons.analytics, size: 48),
            const SizedBox(height: 16),
            const Text('No analysis data available'),
            const SizedBox(height: 16),
            FilledButton(
              onPressed: () => context.read<TuningViewService>().performAnalysis(),
              child: const Text('Run Analysis'),
            ),
          ],
        ),
      );
    }
    
    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(analysis.summary, style: Theme.of(context).textTheme.headlineSmall),
          const SizedBox(height: 24),
          
          // Metrics cards
          Row(
            children: [
              Expanded(child: _buildMetricCard('AF Ratio', analysis.afRatio)),
              const SizedBox(width: 16),
              Expanded(child: _buildMetricCard('VE Efficiency', analysis.veTable)),
              const SizedBox(width: 16),
              Expanded(child: _buildMetricCard('Timing', analysis.timingMap)),
            ],
          ),
          
          const SizedBox(height: 24),
          
          // Recommendations
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text('Recommendations', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
                  const SizedBox(height: 12),
                  ...analysis.recommendations.map((rec) => Padding(
                    padding: const EdgeInsets.only(bottom: 8),
                    child: Row(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        const Icon(Icons.arrow_right, size: 16),
                        const SizedBox(width: 8),
                        Expanded(child: Text(rec)),
                      ],
                    ),
                  )),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }
  
  Widget _buildMetricCard(String title, TuningMetric metric) {
    Color getStatusColor(TuningStatus status) {
      switch (status) {
        case TuningStatus.optimal:
          return Colors.green;
        case TuningStatus.acceptable:
          return Colors.orange;
        case TuningStatus.needsAttention:
          return Colors.red;
        case TuningStatus.critical:
          return Colors.red.shade700;
        case TuningStatus.noData:
          return Colors.grey;
      }
    }
    
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(title, style: const TextStyle(fontWeight: FontWeight.bold)),
            const SizedBox(height: 8),
            Text(
              metric.value.toStringAsFixed(1),
              style: TextStyle(
                fontSize: 24,
                fontWeight: FontWeight.bold,
                color: getStatusColor(metric.status),
              ),
            ),
            Text(
                metric.status.toString().split('.').last.toUpperCase(),
              style: TextStyle(
                fontSize: 12,
                color: getStatusColor(metric.status),
              ),
            ),
          ],
        ),
      ),
    );
  }
  
  Widget _buildPowerCurveView(List<PowerPoint> powerCurve) {
    return Center(
      child: Card(
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              const Icon(Icons.trending_up, size: 48),
              const SizedBox(height: 16),
              const Text('Power Curve Analysis', style: TextStyle(fontSize: 18)),
              const SizedBox(height: 8),
              Text('${powerCurve.length} data points available'),
            ],
          ),
        ),
      ),
    );
  }
  
  Widget _buildAFRView(TuningMetric? afr) {
    return const Center(
      child: Card(
        child: Padding(
          padding: EdgeInsets.all(24),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Icon(Icons.local_fire_department, size: 48),
              SizedBox(height: 16),
              Text('AFR Target Analysis', style: TextStyle(fontSize: 18)),
              SizedBox(height: 8),
              Text('Air-fuel ratio optimization coming soon'),
            ],
          ),
        ),
      ),
    );
  }
  
  Widget _buildTimingView(TuningMetric? timing) {
    return const Center(
      child: Card(
        child: Padding(
          padding: EdgeInsets.all(24),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Icon(Icons.schedule, size: 48),
              SizedBox(height: 16),
              Text('Timing Map Analysis', style: TextStyle(fontSize: 18)),
              SizedBox(height: 8),
              Text('Ignition timing optimization coming soon'),
            ],
          ),
        ),
      ),
    );
  }
  
  void _showHelpDialog() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Tuning & Dyno View Help'),
        content: const Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Advanced tuning analysis and visualization tools:'),
            SizedBox(height: 16),
            Text('• Real-time: Live ECU monitoring'),
            Text('• Analysis: Data-driven tuning recommendations'),
            Text('• Power: Power curve analysis from logs'),
            Text('• AFR: Air-fuel ratio optimization'),
            SizedBox(height: 16),
            Text('Keyboard Shortcuts:'),
            Text('F1 - Show this help'),
            Text('F10 - Toggle tuning view'),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('Close'),
          ),
        ],
      ),
    );
  }
  
  void _exportAnalysis() {
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('Export functionality coming soon')),
    );
  }
}