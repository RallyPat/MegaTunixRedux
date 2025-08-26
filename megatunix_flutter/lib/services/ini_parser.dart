/// TunerStudio INI File Parser
/// Parses TunerStudio .ini files to provide ECU definitions, tables, settings, and protocols
/// This enables drop-in compatibility with all existing TunerStudio-supported ECUs

import 'dart:io';
import 'package:flutter/foundation.dart';

/// ECU Definition parsed from TunerStudio INI file
class ECUDefinition {
  final String name;
  final String version;
  final String signature;
  final Map<String, String> metadata;
  final List<TableDefinition> tables;
  final List<SettingDefinition> settings;
  final List<RealTimeDataField> rtData;
  final CommunicationProtocol protocol;
  final Map<String, String> constants;

  ECUDefinition({
    required this.name,
    required this.version,
    required this.signature,
    required this.metadata,
    required this.tables,
    required this.settings,
    required this.rtData,
    required this.protocol,
    required this.constants,
  });
}

/// Table definition from INI file
class TableDefinition {
  final String name;
  final String displayName;
  final String description;
  final int rows;
  final int cols;
  final String xAxis;
  final String yAxis;
  final String? xAxisType;  // Type of X-axis data (rpm, tps, etc.)
  final String? yAxisType;  // Type of Y-axis data (map, load, etc.)
  final String units;
  final double minValue;
  final double maxValue;
  final double defaultValue;
  final List<double>? xBins;
  final List<double>? yBins;
  final Map<String, String> attributes;

  TableDefinition({
    required this.name,
    required this.displayName,
    required this.description,
    required this.rows,
    required this.cols,
    required this.xAxis,
    required this.yAxis,
    this.xAxisType,
    this.yAxisType,
    required this.units,
    required this.minValue,
    required this.maxValue,
    required this.defaultValue,
    this.xBins,
    this.yBins,
    required this.attributes,
  });
}

/// Setting definition from INI file
class SettingDefinition {
  final String name;
  final String displayName;
  final String description;
  final String type; // bit, byte, word, float, etc.
  final int offset;
  final String units;
  final double minValue;
  final double maxValue;
  final double defaultValue;
  final Map<String, String> enumValues;
  final String? help;
  final Map<String, String> attributes;

  SettingDefinition({
    required this.name,
    required this.displayName,
    required this.description,
    required this.type,
    required this.offset,
    required this.units,
    required this.minValue,
    required this.maxValue,
    required this.defaultValue,
    required this.enumValues,
    this.help,
    required this.attributes,
  });
}

/// Real-time data field definition
class RealTimeDataField {
  final String name;
  final String displayName;
  final String units;
  final String type;
  final int offset;
  final double scale;
  final double translate;
  final int digits;
  final Map<String, String> attributes;

  RealTimeDataField({
    required this.name,
    required this.displayName,
    required this.units,
    required this.type,
    required this.offset,
    required this.scale,
    required this.translate,
    required this.digits,
    required this.attributes,
  });
}

/// Communication protocol definition
class CommunicationProtocol {
  final String type; // rs232, tcp, etc.
  final int baudRate;
  final int dataBits;
  final String parity;
  final int stopBits;
  final Map<String, String> commands;
  final int queryCommand;
  final int signature;
  final int rtDataSize;
  final Map<String, String> attributes;

  CommunicationProtocol({
    required this.type,
    required this.baudRate,
    required this.dataBits,
    required this.parity,
    required this.stopBits,
    required this.commands,
    required this.queryCommand,
    required this.signature,
    required this.rtDataSize,
    required this.attributes,
  });
}

/// TunerStudio INI File Parser
class INIParser {
  static const String _commentChar = ';';
  static const String _sectionStart = '[';
  static const String _sectionEnd = ']';

  /// Parse TunerStudio INI file
  static Future<ECUDefinition> parseFile(String filePath) async {
    final file = File(filePath);
    final content = await file.readAsString();
    return parseContent(content);
  }

