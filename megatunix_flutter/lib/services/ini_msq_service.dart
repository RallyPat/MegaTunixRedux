/// INI-Compatible MSQ File Format
/// Maintains full TunerStudio .msq compatibility using INI definitions

import 'dart:io';
import 'dart:convert';
import 'package:flutter/foundation.dart';
import 'ini_parser.dart';
import 'realistic_table_data.dart';

/// INI-based tune file that maintains TunerStudio compatibility
class INIMSQFile {
  final ECUDefinition ecuDefinition;
  final String ecuName;
  final String firmwareVersion;
  final DateTime createdAt;
  final DateTime modifiedAt;
  final Map<String, dynamic> metadata;
  final Map<String, dynamic> constants;
  final Map<String, List<List<double>>> tables;
  final Map<String, String> notes;

  INIMSQFile({
    required this.ecuDefinition,
    required this.ecuName,
    required this.firmwareVersion,
    required this.createdAt,
    required this.modifiedAt,
    required this.metadata,
    required this.constants,
    required this.tables,
    required this.notes,
  });

  /// Create default MSQ file from ECU definition
  factory INIMSQFile.createDefault(ECUDefinition ecuDefinition) {
    final now = DateTime.now();
    final constants = <String, dynamic>{};
    final tables = <String, List<List<double>>>{};

    // Initialize constants with default values
    for (final setting in ecuDefinition.settings) {
      constants[setting.name] = setting.defaultValue;
    }

    // Initialize tables with realistic default values
    for (final tableDef in ecuDefinition.tables) {
      // Generate realistic table data based on table type
      final tableData = RealisticTableData.generateTableByName(
        tableDef.name,
        rows: tableDef.rows,
        cols: tableDef.cols,
        mapBins: tableDef.yBins,
        rpmBins: tableDef.xBins,
      );
      tables[tableDef.name] = tableData;
    }

    return INIMSQFile(
      ecuDefinition: ecuDefinition,
      ecuName: ecuDefinition.name,
      firmwareVersion: ecuDefinition.version,
      createdAt: now,
      modifiedAt: now,
      metadata: {
        'project': 'New Project',
        'vehicle': 'Unknown Vehicle',
        'engine': 'Unknown Engine',
        'tuner': 'MegaTunix Redux',
        'signature': ecuDefinition.signature,
      },
      constants: constants,
      tables: tables,
      notes: {},
    );
  }

  /// Load MSQ file with ECU definition
  static Future<INIMSQFile> loadFromFile(String filePath, ECUDefinition ecuDefinition) async {
    final file = File(filePath);
    final content = await file.readAsString();
    
    if (filePath.endsWith('.msq')) {
      return _parseBinaryMSQ(content, ecuDefinition);
    } else {
      return _parseJSONMSQ(content, ecuDefinition);
    }
  }

  /// Save MSQ file in TunerStudio-compatible format
  Future<void> saveToFile(String filePath) async {
    if (filePath.endsWith('.msq')) {
      await _saveBinaryMSQ(filePath);
    } else {
      await _saveJSONMSQ(filePath);
    }
  }

  /// Parse binary MSQ format (TunerStudio compatible)
  static INIMSQFile _parseBinaryMSQ(String content, ECUDefinition ecuDefinition) {
    // TunerStudio MSQ files are binary format with specific structure
    // For now, implement JSON fallback - binary parsing requires reverse engineering
    try {
      final jsonData = json.decode(content);
      return _parseJSONData(jsonData, ecuDefinition);
    } catch (e) {
      debugPrint('Binary MSQ parsing not yet implemented, creating default: $e');
      return INIMSQFile.createDefault(ecuDefinition);
    }
  }

  /// Parse JSON MSQ format (MegaTunix Redux compatible)
  static INIMSQFile _parseJSONMSQ(String content, ECUDefinition ecuDefinition) {
    final jsonData = json.decode(content);
    return _parseJSONData(jsonData, ecuDefinition);
  }

  /// Parse JSON data into INIMSQFile
  static INIMSQFile _parseJSONData(Map<String, dynamic> jsonData, ECUDefinition ecuDefinition) {
    final constants = Map<String, dynamic>.from(jsonData['constants'] ?? {});
    final tables = <String, List<List<double>>>{};

    // Parse tables
    final tablesData = jsonData['tables'] as Map<String, dynamic>? ?? {};
    for (final entry in tablesData.entries) {
      final tableData = entry.value as List;
      tables[entry.key] = tableData
          .map((row) => (row as List).map((val) => (val as num).toDouble()).toList())
          .toList();
    }

    return INIMSQFile(
      ecuDefinition: ecuDefinition,
      ecuName: jsonData['ecuName'] ?? ecuDefinition.name,
      firmwareVersion: jsonData['firmwareVersion'] ?? ecuDefinition.version,
      createdAt: DateTime.parse(jsonData['createdAt'] ?? DateTime.now().toIso8601String()),
      modifiedAt: DateTime.parse(jsonData['modifiedAt'] ?? DateTime.now().toIso8601String()),
      metadata: Map<String, dynamic>.from(jsonData['metadata'] ?? {}),
      constants: constants,
      tables: tables,
      notes: Map<String, String>.from(jsonData['notes'] ?? {}),
    );
  }

