# Live Cursor System Documentation

## Overview
The MegaTunix Redux live cursor system provides TunerStudio-style real-time cursor positioning within tuning tables, complete with fade trail animation showing recent engine operating history.

## Architecture

### Core Components
- **`TableCursor`**: Main cursor state management with smoothing and trail tracking
- **`TableCursorPosition`**: Interpolated position calculation from ECU values
- **`CursorTrailPoint`**: Individual trail point with fade opacity
- **`TableCursorOverlay`**: Flutter widget for cursor rendering
- **`RealtimeDataService`**: Data provider with engine simulation

### Key Files
```
lib/models/table_cursor.dart           # Cursor positioning and trail logic
lib/widgets/table_cursor_overlay.dart  # Cursor rendering and visualization
lib/services/realtime_data_service.dart # Real-time data streaming
lib/views/tables_view.dart             # Integration with table editor
```

## Features

### Real-Time Positioning
- **Interpolated positioning** between table cells based on ECU values (RPM, MAP)
- **Sub-cell accuracy** with fractional coordinates for smooth movement
- **10Hz update rate** for responsive cursor tracking

### Trail Animation
- **3-second fade trail** showing recent engine operation history
- **Gradual opacity fade** from current position to trail end
- **200 max trail points** for performance optimization
- **Time-based expiry** removes old positions automatically

### Smoothing & Anti-Jitter
- **Exponential smoothing filter** (0.4 smoothing factor) for stable movement
- **Minimum movement threshold** (0.03 cells) prevents micro-jitter
- **Dual position system** - raw data for accuracy, smoothed for display

### Artifact Prevention
- **Distance filtering** - only connects points ≤1.5 cells apart
- **Time filtering** - only connects points ≤200ms apart
- **Natural gaps** when cursor jumps far distances (no straight lines)

## Configuration

### Trail Parameters
```dart
const Duration trailDuration = Duration(seconds: 3);  // Trail fade time
const int maxTrailPoints = 200;                      // Performance limit
const double maxConnectionDistance = 1.5;            // Max distance to draw lines
const int maxConnectionTime = 200;                   // Max time to draw lines (ms)
```

### Smoothing Parameters
```dart
const double smoothingFactor = 0.4;     // Higher = more responsive
const double minMovement = 0.03;        // Jitter prevention threshold
```

### Visual Parameters
```dart
const Color cursorColor = Color(0xFFFF1493);  // Neon pink
const double trailOpacity = 0.7;              // Max trail opacity
const double strokeWidth = 3.0;               // Trail line width
```

## Usage

### Basic Implementation
```dart
// In table view widget
TableCursorOverlay(
  cursor: realtimeService.getTableCursor('VE_TABLE'),
  rows: veTable.rows,
  cols: veTable.cols,
  cellWidth: cellWidth,
  cellHeight: cellHeight,
  enabled: cursorEnabled,
)
```

### Toggle Control
```dart
CursorToggleButton(
  enabled: cursorEnabled,
  onToggle: () => setState(() => cursorEnabled = !cursorEnabled),
)
```

### Info Display
```dart
CursorInfoWidget(
  cursor: cursor,
  tableName: 'VE Table',
  currentRpm: '${rpm.toInt()}',
  currentLoad: '${map.toInt()}',
)
```

## Technical Details

### Position Interpolation
The cursor calculates fractional positions between table cells:
```dart
// Example: RPM=2500 between axis points [2000, 3000] = position 2.5
final xPos = _interpolatePosition(rpmValue, rpmAxis);
final yPos = _interpolatePosition(mapValue, mapAxis);
```

### Trail Rendering
Trail segments are rendered individually with distance checks:
```dart
// Only draw lines between nearby points
if (distance <= 1.5 && timeDiff <= 200) {
  canvas.drawLine(startPos, endPos, paint);
}
```

### Memory Management
- Trail automatically expires old positions (>3 seconds)
- Limited to 200 trail points maximum for performance
- Efficient gap detection prevents memory buildup

## Performance Characteristics
- **Update Rate**: 10Hz (100ms intervals)
- **Trail Length**: ~30 points typical (3 seconds × 10Hz)  
- **Memory Usage**: <1MB for cursor system
- **CPU Impact**: <2% additional load during active cursor movement
- **Rendering**: 60fps smooth animation regardless of data rate

## Future Enhancements
- Configurable trail length and color schemes
- Multi-table cursor support with different colors
- Trail persistence across table switches
- Export trail data for analysis
- Integration with data logging system