  /// Parse INI content string
  static ECUDefinition parseContent(String content) {
    final lines = content.split('\n');
    final sections = <String, List<String>>{};
    String currentSection = '';

    // Parse sections
    for (var line in lines) {
      line = line.trim();
      
      // Skip empty lines and comments
      if (line.isEmpty || line.startsWith(_commentChar)) continue;
      
      // Check for section header
      if (line.startsWith(_sectionStart) && line.endsWith(_sectionEnd)) {
        currentSection = line.substring(1, line.length - 1).toLowerCase();
        sections[currentSection] = <String>[];
        continue;
      }
      
      // Add line to current section
      if (currentSection.isNotEmpty) {
        sections[currentSection]!.add(line);
      }
    }

    return ECUDefinition(
      name: _parseMetadata(sections['metadata'] ?? [])['name'] ?? 'Unknown ECU',
      version: _parseMetadata(sections['metadata'] ?? [])['version'] ?? '1.0',
      signature: _parseMetadata(sections['metadata'] ?? [])['signature'] ?? '',
      metadata: _parseMetadata(sections['metadata'] ?? []),
      tables: _parseTables(sections),
      settings: _parseSettings(sections['constants'] ?? []),
      rtData: _parseRealTimeData(sections['outputchannels'] ?? []),
      protocol: _parseProtocol(sections),
      constants: _parseConstants(sections['constants'] ?? []),
    );
  }

  /// Parse metadata section
  static Map<String, String> _parseMetadata(List<String> lines) {
    final metadata = <String, String>{};
    
    for (final line in lines) {
      final parts = line.split('=');
      if (parts.length >= 2) {
        final key = parts[0].trim();
        final value = parts.sublist(1).join('=').trim();
        metadata[key] = _cleanValue(value);
      }
    }
    
    return metadata;
  }

  /// Parse table definitions
  static List<TableDefinition> _parseTables(Map<String, List<String>> sections) {
    final tables = <TableDefinition>[];
    final allArrays = <String, Map<String, String>>{}; // Store all array definitions
    
    // Parse traditional VE tables (Speeduino format)
    if (sections.containsKey('table')) {
      for (final line in sections['table']!) {
        final table = _parseTableLine(line);
        if (table != null) tables.add(table);
      }
    }

    // Parse specific table sections
    for (final sectionName in sections.keys) {
      if (sectionName.endsWith('table') || sectionName.contains('map')) {
        final tableDefinition = _parseTableSection(sectionName, sections[sectionName]!);
        if (tableDefinition != null) tables.add(tableDefinition);
      }
    }

    // First pass: collect all array definitions from Constants section
    if (sections.containsKey('constants')) {
      for (final line in sections['constants']!) {
        final arrayInfo = _parseArrayInfo(line);
        if (arrayInfo != null) {
          allArrays[arrayInfo['name']!] = arrayInfo;
        }
      }
    }

    // Second pass: parse tables with proper axis detection
    if (sections.containsKey('constants')) {
      for (final arrayName in allArrays.keys) {
        final table = _parseArrayAsTableWithAxis(arrayName, allArrays);
        if (table != null) tables.add(table);
      }
    }

    return tables;
  }

  /// Parse single table line
  static TableDefinition? _parseTableLine(String line) {
    try {
      // Example: veTable = array, U08, 0, [16x16], "VE", "%", 0, 255, 100
      final parts = line.split(',');
      if (parts.length < 8) return null;

      final name = parts[0].split('=')[0].trim();
      final sizeMatch = RegExp(r'\[(\d+)x(\d+)\]').firstMatch(line);
      if (sizeMatch == null) return null;

      final rows = int.parse(sizeMatch.group(1)!);
      final cols = int.parse(sizeMatch.group(2)!);

      return TableDefinition(
        name: name,
        displayName: _cleanValue(parts[4]),
        description: _cleanValue(parts[4]),
        rows: rows,
        cols: cols,
        xAxis: 'rpm', // Default, should be parsed from INI
        yAxis: 'map', // Default, should be parsed from INI
        xAxisType: _inferAxisType(name, 'x'), // Infer axis type from table name
        yAxisType: _inferAxisType(name, 'y'), // Infer axis type from table name
        units: _cleanValue(parts[5]),
        minValue: double.tryParse(_cleanValue(parts[6])) ?? 0.0,
        maxValue: double.tryParse(_cleanValue(parts[7])) ?? 255.0,
        defaultValue: parts.length > 8 ? (double.tryParse(_cleanValue(parts[8])) ?? 100.0) : 100.0,
        attributes: {},
      );
    } catch (e) {
      debugPrint('Error parsing table line: $line - $e');
      return null;
    }
  }

