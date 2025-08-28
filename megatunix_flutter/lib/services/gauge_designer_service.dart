/// Gauge Designer Service
/// Handles Alt+Arrow gauge manipulation and Ctrl+D duplication functionality
/// Provides professional gauge layout and positioning tools

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'dart:math' as math;

/// Service for managing gauge design and manipulation operations
class GaugeDesignerService extends ChangeNotifier {
  static final GaugeDesignerService _instance = GaugeDesignerService._internal();
  factory GaugeDesignerService() => _instance;
  GaugeDesignerService._internal();

  // Design mode state
  bool _isDesignMode = false;
  String? _selectedGaugeId;
  final Map<String, GaugeConfiguration> _gaugeConfigs = {};
  
  // Manipulation settings
  double _positionStep = 5.0; // pixels per arrow key press
  double _sizeStep = 5.0; // size change per arrow key press
  
  // Grid snapping
  bool _snapToGrid = true;
  double _gridSize = 10.0;
  
  /// Get current design mode state
  bool get isDesignMode => _isDesignMode;
  
  /// Get selected gauge ID
  String? get selectedGaugeId => _selectedGaugeId;
  
  /// Get all gauge configurations
  Map<String, GaugeConfiguration> get gaugeConfigs => Map.unmodifiable(_gaugeConfigs);
  
  /// Toggle design mode
  void toggleDesignMode() {
    _isDesignMode = !_isDesignMode;
    if (!_isDesignMode) {
      _selectedGaugeId = null;
    }
    notifyListeners();
  }
  
  /// Select gauge for manipulation
  void selectGauge(String gaugeId) {
    if (!_isDesignMode) return;
    
    _selectedGaugeId = gaugeId;
    notifyListeners();
  }
  
  /// Deselect current gauge
  void deselectGauge() {
    _selectedGaugeId = null;
    notifyListeners();
  }
  
  /// Register a gauge configuration
  void registerGauge(String gaugeId, GaugeConfiguration config) {
    _gaugeConfigs[gaugeId] = config;
    notifyListeners();
  }
  
  /// Adjust gauge position with Alt+Arrow keys
  bool adjustGaugePosition(LogicalKeyboardKey key) {
    if (!_isDesignMode || _selectedGaugeId == null) return false;
    
    final config = _gaugeConfigs[_selectedGaugeId];
    if (config == null) return false;
    
    double deltaX = 0, deltaY = 0;
    
    switch (key) {
      case LogicalKeyboardKey.arrowUp:
        deltaY = -_positionStep;
        break;
      case LogicalKeyboardKey.arrowDown:
        deltaY = _positionStep;
        break;
      case LogicalKeyboardKey.arrowLeft:
        deltaX = -_positionStep;
        break;
      case LogicalKeyboardKey.arrowRight:
        deltaX = _positionStep;
        break;
      default:
        return false;
    }
    
    // Calculate new position
    var newX = config.x + deltaX;
    var newY = config.y + deltaY;
    
    // Apply grid snapping
    if (_snapToGrid) {
      newX = (newX / _gridSize).round() * _gridSize;
      newY = (newY / _gridSize).round() * _gridSize;
    }
    
    // Update configuration
    final updatedConfig = config.copyWith(x: newX, y: newY);
    _gaugeConfigs[_selectedGaugeId!] = updatedConfig;
    
    notifyListeners();
    return true;
  }
  
  /// Adjust gauge size with Alt+Shift+Arrow keys
  bool adjustGaugeSize(LogicalKeyboardKey key, bool isShiftPressed) {
    if (!_isDesignMode || _selectedGaugeId == null || !isShiftPressed) return false;
    
    final config = _gaugeConfigs[_selectedGaugeId];
    if (config == null) return false;
    
    double deltaSize = 0;
    
    switch (key) {
      case LogicalKeyboardKey.arrowUp:
      case LogicalKeyboardKey.arrowRight:
        deltaSize = _sizeStep;
        break;
      case LogicalKeyboardKey.arrowDown:
      case LogicalKeyboardKey.arrowLeft:
        deltaSize = -_sizeStep;
        break;
      default:
        return false;
    }
    
    // Calculate new size with constraints
    final newSize = math.max(20.0, config.size + deltaSize);
    
    // Update configuration
    final updatedConfig = config.copyWith(size: newSize);
    _gaugeConfigs[_selectedGaugeId!] = updatedConfig;
    
    notifyListeners();
    return true;
  }
  
