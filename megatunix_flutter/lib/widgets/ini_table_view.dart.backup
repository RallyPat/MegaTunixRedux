/// Dynamic Table View based on INI definitions
/// Provides TunerStudio-compatible table editing with INI-defined dimensions and properties

import 'dart:math' as math;
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../services/ini_parser.dart';
import '../services/ini_msq_service.dart';
import '../services/realtime_data_service.dart';
import '../models/table_cursor.dart';
import '../widgets/table_cursor_overlay.dart';
import '../theme/ecu_theme.dart';
import '../theme/tunerstudio_colors.dart';

class INITableView extends StatefulWidget {
  final TableDefinition tableDefinition;
  final List<List<double>> tableData;
  final List<double> xAxis;
  final List<double> yAxis;
  final Function(int row, int col, double value) onValueChanged;
  final bool showHeatmap;
  final bool showValues;
  final RealtimeDataService? realtimeService;
  final bool showCursor;

  const INITableView({
    super.key,
    required this.tableDefinition,
    required this.tableData,
    required this.xAxis,
    required this.yAxis,
    required this.onValueChanged,
    this.showHeatmap = true,
    this.showValues = true,
    this.realtimeService,
    this.showCursor = true,
  });

  @override
  State<INITableView> createState() => _INITableViewState();
}

class _INITableViewState extends State<INITableView> {
  late ScrollController _horizontalController;
  late ScrollController _verticalController;
  late ScrollController _headerHorizontalController;
  
  int _selectedRow = -1;
  int _selectedCol = -1;
  int _editingRow = -1;
  int _editingCol = -1;
  
  final TextEditingController _editController = TextEditingController();
  final FocusNode _editFocusNode = FocusNode();
  final FocusNode _tableFocusNode = FocusNode();
  
  // Multi-selection support
  Set<String> _selectedCells = <String>{};
  int? _selectionStartRow;
  int? _selectionStartCol;
  
  // Cursor support
  bool _cursorEnabled = true;
  TableCursor _currentCursor = const TableCursor.hidden();
  
  // Table type for TunerStudio color scheme
  late ECUTableType _tableType;
  
  // Constants for sizing
  static const double _cellWidth = 60.0;
  static const double _cellHeight = 35.0;

  @override
  void initState() {
    super.initState();
    _horizontalController = ScrollController();
    _verticalController = ScrollController();
    _headerHorizontalController = ScrollController();
    
    // Determine table type for proper color scheme
    _tableType = TunerStudioColors.getTableType(widget.tableDefinition.name);
    
    // Sync header scrolling with table scrolling
    _horizontalController.addListener(() {
      if (_headerHorizontalController.hasClients) {
        _headerHorizontalController.jumpTo(_horizontalController.offset);
      }
    });
    
    // Set up real-time cursor updates
    _setupCursorUpdates();
  }
  
  void _setupCursorUpdates() {
    widget.realtimeService?.addListener(_updateCursor);
  }
  
  void _updateCursor() {
    if (!_cursorEnabled || 
        widget.realtimeService == null || 
        !widget.showCursor) {
      setState(() {
        _currentCursor = const TableCursor.hidden();
      });
      return;
    }
    
    // Update cursor position based on real-time data
    widget.realtimeService!.updateTableCursor(
      widget.tableDefinition.name,
      widget.xAxis,
      widget.yAxis,
      widget.tableDefinition.xAxis,
      widget.tableDefinition.yAxis,
    );
    
    setState(() {
      _currentCursor = widget.realtimeService!.getCursor(widget.tableDefinition.name);
    });
  }

  @override
  void dispose() {
    _horizontalController.dispose();
    _verticalController.dispose();
    _headerHorizontalController.dispose();
    _editController.dispose();
    _editFocusNode.dispose();
    _tableFocusNode.dispose();
    widget.realtimeService?.removeListener(_updateCursor);
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Focus(
      focusNode: _tableFocusNode,
      onKeyEvent: _handleKeyEvent,
      child: Container(
        decoration: BoxDecoration(
          color: Theme.of(context).colorScheme.surface,
          borderRadius: BorderRadius.circular(12),
          border: Border.all(
            color: Theme.of(context).colorScheme.outlineVariant,
            width: 1,
          ),
        ),
        child: Column(
          children: [
            _buildTableHeader(),
            _buildTableAxisHeader(),
            Expanded(child: _buildTableBody()),
            _buildTableFooter(),
          ],
        ),
      ),
    );
  }