  /// Parse array information from a line
  static Map<String, String>? _parseArrayInfo(String line) {
    try {
      if (!line.contains('array') || !line.contains('[') || !line.contains(']')) {
        return null;
      }

      final parts = line.split(',');
      if (parts.length < 5) return null;

      final nameValue = parts[0].split('=');
      if (nameValue.length != 2) return null;

      final name = nameValue[0].trim();
      final sizeMatch = RegExp(r'\[(\d+)(?:x(\d+))?\]').firstMatch(line);
      if (sizeMatch == null) return null;

      final dimensions = sizeMatch.group(0)!;
      final rows = int.parse(sizeMatch.group(1)!);
      final cols = sizeMatch.group(2) != null ? int.parse(sizeMatch.group(2)!) : 1;
      final units = parts.length > 4 ? _cleanValue(parts[4]) : '';

      return {
        'name': name,
        'dimensions': dimensions,
        'rows': rows.toString(),
        'cols': cols.toString(),
        'units': units,
        'line': line,
      };
    } catch (e) {
      return null;
    }
  }

  /// Parse array as table with proper axis detection
  static TableDefinition? _parseArrayAsTableWithAxis(String tableName, Map<String, Map<String, String>> allArrays) {
    final tableInfo = allArrays[tableName];
    if (tableInfo == null) return null;

    final cols = int.parse(tableInfo['cols']!);
    final rows = int.parse(tableInfo['rows']!);
    
    // Only consider 2D arrays as tables, or significant 1D arrays
    if (cols == 1 && rows < 6) return null;
    if (cols < 2 || rows < 2) return null;

    // Check if this looks like a table (not axis bins)
    final lowerName = tableName.toLowerCase();
    final isTableName = lowerName.contains('table') || 
                       lowerName.contains('map') || 
                       lowerName.endsWith('ve') ||
                       lowerName.contains('afr') ||
                       lowerName.contains('timing') ||
                       lowerName.contains('ignition') ||
                       lowerName.contains('fuel') ||
                       lowerName.contains('boost') ||
                       lowerName.contains('correction') ||
                       lowerName.contains('corr');
    
    // Skip if this looks like axis bins
    if (lowerName.contains('bins') || lowerName.contains('rpm') || lowerName.contains('load')) {
      return null;
    }
    
    if (!isTableName) return null;

    // Find axis bin definitions
    String xAxisName = 'rpm'; // default
    String xAxisUnits = 'RPM'; // default
    String yAxisName = 'load'; // default  
    String yAxisUnits = 'Load'; // default

    // Look for associated axis bins with more flexible patterns
    final baseTableName = tableName.replaceAll('Table', '').replaceAll('table', '');
    
    final possibleXBins = [
      // Direct patterns
      '${tableName}RpmBins',
      '${tableName}rpmBins', 
      '${baseTableName}RpmBins',
      '${baseTableName}rpmBins',
      // Pattern-based (injector_battLagCorr -> injector_battLagCorrBattBins)
      '${baseTableName}BattBins', 
      '${baseTableName}battBins',
      '${baseTableName}TpsBins',
      '${baseTableName}tpsBins',
      // Generic
      '${tableName}XBins',
      '${baseTableName}XBins',
    ];
    
    final possibleYBins = [
      // Direct patterns  
      '${tableName}LoadBins',
      '${tableName}loadBins',
      '${baseTableName}LoadBins',
      '${baseTableName}loadBins',
      // Pattern-based (injector_battLagCorr -> injector_battLagCorrPressBins)
      '${baseTableName}PressBins',
      '${baseTableName}pressBins', 
      '${baseTableName}MapBins',
      '${baseTableName}mapBins',
      // Generic
      '${tableName}YBins',
      '${baseTableName}YBins',
      '${tableName}Bins', // Generic bins
      '${baseTableName}Bins',
    ];

    // Check for X-axis bins (usually RPM, but could be other)
    for (final binName in possibleXBins) {
      if (allArrays.containsKey(binName)) {
        final binInfo = allArrays[binName]!;
        xAxisUnits = binInfo['units']!.isEmpty ? 'RPM' : binInfo['units']!;
        xAxisName = _determineAxisTypeFromUnits(xAxisUnits);
        break;
      }
    }

    // Check for Y-axis bins (usually Load/MAP, but could be other)  
    for (final binName in possibleYBins) {
      if (allArrays.containsKey(binName)) {
        final binInfo = allArrays[binName]!;
        yAxisUnits = binInfo['units']!.isEmpty ? 'Load' : binInfo['units']!;
        yAxisName = _determineAxisTypeFromUnits(yAxisUnits);
        break;
      }
    }

    // Parse the actual table line to get other properties
    final tableLine = tableInfo['line']!;
    return _parseArrayAsTable(tableLine, xAxisName, yAxisName);
  }
  static TableDefinition? _parseArrayAsTable(String line, [String? detectedXAxis, String? detectedYAxis]) {
    try {
      // Check if this is a multi-dimensional array that could be a table
      if (!line.contains('[') || !line.contains('x') || !line.contains(']')) {
        return null;
      }

      // Examples:
      // tmfTable = array, U16, 3640, [16x16], "", 0.001, 0, 0, 65, 3
      // torqueTable = array, U08, 4184, [6x6], "Nm", 10.0, 0, 0, 255, 0
      // vvtTable1 = array, S08, 12484, [8x8], "value", 1, 0, -125, 125, 0
      
      final parts = line.split(',');
      if (parts.length < 8) return null;

      final nameValue = parts[0].split('=');
      if (nameValue.length != 2) return null;

      final name = nameValue[0].trim();
      final arrayType = nameValue[1].trim();
      
      // Only parse if it's an array type
      if (arrayType != 'array') return null;

      // Check if the name suggests it's a table (common table naming patterns)
      final lowerName = name.toLowerCase();
      final isTableName = lowerName.contains('table') || 
                         lowerName.contains('map') || 
                         lowerName.endsWith('ve') ||
                         lowerName.contains('afr') ||
                         lowerName.contains('timing') ||
                         lowerName.contains('ignition') ||
                         lowerName.contains('fuel') ||
                         lowerName.contains('boost') ||
                         lowerName.contains('correction') ||
                         lowerName.contains('corr');
      
      if (!isTableName) return null;

      // Parse dimensions [NxM] or [N]
      final sizeMatch = RegExp(r'\[(\d+)(?:x(\d+))?\]').firstMatch(line);
      if (sizeMatch == null) return null;

      final rows = int.parse(sizeMatch.group(1)!);
      final cols = sizeMatch.group(2) != null ? int.parse(sizeMatch.group(2)!) : 1;
      
      // Only consider it a table if it has 2D dimensions or is a reasonable 1D lookup
      if (cols == 1 && rows < 6) return null; // Skip small 1D arrays that aren't tables

      final type = parts[1].trim();
      final offset = int.tryParse(parts[2].trim()) ?? 0;
      final units = parts.length > 4 ? _cleanValue(parts[4]) : '';
      final scale = parts.length > 5 ? (double.tryParse(_cleanValue(parts[5])) ?? 1.0) : 1.0;
      final translate = parts.length > 6 ? (double.tryParse(_cleanValue(parts[6])) ?? 0.0) : 0.0;
      final minValue = parts.length > 7 ? (double.tryParse(_cleanValue(parts[7])) ?? 0.0) : 0.0;
      final maxValue = parts.length > 8 ? (double.tryParse(_cleanValue(parts[8])) ?? 255.0) : 255.0;
      final defaultValue = (minValue + maxValue) / 2; // Reasonable default

      // Generate clean display name from variable name
      String displayName = generateDisplayName(name);

      return TableDefinition(
        name: name,
        displayName: displayName,
        description: displayName,
        rows: rows,
        cols: cols,
        xAxis: detectedXAxis ?? _inferXAxisFromTableName(name),
        yAxis: detectedYAxis ?? _inferYAxisFromTableName(name),
        xAxisType: _inferAxisType(name, 'x'),
        yAxisType: _inferAxisType(name, 'y'),
        units: units,
        minValue: minValue,
        maxValue: maxValue,
        defaultValue: defaultValue,
        attributes: {
          'type': type,
          'offset': offset.toString(),
          'scale': scale.toString(),
          'translate': translate.toString(),
          'format': 'rusEFI'
        },
      );
    } catch (e) {
      debugPrint('Error parsing array as table: $line - $e');
      return null;
    }
  }

