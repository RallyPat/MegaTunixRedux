# NEXT AGENT GUIDE - MegaTunix Redux

## 🎯 **PROJECT OVERVIEW**

**MegaTunix Redux** is a professional ECU tuning software application built with C++/ImGui/SDL2/OpenGL. The project aims to create a comprehensive, cross-platform ECU tuning solution with real-time data visualization, advanced VE table editing, and professional-grade features.

## 🏆 **MAJOR COMPLETED FEATURES**

### ✅ **Real-time Data Visualization** (COMPLETED)
- **Status**: Fully implemented with live gauges, charts, and interactive controls
- **Key Features**:
  - Real-time charts with historical data tracking using circular buffers
  - Multiple gauge types (bar, round, digital, linear) with professional styling
  - Advanced alerting system with configurable thresholds
  - Performance monitoring and statistics (FPS, data points received)
  - Demo data generation for testing without ECU connection
- **Files**: `src/ui/imgui_runtime_display.cpp`, `include/ui/imgui_runtime_display.h`

### ✅ **VE Table Editor** (COMPLETED)
- **Status**: Full implementation with 2D heatmap, 3D view, and table editor
- **Key Features**:
  - 2D heatmap visualization with color-coded VE values
  - 3D view with interactive controls (rotation, zoom, pan)
  - Table editor with direct cell editing
  - Excel-style navigation (Tab forward, Shift+Tab backward)
  - Plus/minus key increment/decrement with real-time visual feedback
- **Files**: `src/main.cpp`, `src/ui/imgui_ve_table.c`, `include/ui/imgui_ve_table.h`

### ✅ **Professional Key Binding System** (COMPLETED)
- **Status**: Professional ECU tuning software-style keybindings with comprehensive legend
- **Key Features**:
  - **Navigation**: Arrow keys, Tab/Shift+Tab for cell navigation
  - **Value Operations**: + and - keys for increment/decrement with configurable amounts
  - **Scale Operations**: * key for percentage scaling with adjustable percentage
  - **Copy/Paste**: Ctrl+C/Ctrl+V for cell copy/paste operations
  - **Interpolation**: I key for two-step interpolation between cells
  - **Visual Feedback**: Real-time status indicators and interpolation mode display
  - **Professional Legend**: Integrated help panel with all keybindings and status
- **Files**: `src/main.cpp`, `include/ui/imgui_key_bindings.h`, `src/ui/imgui_key_bindings.c`

### ✅ **Multi-cell Selection System** (COMPLETED)
- **Status**: Professional multi-cell selection with drag operations
- **Key Features**:
  - **Click and Drag Selection**: Ctrl+Click or Shift+Click to start multi-cell selection
  - **Visual Feedback**: Cyan borders highlight selected cells
  - **Selection State Management**: Proper start/end coordinate tracking
  - **Multi-cell Operations**: Apply operations to entire selection (increment, decrement, scale)
  - **Selection Info Display**: Shows selection bounds and cell count
  - **Clear Selection**: Button to clear both single and multi-cell selections
  - **Professional Integration**: Works seamlessly with existing key binding system
- **Files**: `src/main.cpp` (added multi-cell selection system)

### ✅ **ECU Communication** (COMPLETED)
- **Status**: Robust Speeduino protocol implementation
- **Key Features**:
  - Speeduino CRC binary protocol based on INI file specifications
  - Asynchronous connection with non-blocking UI
  - Real-time data streaming with live statistics
  - Adaptive timing system for optimal communication
- **Files**: `src/ecu/ecu_communication.c`, `include/ecu/ecu_communication.h`

## 🔧 **CURRENT INPUT SYSTEM STATUS**

### ✅ **Recently Improved: Direct Number Entry System**
- **Status**: Working well with some minor inconsistencies
- **Key Features**:
  - **Buffer clearing**: Fixed appending issue (no more "7970" instead of "70")
  - **Visual feedback**: Yellow text shows when typing numbers
  - **Navigation**: Tab/arrow keys work for moving between cells
  - **Auto-apply**: Values are applied when navigating away
  - **Click-to-edit**: Clicking in cells works for detailed editing
- **Implementation**: Uses `g_just_navigated` flag to clear buffer when navigating to new cells
- **Files**: `src/main.cpp` (handle_events function, render_ve_table_2d_view function)

### 🔧 **Key Global Variables for Input System**
```cpp
static char g_cell_edit_buffer[32]; // Buffer for cell editing
static bool g_buffer_updated = false; // Flag for buffer updates
static bool g_input_field_focused = false; // Flag for ImGui input field focus
static bool g_show_input_field = false; // Flag to control input field visibility
static bool g_just_navigated = false; // Flag to track navigation to new cells
```

## 🎯 **IMMEDIATE NEXT PRIORITIES**

### 1. **Data Logging System** 🔥 **RECOMMENDED NEXT**
- **Priority**: HIGH - Diagnostic functionality
- **Description**: Implement comprehensive data logging to files with viewer
- **Tasks**:
  - Implement comprehensive data logging to files
  - Add log file management and rotation
  - Create log viewer with filtering and search
  - Add export functionality (CSV, binary formats)
  - Implement real-time log streaming
- **Files to work on**: New logging module, `src/main.cpp` (render_datalogging_tab)
- **Technical Approach**: 
  - Create logging module with file I/O
  - Add log viewer UI in datalogging tab
  - Implement log filtering and search
  - Add export functionality

### 2. **Advanced VE Table Operations**
- **Priority**: MEDIUM - Enhanced functionality
- **Description**: Implement remaining professional ECU tuning features
- **Tasks**:
  - Implement Set To Value (= key) with input dialog
  - Add Reset operations (R key)
  - Create Undo/Redo system (Z/Y keys)
  - Implement Smoothing algorithms (S key)
  - Add Horizontal/Vertical interpolation (H/V keys)