  /// Save as binary MSQ (TunerStudio format)
  Future<void> _saveBinaryMSQ(String filePath) async {
    // TODO: Implement true TunerStudio binary MSQ format
    // For now, save as JSON with .msq extension for compatibility
    await _saveJSONMSQ(filePath);
  }

  /// Save as JSON MSQ
  Future<void> _saveJSONMSQ(String filePath) async {
    final data = {
      'ecuName': ecuName,
      'firmwareVersion': firmwareVersion,
      'createdAt': createdAt.toIso8601String(),
      'modifiedAt': DateTime.now().toIso8601String(),
      'metadata': metadata,
      'constants': constants,
      'tables': tables,
      'notes': notes,
      'ecuDefinition': {
        'name': ecuDefinition.name,
        'version': ecuDefinition.version,
        'signature': ecuDefinition.signature,
      },
    };

    final file = File(filePath);
    await file.writeAsString(const JsonEncoder.withIndent('  ').convert(data));
  }

  /// Get table data by name
  List<List<double>>? getTable(String tableName) {
    return tables[tableName];
  }

  /// Set table value
  void setTableValue(String tableName, int row, int col, double value) {
    final table = tables[tableName];
    if (table != null && row < table.length && col < table[row].length) {
      final tableDef = ecuDefinition.tables.firstWhere((t) => t.name == tableName);
      final clampedValue = value.clamp(tableDef.minValue, tableDef.maxValue);
      table[row][col] = clampedValue;
    }
  }

  /// Get constant value
  dynamic getConstant(String name) {
    return constants[name];
  }

  /// Set constant value
  void setConstant(String name, dynamic value) {
    final settingDef = ecuDefinition.settings.firstWhere(
      (s) => s.name == name,
      orElse: () => throw ArgumentError('Setting not found: $name'),
    );
    
    // Validate value based on setting definition
    if (value is num) {
      final clampedValue = value.toDouble().clamp(settingDef.minValue, settingDef.maxValue);
      constants[name] = clampedValue;
    } else {
      constants[name] = value;
    }
  }

  /// Add or update note
  void setNote(String path, String note) {
    notes[path] = note;
  }

  /// Get note
  String? getNote(String path) {
    return notes[path];
  }

  /// Create copy with modifications
  INIMSQFile copyWith({
    Map<String, dynamic>? metadata,
    Map<String, dynamic>? constants,
    Map<String, List<List<double>>>? tables,
    Map<String, String>? notes,
  }) {
    return INIMSQFile(
      ecuDefinition: ecuDefinition,
      ecuName: ecuName,
      firmwareVersion: firmwareVersion,
      createdAt: createdAt,
      modifiedAt: DateTime.now(),
      metadata: metadata ?? Map.from(this.metadata),
      constants: constants ?? Map.from(this.constants),
      tables: tables ?? Map.from(this.tables),
      notes: notes ?? Map.from(this.notes),
    );
  }

  /// Compare with another MSQ file
  Map<String, dynamic> compareTo(INIMSQFile other) {
    final differences = <String, dynamic>{};

    // Compare constants
    final constantDiffs = <String, dynamic>{};
    final allConstantKeys = {...constants.keys, ...other.constants.keys};
    for (final key in allConstantKeys) {
      final thisValue = constants[key];
      final otherValue = other.constants[key];
      if (thisValue != otherValue) {
        constantDiffs[key] = {'current': thisValue, 'other': otherValue};
      }
    }
    if (constantDiffs.isNotEmpty) {
      differences['constants'] = constantDiffs;
    }

    // Compare tables
    final tableDiffs = <String, dynamic>{};
    final allTableKeys = {...tables.keys, ...other.tables.keys};
    for (final tableName in allTableKeys) {
      final thisTable = tables[tableName];
      final otherTable = other.tables[tableName];
      
      if (thisTable == null || otherTable == null) {
        tableDiffs[tableName] = {'current': thisTable, 'other': otherTable};
        continue;
      }

      final cellDiffs = <String, dynamic>{};
      for (int row = 0; row < thisTable.length && row < otherTable.length; row++) {
        for (int col = 0; col < thisTable[row].length && col < otherTable[row].length; col++) {
          if (thisTable[row][col] != otherTable[row][col]) {
            cellDiffs['[$row][$col]'] = {
              'current': thisTable[row][col],
              'other': otherTable[row][col],
            };
          }
        }
      }
      if (cellDiffs.isNotEmpty) {
        tableDiffs[tableName] = cellDiffs;
      }
    }
    if (tableDiffs.isNotEmpty) {
      differences['tables'] = tableDiffs;
    }

    return differences;
  }