  /// Infer X-axis type from table name
  static String _inferXAxisFromTableName(String tableName) {
    final lowerName = tableName.toLowerCase();
    if (lowerName.contains('rpm') || lowerName.contains('timing') || lowerName.contains('ignition')) {
      return 'rpm';
    } else if (lowerName.contains('tps') || lowerName.contains('pedal')) {
      return 'tps';
    } else if (lowerName.contains('clt') || lowerName.contains('temp')) {
      return 'temperature';
    } else if (lowerName.contains('iat')) {
      return 'iat';
    } else {
      return 'rpm'; // Default fallback
    }
  }

  /// Determine axis type from units string
  static String _determineAxisTypeFromUnits(String units) {
    final lowerUnits = units.toLowerCase();
    
    if (lowerUnits.contains('rpm')) return 'rpm';
    if (lowerUnits.contains('volt')) return 'battery';
    if (lowerUnits.contains('kpa') || lowerUnits.contains('pressure')) return 'pressure';
    if (lowerUnits.contains('tps') || lowerUnits.contains('% tps')) return 'tps';
    if (lowerUnits.contains('load') || lowerUnits == 'l') return 'load';
    if (lowerUnits.contains('temp') || lowerUnits.contains('deg')) return 'temperature';
    if (lowerUnits.contains('gear')) return 'gear';
    
    // Fallback based on common patterns
    if (lowerUnits.isEmpty) return 'unknown';
    return lowerUnits;
  }

