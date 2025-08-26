/// MegaSquirt MSQ File Format Implementation
/// Based on TunerStudio MSQ file specification and MegaSquirt configuration data

import 'dart:convert';
import 'dart:io';

/// MSQ File Header Information
class MSQHeader {
  final String fileVersion;
  final String timestamp;
  final String ecuType;
  final String firmwareVersion;
  final String tunerStudioVersion;
  final Map<String, String> metadata;

  MSQHeader({
    required this.fileVersion,
    required this.timestamp,
    required this.ecuType,
    required this.firmwareVersion,
    required this.tunerStudioVersion,
    required this.metadata,
  });

  factory MSQHeader.fromMap(Map<String, dynamic> map) {
    return MSQHeader(
      fileVersion: map['fileVersion'] ?? '1.0',
      timestamp: map['timestamp'] ?? DateTime.now().toIso8601String(),
      ecuType: map['ecuType'] ?? 'Unknown',
      firmwareVersion: map['firmwareVersion'] ?? '1.0',
      tunerStudioVersion: map['tunerStudioVersion'] ?? 'MegaTunix Redux 1.0',
      metadata: Map<String, String>.from(map['metadata'] ?? {}),
    );
  }

  Map<String, dynamic> toMap() {
    return {
      'fileVersion': fileVersion,
      'timestamp': timestamp,
      'ecuType': ecuType,
      'firmwareVersion': firmwareVersion,
      'tunerStudioVersion': tunerStudioVersion,
      'metadata': metadata,
    };
  }
}

/// VE Table Data Structure
class VETable {
  final List<List<double>> values;
  final List<double> rpmBins;
  final List<double> mapBins;
  final String name;
  final String units;
  final double minValue;
  final double maxValue;

  VETable({
    required this.values,
    required this.rpmBins,
    required this.mapBins,
    required this.name,
    this.units = '%',
    this.minValue = 0.0,
    this.maxValue = 255.0,
  });

  factory VETable.createDefault() {
    // Create a 16x16 default VE table with reasonable values
    const rpmBins = [500, 800, 1200, 1600, 2000, 2400, 2800, 3200, 3600, 4000, 4400, 4800, 5200, 5600, 6000, 6500];
    const mapBins = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160];
    
    // Generate reasonable VE values based on typical engine characteristics
    final values = List.generate(16, (mapIndex) {
      return List.generate(16, (rpmIndex) {
        // Simple VE curve approximation
        double baseVE = 80.0; // Base VE percentage
        double mapEffect = (mapBins[mapIndex] - 100) * 0.1; // MAP effect
        double rpmEffect = rpmIndex < 8 ? rpmIndex * 2.0 : (16 - rpmIndex) * 1.5; // RPM effect
        return (baseVE + mapEffect + rpmEffect).clamp(20.0, 120.0);
      });
    });

    return VETable(
      values: values,
      rpmBins: rpmBins.map((e) => e.toDouble()).toList(),
      mapBins: mapBins.map((e) => e.toDouble()).toList(),
      name: 'VE Table',
      units: '%',
    );
  }

  factory VETable.fromMap(Map<String, dynamic> map) {
    return VETable(
      values: (map['values'] as List).map((row) => 
        (row as List).map((val) => (val as num).toDouble()).toList()
      ).toList(),
      rpmBins: (map['rpmBins'] as List).map((val) => (val as num).toDouble()).toList(),
      mapBins: (map['mapBins'] as List).map((val) => (val as num).toDouble()).toList(),
      name: map['name'] ?? 'VE Table',
      units: map['units'] ?? '%',
      minValue: (map['minValue'] as num?)?.toDouble() ?? 0.0,
      maxValue: (map['maxValue'] as num?)?.toDouble() ?? 255.0,
    );
  }

  Map<String, dynamic> toMap() {
    return {
      'values': values,
      'rpmBins': rpmBins,
      'mapBins': mapBins,
      'name': name,
      'units': units,
      'minValue': minValue,
      'maxValue': maxValue,
    };
  }

  /// Get interpolated value for given RPM and MAP
  double getValue(double rpm, double map) {
    // Find surrounding bins
    int rpmLow = 0, rpmHigh = rpmBins.length - 1;
    int mapLow = 0, mapHigh = mapBins.length - 1;

    // Find RPM bin indices
    for (int i = 0; i < rpmBins.length - 1; i++) {
      if (rpm >= rpmBins[i] && rpm <= rpmBins[i + 1]) {
        rpmLow = i;
        rpmHigh = i + 1;
        break;
      }
    }

    // Find MAP bin indices
    for (int i = 0; i < mapBins.length - 1; i++) {
      if (map >= mapBins[i] && map <= mapBins[i + 1]) {
        mapLow = i;
        mapHigh = i + 1;
        break;
      }
    }

    // Bilinear interpolation
    double rpmFactor = rpmLow == rpmHigh ? 0 : (rpm - rpmBins[rpmLow]) / (rpmBins[rpmHigh] - rpmBins[rpmLow]);
    double mapFactor = mapLow == mapHigh ? 0 : (map - mapBins[mapLow]) / (mapBins[mapHigh] - mapBins[mapLow]);

    double val1 = values[mapLow][rpmLow] * (1 - rpmFactor) + values[mapLow][rpmHigh] * rpmFactor;
    double val2 = values[mapHigh][rpmLow] * (1 - rpmFactor) + values[mapHigh][rpmHigh] * rpmFactor;

    return val1 * (1 - mapFactor) + val2 * mapFactor;
  }

  /// Set value at specific bin indices
  void setValue(int rpmIndex, int mapIndex, double value) {
    if (rpmIndex >= 0 && rpmIndex < rpmBins.length && 
        mapIndex >= 0 && mapIndex < mapBins.length) {
      values[mapIndex][rpmIndex] = value.clamp(minValue, maxValue);
    }
  }
}

