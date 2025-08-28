# AGENTS.md - Development Guide for MegaTunix Redux

## Build/Lint/Test Commands
- **Flutter Build**: `cd megatunix_flutter && flutter build linux` (or windows/macos)
- **Run Development**: `cd megatunix_flutter && flutter run -d linux` (with hot reload)
- **Test All**: `cd megatunix_flutter && flutter test`
- **Test Single**: `cd megatunix_flutter && flutter test test/specific_test.dart`
- **Integration Test**: `./test_serial_simple.sh` and `./test_speeduino.sh` (ECU hardware tests)
- **Lint**: `cd megatunix_flutter && flutter analyze`
- **Native Build**: `mkdir build && cd build && cmake .. && make`
- **Clean**: `cd megatunix_flutter && flutter clean && flutter pub get`

## Code Style Guidelines
- **Language**: Dart (Flutter) + C/C++17, strong type system with null safety
- **Imports**: Relative (`../`) for local files, absolute (`package:`) for packages
- **Naming**: camelCase variables/functions, PascalCase classes, SCREAMING_SNAKE_CASE constants
- **Formatting**: Single quotes preferred, const constructors, trailing commas
- **Error Handling**: try-catch blocks, async/await patterns, Result types for operations
- **State Management**: Provider pattern, ChangeNotifier for reactive updates
- **Performance**: const constructors, optimized collections, efficient rendering
- **Comments**: `///` for public API docs, detailed implementation comments
- **Linting**: flutter_lints enabled, strict analysis_options.yaml rules

## Cursor/Copilot Rules
- No Cursor rules (.cursorrules) or Copilot rules (.github/copilot-instructions.md) configured
- Follow standard Flutter/Dart best practices and analysis_options.yaml settings

## ECU Integration Features - COMPLETED ✅

### **Speeduino Protocol Implementation**
- **CRC Protocol Support**: Full CRC32 calculation and validation for data integrity
- **ASCII Fallback**: Robust fallback to ASCII commands when CRC fails
- **Real-Time Communication**: Live data streaming from Speeduino UA4C ECU
- **Hardware Integration**: Direct serial port communication with ECU hardware
- **Protocol Detection**: Automatic detection of ECU type and protocol

### **Serial Communication System**
- **Multi-Protocol Support**: Speeduino, MegaSquirt, EpicECU protocol handlers
- **Robust Connection Management**: Automatic reconnection and error recovery
- **Platform Abstraction**: Cross-platform serial port access (Linux/Windows/macOS)
- **Performance Optimized**: Efficient data buffering and processing
- **Error Handling**: Comprehensive error reporting and recovery

### **Live Data Integration**
- **Real-Time Parameters**: RPM, MAP, TPS, coolant temp, battery voltage, AFR, timing
- **Data Validation**: CRC checking and data integrity verification
- **Stream Processing**: Efficient real-time data streaming and display
- **Performance Monitoring**: Connection quality and data rate statistics
- **User Feedback**: Clear status indicators and connection state display

## Professional Table Editor Features - PLANNED 🔄

### **Complete TunerStudio Ultra Parity**
- **Navigation**: Arrow keys, Shift+Arrow selection (Excel-style), Ctrl+Arrow multi-select
- **Professional Shortcuts**: F2, Ctrl+I/H/V (interpolation), Ctrl+S (smoothing), Ctrl+A (select all)
- **Clipboard Integration**: Ctrl+C (copy), Ctrl+Shift+V (paste) - TunerStudio standard
- **Advanced Operations**: 2D interpolation, horizontal/vertical interpolation, smoothing filters
- **Professional Workflow**: Matches TunerStudio muscle memory and keyboard shortcuts