  /// Duplicate selected gauge (Ctrl+D)
  String? duplicateSelectedGauge() {
    if (!_isDesignMode || _selectedGaugeId == null) return null;
    
    final config = _gaugeConfigs[_selectedGaugeId];
    if (config == null) return null;
    
    // Generate new unique ID
    final duplicateId = '${config.type}_${DateTime.now().millisecondsSinceEpoch}';
    
    // Create duplicate configuration with offset position
    final duplicateConfig = config.copyWith(
      id: duplicateId,
      name: '${config.name} Copy',
      x: config.x + 50, // Offset by 50 pixels
      y: config.y + 20,
    );
    
    // Register duplicate
    _gaugeConfigs[duplicateId] = duplicateConfig;
    
    // Select the new duplicate
    _selectedGaugeId = duplicateId;
    
    notifyListeners();
    return duplicateId;
  }
  
  /// Remove gauge
  void removeGauge(String gaugeId) {
    _gaugeConfigs.remove(gaugeId);
    if (_selectedGaugeId == gaugeId) {
      _selectedGaugeId = null;
    }
    notifyListeners();
  }
  
  /// Reset all gauges to default positions
  void resetGaugeLayout() {
    final defaultConfigs = _generateDefaultGaugeLayout();
    _gaugeConfigs.clear();
    _gaugeConfigs.addAll(defaultConfigs);
    _selectedGaugeId = null;
    notifyListeners();
  }
  
  /// Get default gauge layout
  Map<String, GaugeConfiguration> _generateDefaultGaugeLayout() {
    return {
      'rpm_gauge': GaugeConfiguration(
        id: 'rpm_gauge',
        name: 'RPM',
        type: GaugeType.circular,
        parameter: 'rpm',
        x: 50,
        y: 50,
        size: 120,
        minValue: 0,
        maxValue: 8000,
        unit: 'RPM',
        color: Colors.red,
      ),
      'map_gauge': GaugeConfiguration(
        id: 'map_gauge',
        name: 'MAP',
        type: GaugeType.circular,
        parameter: 'map',
        x: 200,
        y: 50,
        size: 120,
        minValue: 0,
        maxValue: 100,
        unit: 'kPa',
        color: Colors.blue,
      ),
      'tps_gauge': GaugeConfiguration(
        id: 'tps_gauge',
        name: 'TPS',
        type: GaugeType.linear,
        parameter: 'tps',
        x: 350,
        y: 50,
        size: 120,
        minValue: 0,
        maxValue: 100,
        unit: '%',
        color: Colors.green,
      ),
      'coolant_gauge': GaugeConfiguration(
        id: 'coolant_gauge',
        name: 'Coolant',
        type: GaugeType.circular,
        parameter: 'coolantTemp',
        x: 50,
        y: 220,
        size: 120,
        minValue: 0,
        maxValue: 120,
        unit: '°C',
        color: Colors.orange,
      ),
      'afr_gauge': GaugeConfiguration(
        id: 'afr_gauge',
        name: 'AFR',
        type: GaugeType.linear,
        parameter: 'afr',
        x: 200,
        y: 220,
        size: 120,
        minValue: 10,
        maxValue: 20,
        unit: ':1',
        color: Colors.purple,
      ),
      'boost_gauge': GaugeConfiguration(
        id: 'boost_gauge',
        name: 'Boost',
        type: GaugeType.circular,
        parameter: 'boost',
        x: 350,
        y: 220,
        size: 120,
        minValue: -10,
        maxValue: 30,
        unit: 'psi',
        color: Colors.teal,
      ),
    };
  }
  
  /// Configure grid snapping
  void configureGrid({bool? snapToGrid, double? gridSize}) {
    if (snapToGrid != null) _snapToGrid = snapToGrid;
    if (gridSize != null) _gridSize = gridSize;
    notifyListeners();
  }
  