/// Ignition Table Data Structure
class IgnitionTable {
  final List<List<double>> values;
  final List<double> rpmBins;
  final List<double> mapBins;
  final String name;
  final String units;

  IgnitionTable({
    required this.values,
    required this.rpmBins,
    required this.mapBins,
    required this.name,
    this.units = '°',
  });

  factory IgnitionTable.createDefault() {
    const rpmBins = [500, 800, 1200, 1600, 2000, 2400, 2800, 3200, 3600, 4000, 4400, 4800, 5200, 5600, 6000, 6500];
    const mapBins = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160];
    
    // Generate reasonable timing values
    final values = List.generate(16, (mapIndex) {
      return List.generate(16, (rpmIndex) {
        // Conservative timing curve
        double baseTiming = 15.0; // Base timing
        double mapEffect = -(mapBins[mapIndex] - 100) * 0.1; // Retard under boost
        double rpmEffect = rpmIndex * 0.5; // Advance with RPM
        return (baseTiming + mapEffect + rpmEffect).clamp(-10.0, 35.0);
      });
    });

    return IgnitionTable(
      values: values,
      rpmBins: rpmBins.map((e) => e.toDouble()).toList(),
      mapBins: mapBins.map((e) => e.toDouble()).toList(),
      name: 'Ignition Table',
    );
  }

  factory IgnitionTable.fromMap(Map<String, dynamic> map) {
    return IgnitionTable(
      values: (map['values'] as List).map((row) => 
        (row as List).map((val) => (val as num).toDouble()).toList()
      ).toList(),
      rpmBins: (map['rpmBins'] as List).map((val) => (val as num).toDouble()).toList(),
      mapBins: (map['mapBins'] as List).map((val) => (val as num).toDouble()).toList(),
      name: map['name'] ?? 'Ignition Table',
      units: map['units'] ?? '°',
    );
  }

  Map<String, dynamic> toMap() {
    return {
      'values': values,
      'rpmBins': rpmBins,
      'mapBins': mapBins,
      'name': name,
      'units': units,
    };
  }

  double getValue(double rpm, double map) {
    // Similar interpolation logic as VETable
    int rpmLow = 0, rpmHigh = rpmBins.length - 1;
    int mapLow = 0, mapHigh = mapBins.length - 1;

    for (int i = 0; i < rpmBins.length - 1; i++) {
      if (rpm >= rpmBins[i] && rpm <= rpmBins[i + 1]) {
        rpmLow = i;
        rpmHigh = i + 1;
        break;
      }
    }

    for (int i = 0; i < mapBins.length - 1; i++) {
      if (map >= mapBins[i] && map <= mapBins[i + 1]) {
        mapLow = i;
        mapHigh = i + 1;
        break;
      }
    }

    double rpmFactor = rpmLow == rpmHigh ? 0 : (rpm - rpmBins[rpmLow]) / (rpmBins[rpmHigh] - rpmBins[rpmLow]);
    double mapFactor = mapLow == mapHigh ? 0 : (map - mapBins[mapLow]) / (mapBins[mapHigh] - mapBins[mapLow]);

    double val1 = values[mapLow][rpmLow] * (1 - rpmFactor) + values[mapLow][rpmHigh] * rpmFactor;
    double val2 = values[mapHigh][rpmLow] * (1 - rpmFactor) + values[mapHigh][rpmHigh] * rpmFactor;

    return val1 * (1 - mapFactor) + val2 * mapFactor;
  }

  void setValue(int rpmIndex, int mapIndex, double value) {
    if (rpmIndex >= 0 && rpmIndex < rpmBins.length && 
        mapIndex >= 0 && mapIndex < mapBins.length) {
      values[mapIndex][rpmIndex] = value.clamp(-10.0, 50.0);
    }
  }
}

/// ECU Configuration Settings
class ECUSettings {
  final Map<String, dynamic> engineSettings;
  final Map<String, dynamic> injectorSettings;
  final Map<String, dynamic> ignitionSettings;
  final Map<String, dynamic> sensorSettings;
  final Map<String, String> notes;

  ECUSettings({
    required this.engineSettings,
    required this.injectorSettings,
    required this.ignitionSettings,
    required this.sensorSettings,
    required this.notes,
  });

