/// Global Shortcuts Service
/// Handles application-wide keyboard shortcuts matching TunerStudio functionality
/// Includes F1 help, F5-F7 data logging, F10 tuning view, and other global shortcuts

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import 'data_logging_service.dart';
import 'help_service.dart';
import 'tuning_view_service.dart';
import 'gauge_designer_service.dart';
import '../models/ecu_data.dart';

/// Service for managing global application shortcuts
class GlobalShortcutsService extends ChangeNotifier {
  static final GlobalShortcutsService _instance = GlobalShortcutsService._internal();
  factory GlobalShortcutsService() => _instance;
  GlobalShortcutsService._internal();

  // Services
  late DataLoggingService _dataLoggingService;
  late HelpService _helpService;
  late TuningViewService _tuningViewService;
  late GaugeDesignerService _gaugeDesignerService;
  
  // Focus management
  final FocusNode _globalFocusNode = FocusNode();
  
  // Undo/Redo system
  final List<UndoRedoAction> _undoStack = [];
  final List<UndoRedoAction> _redoStack = [];
  static const int _maxUndoStackSize = 50;
  
  // Tab cycling
  List<Widget> _cyclableElements = [];
  int _currentCycleIndex = 0;
  
  // Initialization
  void initialize({
    required DataLoggingService dataLoggingService,
    required HelpService helpService,
    required TuningViewService tuningViewService,
    required GaugeDesignerService gaugeDesignerService,
  }) {
    _dataLoggingService = dataLoggingService;
    _helpService = helpService;
    _tuningViewService = tuningViewService;
    _gaugeDesignerService = gaugeDesignerService;
  }
  
  /// Handle global keyboard shortcuts
  KeyEventResult handleGlobalShortcut(KeyEvent event, BuildContext context) {
    if (event is! KeyDownEvent) return KeyEventResult.ignored;
    
    final isCtrlPressed = HardwareKeyboard.instance.isControlPressed;
    final isShiftPressed = HardwareKeyboard.instance.isShiftPressed;
    final isAltPressed = HardwareKeyboard.instance.isAltPressed;
    
    switch (event.logicalKey) {
      // F1 - Context-sensitive help
      case LogicalKeyboardKey.f1:
        _showContextHelp(context);
        return KeyEventResult.handled;
        
      // F2 - Rename (handled by individual widgets, but we can show rename dialog)
      case LogicalKeyboardKey.f2:
        _showRenameDialog(context);
        return KeyEventResult.handled;
        
      // F5 - Start data logging
      case LogicalKeyboardKey.f5:
        _dataLoggingService.startLogging();
        _showStatusMessage(context, 'Data logging started');
        return KeyEventResult.handled;
        
      // F6 - Stop data logging
      case LogicalKeyboardKey.f6:
        _dataLoggingService.stopLogging();
        _showStatusMessage(context, 'Data logging stopped');
        return KeyEventResult.handled;
        
      // F7 - Mark data log
      case LogicalKeyboardKey.f7:
        _dataLoggingService.markLog();
        _showStatusMessage(context, 'Log marker added');
        return KeyEventResult.handled;
        
      // F10 - Open Tuning & Dyno View Editor
      case LogicalKeyboardKey.f10:
        _tuningViewService.openTuningView(context);
        return KeyEventResult.handled;
        
      // Tab - Cycle through stacked elements
      case LogicalKeyboardKey.tab:
        if (!isCtrlPressed && !isAltPressed) {
          _cycleStackedElements(isShiftPressed);
          return KeyEventResult.handled;
        }
        break;
        
      // Ctrl+Z - Undo
      case LogicalKeyboardKey.keyZ:
        if (isCtrlPressed && !isShiftPressed) {
          _performUndo(context);
          return KeyEventResult.handled;
        }
        break;
        
      // Ctrl+Y or Ctrl+Shift+Z - Redo
      case LogicalKeyboardKey.keyY:
        if (isCtrlPressed) {
          _performRedo(context);
          return KeyEventResult.handled;
        }
        break;
        
      // Ctrl+D - Duplicate
      case LogicalKeyboardKey.keyD:
        if (isCtrlPressed) {
          _duplicateSelectedElement(context);
          return KeyEventResult.handled;
        }
        break;
        
      // Ctrl+T - Open Table Tune dialog
      case LogicalKeyboardKey.keyT:
        if (isCtrlPressed) {
          _openTableTuneDialog(context);
          return KeyEventResult.handled;
        }
        break;
        
      // Ctrl+S - Global save (if not handled by focused widget)
      case LogicalKeyboardKey.keyS:
        if (isCtrlPressed) {
          _performGlobalSave(context);
          return KeyEventResult.handled;
        }
        break;
        
      // Arrow keys with Alt - Gauge manipulation
      case LogicalKeyboardKey.arrowUp:
      case LogicalKeyboardKey.arrowDown:
      case LogicalKeyboardKey.arrowLeft:
      case LogicalKeyboardKey.arrowRight:
        if (isAltPressed) {
          final handled = _gaugeDesignerService.adjustGaugeSize(event.logicalKey, isShiftPressed) ||
                         _gaugeDesignerService.adjustGaugePosition(event.logicalKey);
          if (handled) {
            return KeyEventResult.handled;
          }
        }
        break;
    }
    
    return KeyEventResult.ignored;
  }
  
  /// Show context-sensitive help based on current screen/widget
  void _showContextHelp(BuildContext context) {
    final currentRoute = ModalRoute.of(context)?.settings.name ?? '';
    _helpService.showContextHelp(context, currentRoute);
  }
  
