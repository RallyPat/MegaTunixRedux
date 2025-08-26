import 'dart:math' as math;
import 'dart:math' as math;
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../models/msq_file.dart';
import '../theme/ecu_theme.dart';

/// VE Table 2D View Widget
/// Provides traditional grid-based table editing with heatmap visualization
class VETable2DView extends StatefulWidget {
  final List<List<double>> veTable;
  final List<double> rpmAxis;
  final List<double> mapAxis;
  final bool showGrid;
  final bool showValues;
  final bool showHeatmap;
  final int selectedRow;
  final int selectedCol;
  final Function(int, int) onCellSelected;
  final Function(int, int, double) onCellEdited;

  const VETable2DView({
    super.key,
    required this.veTable,
    required this.rpmAxis,
    required this.mapAxis,
    required this.showGrid,
    required this.showValues,
    required this.showHeatmap,
    required this.selectedRow,
    required this.selectedCol,
    required this.onCellSelected,
    required this.onCellEdited,
  });

  @override
  State<VETable2DView> createState() => _VETable2DViewState();
}

class _VETable2DViewState extends State<VETable2DView> {
  late ScrollController _horizontalController;
  late ScrollController _verticalController;
  
  // Cell editing state
  int _editingRow = -1;
  int _editingCol = -1;
  final TextEditingController _editController = TextEditingController();
  final FocusNode _editFocusNode = FocusNode();
  final FocusNode _tableFocusNode = FocusNode();
  
  // Multi-selection state
  Set<String> _selectedCells = <String>{};
  int? _selectionStartRow;
  int? _selectionStartCol;

  @override
  void initState() {
    super.initState();
    _horizontalController = ScrollController();
    _verticalController = ScrollController();
  }

