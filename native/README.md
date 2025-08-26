# Native Library for Flutter FFI Integration

This directory contains native C++ code optimized for Flutter FFI integration.

## Purpose

Provide high-performance native implementations for:
- ECU communication protocols (Speeduino, MegaSquirt, etc.)
- Real-time data streaming and processing
- Hardware-level operations that benefit from native code

## Structure

```
native/
├── ecu_communication/          # ECU protocol implementations
│   ├── speeduino_protocol.c    # Speeduino-specific protocol
│   ├── megasquirt_protocol.c   # MegaSquirt protocols
│   └── ecu_base.c              # Common ECU functionality
├── data_bridge/                # Real-time data streaming
│   ├── stream_manager.cpp      # Data streaming management
│   └── bridge_ffi.cpp          # FFI bridge implementation
├── include/                    # Public FFI headers
│   ├── megatunix_native.h      # Main FFI interface
│   ├── ecu_ffi.h              # ECU FFI definitions
│   └── stream_ffi.h           # Streaming FFI definitions
├── CMakeLists.txt             # Native library build
└── README.md                  # This file
```

## Usage from Flutter

### 1. Build Native Library
```bash
cd native
mkdir build && cd build
cmake ..
make
```

### 2. Flutter FFI Integration
```dart
import 'dart:ffi';
import 'package:ffi/ffi.dart';

class NativeECU {
  late DynamicLibrary _nativeLib;
  
  void initialize() {
    _nativeLib = DynamicLibrary.open('libmegatunix_native.so');
    // Bind native functions...
  }
}
```

## Development Status

**PLANNED** - This structure is ready for implementation when FFI integration begins.

## Integration with Flutter App

The Flutter application in `megatunix_flutter/` will use this native library via FFI for:
- Real ECU hardware communication
- High-performance data processing
- Platform-specific optimizations

## Build Requirements

- CMake 3.16+
- C++17 compiler
- Compatible with Flutter FFI requirements