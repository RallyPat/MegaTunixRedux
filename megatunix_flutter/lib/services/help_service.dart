/// Help Service
/// Handles F1 context-sensitive help functionality
/// Provides comprehensive help system for all application features

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

/// Service for managing context-sensitive help system
class HelpService extends ChangeNotifier {
  static final HelpService _instance = HelpService._internal();
  factory HelpService() => _instance;
  HelpService._internal();

  // Help content registry
  final Map<String, HelpContent> _helpRegistry = {};
  
  // Current help context
  String? _currentContext;
  
  /// Initialize help system with all content
  void initialize() {
    _registerAllHelpContent();
  }
  
  /// Show context-sensitive help (F1)
  void showContextHelp(BuildContext context, String? route) {
    _currentContext = route ?? _detectCurrentContext(context);
    final helpContent = _getHelpForContext(_currentContext!);
    
    showDialog(
      context: context,
      builder: (context) => HelpDialog(
        content: helpContent,
        onClose: () => Navigator.of(context).pop(),
      ),
    );
  }
  
  /// Show specific help topic
  void showHelpTopic(BuildContext context, String topic) {
    final helpContent = _helpRegistry[topic] ?? _getDefaultHelp();
    
    showDialog(
      context: context,
      builder: (context) => HelpDialog(
        content: helpContent,
        onClose: () => Navigator.of(context).pop(),
      ),
    );
  }
  