### **Real-time Cursor System**
- **Update Rate**: 10Hz real-time data updates for smooth cursor movement
- **Trail Duration**: 3-second neon pink (#FF1493) fade trail showing engine operation history
- **Anti-Jitter**: Exponential smoothing filter (0.4 factor) with movement thresholds
- **Performance**: Optimized rendering with 200 max trail points and efficient memory management
- **Integration**: Seamless with table editing - cursor overlay uses IgnorePointer for interactions

### **Intelligent Table Scaling**
- **Automatic Sizing**: Tables scale to fit any viewport size (tested up to 32x32)
- **Proportional Scaling**: Headers, fonts, and cells scale together maintaining readability
- **Size Strategies**: Different scaling rules for small (<16x16) vs large (>16x16) tables
- **Performance**: Efficient layout calculation with viewport-aware optimization

### **Professional Heatmap System**
- **TunerStudio Colors**: Industry-standard thermal gradient (blue → red)
- **Table-Specific**: Different color schemes based on table type (VE, fuel, timing, etc.)
- **Dynamic Range**: Automatic value range detection with fallback to table definition ranges
- **High Contrast**: Optimized text colors for readability across thermal ranges

## Current Architecture Notes

### **Core Components**
- **ECUService**: Multi-protocol ECU communication with automatic detection
- **SpeeduinoProtocolHandler**: Full CRC protocol implementation for Speeduino UA4C
- **SerialServiceFile**: Robust serial port communication with error recovery
- **ECUConnectionPanel**: Professional connection management interface
- **ECUStatusPanel**: Real-time connection status and statistics display
- **DashboardView**: Beautiful automotive-themed main interface

### **Performance Characteristics**
- **Response Time**: <16ms for all user interactions (professional standard)
- **Update Rate**: Real-time ECU data streaming with live parameter updates
- **Memory Usage**: Optimized for continuous data streaming and UI updates
- **Connection Reliability**: Automatic reconnection and error recovery
- **Cross-Platform**: Consistent performance across Linux, Windows, macOS

### **Professional Quality Standards**
- **ECU Protocol Support**: Speeduino CRC protocol with ASCII fallback
- **Real-Time Communication**: Live data streaming from ECU hardware
- **Error Handling**: Comprehensive error reporting and recovery mechanisms
- **User Experience**: Clear status indicators and intuitive connection management
- **Code Quality**: Modern Flutter architecture with detailed documentation

## Development Workflow

### **Code Quality Standards**
- **Comprehensive Documentation**: All classes and methods have detailed comments
- **Type Safety**: Full Dart type system usage with null safety
- **Error Handling**: Graceful degradation with user feedback
- **Performance**: Optimized rendering and memory management
- **Testing**: Unit tests and integration tests with real ECU hardware

### **Professional Features Implemented**
1. **ECU Communication**: Full Speeduino protocol with CRC support
2. **Real-Time Data**: Live RPM, MAP, TPS, and sensor data streaming
3. **Serial Integration**: Robust hardware communication with error recovery
4. **Connection Management**: Professional ECU connection interface
5. **Status Monitoring**: Real-time connection quality and statistics
6. **Cross-Platform UI**: Beautiful automotive-themed interface

## Next Development Phases

### **Phase 3: Table Editors & Tuning Tools** (Q1 2025)
- Professional VE table editor with 3D visualization
- Ignition timing table editor with real-time feedback
- AFR table tuning with lambda overlay
- Table interpolation and smoothing tools
- Real-time table validation and error checking

### **Phase 4: Advanced Data Analysis** (Q2 2025)
- Professional data logging with high-speed acquisition
- Advanced charting and graphing with multiple chart types
- Statistical analysis tools and reporting capabilities
- Data export/import in multiple formats
- Performance benchmarking and comparison tools

### **Phase 5: ECU Management & Diagnostics** (Q3 2025)
- Firmware management with backup and restore
- Advanced configuration tools with templates
- Professional diagnostics with error code management
- Configuration validation and testing tools
- ECU health monitoring and maintenance alerts

## Quality Assurance

### **Current Status: Professional Grade**
- **Feature Completeness**: Complete ECU integration with Speeduino protocol support
- **Performance**: Real-time data streaming with robust error handling
- **User Experience**: Professional automotive interface with clear status indicators
- **Code Quality**: Modern Flutter architecture with comprehensive documentation
- **Hardware Integration**: Reliable serial communication with ECU hardware
- **Cross-platform**: Consistent experience across Linux, Windows, macOS

### **Testing & Validation**
- **Unit Tests**: Core functionality with comprehensive test coverage
- **Integration Tests**: Real ECU hardware validation (Speeduino UA4C confirmed working)
- **Protocol Tests**: CRC and ASCII communication validation
- **Performance Tests**: Real-time data streaming and UI responsiveness
- **User Acceptance**: Professional tuner workflow validation

---

**Current Achievement**: Complete ECU integration system with Speeduino protocol support, real-time data streaming, and professional automotive interface. Ready for table editors and advanced tuning features.