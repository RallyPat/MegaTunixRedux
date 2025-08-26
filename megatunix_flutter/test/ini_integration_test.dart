import 'package:flutter_test/flutter_test.dart';
import 'package:megatunix_flutter/services/ini_parser.dart';
import 'dart:io';

void main() {
  group('INI Parser Integration Tests', () {
    test('Parse real TunerStudio INI file', () async {
      // Check if sample INI file exists
      final sampleIniFile = File('../docs/TunerStudioINIFiles/202501.4.ini');
      
      if (!sampleIniFile.existsSync()) {
        print('Sample INI file not found, skipping integration test');
        return;
      }

      final iniContent = await sampleIniFile.readAsString();
      
      // Test basic parsing doesn't crash
      expect(() => INIParser.parseContent(iniContent), returnsNormally);
      
      final ecuDefinition = INIParser.parseContent(iniContent);
      
      // Verify basic info is parsed
      expect(ecuDefinition.name, isNotNull);
      expect(ecuDefinition.version, isNotNull);
      expect(ecuDefinition.signature, isNotNull);
      
      print('✓ Successfully parsed ECU: ${ecuDefinition.name} v${ecuDefinition.version}');
      print('✓ Found ${ecuDefinition.tables.length} tables');
      print('✓ Found ${ecuDefinition.settings.length} settings');
    });
    
    test('INI parser handles simple sections', () {
      const sampleIni = '''
[MegaTune]
MTversion = 2.25
signature = "speeduino 202501"

[TunerStudio]
iniSpecVersion = 3.64
''';

      final ecuDefinition = INIParser.parseContent(sampleIni);
      
      // Check basic parsing
      expect(ecuDefinition.name, isNotNull);
      expect(ecuDefinition.signature, isNotNull);
      
      print('✓ Successfully parsed simple INI structure');
    });
  });
}