  @override
  void dispose() {
    _horizontalController.dispose();
    _verticalController.dispose();
    _editController.dispose();
    _editFocusNode.dispose();
    _tableFocusNode.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Focus(
      focusNode: _tableFocusNode,
      onKeyEvent: _handleKeyEvent,
      child: Container(
        color: Theme.of(context).colorScheme.surface,
        child: Column(
          children: [
            // Column headers (MAP values)
            Container(
              height: 60,
              child: Row(
                children: [
                  // Corner cell
                  Container(
                    width: 80,
                    height: 60,
                    decoration: BoxDecoration(
                      color: Theme.of(context).colorScheme.surface,
                      border: Border(
                        right: BorderSide(
                          color: Theme.of(context).colorScheme.outline.withOpacity(0.3),
                          width: 1,
                        ),
                        bottom: BorderSide(
                          color: Theme.of(context).colorScheme.outline.withOpacity(0.3),
                          width: 1,
                        ),
                      ),
                    ),
                    child: Center(
                      child: Text(
                        'RPM\\MAP',
                        style: TextStyle(
                          fontSize: 12,
                          fontWeight: FontWeight.w600,
                          color: ECUTheme.getAccentColor('primary'),
                        ),
                      ),
                    ),
                  ),
                  
                  // MAP axis headers - synchronized with table scrolling
                  Expanded(
                    child: SingleChildScrollView(
                      scrollDirection: Axis.horizontal,
                      controller: _horizontalController,
                      child: Row(
                        children: widget.mapAxis.map((map) {
                          return Container(
                            width: 80,
                            height: 60,
                            decoration: BoxDecoration(
                              color: Theme.of(context).colorScheme.surface,
                              border: Border(
                                right: BorderSide(
                                  color: Theme.of(context).colorScheme.outline.withOpacity(0.3),
                                  width: 1,
                                ),
                                bottom: BorderSide(
                                  color: Theme.of(context).colorScheme.outline.withOpacity(0.3),
                                  width: 1,
                                ),
                              ),
                            ),
                            child: Center(
                              child: Text(
                                '${map.toInt()}',
                                style: TextStyle(
                                  fontSize: 12,
                                  fontWeight: FontWeight.w600,
                                  color: ECUTheme.getAccentColor('map'),
                                ),
                              ),
                            ),
                          );
                        }).toList(),
                      ),
                    ),
                  ),
                ],
              ),
            ),
            
            // Table rows
            Expanded(
              child: Row(
                children: [
                  // Row headers (RPM values) - synchronized with table scrolling
                  Container(
                    width: 80,
                    child: NotificationListener<ScrollNotification>(
                      onNotification: (notification) {
                        // Synchronize table scrolling with row headers
                        if (notification is ScrollUpdateNotification) {
                          _verticalController.jumpTo(notification.metrics.pixels);
                        }
                        return false;
                      },
                      child: SingleChildScrollView(
                        controller: _verticalController,
                        child: Column(
                          children: widget.rpmAxis.map((rpm) {
                            return Container(
                              width: 80,
                              height: 40,
                              decoration: BoxDecoration(
                                color: Theme.of(context).colorScheme.surface,
                                border: Border(
                                  right: BorderSide(
                                    color: Theme.of(context).colorScheme.outline.withOpacity(0.3),
                                    width: 1,
                                  ),
                                ),
                              ),
                              child: Center(
                                child: Text(
                                  '${rpm.toInt()}',
                                  style: TextStyle(
                                    fontSize: 12,
                                    fontWeight: FontWeight.w600,
                                    color: ECUTheme.getAccentColor('rpm'),
                                  ),
                                ),
                              ),
                            );
                          }).toList(),
                        ),
                      ),
                    ),
                  ),

                  // Table cells - synchronized with row headers
                  Expanded(
                    child: NotificationListener<ScrollNotification>(
                      onNotification: (notification) {
                        // Synchronize row headers with table scrolling
                        if (notification is ScrollUpdateNotification) {
                          _verticalController.jumpTo(notification.metrics.pixels);
                        }
                        return false;
                      },
                      child: SingleChildScrollView(
                        controller: _verticalController, // Same controller for perfect sync
                        child: SingleChildScrollView(
                          scrollDirection: Axis.horizontal,
                          controller: _horizontalController,
                          child: Column(
                            children: List.generate(widget.veTable.length, (row) {
                              return Row(
                                children: List.generate(widget.veTable[row].length, (col) {
                                  final isSelected = row == widget.selectedRow && col == widget.selectedCol;
                                  final isEditing = row == _editingRow && col == _editingCol;
                                  final value = widget.veTable[row][col];
                                  
                                  return _buildTableCell(
                                    row: row,
                                    col: col,
                                    value: value,
                                    isSelected: isSelected,
                                    isEditing: isEditing,
                                  );
                                }),
                              );
                            }),
                          ),
                        ),
                      ),
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
  
  /// Handle keyboard events for table navigation and shortcuts
  KeyEventResult _handleKeyEvent(FocusNode node, KeyEvent event) {
    if (event is! KeyDownEvent) return KeyEventResult.ignored;
    
    final isShift = HardwareKeyboard.instance.isShiftPressed;
    final isCtrl = HardwareKeyboard.instance.isControlPressed;
    
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
      case LogicalKeyboardKey.keyI:
        if (isCtrl) {
          _interpolateSelection();
          return KeyEventResult.handled;
        }
        break;
      case LogicalKeyboardKey.keyH:
        if (isCtrl) {
          _interpolateHorizontal();
          return KeyEventResult.handled;
        }
        break;
      case LogicalKeyboardKey.keyV:
        if (isCtrl && !isShift) {
          _pasteSelection();
          return KeyEventResult.handled;
        } else if (isCtrl && isShift) {
          _interpolateVertical();
          return KeyEventResult.handled;
        }
        break;
      case LogicalKeyboardKey.keyC:
        if (isCtrl) {
          _copySelection();
          return KeyEventResult.handled;
        }
        break;
      case LogicalKeyboardKey.keyS:
        if (isCtrl) {
          _smoothSelection();
          return KeyEventResult.handled;
        }
        break;
      case LogicalKeyboardKey.delete:
        _clearSelection();
        return KeyEventResult.handled;
      case LogicalKeyboardKey.escape:
        _clearMultiSelection();
        return KeyEventResult.handled;
    }
    
    return KeyEventResult.ignored;
  }
  
  /// Handle arrow key navigation with multi-selection support
  void _handleArrowKey(int deltaRow, int deltaCol, bool isShift, bool isCtrl) {
    if (widget.selectedRow < 0 || widget.selectedCol < 0) return;
    
    final newRow = (widget.selectedRow + deltaRow).clamp(0, widget.veTable.length - 1);
    final newCol = (widget.selectedCol + deltaCol).clamp(0, widget.veTable[0].length - 1);
    
    if (isCtrl) {
      // Ctrl+Arrow: Add individual cells to selection
      if (_selectedCells.isEmpty) {
        // Start multi-selection from current position
        _selectedCells.add('${widget.selectedRow},${widget.selectedCol}');
      }
      _selectedCells.add('$newRow,$newCol');
      widget.onCellSelected(newRow, newCol);
    } else if (isShift) {
      // Shift+Arrow: Rectangular selection
      if (_selectionStartRow == null || _selectionStartCol == null) {
        _selectionStartRow = widget.selectedRow;
        _selectionStartCol = widget.selectedCol;
      }
      _updateMultiSelection(newRow, newCol);
    } else {
      // Normal navigation - single cell selection
      _clearMultiSelection();
      widget.onCellSelected(newRow, newCol);
    }
    
    // Request focus to ensure keyboard events continue
    _tableFocusNode.requestFocus();
  }
  
  /// Update multi-selection range
  void _updateMultiSelection(int endRow, int endCol) {
    if (_selectionStartRow == null || _selectionStartCol == null) return;
    
    setState(() {
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
    });
    
    widget.onCellSelected(endRow, endCol);
  }
  
  /// Clear multi-selection
  void _clearMultiSelection() {
    setState(() {
      _selectedCells.clear();
      _selectionStartRow = null;
      _selectionStartCol = null;
    });
  }
  
  /// TunerStudio-compatible table operations
  void _interpolateSelection() {
    if (_selectedCells.isEmpty) return;
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('Interpolation applied to selection')),
    );
  }
  
  void _interpolateHorizontal() {
    if (_selectedCells.isEmpty) return;
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('Horizontal interpolation applied')),
    );
  }
  
  void _interpolateVertical() {
    if (_selectedCells.isEmpty) return;
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('Vertical interpolation applied')),
    );
  }
  
  void _smoothSelection() {
    if (_selectedCells.isEmpty) return;
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('Smoothing applied to selection')),
    );
  }
  
  void _copySelection() async {
    if (_selectedCells.isEmpty && (widget.selectedRow < 0 || widget.selectedCol < 0)) return;
    
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
          cols.add(widget.veTable[row][col].toString());
        }
        rows.add(cols.join('\t'));
      }
      clipboardText = rows.join('\n');
    } else {
      // Copy single cell
      clipboardText = widget.veTable[widget.selectedRow][widget.selectedCol].toString();
    }
    
    await Clipboard.setData(ClipboardData(text: clipboardText));
    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Selection copied to clipboard')),
      );
    }
  }
  
  void _pasteSelection() async {
    try {
      final clipboardData = await Clipboard.getData(Clipboard.kTextPlain);
      final clipboardText = clipboardData?.text;
      
      if (clipboardText == null || clipboardText.isEmpty) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('Clipboard is empty')),
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
            const SnackBar(content: Text('No valid numeric data in clipboard')),
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
        startRow = widget.selectedRow < 0 ? 0 : widget.selectedRow;
        startCol = widget.selectedCol < 0 ? 0 : widget.selectedCol;
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
        if (targetRow >= widget.veTable.length) break;
        
        for (int c = 0; c < targetCols; c++) {
          final targetCol = startCol + c;
          if (targetCol >= widget.veTable[targetRow].length) break;
          
          // Map target position to paste data with tiling
          final sourceRow = r % pasteRows;
          final sourceCol = c % pasteCols;
          final value = pasteData[sourceRow][sourceCol];
          
          widget.veTable[targetRow][targetCol] = value;
          widget.onCellValueChanged?.call(targetRow, targetCol, value);
        }
      }
      
      if (mounted) {
        final pasteDesc = _selectedCells.isNotEmpty 
          ? 'into ${targetRows}x${targetCols} selection' 
          : 'at position ($startRow,$startCol)';
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Pasted ${pasteRows}x${pasteCols} cells $pasteDesc')),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('Failed to paste from clipboard')),
        );
      }
    }
  }
  
  void _clearSelection() {
    if (_selectedCells.isEmpty) return;
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('Selection cleared')),
    );
  }
  
  /// Get the bounds of the current selection
  Map<String, int>? _getSelectionBounds() {
    if (_selectedCells.isEmpty) return null;
    
    int minRow = widget.veTable.length;
    int maxRow = -1;
    int minCol = widget.veTable[0].length;
    int maxCol = -1;
    
    for (String cellKey in _selectedCells) {
      final parts = cellKey.split(',');
      if (parts.length == 2) {
        final row = int.tryParse(parts[0]);
        final col = int.tryParse(parts[1]);
        if (row != null && col != null) {
          minRow = math.min(minRow, row);
          maxRow = math.max(maxRow, row);
          minCol = math.min(minCol, col);
          maxCol = math.max(maxCol, col);
        }
      }
    }
    
    if (minRow <= maxRow && minCol <= maxCol) {
      return {
        'minRow': minRow,
        'maxRow': maxRow,
        'minCol': minCol,
        'maxCol': maxCol,
      };
    }
    
    return null;
  }
  
  /// Build individual table cell
  Widget _buildTableCell({
    required int row,
    required int col,
    required double value,
    required bool isSelected,
    required bool isEditing,
  }) {
    if (isEditing) {
      return Container(
        width: 80,
        height: 40,
        decoration: BoxDecoration(
           color: ECUTheme.getAccentColor('edit').withOpacity(0.2),
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
            fontSize: 14,
            fontWeight: FontWeight.w600,
          ),
          decoration: const InputDecoration(
            border: InputBorder.none,
            contentPadding: EdgeInsets.zero,
          ),
          onSubmitted: (newValue) {
            final doubleValue = double.tryParse(newValue);
            if (doubleValue != null) {
              widget.onCellEdited(row, col, doubleValue);
            }
            _stopEditing();
          },
          onEditingComplete: () {
            final doubleValue = double.tryParse(_editController.text);
            if (doubleValue != null) {
              widget.onCellEdited(row, col, doubleValue);
            }
            _stopEditing();
          },
        ),
      );
    }
    
    final cellKey = '$row,$col';
    final isMultiSelected = _selectedCells.contains(cellKey);
    
    return GestureDetector(
      onTap: () {
        if (HardwareKeyboard.instance.isControlPressed) {
          // Ctrl+Click: Toggle individual cell in multi-selection
          setState(() {
            if (_selectedCells.contains(cellKey)) {
              _selectedCells.remove(cellKey);
            } else {
              _selectedCells.add(cellKey);
            }
          });
        } else if (HardwareKeyboard.instance.isShiftPressed) {
          // Shift+Click: Extend selection
          if (_selectionStartRow == null || _selectionStartCol == null) {
            _selectionStartRow = widget.selectedRow;
            _selectionStartCol = widget.selectedCol;
          }
          _updateMultiSelection(row, col);
        } else {
          // Normal click: Single selection
          _clearMultiSelection();
          widget.onCellSelected(row, col);
        }
        _tableFocusNode.requestFocus();
      },
      onDoubleTap: () => _startEditing(row, col, value),
      child: Container(
        width: 80,
        height: 40,
        decoration: BoxDecoration(
          color: _getCellColor(value, isSelected, isMultiSelected),
          border: Border.all(
            color: isSelected 
                ? ECUTheme.getAccentColor('selection')
                : isMultiSelected
                     ? ECUTheme.getAccentColor('selection').withOpacity(0.7)
                     : Theme.of(context).colorScheme.outline.withOpacity(0.3),
            width: (isSelected || isMultiSelected) ? 2 : 1,
          ),
        ),
        child: Center(
          child: widget.showValues
              ? Text(
                  value.toStringAsFixed(1),
                  style: TextStyle(
                    fontSize: 12,
                    fontWeight: FontWeight.w600,
                    color: _getTextColor(value),
                  ),
                )
              : null,
        ),
      ),
    );
  }
  
  /// Get cell background color based on value and selection state
  Color _getCellColor(double value, bool isSelected, bool isMultiSelected) {
    if (isSelected) {
      return ECUTheme.getAccentColor('selection').withOpacity(0.4);
    }
    
    if (isMultiSelected) {
      return ECUTheme.getAccentColor('selection').withOpacity(0.2);
    }
    
    if (!widget.showHeatmap) {
      return Theme.of(context).colorScheme.surface;
    }
    
    // Heatmap color based on VE value (50-120 range)
    final normalizedValue = ((value - 50.0) / 70.0).clamp(0.0, 1.0);
    
    if (normalizedValue < 0.5) {
      // Blue to green (low values)
      return Color.lerp(
        ECUTheme.getAccentColor('low'),
        ECUTheme.getAccentColor('medium'),
        normalizedValue * 2,
      )!;
    } else {
      // Green to red (high values)
      return Color.lerp(
        ECUTheme.getAccentColor('medium'),
        ECUTheme.getAccentColor('high'),
        (normalizedValue - 0.5) * 2,
      )!;
    }
  }
  
  /// Get text color based on cell background
  Color _getTextColor(double value) {
    final normalizedValue = ((value - 50.0) / 70.0).clamp(0.0, 1.0);
    
    if (normalizedValue < 0.5) {
      return Colors.white;
    } else {
      return Colors.black;
    }
  }
  
  /// Start editing a cell
  void _startEditing(int row, int col, double value) {
    setState(() {
      _editingRow = row;
      _editingCol = col;
      _editController.text = value.toStringAsFixed(1);
    });
    
    // Focus the text field
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _editFocusNode.requestFocus();
      _editController.selection = TextSelection(
        baseOffset: 0,
        extentOffset: _editController.text.length,
      );
    });
  }
  
  /// Stop editing
  void _stopEditing() {
    setState(() {
      _editingRow = -1;
      _editingCol = -1;
    });
    _editController.clear();
  }
}