- **Files to work on**: `src/main.cpp`, `include/ui/imgui_key_bindings.h`
- **Technical Approach**:
  - Add input dialog for Set To Value operation
  - Implement undo/redo system with action history
  - Add smoothing algorithms for table optimization
  - Implement directional interpolation modes

### 3. **Advanced ECU Communication Features**
- **Priority**: MEDIUM - Enhanced functionality
- **Description**: Implement advanced ECU communication features
- **Tasks**:
  - Implement firmware upload/download
  - Add configuration backup/restore
  - Create connection diagnostics
  - Add protocol detection and auto-configuration
  - Implement multi-ECU support
- **Files to work on**: `src/ecu/ecu_communication.c`, communications tab
- **Technical Approach**:
  - Add firmware management functions
  - Implement configuration backup system
  - Create diagnostic tools for connection issues
  - Add protocol auto-detection

## 🏗️ **PROJECT ARCHITECTURE**

### **Core Components**
- **Main Application**: `src/main.cpp` - Main application loop, UI rendering, event handling
- **UI Components**: `src/ui/` - ImGui-based user interface components
- **ECU Communication**: `src/ecu/` - ECU protocol implementation
- **External Libraries**: `external/` - Third-party libraries (ImGui, yaml-cpp)

### **Key Data Structures**
- **VE Table**: `ImguiTable* g_ve_table` - Main VE table data structure
- **Key Bindings**: `KeyBindingState g_key_binding_state` - Key binding state management
- **Multi-cell Selection**: `MultiCellSelection g_multi_selection` - Multi-cell selection state
- **Clipboard**: Global variables for copy/paste operations
- **Selection**: Cell selection state (single cell and multi-cell support)

### **Build System**
- **CMake**: Cross-platform build system
- **Linux Focus**: Primary development platform
- **Dependencies**: SDL2, OpenGL, ImGui, yaml-cpp

## 🔧 **DEVELOPMENT GUIDELINES**

### **Code Style**
- **C++17**: Use modern C++ features where appropriate
- **C Compatibility**: Maintain C compatibility for external libraries
- **Memory Management**: Proper cleanup and resource management
- **Error Handling**: Comprehensive error checking and user feedback

### **UI Development**
- **ImGui**: Use Dear ImGui for all UI components
- **Responsive Design**: UI should adapt to window size changes
- **Professional Look**: Match professional ECU tuning software aesthetics
- **Performance**: Maintain 60 FPS target for real-time displays

### **Testing Strategy**
- **Manual Testing**: Test all features manually before considering complete
- **UI Testing**: Validate user interface behavior
- **Performance Testing**: Ensure real-time data handling works smoothly
- **Cross-platform**: Test on Linux (primary), Windows, and Mac

## 🚨 **KNOWN ISSUES & LIMITATIONS**

### **Input System**
- **Minor Inconsistencies**: Direct number entry works well but may have minor edge cases
- **Buffer Management**: Complex interaction between direct input and ImGui input fields
- **Navigation**: Works reliably but may need refinement for edge cases

### **Performance**
- **Real-time Updates**: Maintains 60 FPS target
- **Memory Usage**: Efficient circular buffers for historical data
- **CPU Usage**: Optimized for real-time operation

### **Compatibility**
- **Linux Focus**: Primary development platform
- **Windows/Mac**: Supported but may need testing
- **ECU Protocols**: Speeduino CRC protocol fully implemented

## 📋 **DEVELOPMENT WORKFLOW**

### **Getting Started**
1. **Build the project**: `cd build_linux && make -j$(nproc)`
2. **Run the application**: `./megatunix-redux`
3. **Test current features**: Navigate to VE Table tab and test keybindings and multi-cell selection
4. **Review code**: Focus on `src/main.cpp` for UI logic and `src/ui/` for components

### **Feature Development**
1. **Choose priority**: Select from immediate next priorities
2. **Plan implementation**: Review existing code patterns
3. **Implement incrementally**: Build and test frequently
4. **Update documentation**: Keep docs current with changes

### **Testing Checklist**
- [ ] Builds successfully on Linux
- [ ] Application launches without crashes
- [ ] All tabs are functional
- [ ] VE table editing works correctly
- [ ] Keybindings function as expected
- [ ] Multi-cell selection works properly
- [ ] Real-time displays update smoothly
- [ ] ECU communication works (if hardware available)

## 🎯 **SUCCESS CRITERIA**

### **For Data Logging System**
- [ ] Data is logged to files correctly
- [ ] Log viewer displays data properly
- [ ] Filtering and search work
- [ ] Export functionality works
- [ ] Real-time log streaming functions

### **For Advanced VE Table Operations**
- [ ] Set To Value dialog works correctly
- [ ] Reset operations function properly
- [ ] Undo/Redo system tracks actions
- [ ] Smoothing algorithms work
- [ ] Directional interpolation functions

## 🚀 **READY TO CONTINUE**

The project is in excellent shape with four major feature sets completed:
1. **Real-time Data Visualization** ✅
2. **VE Table Editor** ✅  
3. **Professional Key Binding System** ✅
4. **Multi-cell Selection System** ✅

The codebase is well-structured, documented, and ready for continued development. The next agent should choose from the immediate priorities based on user needs and project goals.

**Recommended next focus**: Data Logging System for comprehensive diagnostic capabilities, followed by Advanced VE Table Operations for enhanced functionality.

---

**Last Updated**: August 2025 - Multi-cell Selection System completed
**Next Priority**: Data Logging System for comprehensive diagnostic capabilities 