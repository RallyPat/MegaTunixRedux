/// Quick validation test for Speeduino INI format compatibility
import 'package:flutter_test/flutter_test.dart';
import '../lib/services/ini_parser.dart';

void main() {
  group('Speeduino INI Compatibility Tests', () {
    test('Speeduino traditional table format parsing', () async {
      // Load Speeduino INI file
      final definition = await INIParser.parseFile('/home/pat/Documents/GitHubRepos/MegaTunixRedux/docs/TunerStudioINIFiles/202501.4.ini');
      
      print('📁 Speeduino INI loaded successfully');
      print('   Tables found: ${definition.tables.length}');
      print('   Settings found: ${definition.settings.length}');
      print('   RT Data fields: ${definition.rtData.length}');
      
      // Should find traditional VE table and other Speeduino tables
      expect(definition.tables.isNotEmpty, true, reason: 'Should find tables in Speeduino INI');
      
      // Look for classic speeduino tables
      final veTable = definition.tables.where((t) => t.name.contains('veTable')).toList();
      final afrTable = definition.tables.where((t) => t.name.contains('afrTable')).toList();
      
      print('   VE Tables: ${veTable.length}');
      print('   AFR Tables: ${afrTable.length}');
      
      if (veTable.isNotEmpty) {
        final vt = veTable.first;
        print('   First VE Table: "${vt.name}" (${vt.rows}x${vt.cols}) ${vt.units}');
      }
      
      print('✅ Speeduino compatibility validated');
    });
  });
}