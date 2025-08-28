// Test compilation of key services
import '../lib/services/global_shortcuts_service.dart';
import '../lib/services/data_logging_service.dart';
import '../lib/services/help_service.dart';
import '../lib/services/tuning_view_service.dart';
import '../lib/services/gauge_designer_service.dart';

void main() {
  print('Testing service imports...');
  
  // Test basic instantiation
  final globalShortcuts = GlobalShortcutsService();
  final dataLogging = DataLoggingService();
  final helpService = HelpService();
  final tuningView = TuningViewService();
  final gaugeDesigner = GaugeDesignerService();
  
  print('All services instantiated successfully!');
}