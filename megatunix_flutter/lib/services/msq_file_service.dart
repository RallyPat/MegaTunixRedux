/// MSQ File Management Service
/// Handles loading, saving, and managing MegaSquirt tune files

import 'package:flutter/foundation.dart';
import 'package:path/path.dart' as path;
import '../models/msq_file.dart';

/// Service for managing MSQ tune files
class MSQFileService extends ChangeNotifier {
  MSQFile? _currentFile;
  String? _currentFilePath;
  bool _hasUnsavedChanges = false;
  final List<MSQFile> _recentFiles = [];
  
  /// Currently loaded MSQ file
  MSQFile? get currentFile => _currentFile;
  
  /// Path to currently loaded file
  String? get currentFilePath => _currentFilePath;
  
  /// Whether there are unsaved changes
  bool get hasUnsavedChanges => _hasUnsavedChanges;
  
  /// Recently opened files
  List<MSQFile> get recentFiles => List.unmodifiable(_recentFiles);
  
  /// Whether a file is currently loaded
  bool get hasFile => _currentFile != null;

  /// Create a new MSQ file with default values
  void createNewFile() {
    _currentFile = MSQFile.createDefault();
    _currentFilePath = null;
    _hasUnsavedChanges = true;
    notifyListeners();
  }

  /// Load MSQ file from path
  Future<bool> loadFile(String filePath) async {
    try {
      final file = await MSQFile.loadFromFile(filePath);
      _currentFile = file;
      _currentFilePath = filePath;
      _hasUnsavedChanges = false;
      
      // Add to recent files
      _addToRecentFiles(file);
      
      notifyListeners();
      return true;
    } catch (e) {
      debugPrint('Error loading MSQ file: $e');
      return false;
    }
  }

  /// Save current file to disk
  Future<bool> saveFile([String? filePath]) async {
    if (_currentFile == null) return false;

    try {
      final savePath = filePath ?? _currentFilePath;
      if (savePath == null) return false;

      // Update header timestamp
      final updatedHeader = MSQHeader(
        fileVersion: _currentFile!.header.fileVersion,
        timestamp: DateTime.now().toIso8601String(),
        ecuType: _currentFile!.header.ecuType,
        firmwareVersion: _currentFile!.header.firmwareVersion,
        tunerStudioVersion: _currentFile!.header.tunerStudioVersion,
        metadata: _currentFile!.header.metadata,
      );

      final updatedFile = _currentFile!.copyWith(header: updatedHeader);
      
      await updatedFile.saveToFile(savePath);
      
      _currentFile = updatedFile;
      _currentFilePath = savePath;
      _hasUnsavedChanges = false;
      
      notifyListeners();
      return true;
    } catch (e) {
      debugPrint('Error saving MSQ file: $e');
      return false;
    }
  }

  /// Save file with a new name/path
  Future<bool> saveFileAs(String filePath) async {
    if (_currentFile == null) return false;

    try {
      // Ensure the file has .msq extension
      String finalPath = filePath;
      if (!filePath.toLowerCase().endsWith('.msq')) {
        finalPath = '$filePath.msq';
      }

      final success = await saveFile(finalPath);
      if (success) {
        _currentFilePath = finalPath;
      }
      return success;
    } catch (e) {
      debugPrint('Error saving MSQ file as: $e');
      return false;
    }
  }

  /// Update VE table value
  void updateVETableValue(int rpmIndex, int mapIndex, double value) {
    if (_currentFile == null) return;

    _currentFile!.veTable.setValue(rpmIndex, mapIndex, value);
    _hasUnsavedChanges = true;
    notifyListeners();
  }

  /// Update ignition table value
  void updateIgnitionTableValue(int rpmIndex, int mapIndex, double value) {
    if (_currentFile == null) return;

    _currentFile!.ignitionTable.setValue(rpmIndex, mapIndex, value);
    _hasUnsavedChanges = true;
    notifyListeners();
  }

  /// Update engine setting
  void updateEngineSetting(String key, dynamic value) {
    if (_currentFile == null) return;

    _currentFile!.settings.engineSettings[key] = value;
    _hasUnsavedChanges = true;
    notifyListeners();
  }

  /// Update injector setting
  void updateInjectorSetting(String key, dynamic value) {
    if (_currentFile == null) return;

    _currentFile!.settings.injectorSettings[key] = value;
    _hasUnsavedChanges = true;
    notifyListeners();
  }

  /// Update ignition setting
  void updateIgnitionSetting(String key, dynamic value) {
    if (_currentFile == null) return;

    _currentFile!.settings.ignitionSettings[key] = value;
    _hasUnsavedChanges = true;
    notifyListeners();
  }

  /// Update sensor setting
  void updateSensorSetting(String key, dynamic value) {
    if (_currentFile == null) return;

    _currentFile!.settings.sensorSettings[key] = value;
    _hasUnsavedChanges = true;
    notifyListeners();
  }

  /// Add or update a note for a setting
  void addSettingNote(String settingPath, String note) {
    if (_currentFile == null) return;

    _currentFile!.settings.addNote(settingPath, note);
    _hasUnsavedChanges = true;
    notifyListeners();
  }

  /// Get note for a setting
  String? getSettingNote(String settingPath) {
    return _currentFile?.settings.getNote(settingPath);
  }

