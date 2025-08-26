# Legacy C++ Implementation

This directory contains the original C++ implementation of MegaTunix Redux that has been superseded by the modern Flutter implementation in `megatunix_flutter/`.

## Contents

- `src/` - Original C++ source code
- `include/` - Original C++ header files  
- `CMakeLists.txt` - Original CMake build configuration

## Status

**ARCHIVED** - This implementation is no longer the primary codebase.

## Useful Components for Integration

The following components from this legacy implementation are candidates for FFI integration with the Flutter app:

### ECU Communication
- `src/ecu/ecu_communication.c` - Working Speeduino protocol implementation
- `src/ecu/ecu_ini_parser.c` - ECU configuration parsing
- `include/ecu/ecu_communication.h` - ECU communication interfaces

### Data Bridge
- `src/core/data_bridge.cpp` - High-performance real-time data streaming
- `include/core/data_bridge.h` - Data bridge interfaces

### Plugin System
- `src/plugin/plugin_manager.cpp` - Plugin architecture
- `include/plugin/plugin_interface.h` - Plugin interfaces

## Migration Notes

The Flutter implementation in `megatunix_flutter/` provides:
- ✅ Modern Material Design 3 UI
- ✅ Cross-platform support (Linux, Windows, macOS)
- ✅ Professional automotive theming
- ✅ Real-time data visualization
- ✅ Clean architecture with Dart/Flutter conventions

This legacy C++ code can be integrated via Flutter's FFI (Foreign Function Interface) to provide native ECU communication capabilities.

## Build Instructions (Legacy)

**Note**: This is for reference only. The main application is now the Flutter version.

```bash
mkdir build_linux && cd build_linux
cmake ..
make -j$(nproc)
```

## Do Not Use for New Development

All new development should be done in the Flutter application at `megatunix_flutter/`.

This legacy code is preserved for:
1. Reference and documentation purposes
2. Potential FFI integration components
3. Historical preservation of the original implementation