  /// Register all help content
  void _registerAllHelpContent() {
    // Dashboard help
    _helpRegistry['dashboard'] = HelpContent(
      title: 'Dashboard',
      sections: [
        HelpSection(
          title: 'Real-time Gauges',
          content: 'View live ECU data including RPM, MAP, TPS, temperatures, and more. '
                  'Gauges update at 10Hz for smooth real-time monitoring.',
          shortcuts: [
            'F5 - Start data logging',
            'F6 - Stop data logging', 
            'F7 - Add log marker',
          ],
        ),
        HelpSection(
          title: 'Connection Status',
          content: 'Monitor ECU connection status in the top-right corner. '
                  'Green indicates connected, red indicates disconnected.',
        ),
        HelpSection(
          title: 'Gauge Customization',
          content: 'Right-click on gauges to customize appearance and ranges. '
                  'Use Alt+Arrow keys to adjust gauge positions in designer mode.',
          shortcuts: [
            'Alt+↑/↓/←/→ - Adjust gauge position',
            'Ctrl+D - Duplicate gauge',
          ],
        ),
      ],
    );
    
    // Tables help
    _helpRegistry['tables'] = HelpContent(
      title: 'Table Editor',
      sections: [
        HelpSection(
          title: 'Navigation',
          content: 'Use arrow keys to navigate between cells. Professional Excel-style '
                  'selection with Shift+Arrow for ranges and Ctrl+Click for multi-select.',
          shortcuts: [
            '↑/↓/←/→ - Navigate cells',
            'Shift+Arrow - Select range',
            'Ctrl+Click - Multi-select',
            'Ctrl+A - Select all',
          ],
        ),
        HelpSection(
          title: 'Editing',
          content: 'Double-click or press F2/Enter to edit cells. Type numbers directly '
                  'for quick value entry. Press Escape to cancel editing.',
          shortcuts: [
            'F2 / Enter - Edit cell',
            'Escape - Cancel edit',
            'Delete - Clear cells',
            'Ctrl+Z - Undo',
            'Ctrl+Y - Redo',
          ],
        ),
        HelpSection(
          title: 'Table Operations',
          content: 'Advanced interpolation and smoothing operations using TunerStudio-compatible shortcuts.',
          shortcuts: [
            'Ctrl+I - 2D Interpolation',
            'Ctrl+H - Horizontal Interpolation',
            'Ctrl+V - Vertical Interpolation',
            'Ctrl+S - Smooth selection',
            'Ctrl+C - Copy',
            'Ctrl+Shift+V - Paste',
          ],
        ),
        HelpSection(
          title: 'Real-time Cursor',
          content: 'Pink cursor shows current engine operating point in real-time. '
                  'Trail shows 3-second history with anti-jitter filtering.',
        ),
      ],
    );
    
    // Tools help
    _helpRegistry['tools'] = HelpContent(
      title: 'Tools',
      sections: [
        HelpSection(
          title: 'Table Tuning',
          content: 'Advanced table tuning operations and wizards.',
          shortcuts: [
            'Ctrl+T - Open Table Tune dialog',
            'F10 - Tuning & Dyno View Editor',
          ],
        ),
        HelpSection(
          title: 'Data Logging',
          content: 'Record ECU data for analysis and tuning.',
          shortcuts: [
            'F5 - Start logging',
            'F6 - Stop logging',
            'F7 - Add marker',
          ],
        ),
      ],
    );
    
    // Settings help
    _helpRegistry['settings'] = HelpContent(
      title: 'Settings',
      sections: [
        HelpSection(
          title: 'ECU Connection',
          content: 'Configure serial port settings for ECU communication. '
                  'Supports Speeduino, MegaSquirt, and other protocols.',
        ),
        HelpSection(
          title: 'Display Options',
          content: 'Customize themes, gauge layouts, and display preferences.',
        ),
      ],
    );
    
    // Global shortcuts help
    _helpRegistry['shortcuts'] = HelpContent(
      title: 'Keyboard Shortcuts',
      sections: [
        HelpSection(
          title: 'Global Application Shortcuts',
          shortcuts: [
            'F1 - Context-sensitive help',
            'Shift+F1 - Global shortcuts reference',
            'F2 - Rename current item',
            'F5 - Start data logging with CSV export',
            'F6 - Stop data logging and save file',
            'F7 - Add marker to current log',
            'F10 - Advanced tuning & dyno analysis view',
            'Tab - Cycle main application tabs',
            'Ctrl+S - Global save (tune, logs, state)',
            'Ctrl+O - Open tune file dialog',
            'Ctrl+N - New tune/configuration wizard',
            'Ctrl+T - Table tune dialog',
            'Ctrl+Z - Undo (50-action stack)',
            'Ctrl+Y - Redo last undone action',
          ],
        ),
        HelpSection(
          title: 'Table Editor Shortcuts',
          shortcuts: [
            '↑/↓/←/→ - Navigate cells',
            'Shift+Arrow - Excel-style range selection',
            'Ctrl+Arrow - Multi-select individual cells',
            'Ctrl+Click - Add cells to selection',
            'F2/Enter - Edit selected cell',
            'Escape - Cancel edit/clear selection',
            'Delete - Clear/zero selected cells',
            'Ctrl+A - Select entire table',
            'Ctrl+C - Copy selection (tab-separated)',
            'Ctrl+Shift+V - Paste data (TunerStudio compatible)',
            'Ctrl+I - 2D Bilinear Interpolation',
            'Ctrl+H - Horizontal-only Interpolation',
            'Ctrl+V - Vertical-only Interpolation',
            'Ctrl+S - Smooth selection (8-neighbor filter)',
            'Ctrl+Z - Undo table operations',
            'Ctrl+Y - Redo table operations',
          ],
        ),
        HelpSection(
          title: 'Gauge Designer Shortcuts',
          shortcuts: [
            'Alt+↑/↓/←/→ - Adjust gauge position (1px precision)',
            'Alt+Shift+Arrow - Resize selected gauge',
            'Ctrl+D - Duplicate selected gauge',
            'Right-click - Access gauge properties',
            'Mouse drag - Move gauge freely',
          ],
        ),
        HelpSection(
          title: 'Data Logging Shortcuts', 
          shortcuts: [
            'F5 - Start logging with timestamp',
            'F6 - Stop logging and save CSV file',
            'F7 - Add marker with current timestamp',
            'Ctrl+L - View log history',
          ],
        ),
      ],
    );
    
    // Protocol-specific help
    _helpRegistry['speeduino'] = HelpContent(
      title: 'Speeduino Protocol',
      sections: [
        HelpSection(
          title: 'Connection Setup',
          content: 'Speeduino ECUs typically use 115200 baud rate over USB serial. '
                  'Auto-detection will scan common ports like /dev/ttyACM0 on Linux.',
        ),
        HelpSection(
          title: 'Real-time Data',
          content: 'Supports full real-time data streaming at 10Hz including RPM, MAP, '
                  'TPS, temperatures, timing, and all sensor readings.',
        ),
      ],
    );
  }
  
  /// Detect current context from widget tree
  String _detectCurrentContext(BuildContext context) {
    // Try to determine context from current widget/route
    final widget = context.widget;
    final widgetType = widget.runtimeType.toString();
    
    if (widgetType.contains('Dashboard')) return 'dashboard';
    if (widgetType.contains('Table')) return 'tables';
    if (widgetType.contains('Tools')) return 'tools';
    if (widgetType.contains('Settings')) return 'settings';
    
    // Check route name
    final route = ModalRoute.of(context)?.settings.name;
    if (route != null) {
      if (route.contains('dashboard')) return 'dashboard';
      if (route.contains('tables')) return 'tables';
      if (route.contains('tools')) return 'tools';
      if (route.contains('settings')) return 'settings';
    }
    
    return 'general';
  }
  
  /// Get help content for specific context
  HelpContent _getHelpForContext(String context) {
    return _helpRegistry[context] ?? _getDefaultHelp();
  }
  