  /// Update file metadata
  void updateMetadata(String key, String value) {
    if (_currentFile == null) return;

    final updatedMetadata = Map<String, String>.from(_currentFile!.header.metadata);
    updatedMetadata[key] = value;
    
    final updatedHeader = MSQHeader(
      fileVersion: _currentFile!.header.fileVersion,
      timestamp: _currentFile!.header.timestamp,
      ecuType: _currentFile!.header.ecuType,
      firmwareVersion: _currentFile!.header.firmwareVersion,
      tunerStudioVersion: _currentFile!.header.tunerStudioVersion,
      metadata: updatedMetadata,
    );

    _currentFile = _currentFile!.copyWith(header: updatedHeader);
    _hasUnsavedChanges = true;
    notifyListeners();
  }

  /// Close current file
  void closeFile() {
    _currentFile = null;
    _currentFilePath = null;
    _hasUnsavedChanges = false;
    notifyListeners();
  }

  /// Get file info for display
  Map<String, String> getFileInfo() {
    if (_currentFile == null) return {};

    return {
      'File Name': _currentFilePath != null ? path.basename(_currentFilePath!) : 'Untitled',
      'ECU Type': _currentFile!.header.ecuType,
      'Firmware': _currentFile!.header.firmwareVersion,
      'Last Modified': _formatDateTime(_currentFile!.lastModified),
      'Project': _currentFile!.header.metadata['project'] ?? 'Unknown',
      'Vehicle': _currentFile!.header.metadata['vehicle'] ?? 'Unknown',
    };
  }

  /// Compare two MSQ files and return differences
  Map<String, dynamic> compareFiles(MSQFile otherFile) {
    if (_currentFile == null) return {};

    final differences = <String, dynamic>{};

    // Compare VE tables
    final veTableDiffs = _compareVETables(_currentFile!.veTable, otherFile.veTable);
    if (veTableDiffs.isNotEmpty) {
      differences['veTable'] = veTableDiffs;
    }

    // Compare ignition tables
    final ignitionTableDiffs = _compareIgnitionTables(_currentFile!.ignitionTable, otherFile.ignitionTable);
    if (ignitionTableDiffs.isNotEmpty) {
      differences['ignitionTable'] = ignitionTableDiffs;
    }

    // Compare settings
    final settingsDiffs = _compareSettings(_currentFile!.settings, otherFile.settings);
    if (settingsDiffs.isNotEmpty) {
      differences['settings'] = settingsDiffs;
    }

    return differences;
  }

  /// Add file to recent files list
  void _addToRecentFiles(MSQFile file) {
    _recentFiles.removeWhere((f) => f.header.timestamp == file.header.timestamp);
    _recentFiles.insert(0, file);
    
    // Keep only the 10 most recent files
    if (_recentFiles.length > 10) {
      _recentFiles.removeRange(10, _recentFiles.length);
    }
  }

  /// Compare VE tables
  Map<String, dynamic> _compareVETables(VETable table1, VETable table2) {
    final differences = <String, dynamic>{};
    
    for (int mapIndex = 0; mapIndex < table1.values.length; mapIndex++) {
      for (int rpmIndex = 0; rpmIndex < table1.values[mapIndex].length; rpmIndex++) {
        if (table1.values[mapIndex][rpmIndex] != table2.values[mapIndex][rpmIndex]) {
          differences['[$mapIndex][$rpmIndex]'] = {
            'current': table1.values[mapIndex][rpmIndex],
            'other': table2.values[mapIndex][rpmIndex],
          };
        }
      }
    }
    
    return differences;
  }

  /// Compare ignition tables
  Map<String, dynamic> _compareIgnitionTables(IgnitionTable table1, IgnitionTable table2) {
    final differences = <String, dynamic>{};
    
    for (int mapIndex = 0; mapIndex < table1.values.length; mapIndex++) {
      for (int rpmIndex = 0; rpmIndex < table1.values[mapIndex].length; rpmIndex++) {
        if (table1.values[mapIndex][rpmIndex] != table2.values[mapIndex][rpmIndex]) {
          differences['[$mapIndex][$rpmIndex]'] = {
            'current': table1.values[mapIndex][rpmIndex],
            'other': table2.values[mapIndex][rpmIndex],
          };
        }
      }
    }
    
    return differences;
  }

  /// Compare settings
  Map<String, dynamic> _compareSettings(ECUSettings settings1, ECUSettings settings2) {
    final differences = <String, dynamic>{};

    // Compare each settings category
    _compareSettingsCategory(settings1.engineSettings, settings2.engineSettings, 'engine', differences);
    _compareSettingsCategory(settings1.injectorSettings, settings2.injectorSettings, 'injector', differences);
    _compareSettingsCategory(settings1.ignitionSettings, settings2.ignitionSettings, 'ignition', differences);
    _compareSettingsCategory(settings1.sensorSettings, settings2.sensorSettings, 'sensor', differences);

    return differences;
  }

  /// Compare individual settings category
  void _compareSettingsCategory(Map<String, dynamic> settings1, Map<String, dynamic> settings2, 
                               String category, Map<String, dynamic> differences) {
    final categoryDiffs = <String, dynamic>{};
    
    final allKeys = <String>{...settings1.keys, ...settings2.keys};
    for (final key in allKeys) {
      final value1 = settings1[key];
      final value2 = settings2[key];
      
      if (value1 != value2) {
        categoryDiffs[key] = {
          'current': value1,
          'other': value2,
        };
      }
    }
    
    if (categoryDiffs.isNotEmpty) {
      differences[category] = categoryDiffs;
    }
  }

  /// Format DateTime for display
  String _formatDateTime(DateTime dateTime) {
    return '${dateTime.year}-${dateTime.month.toString().padLeft(2, '0')}-${dateTime.day.toString().padLeft(2, '0')} '
           '${dateTime.hour.toString().padLeft(2, '0')}:${dateTime.minute.toString().padLeft(2, '0')}';
  }
}