  /// Generate clean display name from variable name
  static String generateDisplayName(String name) {
    // Simple approach: replace underscores and capitalize
    return name
        .replaceAll('_', ' ')
        .split(' ')
        .map((word) => word.isEmpty ? word : word[0].toUpperCase() + word.substring(1))
        .join(' ');
  }

  /// Infer Y-axis type from table name  
  static String _inferYAxisFromTableName(String tableName) {
    final lowerName = tableName.toLowerCase();
    if (lowerName.contains('map') || lowerName.contains('boost') || lowerName.contains('pressure')) {
      return 'map';
    } else if (lowerName.contains('tps')) {
      return 'tps';
    } else if (lowerName.contains('load')) {
      return 'load';
    } else if (lowerName.contains('temp') || lowerName.contains('clt') || lowerName.contains('iat')) {
      return 'temperature';
    } else {
      return 'map'; // Default fallback
    }
  }

  /// Parse table section
  static TableDefinition? _parseTableSection(String sectionName, List<String> lines) {
    final attributes = <String, String>{};
    
    for (final line in lines) {
      final parts = line.split('=');
      if (parts.length >= 2) {
        attributes[parts[0].trim()] = _cleanValue(parts.sublist(1).join('='));
      }
    }

    // Extract basic info from attributes
    final name = sectionName;
    final displayName = attributes['displayName'] ?? name;
    final rows = int.tryParse(attributes['rows'] ?? '16') ?? 16;
    final cols = int.tryParse(attributes['cols'] ?? '16') ?? 16;

    return TableDefinition(
      name: name,
      displayName: displayName,
      description: attributes['description'] ?? displayName,
      rows: rows,
      cols: cols,
      xAxis: attributes['xAxis'] ?? 'rpm',
      yAxis: attributes['yAxis'] ?? 'map',
      xAxisType: attributes['xAxisType'] ?? _inferAxisType(name, 'x'),
      yAxisType: attributes['yAxisType'] ?? _inferAxisType(name, 'y'),
      units: attributes['units'] ?? '%',
      minValue: double.tryParse(attributes['minValue'] ?? '0') ?? 0.0,
      maxValue: double.tryParse(attributes['maxValue'] ?? '255') ?? 255.0,
      defaultValue: double.tryParse(attributes['defaultValue'] ?? '100') ?? 100.0,
      attributes: attributes,
    );
  }