  factory ECUSettings.createDefault() {
    return ECUSettings(
      engineSettings: {
        'cylinderCount': 4,
        'displacement': 2000.0,
        'compressionRatio': 9.5,
        'strokeLength': 86.0,
        'borehttps': 86.0,
        'firingOrder': '1-3-4-2',
      },
      injectorSettings: {
        'injectorSize': 380.0,
        'injectorFlow': 38.0,
        'injectorAngle': 0.0,
        'injectorPressure': 3.0,
        'deadTime': 1.2,
      },
      ignitionSettings: {
        'coilChargingTime': 3.0,
        'sparkDuration': 1.0,
        'dwellTime': 3.0,
        'triggerAngle': 60.0,
      },
      sensorSettings: {
        'mapSensorMin': 0.0,
        'mapSensorMax': 255.0,
        'tpsSensorMin': 0.0,
        'tpsSensorMax': 255.0,
        'cltSensorBias': 2490.0,
        'iatSensorBias': 2490.0,
      },
      notes: {},
    );
  }

  factory ECUSettings.fromMap(Map<String, dynamic> map) {
    return ECUSettings(
      engineSettings: Map<String, dynamic>.from(map['engineSettings'] ?? {}),
      injectorSettings: Map<String, dynamic>.from(map['injectorSettings'] ?? {}),
      ignitionSettings: Map<String, dynamic>.from(map['ignitionSettings'] ?? {}),
      sensorSettings: Map<String, dynamic>.from(map['sensorSettings'] ?? {}),
      notes: Map<String, String>.from(map['notes'] ?? {}),
    );
  }

  Map<String, dynamic> toMap() {
    return {
      'engineSettings': engineSettings,
      'injectorSettings': injectorSettings,
      'ignitionSettings': ignitionSettings,
      'sensorSettings': sensorSettings,
      'notes': notes,
    };
  }

  /// Add or update a note for a specific setting
  void addNote(String settingPath, String note) {
    notes[settingPath] = note;
  }

  /// Get note for a specific setting
  String? getNote(String settingPath) {
    return notes[settingPath];
  }
}

/// Complete MSQ File Structure
class MSQFile {
  final MSQHeader header;
  final VETable veTable;
  final IgnitionTable ignitionTable;
  final ECUSettings settings;
  final DateTime lastModified;

  MSQFile({
    required this.header,
    required this.veTable,
    required this.ignitionTable,
    required this.settings,
    required this.lastModified,
  });

  factory MSQFile.createDefault() {
    return MSQFile(
      header: MSQHeader(
        fileVersion: '1.0',
        timestamp: DateTime.now().toIso8601String(),
        ecuType: 'MegaSquirt',
        firmwareVersion: '1.0',
        tunerStudioVersion: 'MegaTunix Redux 1.0',
        metadata: {
          'project': 'New Project',
          'vehicle': 'Unknown Vehicle',
          'engine': 'Unknown Engine',
        },
      ),
      veTable: VETable.createDefault(),
      ignitionTable: IgnitionTable.createDefault(),
      settings: ECUSettings.createDefault(),
      lastModified: DateTime.now(),
    );
  }

  factory MSQFile.fromJson(String jsonString) {
    final map = json.decode(jsonString) as Map<String, dynamic>;
    return MSQFile.fromMap(map);
  }

  factory MSQFile.fromMap(Map<String, dynamic> map) {
    return MSQFile(
      header: MSQHeader.fromMap(map['header'] ?? {}),
      veTable: VETable.fromMap(map['veTable'] ?? {}),
      ignitionTable: IgnitionTable.fromMap(map['ignitionTable'] ?? {}),
      settings: ECUSettings.fromMap(map['settings'] ?? {}),
      lastModified: DateTime.parse(map['lastModified'] ?? DateTime.now().toIso8601String()),
    );
  }

  Map<String, dynamic> toMap() {
    return {
      'header': header.toMap(),
      'veTable': veTable.toMap(),
      'ignitionTable': ignitionTable.toMap(),
      'settings': settings.toMap(),
      'lastModified': lastModified.toIso8601String(),
    };
  }

  String toJson() {
    return const JsonEncoder.withIndent('  ').convert(toMap());
  }

  /// Save MSQ file to disk
  Future<void> saveToFile(String filePath) async {
    final file = File(filePath);
    await file.writeAsString(toJson());
  }

  /// Load MSQ file from disk
  static Future<MSQFile> loadFromFile(String filePath) async {
    final file = File(filePath);
    final jsonString = await file.readAsString();
    return MSQFile.fromJson(jsonString);
  }

  /// Create a copy of this MSQ file with modifications
  MSQFile copyWith({
    MSQHeader? header,
    VETable? veTable,
    IgnitionTable? ignitionTable,
    ECUSettings? settings,
  }) {
    return MSQFile(
      header: header ?? this.header,
      veTable: veTable ?? this.veTable,
      ignitionTable: ignitionTable ?? this.ignitionTable,
      settings: settings ?? this.settings,
      lastModified: DateTime.now(),
    );
  }
}