  /// Configure manipulation steps
  void configureManipulation({double? positionStep, double? sizeStep}) {
    if (positionStep != null) _positionStep = positionStep;
    if (sizeStep != null) _sizeStep = sizeStep;
    notifyListeners();
  }
  
  /// Export gauge layout configuration
  Map<String, dynamic> exportLayout() {
    return {
      'version': '1.0',
      'timestamp': DateTime.now().toIso8601String(),
      'gauges': _gaugeConfigs.map((id, config) => MapEntry(id, config.toMap())),
      'settings': {
        'snapToGrid': _snapToGrid,
        'gridSize': _gridSize,
        'positionStep': _positionStep,
        'sizeStep': _sizeStep,
      },
    };
  }
  
  /// Import gauge layout configuration
  void importLayout(Map<String, dynamic> layoutData) {
    try {
      final gaugesData = layoutData['gauges'] as Map<String, dynamic>?;
      if (gaugesData != null) {
        _gaugeConfigs.clear();
        for (final entry in gaugesData.entries) {
          final config = GaugeConfiguration.fromMap(entry.value);
          _gaugeConfigs[entry.key] = config;
        }
      }
      
      final settings = layoutData['settings'] as Map<String, dynamic>?;
      if (settings != null) {
        _snapToGrid = settings['snapToGrid'] ?? _snapToGrid;
        _gridSize = settings['gridSize'] ?? _gridSize;
        _positionStep = settings['positionStep'] ?? _positionStep;
        _sizeStep = settings['sizeStep'] ?? _sizeStep;
      }
      
      notifyListeners();
    } catch (e) {
      print('Error importing gauge layout: $e');
    }
  }
}

/// Gauge configuration data class
class GaugeConfiguration {
  final String id;
  final String name;
  final GaugeType type;
  final String parameter;
  final double x;
  final double y;
  final double size;
  final double minValue;
  final double maxValue;
  final String unit;
  final Color color;
  final bool visible;
  
  const GaugeConfiguration({
    required this.id,
    required this.name,
    required this.type,
    required this.parameter,
    required this.x,
    required this.y,
    required this.size,
    required this.minValue,
    required this.maxValue,
    required this.unit,
    required this.color,
    this.visible = true,
  });
  
  GaugeConfiguration copyWith({
    String? id,
    String? name,
    GaugeType? type,
    String? parameter,
    double? x,
    double? y,
    double? size,
    double? minValue,
    double? maxValue,
    String? unit,
    Color? color,
    bool? visible,
  }) {
    return GaugeConfiguration(
      id: id ?? this.id,
      name: name ?? this.name,
      type: type ?? this.type,
      parameter: parameter ?? this.parameter,
      x: x ?? this.x,
      y: y ?? this.y,
      size: size ?? this.size,
      minValue: minValue ?? this.minValue,
      maxValue: maxValue ?? this.maxValue,
      unit: unit ?? this.unit,
      color: color ?? this.color,
      visible: visible ?? this.visible,
    );
  }
  
  Map<String, dynamic> toMap() {
    return {
      'id': id,
      'name': name,
      'type': type.toString().split('.').last, // Extract enum name manually
      'parameter': parameter,
      'x': x,
      'y': y,
      'size': size,
      'minValue': minValue,
      'maxValue': maxValue,
      'unit': unit,
      'color': color.value,
      'visible': visible,
    };
  }
  
  factory GaugeConfiguration.fromMap(Map<String, dynamic> map) {
    // Find enum value by name
    GaugeType? gaugeType;
    final typeName = map['type'] as String;
    for (final type in GaugeType.values) {
      if (type.toString().split('.').last == typeName) {
        gaugeType = type;
        break;
      }
    }
    
    return GaugeConfiguration(
      id: map['id'],
      name: map['name'],
      type: gaugeType ?? GaugeType.circular,
      parameter: map['parameter'],
      x: map['x'].toDouble(),
      y: map['y'].toDouble(),
      size: map['size'].toDouble(),
      minValue: map['minValue'].toDouble(),
      maxValue: map['maxValue'].toDouble(),
      unit: map['unit'],
      color: Color(map['color']),
      visible: map['visible'] ?? true,
    );
  }
}

/// Gauge types
enum GaugeType {
  circular,
  linear,
  digital,
  bar,
}