  /// Parse settings/constants
  static List<SettingDefinition> _parseSettings(List<String> lines) {
    final settings = <SettingDefinition>[];
    
    for (final line in lines) {
      final setting = _parseSettingLine(line);
      if (setting != null) settings.add(setting);
    }
    
    return settings;
  }

  /// Parse single setting line
  static SettingDefinition? _parseSettingLine(String line) {
    try {
      // Example: nCylinders = scalar, U08, 0, "Cylinders", "", 1, 12, 4
      final parts = line.split(',');
      if (parts.length < 6) return null;

      final nameValue = parts[0].split('=');
      if (nameValue.length != 2) return null;

      final name = nameValue[0].trim();
      final type = parts[1].trim();
      final offset = int.tryParse(parts[2].trim()) ?? 0;

      return SettingDefinition(
        name: name,
        displayName: _cleanValue(parts[3]),
        description: _cleanValue(parts[4]),
        type: type,
        offset: offset,
        units: parts.length > 4 ? _cleanValue(parts[4]) : '',
        minValue: parts.length > 5 ? (double.tryParse(_cleanValue(parts[5])) ?? 0.0) : 0.0,
        maxValue: parts.length > 6 ? (double.tryParse(_cleanValue(parts[6])) ?? 255.0) : 255.0,
        defaultValue: parts.length > 7 ? (double.tryParse(_cleanValue(parts[7])) ?? 0.0) : 0.0,
        enumValues: {},
        attributes: {},
      );
    } catch (e) {
      debugPrint('Error parsing setting line: $line - $e');
      return null;
    }
  }

  /// Parse real-time data fields
  static List<RealTimeDataField> _parseRealTimeData(List<String> lines) {
    final fields = <RealTimeDataField>[];
    
    for (final line in lines) {
      final field = _parseRTDataLine(line);
      if (field != null) fields.add(field);
    }
    
    return fields;
  }

  /// Parse single real-time data line
  static RealTimeDataField? _parseRTDataLine(String line) {
    try {
      // Example: rpm = scalar, U16, 0, "RPM", "rpm", 0, 8000, 0, 0, 0
      final parts = line.split(',');
      if (parts.length < 6) return null;

      final nameValue = parts[0].split('=');
      if (nameValue.length != 2) return null;

      final name = nameValue[0].trim();
      final type = parts[1].trim();
      final offset = int.tryParse(parts[2].trim()) ?? 0;

      return RealTimeDataField(
        name: name,
        displayName: _cleanValue(parts[3]),
        units: _cleanValue(parts[4]),
        type: type,
        offset: offset,
        scale: parts.length > 7 ? (double.tryParse(_cleanValue(parts[7])) ?? 1.0) : 1.0,
        translate: parts.length > 8 ? (double.tryParse(_cleanValue(parts[8])) ?? 0.0) : 0.0,
        digits: parts.length > 9 ? (int.tryParse(_cleanValue(parts[9])) ?? 0) : 0,
        attributes: {},
      );
    } catch (e) {
      debugPrint('Error parsing RT data line: $line - $e');
      return null;
    }
  }