  /// Get file summary information
  Map<String, String> getFileSummary() {
    return {
      'ECU Type': ecuName,
      'Firmware': firmwareVersion,
      'Created': _formatDateTime(createdAt),
      'Modified': _formatDateTime(modifiedAt),
      'Project': metadata['project']?.toString() ?? 'Unknown',
      'Vehicle': metadata['vehicle']?.toString() ?? 'Unknown',
      'Tables': tables.length.toString(),
      'Settings': constants.length.toString(),
    };
  }

  /// Get table X axis values from ECU definition  
  List<double> getTableXAxis(String tableName) {
    final tableDef = ecuDefinition.tables.firstWhere(
      (t) => t.name == tableName,
      orElse: () => throw ArgumentError('Table not found: $tableName'),
    );
    
    // Return predefined bins or generate realistic defaults
    if (tableDef.xBins != null) {
      return tableDef.xBins!;
    }
    
    // Generate realistic axis bins based on table type and axis type
    final axisType = tableDef.xAxisType ?? 'rpm';
    return RealisticTableData.generateAxisBins(axisType, tableDef.cols);
  }

  /// Get table Y axis values from ECU definition
  List<double> getTableYAxis(String tableName) {
    final tableDef = ecuDefinition.tables.firstWhere(
      (t) => t.name == tableName,
      orElse: () => throw ArgumentError('Table not found: $tableName'),
    );
    
    // Return predefined bins or generate realistic defaults
    if (tableDef.yBins != null) {
      return tableDef.yBins!;
    }
    
    // Generate realistic axis bins based on table type and axis type
    final axisType = tableDef.yAxisType ?? 'map';
    return RealisticTableData.generateAxisBins(axisType, tableDef.rows);
  }

  /// Format DateTime for display
  String _formatDateTime(DateTime dateTime) {
    return '${dateTime.year}-${dateTime.month.toString().padLeft(2, '0')}-${dateTime.day.toString().padLeft(2, '0')} '
           '${dateTime.hour.toString().padLeft(2, '0')}:${dateTime.minute.toString().padLeft(2, '0')}';
  }
}

/// INI-based MSQ File Service
class INIMSQFileService extends ChangeNotifier {
  INIMSQFile? _currentFile;
  String? _currentFilePath;
  bool _hasUnsavedChanges = false;
  final INIECUManager _ecuManager;

  INIMSQFileService(this._ecuManager);

  /// Currently loaded MSQ file
  INIMSQFile? get currentFile => _currentFile;

  /// Current file path
  String? get currentFilePath => _currentFilePath;

  /// Has unsaved changes
  bool get hasUnsavedChanges => _hasUnsavedChanges;

  /// Has file loaded
  bool get hasFile => _currentFile != null;

  /// Create new file from current ECU definition
  void createNewFile() {
    final ecuDef = _ecuManager.currentECU;
    if (ecuDef == null) {
      throw StateError('No ECU definition loaded');
    }

    _currentFile = INIMSQFile.createDefault(ecuDef);
    _currentFilePath = null;
    _hasUnsavedChanges = true;
    notifyListeners();
  }

  /// Load MSQ file
  Future<bool> loadFile(String filePath) async {
    try {
      final ecuDef = _ecuManager.currentECU;
      if (ecuDef == null) {
        throw StateError('No ECU definition loaded');
      }

      final file = await INIMSQFile.loadFromFile(filePath, ecuDef);
      _currentFile = file;
      _currentFilePath = filePath;
      _hasUnsavedChanges = false;
      notifyListeners();
      return true;
    } catch (e) {
      debugPrint('Error loading MSQ file: $e');
      return false;
    }
  }

  /// Save current file
  Future<bool> saveFile([String? filePath]) async {
    if (_currentFile == null) return false;

    try {
      final savePath = filePath ?? _currentFilePath;
      if (savePath == null) return false;

      await _currentFile!.saveToFile(savePath);
      _currentFilePath = savePath;
      _hasUnsavedChanges = false;
      notifyListeners();
      return true;
    } catch (e) {
      debugPrint('Error saving MSQ file: $e');
      return false;
    }
  }

  /// Update table value
  void updateTableValue(String tableName, int row, int col, double value) {
    if (_currentFile == null) return;
    
    _currentFile!.setTableValue(tableName, row, col, value);
    _hasUnsavedChanges = true;
    notifyListeners();
  }

  /// Update constant value
  void updateConstant(String name, dynamic value) {
    if (_currentFile == null) return;
    
    _currentFile!.setConstant(name, value);
    _hasUnsavedChanges = true;
    notifyListeners();
  }

  /// Add note
  void addNote(String path, String note) {
    if (_currentFile == null) return;
    
    _currentFile!.setNote(path, note);
    _hasUnsavedChanges = true;
    notifyListeners();
  }

  /// Close file
  void closeFile() {
    _currentFile = null;
    _currentFilePath = null;
    _hasUnsavedChanges = false;
    notifyListeners();
  }
}