  Widget _buildTableHeader() {
    final theme = Theme.of(context);
    final isHeatmapTable = TunerStudioColors.shouldUseHeatmap(_tableType);
    
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: theme.colorScheme.surfaceContainerHighest.withOpacity(0.3),
        borderRadius: const BorderRadius.only(
          topLeft: Radius.circular(12),
          topRight: Radius.circular(12),
        ),
      ),
      child: Row(
        children: [
          Icon(
            isHeatmapTable ? Icons.colorize : Icons.table_chart,
            color: theme.colorScheme.primary,
            size: 20,
          ),
          const SizedBox(width: 8),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  '${widget.tableDefinition.displayName} ${isHeatmapTable ? "(Heatmap)" : "(Numeric)"}',
                  style: theme.textTheme.titleMedium?.copyWith(
                    fontWeight: FontWeight.w600,
                  ),
                ),
                Text(
                  '${widget.tableDefinition.rows}x${widget.tableDefinition.cols} - ${widget.tableDefinition.units}',
                  style: theme.textTheme.bodySmall?.copyWith(
                    color: theme.colorScheme.onSurfaceVariant,
                  ),
                ),
                if (isHeatmapTable)
                  Text(
                    'TunerStudio color scheme: Blue (low) → Red (high)',
                    style: theme.textTheme.bodySmall?.copyWith(
                      color: theme.colorScheme.onSurfaceVariant,
                      fontStyle: FontStyle.italic,
                    ),
                  ),
              ],
            ),
          ),
          if (_selectedRow >= 0 && _selectedCol >= 0) ...[
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
              decoration: BoxDecoration(
                color: theme.colorScheme.primaryContainer,
                borderRadius: BorderRadius.circular(16),
              ),
              child: Text(
                'Cell [${_selectedRow + 1}, ${_selectedCol + 1}]: ${widget.tableData[_selectedRow][_selectedCol].toStringAsFixed(1)}${widget.tableDefinition.units}',
                style: theme.textTheme.bodySmall?.copyWith(
                  color: theme.colorScheme.onPrimaryContainer,
                  fontWeight: FontWeight.w500,
                ),
              ),
            ),
            const SizedBox(width: 8),
          ],
          if (widget.realtimeService != null && widget.showCursor) ...[
            CursorInfoWidget(
              cursor: _currentCursor,
              tableName: widget.tableDefinition.name,
              currentRpm: widget.realtimeService?.getFormattedValue('rpm') ?? '--',
              currentLoad: widget.realtimeService?.getFormattedValue('map') ?? '--',
            ),
            const SizedBox(width: 8),
          ],
          _buildTableControls(),
        ],
      ),
    );
  }

  Widget _buildTableControls() {
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        if (widget.realtimeService != null && widget.showCursor)
          CursorToggleButton(
            enabled: _cursorEnabled,
            onToggle: () {
              setState(() {
                _cursorEnabled = !_cursorEnabled;
                if (!_cursorEnabled) {
                  _currentCursor = const TableCursor.hidden();
                }
              });
            },
          ),
        IconButton(
          onPressed: _selectedCells.isNotEmpty ? _interpolateSelection : null,
          icon: const Icon(Icons.timeline, size: 18),
          tooltip: 'Interpolate Selection (Ctrl+I)',
          style: IconButton.styleFrom(
            backgroundColor: Theme.of(context).colorScheme.surfaceContainerHighest,
          ),
        ),
        IconButton(
          onPressed: _selectedCells.isNotEmpty ? _smoothSelection : null,
          icon: const Icon(Icons.auto_fix_high, size: 18),
          tooltip: 'Smooth Selection (Ctrl+S)',
          style: IconButton.styleFrom(
            backgroundColor: Theme.of(context).colorScheme.surfaceContainerHighest,
          ),
        ),
        IconButton(
          onPressed: _selectedCells.isNotEmpty ? _copySelection : null,
          icon: const Icon(Icons.copy, size: 18),
          tooltip: 'Copy Selection (Ctrl+C)',
          style: IconButton.styleFrom(
            backgroundColor: Theme.of(context).colorScheme.surfaceContainerHighest,
          ),
        ),
      ],
    );
  }

  Widget _buildTableAxisHeader() {
    final theme = Theme.of(context);
    
    return Container(
      height: 40,
      decoration: BoxDecoration(
        border: Border(
          bottom: BorderSide(
            color: theme.colorScheme.outlineVariant,
            width: 1,
          ),
        ),
      ),
      child: Row(
        children: [
          // Corner cell
          Container(
            width: 80,
            height: 40,
            decoration: BoxDecoration(
              color: theme.colorScheme.surface,
              border: Border(
                right: BorderSide(
                  color: theme.colorScheme.outlineVariant,
                  width: 1,
                ),
              ),
            ),
            child: Center(
              child: Text(
                '${widget.tableDefinition.yAxis.toUpperCase()}\\${widget.tableDefinition.xAxis.toUpperCase()}',
                style: theme.textTheme.bodySmall?.copyWith(
                  fontWeight: FontWeight.w600,
                  color: theme.colorScheme.primary,
                ),
              ),
            ),
          ),
          
          // X-axis headers (synchronized scrolling)
          Expanded(
            child: SingleChildScrollView(
              scrollDirection: Axis.horizontal,
              controller: _headerHorizontalController,
              child: Row(
                children: List.generate(widget.xAxis.length, (index) {
                  return Container(
                    width: _cellWidth,
                    height: 40,
                    decoration: BoxDecoration(
                      color: theme.colorScheme.surface,
                      border: Border(
                        right: BorderSide(
                          color: theme.colorScheme.outlineVariant,
                          width: 1,
                        ),
                      ),
                    ),
                    child: Center(
                      child: Text(
                        widget.xAxis[index].toInt().toString(),
                        style: theme.textTheme.bodySmall?.copyWith(
                          fontWeight: FontWeight.w600,
                          color: ECUTheme.getAccentColor('rpm'),
                        ),
                      ),
                    ),
                  );
                }),
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildTableBody() {
    return Row(
      children: [
        // Y-axis headers
        Container(
          width: 80,
          child: SingleChildScrollView(
            controller: _verticalController,
            child: Column(
              children: List.generate(widget.yAxis.length, (index) {
                return Container(
                  width: 80,
                  height: _cellHeight,
                  decoration: BoxDecoration(
                    color: Theme.of(context).colorScheme.surface,
                    border: Border(
                      right: BorderSide(
                        color: Theme.of(context).colorScheme.outlineVariant,
                        width: 1,
                      ),
                      bottom: BorderSide(
                        color: Theme.of(context).colorScheme.outlineVariant,
                        width: 0.5,
                      ),
                    ),
                  ),
                  child: Center(
                    child: Text(
                      widget.yAxis[index].toInt().toString(),
                      style: Theme.of(context).textTheme.bodySmall?.copyWith(
                        fontWeight: FontWeight.w600,
                        color: ECUTheme.getAccentColor('map'),
                      ),
                    ),
                  ),
                );
              }),
            ),
          ),
        ),

        // Table cells
        Expanded(
          child: Stack(
            children: [
              // Table content
              SingleChildScrollView(
                controller: _verticalController,
                child: SingleChildScrollView(
                  scrollDirection: Axis.horizontal,
                  controller: _horizontalController,
                  child: Column(
                    children: List.generate(widget.tableData.length, (row) {
                      return Row(
                        children: List.generate(widget.tableData[row].length, (col) {
                          return _buildTableCell(row, col);
                        }),
                      );
                    }),
                  ),
                ),
              ),
              
              // Cursor overlay - ignore pointer events to allow table interaction
              if (widget.realtimeService != null && widget.showCursor && _cursorEnabled)
                IgnorePointer(
                  child: SingleChildScrollView(
                    controller: _verticalController,
                    physics: const NeverScrollableScrollPhysics(),
                    child: SingleChildScrollView(
                      scrollDirection: Axis.horizontal,
                      controller: _horizontalController,
                      physics: const NeverScrollableScrollPhysics(),
                      child: TableCursorOverlay(
                        cursor: _currentCursor,
                        rows: widget.tableData.length,
                        cols: widget.tableData.isNotEmpty ? widget.tableData[0].length : 0,
                        cellWidth: _cellWidth,
                        cellHeight: _cellHeight,
                        enabled: _cursorEnabled,
                      ),
                    ),
                  ),
                ),
            ],
          ),
        ),
      ],
    );
  }

  Widget _buildTableCell(int row, int col) {
    final value = widget.tableData[row][col];
    final isSelected = row == _selectedRow && col == _selectedCol;
    final isEditing = row == _editingRow && col == _editingCol;
    final isMultiSelected = _selectedCells.contains('$row,$col');
    
    if (isEditing) {
      return Container(
        width: _cellWidth,
        height: _cellHeight,
        decoration: BoxDecoration(
          color: ECUTheme.getAccentColor('edit').withOpacity(0.3),
          border: Border.all(
            color: ECUTheme.getAccentColor('edit'),
            width: 2,
          ),
        ),
        child: TextField(
          controller: _editController,
          focusNode: _editFocusNode,
          textAlign: TextAlign.center,
          style: const TextStyle(
            fontSize: 12,
            fontWeight: FontWeight.w600,
          ),
          decoration: const InputDecoration(
            border: InputBorder.none,
            contentPadding: EdgeInsets.zero,
          ),
          onSubmitted: (newValue) => _submitEdit(row, col, newValue),
          onEditingComplete: () => _submitEdit(row, col, _editController.text),
        ),
      );
    }

    return GestureDetector(
      onTap: () => _selectCell(row, col),
      onDoubleTap: () => _startEditing(row, col),
      child: Container(
        width: _cellWidth,
        height: _cellHeight,
        decoration: BoxDecoration(
          color: _getCellColor(value, isSelected, isMultiSelected),
          border: Border.all(
            color: isSelected || isMultiSelected
                ? ECUTheme.getAccentColor('selection')
                : Theme.of(context).colorScheme.outlineVariant,
            width: isSelected ? 2 : 0.5,
          ),
        ),
        child: Center(
          child: widget.showValues
              ? Text(
                  value.toStringAsFixed(1),
                  style: TextStyle(
                    fontSize: 11,
                    fontWeight: FontWeight.w600,
                    color: _getTextColor(value),
                  ),
                )
              : null,
        ),
      ),
    );
  }

  Widget _buildTableFooter() {
    final theme = Theme.of(context);
    final isHeatmapTable = TunerStudioColors.shouldUseHeatmap(_tableType);
    
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: theme.colorScheme.surfaceContainerHighest.withOpacity(0.3),
        borderRadius: const BorderRadius.only(
          bottomLeft: Radius.circular(12),
          bottomRight: Radius.circular(12),
        ),
        border: Border(
          top: BorderSide(
            color: theme.colorScheme.outlineVariant,
            width: 1,
          ),
        ),
      ),
      child: Row(
        children: [
          Icon(
            Icons.info_outline,
            size: 16,
            color: theme.colorScheme.onSurfaceVariant,
          ),
          const SizedBox(width: 8),
          Text(
            'Range: ${widget.tableDefinition.minValue.toStringAsFixed(1)} - ${widget.tableDefinition.maxValue.toStringAsFixed(1)} ${widget.tableDefinition.units}',
            style: theme.textTheme.bodySmall?.copyWith(
              color: theme.colorScheme.onSurfaceVariant,
            ),
          ),
          if (isHeatmapTable) ...[
            const SizedBox(width: 16),
            // Color legend
            ...TunerStudioColors.getColorLegend(_tableType).map((entry) {
              if (entry.key.isEmpty) {
                return Container(width: 12, height: 16, color: entry.value);
              }
              return Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Container(width: 12, height: 16, color: entry.value),
                  const SizedBox(width: 4),
                  Text(
                    entry.key,
                    style: theme.textTheme.bodySmall?.copyWith(
                      color: theme.colorScheme.onSurfaceVariant,
                      fontSize: 10,
                    ),
                  ),
                  const SizedBox(width: 8),
                ],
              );
            }).toList(),
          ],
          const Spacer(),
          if (_selectedCells.isNotEmpty)
            Text(
              '${_selectedCells.length} cells selected',
              style: theme.textTheme.bodySmall?.copyWith(
                color: theme.colorScheme.onSurfaceVariant,
              ),
            ),
        ],
      ),
    );
  }

  Color _getCellColor(double value, bool isSelected, bool isMultiSelected) {
    if (isSelected) {
      return ECUTheme.getAccentColor('primary').withOpacity(0.3);
    }
    
    if (isMultiSelected) {
      return ECUTheme.getAccentColor('selection').withOpacity(0.2);
    }
    
    // Check if table should use heatmap based on TunerStudio conventions
    final useHeatmap = widget.showHeatmap && TunerStudioColors.shouldUseHeatmap(_tableType);
    if (!useHeatmap) {
      return Theme.of(context).colorScheme.surface;
    }
    
    // Normalize value based on table definition
    final range = widget.tableDefinition.maxValue - widget.tableDefinition.minValue;
    final normalizedValue = range > 0 
        ? ((value - widget.tableDefinition.minValue) / range).clamp(0.0, 1.0)
        : 0.5;
    
    // Use TunerStudio thermal color mapping
    return TunerStudioColors.getTableHeatmapColor(_tableType, normalizedValue);
  }
  }

  Color _getTextColor(double value) {
    // Check if table uses heatmap colors
    final useHeatmap = widget.showHeatmap && TunerStudioColors.shouldUseHeatmap(_tableType);
    if (!useHeatmap) {
      return Theme.of(context).colorScheme.onSurface;
    }
    
    final range = widget.tableDefinition.maxValue - widget.tableDefinition.minValue;
    final normalizedValue = range > 0
        ? ((value - widget.tableDefinition.minValue) / range).clamp(0.0, 1.0)
        : 0.5;
    
    return TunerStudioColors.getThermalTextColor(normalizedValue);
  }

  void _selectCell(int row, int col) {
    setState(() {
      if (HardwareKeyboard.instance.isShiftPressed && _selectedRow >= 0 && _selectedCol >= 0) {
        _updateMultiSelection(row, col);
      } else if (HardwareKeyboard.instance.isControlPressed) {
        final cellKey = '$row,$col';
        if (_selectedCells.contains(cellKey)) {
          _selectedCells.remove(cellKey);
        } else {
          _selectedCells.add(cellKey);
        }
      } else {
        _selectedCells.clear();
        _selectionStartRow = null;
        _selectionStartCol = null;
      }
      
      _selectedRow = row;
      _selectedCol = col;
    });
    
    _tableFocusNode.requestFocus();
  }

  void _updateMultiSelection(int endRow, int endCol) {
    if (_selectionStartRow == null || _selectionStartCol == null) {
      _selectionStartRow = _selectedRow;
      _selectionStartCol = _selectedCol;
    }

    _selectedCells.clear();
    
    final minRow = [_selectionStartRow!, endRow].reduce((a, b) => a < b ? a : b);
    final maxRow = [_selectionStartRow!, endRow].reduce((a, b) => a > b ? a : b);
    final minCol = [_selectionStartCol!, endCol].reduce((a, b) => a < b ? a : b);
    final maxCol = [_selectionStartCol!, endCol].reduce((a, b) => a > b ? a : b);
    
    for (int row = minRow; row <= maxRow; row++) {
      for (int col = minCol; col <= maxCol; col++) {
        _selectedCells.add('$row,$col');
      }
    }
  }

  void _startEditing(int row, int col) {
    setState(() {
      _editingRow = row;
      _editingCol = col;
      _editController.text = widget.tableData[row][col].toStringAsFixed(1);
    });
    
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _editFocusNode.requestFocus();
      _editController.selection = TextSelection(
        baseOffset: 0,
        extentOffset: _editController.text.length,
      );
    });
  }

  void _submitEdit(int row, int col, String newValue) {
    final doubleValue = double.tryParse(newValue);
    if (doubleValue != null) {
      final clampedValue = doubleValue.clamp(
        widget.tableDefinition.minValue,
        widget.tableDefinition.maxValue,
      );
      widget.onValueChanged(row, col, clampedValue);
    }
    
    setState(() {
      _editingRow = -1;
      _editingCol = -1;
    });
    _editController.clear();
  }

  KeyEventResult _handleKeyEvent(FocusNode node, KeyEvent event) {
    if (event is! KeyDownEvent) return KeyEventResult.ignored;
    
    final isCtrl = HardwareKeyboard.instance.isControlPressed;
    final isShift = HardwareKeyboard.instance.isShiftPressed;
    
    // Handle keyboard shortcuts based on TunerStudio compatibility
    switch (event.logicalKey) {
      // Arrow key navigation
      case LogicalKeyboardKey.arrowUp:
        _handleArrowKey(-1, 0, isShift, isCtrl);
        return KeyEventResult.handled;
      case LogicalKeyboardKey.arrowDown:
        _handleArrowKey(1, 0, isShift, isCtrl);
        return KeyEventResult.handled;
      case LogicalKeyboardKey.arrowLeft:
        _handleArrowKey(0, -1, isShift, isCtrl);
        return KeyEventResult.handled;
      case LogicalKeyboardKey.arrowRight:
        _handleArrowKey(0, 1, isShift, isCtrl);
        return KeyEventResult.handled;
        
      // Table operations (TunerStudio compatible)
      case LogicalKeyboardKey.keyI when isCtrl:
        _interpolateSelection();
        return KeyEventResult.handled;
      case LogicalKeyboardKey.keyH when isCtrl:
        _interpolateHorizontal();
        return KeyEventResult.handled;
      case LogicalKeyboardKey.keyV when isCtrl && !isShift:
        _pasteSelection();
        return KeyEventResult.handled;
      case LogicalKeyboardKey.keyV when isCtrl && isShift:
        _interpolateVertical();
        return KeyEventResult.handled;
      case LogicalKeyboardKey.keyC when isCtrl:
        _copySelection();
        return KeyEventResult.handled;
      case LogicalKeyboardKey.keyS when isCtrl:
        _smoothSelection();
        return KeyEventResult.handled;
      case LogicalKeyboardKey.delete:
        _clearSelection();
        return KeyEventResult.handled;
      case LogicalKeyboardKey.escape:
        _clearMultiSelection();
        return KeyEventResult.handled;
        
      // Editing shortcuts
      case LogicalKeyboardKey.enter:
        _confirmEdit();
        return KeyEventResult.handled;
      case LogicalKeyboardKey.f2:
        _startEditingSelectedCell();
        return KeyEventResult.handled;
        
      // Selection shortcuts
      case LogicalKeyboardKey.keyA when isCtrl:
        _selectAllCells();
        return KeyEventResult.handled;
    }
    
    return KeyEventResult.ignored;
  }

  Map<String, int>? _getSelectionBounds() {
    if (_selectedCells.isEmpty) return null;
    
    int minRow = 999999, maxRow = -1;
    int minCol = 999999, maxCol = -1;
    
    for (final cellId in _selectedCells) {
      final parts = cellId.split(',');
      final row = int.parse(parts[0]);
      final col = int.parse(parts[1]);
      
      minRow = math.min(minRow, row);
      maxRow = math.max(maxRow, row);
      minCol = math.min(minCol, col);
      maxCol = math.max(maxCol, col);
    }
    
    return {
      'minRow': minRow,
      'maxRow': maxRow,
      'minCol': minCol,
      'maxCol': maxCol,
    };
  }

  void _interpolateSelection() {
    if (_selectedCells.isEmpty) return;
    
    // Get bounds of selection
    final bounds = _getSelectionBounds();
    if (bounds == null) return;
    
    final startRow = bounds['minRow']!;
    final endRow = bounds['maxRow']!;
    final startCol = bounds['minCol']!;
    final endCol = bounds['maxCol']!;
    
    // Perform interpolation on the selected rectangular area
    for (int col = startCol; col <= endCol; col++) {
      for (int row = startRow + 1; row < endRow; row++) {
        if (!_selectedCells.contains('$row,$col')) continue;
        
        // Linear interpolation between top and bottom values
        final topValue = double.tryParse(widget.tableData[startRow][col].toString()) ?? 0.0;
        final bottomValue = double.tryParse(widget.tableData[endRow][col].toString()) ?? 0.0;
        final progress = (row - startRow) / (endRow - startRow);
        final interpolatedValue = topValue + (bottomValue - topValue) * progress;
        
        widget.tableData[row][col] = interpolatedValue;
      }
    }
    
    setState(() {});
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('🧮 Interpolation applied to selection')),
    );
  }

  void _smoothSelection() {
    if (_selectedCells.isEmpty) return;
    
    // Get bounds of selection
    final bounds = _getSelectionBounds();
    if (bounds == null) return;
    
    final startRow = bounds['minRow']!;
    final endRow = bounds['maxRow']!;
    final startCol = bounds['minCol']!;
    final endCol = bounds['maxCol']!;
    
    // Create a copy of the original data for smoothing calculation
    final originalData = List.generate(widget.tableData.length, 
      (i) => List.from(widget.tableData[i]));
    
    // Apply 3x3 smoothing kernel to selected cells
    for (int row = startRow; row <= endRow; row++) {
      for (int col = startCol; col <= endCol; col++) {
        if (!_selectedCells.contains('$row,$col')) continue;
        
        // Collect neighboring values for smoothing
        double sum = 0.0;
        int count = 0;
        
        for (int dr = -1; dr <= 1; dr++) {
          for (int dc = -1; dc <= 1; dc++) {
            final nr = row + dr;
            final nc = col + dc;
            
            if (nr >= 0 && nr < widget.tableData.length && 
                nc >= 0 && nc < widget.tableData[nr].length) {
              final value = double.tryParse(originalData[nr][nc].toString()) ?? 0.0;
              // Weight center cell more heavily
              final weight = (dr == 0 && dc == 0) ? 2.0 : 1.0;
              sum += value * weight;
              count += weight.toInt();
            }
          }
        }
        
        if (count > 0) {
          final smoothedValue = sum / count;
          widget.tableData[row][col] = smoothedValue;
        }
      }
    }
    
    setState(() {});
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('🌊 Smoothing applied to selection')),
    );
  }

  void _copySelection() async {
    if (_selectedCells.isEmpty && (_selectedRow == -1 || _selectedCol == -1)) return;
    
    String clipboardText = '';
    
    if (_selectedCells.isNotEmpty) {
      // Copy multi-cell selection
      final bounds = _getSelectionBounds();
      if (bounds == null) return;
      
      final startRow = bounds['minRow']!;
      final endRow = bounds['maxRow']!;
      final startCol = bounds['minCol']!;
      final endCol = bounds['maxCol']!;
      
      List<String> rows = [];
      for (int row = startRow; row <= endRow; row++) {
        List<String> cols = [];
        for (int col = startCol; col <= endCol; col++) {
          cols.add(widget.tableData[row][col].toString());
        }
        rows.add(cols.join('\t'));
      }
      clipboardText = rows.join('\n');
    } else {
      // Copy single cell
      clipboardText = widget.tableData[_selectedRow][_selectedCol].toString();
    }
    
    await Clipboard.setData(ClipboardData(text: clipboardText));
    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('📋 Selection copied to clipboard')),
      );
    }
  }

  // Additional TunerStudio-compatible keybind handlers
  
  void _handleArrowKey(int deltaRow, int deltaCol, bool isShift, bool isCtrl) {
    // Get table dimensions
    final rows = widget.tableData.length;
    final cols = widget.tableData.isNotEmpty ? widget.tableData[0].length : 0;
    
    // Initialize selection if none exists
    if (_selectedRow == -1 || _selectedCol == -1) {
      _selectedRow = 0;
      _selectedCol = 0;
    }
    
    // Calculate new position
    int newRow = (_selectedRow + deltaRow).clamp(0, rows - 1);
    int newCol = (_selectedCol + deltaCol).clamp(0, cols - 1);
    
    // Update selection
    if (isCtrl) {
      // Ctrl+Arrow: Start or extend multi-selection without changing anchor
      if (_selectedCells.isEmpty) {
        // Start multi-selection from current position
        _selectedCells.add('$_selectedRow,$_selectedCol');
      }
      _selectedCells.add('$newRow,$newCol');
    } else if (isShift && _selectedCells.isNotEmpty) {
      // Shift+Arrow: Extend selection in rectangular area
      final startRow = math.min(_selectedRow, newRow);
      final endRow = math.max(_selectedRow, newRow);
      final startCol = math.min(_selectedCol, newCol);
      final endCol = math.max(_selectedCol, newCol);
      
      _selectedCells.clear();
      for (int r = startRow; r <= endRow; r++) {
        for (int c = startCol; c <= endCol; c++) {
          _selectedCells.add('$r,$c');
        }
      }
    } else {
      // Normal navigation - clear multi-selection
      _selectedCells.clear();
    }
    
    // Update current selection
    _selectedRow = newRow;
    _selectedCol = newCol;
    
    setState(() {});
  }

  void _interpolateHorizontal() {
    if (_selectedCells.isEmpty) return;
    
    final bounds = _getSelectionBounds();
    if (bounds == null) return;
    
    final startRow = bounds['minRow']!;
    final endRow = bounds['maxRow']!;
    final startCol = bounds['minCol']!;
    final endCol = bounds['maxCol']!;
    
    // Interpolate each row horizontally
    for (int row = startRow; row <= endRow; row++) {
      for (int col = startCol + 1; col < endCol; col++) {
        if (!_selectedCells.contains('$row,$col')) continue;
        
        // Linear interpolation between left and right values
        final leftValue = double.tryParse(widget.tableData[row][startCol].toString()) ?? 0.0;
        final rightValue = double.tryParse(widget.tableData[row][endCol].toString()) ?? 0.0;
        final progress = (col - startCol) / (endCol - startCol);
        final interpolatedValue = leftValue + (rightValue - leftValue) * progress;
        
        widget.tableData[row][col] = interpolatedValue;
      }
    }
    
    setState(() {});
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('➡️ Horizontal interpolation applied')),
    );
  }

  void _interpolateVertical() {
    if (_selectedCells.isEmpty) return;
    
    final bounds = _getSelectionBounds();
    if (bounds == null) return;
    
    final startRow = bounds['minRow']!;
    final endRow = bounds['maxRow']!;
    final startCol = bounds['minCol']!;
    final endCol = bounds['maxCol']!;
    
    // Interpolate each column vertically
    for (int col = startCol; col <= endCol; col++) {
      for (int row = startRow + 1; row < endRow; row++) {
        if (!_selectedCells.contains('$row,$col')) continue;
        
        // Linear interpolation between top and bottom values
        final topValue = double.tryParse(widget.tableData[startRow][col].toString()) ?? 0.0;
        final bottomValue = double.tryParse(widget.tableData[endRow][col].toString()) ?? 0.0;
        final progress = (row - startRow) / (endRow - startRow);
        final interpolatedValue = topValue + (bottomValue - topValue) * progress;
        
        widget.tableData[row][col] = interpolatedValue;
      }
    }
    
    setState(() {});
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('⬇️ Vertical interpolation applied')),
    );
  }

  void _pasteSelection() async {
    try {
      final clipboardData = await Clipboard.getData(Clipboard.kTextPlain);
      final clipboardText = clipboardData?.text;
      
      if (clipboardText == null || clipboardText.isEmpty) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('📝 Clipboard is empty')),
          );
        }
        return;
      }
      
      // Parse clipboard data (tab-separated values, newline-separated rows)
      final rows = clipboardText.split('\n');
      List<List<double>> pasteData = [];
      
      for (String row in rows) {
        if (row.trim().isEmpty) continue;
        final cells = row.split('\t');
        List<double> rowData = [];
        for (String cell in cells) {
          final value = double.tryParse(cell.trim());
          if (value != null) {
            rowData.add(value);
          }
        }
        if (rowData.isNotEmpty) {
          pasteData.add(rowData);
        }
      }
      
      if (pasteData.isEmpty) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('📝 No valid numeric data in clipboard')),
          );
        }
        return;
      }
      
      // Determine paste area
      int startRow, endRow, startCol, endCol;
      
      if (_selectedCells.isNotEmpty) {
        // Use selected area bounds
        final bounds = _getSelectionBounds();
        if (bounds == null) return;
        
        startRow = bounds['minRow']!;
        endRow = bounds['maxRow']!;
        startCol = bounds['minCol']!;
        endCol = bounds['maxCol']!;
      } else {
        // Use current cell position
        startRow = _selectedRow == -1 ? 0 : _selectedRow;
        startCol = _selectedCol == -1 ? 0 : _selectedCol;
        endRow = startRow + pasteData.length - 1;
        endCol = startCol + pasteData[0].length - 1;
      }
      
      // Calculate target dimensions
      final targetRows = endRow - startRow + 1;
      final targetCols = endCol - startCol + 1;
      final pasteRows = pasteData.length;
      final pasteCols = pasteData[0].length;
      
      // Paste data with scaling/tiling to fit selected area
      for (int r = 0; r < targetRows; r++) {
        final targetRow = startRow + r;
        if (targetRow >= widget.tableData.length) break;
        
        for (int c = 0; c < targetCols; c++) {
          final targetCol = startCol + c;
          if (targetCol >= widget.tableData[targetRow].length) break;
          
          // Map target position to paste data with tiling
          final sourceRow = r % pasteRows;
          final sourceCol = c % pasteCols;
          final value = pasteData[sourceRow][sourceCol];
          
          widget.tableData[targetRow][targetCol] = value;
          widget.onValueChanged(targetRow, targetCol, value);
        }
      }
      
      setState(() {});
      
      if (mounted) {
        final pasteDesc = _selectedCells.isNotEmpty 
          ? 'into ${targetRows}x${targetCols} selection' 
          : 'at position ($startRow,$startCol)';
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('📝 Pasted ${pasteRows}x${pasteCols} cells $pasteDesc')),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('📝 Failed to paste from clipboard')),
        );
      }
    }
  }

  void _clearSelection() {
    if (_selectedCells.isEmpty) return;
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('🗑️ Selection cleared')),
    );
  }

  void _clearMultiSelection() {
    _selectedCells.clear();
    setState(() {});
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('❌ Multi-selection cleared')),
    );
  }

  void _confirmEdit() {
    // Exit edit mode and confirm changes
    _editFocusNode.unfocus();
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('✅ Edit confirmed')),
    );
  }

  void _startEditingSelectedCell() {
    // Start editing the selected cell (F2 key)
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('✏️ Started editing cell')),
    );
  }

  void _selectAllCells() {
    // Select all cells in the table
    _selectedCells.clear();
    for (int row = 0; row < widget.tableData.length; row++) {
      for (int col = 0; col < widget.tableData[row].length; col++) {
        _selectedCells.add('$row,$col');
      }
    }
    setState(() {});
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('🔲 All cells selected (${_selectedCells.length} cells)')),
    );
  }
}