  /// Parse communication protocol
  static CommunicationProtocol _parseProtocol(Map<String, List<String>> sections) {
    final rs232Section = sections['rs232'] ?? [];
    final protocolData = <String, String>{};
    
    for (final line in rs232Section) {
      final parts = line.split('=');
      if (parts.length >= 2) {
        protocolData[parts[0].trim()] = _cleanValue(parts.sublist(1).join('='));
      }
    }

    return CommunicationProtocol(
      type: 'rs232',
      baudRate: int.tryParse(protocolData['baudrate'] ?? '115200') ?? 115200,
      dataBits: int.tryParse(protocolData['databits'] ?? '8') ?? 8,
      parity: protocolData['parity'] ?? 'N',
      stopBits: int.tryParse(protocolData['stopbits'] ?? '1') ?? 1,
      commands: protocolData,
      queryCommand: int.tryParse(protocolData['querycommand'] ?? '81') ?? 81,
      signature: int.tryParse(protocolData['signature'] ?? '0') ?? 0,
      rtDataSize: int.tryParse(protocolData['rtdatasize'] ?? '64') ?? 64,
      attributes: protocolData,
    );
  }

  /// Parse constants section
  static Map<String, String> _parseConstants(List<String> lines) {
    final constants = <String, String>{};
    
    for (final line in lines) {
      final parts = line.split('=');
      if (parts.length >= 2) {
        constants[parts[0].trim()] = _cleanValue(parts.sublist(1).join('='));
      }
    }
    
    return constants;
  }

  /// Clean quoted values and remove whitespace
  static String _cleanValue(String value) {
    value = value.trim();
    if (value.startsWith('"') && value.endsWith('"')) {
      return value.substring(1, value.length - 1);
    }
    return value;
  }

  /// Infer axis type from table name and position
  static String _inferAxisType(String tableName, String axis) {
    final name = tableName.toLowerCase();
    
    if (axis == 'x') {
      // X-axis is typically RPM for most automotive tables
      if (name.contains('boost') || name.contains('turbo')) {
        return 'tps'; // Boost tables often use TPS on X-axis
      } else if (name.contains('coolant') || name.contains('temp')) {
        return 'coolant'; // Temperature compensation tables
      } else {
        return 'rpm'; // Default X-axis
      }
    } else {
      // Y-axis varies more by table type
      if (name.contains('boost') || name.contains('turbo')) {
        return 'rpm'; // Boost tables use RPM on Y-axis
      } else if (name.contains('coolant') || name.contains('temp')) {
        return 'intake'; // Temperature tables
      } else if (name.contains('tps') || name.contains('throttle')) {
        return 'tps'; // TPS-based tables
      } else {
        return 'map'; // Default Y-axis (load/MAP)
      }
    }
  }
}

/// INI-based ECU Manager
class INIECUManager extends ChangeNotifier {
  final Map<String, ECUDefinition> _ecuDefinitions = {};
  ECUDefinition? _currentECU;

  /// Currently loaded ECU definition
  ECUDefinition? get currentECU => _currentECU;

  /// Available ECU definitions
  Map<String, ECUDefinition> get ecuDefinitions => Map.unmodifiable(_ecuDefinitions);

  /// Load ECU definition from INI file
  Future<bool> loadECUDefinition(String iniFilePath) async {
    try {
      final definition = await INIParser.parseFile(iniFilePath);
      _ecuDefinitions[definition.name] = definition;
      notifyListeners();
      return true;
    } catch (e) {
      debugPrint('Error loading ECU definition: $e');
      return false;
    }
  }

  /// Set current ECU
  void setCurrentECU(String ecuName) {
    if (_ecuDefinitions.containsKey(ecuName)) {
      _currentECU = _ecuDefinitions[ecuName];
      notifyListeners();
    }
  }

  /// Get table definition by name
  TableDefinition? getTableDefinition(String tableName) {
    return _currentECU?.tables.firstWhere(
      (table) => table.name == tableName,
      orElse: () => throw StateError('Table not found: $tableName'),
    );
  }

  /// Get setting definition by name
  SettingDefinition? getSettingDefinition(String settingName) {
    return _currentECU?.settings.firstWhere(
      (setting) => setting.name == settingName,
      orElse: () => throw StateError('Setting not found: $settingName'),
    );
  }

  /// Get real-time data field definition
  RealTimeDataField? getRTDataField(String fieldName) {
    return _currentECU?.rtData.firstWhere(
      (field) => field.name == fieldName,
      orElse: () => throw StateError('RT data field not found: $fieldName'),
    );
  }
}