  /// Get default/general help content
  HelpContent _getDefaultHelp() {
    return HelpContent(
      title: 'MegaTunix Redux Help',
      sections: [
        HelpSection(
          title: 'Welcome to MegaTunix Redux',
          content: 'Professional ECU tuning software with modern Flutter interface. '
                  'Features real-time data monitoring, professional table editing, '
                  'and comprehensive data logging.',
        ),
        HelpSection(
          title: 'Getting Started',
          content: '1. Connect your ECU via USB/serial port\n'
                  '2. Configure connection settings\n'
                  '3. View real-time data on the Dashboard\n'
                  '4. Edit tables with professional tools\n'
                  '5. Log data for analysis and tuning',
        ),
        HelpSection(
          title: 'Key Features',
          content: '• Real-time data streaming at 10Hz\n'
                  '• Professional table editor with TunerStudio parity\n'
                  '• Advanced interpolation and smoothing\n'
                  '• Comprehensive data logging\n'
                  '• Cross-platform compatibility',
        ),
        HelpSection(
          title: 'Quick Shortcuts Reference',
          shortcuts: [
            'F1 - Context help (this dialog)',
            'F5 - Start data logging with CSV export',
            'F6 - Stop data logging and save',
            'F7 - Add timestamped log marker', 
            'F10 - Advanced tuning & dyno view',
            'Ctrl+T - Table tuning dialog',
            'Ctrl+Z/Y - Professional undo/redo system',
          ],
        ),
      ],
    );
  }
  
  /// Get all available help topics
  List<String> getAvailableTopics() {
    return _helpRegistry.keys.toList()..sort();
  }
}

/// Help content data structure
class HelpContent {
  final String title;
  final List<HelpSection> sections;
  
  HelpContent({
    required this.title,
    required this.sections,
  });
}

/// Help section within help content
class HelpSection {
  final String title;
  final String? content;
  final List<String>? shortcuts;
  
  HelpSection({
    required this.title,
    this.content,
    this.shortcuts,
  });
}

/// Help dialog widget
class HelpDialog extends StatefulWidget {
  final HelpContent content;
  final VoidCallback onClose;
  
  const HelpDialog({
    super.key,
    required this.content,
    required this.onClose,
  });
  
  @override
  State<HelpDialog> createState() => _HelpDialogState();
}

class _HelpDialogState extends State<HelpDialog> {
  final ScrollController _scrollController = ScrollController();
  
  @override
  void dispose() {
    _scrollController.dispose();
    super.dispose();
  }
  
  @override
  Widget build(BuildContext context) {
    return Dialog(
      child: Container(
        width: 800,
        height: 600,
        padding: const EdgeInsets.all(24),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Header
            Row(
              children: [
                Icon(
                  Icons.help_outline,
                  size: 28,
                  color: Theme.of(context).colorScheme.primary,
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: Text(
                    widget.content.title,
                    style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ),
                IconButton(
                  onPressed: widget.onClose,
                  icon: const Icon(Icons.close),
                  tooltip: 'Close (Esc)',
                ),
              ],
            ),
            const Divider(),
            
            // Content
            Expanded(
              child: Scrollbar(
                controller: _scrollController,
                child: SingleChildScrollView(
                  controller: _scrollController,
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: widget.content.sections.map((section) => 
                      _buildSection(section)).toList(),
                  ),
                ),
              ),
            ),
            
            // Footer
            const Divider(),
            Row(
              children: [
                const Spacer(),
                TextButton(
                  onPressed: widget.onClose,
                  child: const Text('Close'),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
  
  Widget _buildSection(HelpSection section) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            section.title,
            style: Theme.of(context).textTheme.titleMedium?.copyWith(
              fontWeight: FontWeight.bold,
              color: Theme.of(context).colorScheme.primary,
            ),
          ),
          const SizedBox(height: 8),
          
          if (section.content != null) ...[
            Text(
              section.content!,
              style: Theme.of(context).textTheme.bodyMedium,
            ),
            const SizedBox(height: 12),
          ],
          
          if (section.shortcuts != null && section.shortcuts!.isNotEmpty) ...[
            Text(
              'Keyboard Shortcuts:',
              style: Theme.of(context).textTheme.titleSmall?.copyWith(
                fontWeight: FontWeight.w600,
              ),
            ),
            const SizedBox(height: 8),
            ...section.shortcuts!.map((shortcut) => Padding(
              padding: const EdgeInsets.only(bottom: 4, left: 16),
              child: Text(
                shortcut,
                style: Theme.of(context).textTheme.bodySmall?.copyWith(
                  fontFamily: 'monospace',
                  backgroundColor: Theme.of(context).colorScheme.surfaceVariant,
                ),
              ),
            )),
          ],
        ],
      ),
    );
  }
}