  /// Show rename dialog for current context
  void _showRenameDialog(BuildContext context) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Rename'),
        content: const TextField(
          decoration: InputDecoration(
            labelText: 'New name',
            hintText: 'Enter new name...',
          ),
          autofocus: true,
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () {
              // TODO: Implement rename functionality based on context
              Navigator.of(context).pop();
              _showStatusMessage(context, 'Rename functionality coming soon');
            },
            child: const Text('Rename'),
          ),
        ],
      ),
    );
  }
  
  /// Cycle through stacked UI elements
  void _cycleStackedElements(bool reverse) {
    if (_cyclableElements.isEmpty) return;
    
    if (reverse) {
      _currentCycleIndex = (_currentCycleIndex - 1 + _cyclableElements.length) % _cyclableElements.length;
    } else {
      _currentCycleIndex = (_currentCycleIndex + 1) % _cyclableElements.length;
    }
    
    // TODO: Focus the selected element
    notifyListeners();
  }
  
  /// Perform undo operation
  void _performUndo(BuildContext context) {
    if (_undoStack.isEmpty) {
      _showStatusMessage(context, 'Nothing to undo');
      return;
    }
    
    final action = _undoStack.removeLast();
    _redoStack.add(action);
    
    try {
      action.undo();
      _showStatusMessage(context, 'Undid: ${action.description}');
    } catch (e) {
      _showStatusMessage(context, 'Undo failed: $e');
    }
    
    notifyListeners();
  }
  
  /// Perform redo operation
  void _performRedo(BuildContext context) {
    if (_redoStack.isEmpty) {
      _showStatusMessage(context, 'Nothing to redo');
      return;
    }
    
    final action = _redoStack.removeLast();
    _undoStack.add(action);
    
    try {
      action.redo();
      _showStatusMessage(context, 'Redid: ${action.description}');
    } catch (e) {
      _showStatusMessage(context, 'Redo failed: $e');
    }
    
    notifyListeners();
  }
  
  /// Add action to undo stack
  void addUndoAction(UndoRedoAction action) {
    _undoStack.add(action);
    _redoStack.clear(); // Clear redo stack on new action
    
    // Limit stack size
    if (_undoStack.length > _maxUndoStackSize) {
      _undoStack.removeAt(0);
    }
    
    notifyListeners();
  }
  
  /// Duplicate selected element
  void _duplicateSelectedElement(BuildContext context) {
    // Try gauge duplication first (if in design mode)
    final duplicatedGaugeId = _gaugeDesignerService.duplicateSelectedGauge();
    if (duplicatedGaugeId != null) {
      _showStatusMessage(context, 'Duplicated gauge: $duplicatedGaugeId');
      return;
    }
    
    // TODO: Add table duplication, dashboard tab duplication, etc.
    _showStatusMessage(context, 'Duplication: Select a gauge or element to duplicate');
  }
  
  /// Open Table Tune dialog
  void _openTableTuneDialog(BuildContext context) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Table Tune'),
        content: const Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text('Advanced table tuning operations:'),
            SizedBox(height: 16),
            ListTile(
              leading: Icon(Icons.trending_up),
              title: Text('Auto-tune VE table'),
              subtitle: Text('Automatically optimize VE values'),
            ),
            ListTile(
              leading: Icon(Icons.timeline),
              title: Text('Generate smooth curve'),
              subtitle: Text('Create smooth transitions'),
            ),
            ListTile(
              leading: Icon(Icons.restore),
              title: Text('Reset to defaults'),
              subtitle: Text('Restore factory table values'),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('Cancel'),
          ),
          FilledButton(
            onPressed: () {
              Navigator.of(context).pop();
              _showStatusMessage(context, 'Table tuning functionality coming soon');
            },
            child: const Text('Apply'),
          ),
        ],
      ),
    );
  }
  
  /// Perform global save operation
  void _performGlobalSave(BuildContext context) {
    // TODO: Implement global save based on current context
    _showStatusMessage(context, 'Global save functionality coming soon');
  }
  
  /// Show status message to user
  void _showStatusMessage(BuildContext context, String message) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),
        duration: const Duration(seconds: 2),
        behavior: SnackBarBehavior.floating,
      ),
    );
  }
  
  /// Register elements that can be cycled through with Tab
  void registerCyclableElements(List<Widget> elements) {
    _cyclableElements = elements;
    _currentCycleIndex = 0;
    notifyListeners();
  }
  
  /// Get focus node for global shortcuts
  FocusNode get globalFocusNode => _globalFocusNode;
  
  /// Check if undo is available
  bool get canUndo => _undoStack.isNotEmpty;
  
  /// Check if redo is available
  bool get canRedo => _redoStack.isNotEmpty;
  
  @override
  void dispose() {
    _globalFocusNode.dispose();
    super.dispose();
  }
}

/// Undo/Redo action interface
abstract class UndoRedoAction {
  String get description;
  void undo();
  void redo();
}

/// Table edit undo action
class TableEditUndoAction implements UndoRedoAction {
  final String tableName;
  final int row;
  final int col;
  final dynamic oldValue;
  final dynamic newValue;
  final Function(int, int, dynamic) setValue;
  
  TableEditUndoAction({
    required this.tableName,
    required this.row,
    required this.col,
    required this.oldValue,
    required this.newValue,
    required this.setValue,
  });
  
  @override
  String get description => 'Edit $tableName[$row,$col]';
  
  @override
  void undo() => setValue(row, col, oldValue);
  
  @override
  void redo() => setValue(row, col, newValue);
}

/// Generic value change undo action
class ValueChangeUndoAction implements UndoRedoAction {
  final String description;
  final VoidCallback undoCallback;
  final VoidCallback redoCallback;
  
  ValueChangeUndoAction({
    required this.description,
    required this.undoCallback,
    required this.redoCallback,
  });
  
  @override
  void undo() => undoCallback();
  
  @override
  void redo() => redoCallback();
}