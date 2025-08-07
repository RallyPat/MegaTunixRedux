# DESIGN DOCUMENT - MegaTunix Redux

## 🎯 **Project Overview**

MegaTunix Redux is a modern, cross-platform ECU tuning application built with Dear ImGui, SDL2, and OpenGL. The application provides professional-grade ECU communication, real-time data visualization, and advanced table editing capabilities.

## 🏗️ **Architecture Overview**

### **Core Technologies**
- **Language**: C/C++ with C++17 features
- **UI Framework**: Dear ImGui with SDL2
- **Graphics**: OpenGL for rendering
- **Communication**: Custom ECU library with Speeduino CRC protocol
- **Build System**: CMake for cross-platform compilation

### **Key Components**

#### **Main Application (`src/main.cpp`)**
- Application entry point and UI orchestration
- Event handling and SDL integration
- Tab-based interface management
- Real-time data processing and visualization

#### **UI Components (`src/ui/`)**
- **ImGui Runtime Display** (`imgui_runtime_display.cpp`): Real-time visualization with gauges and charts
- **ImGui VE Table** (`imgui_ve_table.c`): 2D/3D table visualization and editing
- **ImGui Communications** (`imgui_communications.cpp`): Communication status and controls
- **ImGui File Dialog** (`imgui_file_dialog.cpp`): File operations interface
- **ImGui Key Bindings** (`imgui_key_bindings.c`): Professional keybind system

#### **ECU Communication (`src/ecu/`)**
- **ECU Communication** (`ecu_communication.c`): Protocol implementation
- **ECU Dynamic Protocols** (`ecu_dynamic_protocols.c`): Adaptive communication timing
- **ECU INI Parser** (`ecu_ini_parser.c`): Configuration file parsing

## 🎨 **UI Design Philosophy**

### **Professional Interface**
- Clean, modern design inspired by commercial tuning software
- Consistent color scheme and typography
- Responsive layout that adapts to window size
- Intuitive navigation with tab-based organization

### **Real-time Visualization**
- Live gauges with configurable ranges and color-coded zones
- Historical data charts with circular buffer implementation
- Performance monitoring with FPS and data point statistics
- Alert system with configurable thresholds

### **Advanced Table Editing**
- 2D heatmap visualization with color-coded values
- 3D view with interactive controls (rotation, zoom, pan)
- Direct cell editing with keyboard input
- Professional keybind system with integrated legend

## 🔧 **Technical Implementation**

### **Real-time Data Visualization**

#### **Data Management**
```cpp
struct DataSeries {
    float data[MAX_DATA_POINTS];
    int head;
    int count;
    bool full;
};
```

#### **Gauge System**
- Multiple gauge types (bar, round, digital, linear)
- Configurable ranges and color zones
- Professional styling with gradients and shadows
- Real-time value updates with smooth animations

#### **Chart System**
- Historical data tracking with circular buffers
- Performance-optimized rendering (500 point limit)
- Axis labels and subtitles
- Throttled updates for optimal performance

### **VE Table Editor**

#### **2D Visualization**
- Heatmap with color-coded VE values
- Professional color legend
- Axis labels on all four sides
- Mouse interaction for cell selection

#### **3D Visualization**
- OpenGL-based 3D rendering
- Interactive controls (rotation, zoom, pan)
- Texture-based rendering for ImGui integration
- Proper 3D transformations with rotation matrices

#### **Professional Keybind System**
- Plus/minus key increment/decrement with configurable amounts
- Asterisk (*) key for percentage-based scaling
- Auto-exit text fields and immediate action application
- Integrated legend panel with responsive design
- Interactive sliders for parameter adjustment

### **ECU Communication**

#### **Speeduino Protocol**
- Full CRC binary protocol implementation
- Based on INI file specifications (202501.4.ini)
- Adaptive timing system for optimal performance
- Comprehensive error handling and recovery

#### **Connection Management**
- Asynchronous connection handling
- Non-blocking UI during connection attempts
- Real-time connection status feedback
- Automatic protocol detection and configuration

## 📊 **Performance Considerations**

### **Real-time Processing**
- 60 FPS target for all visualizations
- Throttled updates when ECU connected (every 2 frames)
- Efficient data point storage with `memmove`
- Limited chart processing to 500 points

### **Memory Management**
- Proper cleanup and resource management
- Circular buffers for historical data
- Unique IDs for ImGui elements to prevent conflicts
- Safe ImGui calls only during rendering phase

### **Cross-platform Compatibility**
- Linux-focused development
- Windows and Mac support via CMake
- SDL2 for platform abstraction
- OpenGL for graphics rendering

## 🎯 **User Experience Design**

### **Professional Workflow**
- Intuitive tab-based navigation
- Consistent keyboard shortcuts
- Real-time feedback for all operations
- Comprehensive help system with integrated legend

### **Accessibility**
- High contrast color schemes
- Configurable font sizes
- Keyboard navigation support
- Clear visual feedback for all actions

### **Error Handling**
- Comprehensive error checking
- User-friendly error messages
- Graceful degradation on failures
- Recovery mechanisms for communication issues

## 🚀 **Future Architecture Considerations**

### **Modular Design**
- Component-based architecture for easy extension
- Plugin system for additional ECU protocols
- Configurable UI layouts
- Extensible data visualization system

### **Scalability**
- Support for multiple ECU connections
- Distributed processing capabilities
- Cloud-based configuration storage
- Multi-user collaboration features

### **Advanced Features**
- AI-driven table optimization (lowest priority)
- Machine learning for tuning recommendations
- Automated safety validation
- Professional reporting and analysis tools

## 📋 **Development Guidelines**

### **Code Quality**
- Comprehensive error handling
- Performance optimization for real-time operations
- Cross-platform compatibility
- Professional documentation and comments

### **Testing Strategy**
- Unit testing for core functionality
- Integration testing for ECU communication
- UI testing for user interface validation
- Performance testing for real-time operations

### **Documentation Standards**
- Inline code documentation
- Architecture documentation
- User interface design documentation
- API documentation for external interfaces

---

**Last Updated**: August 2025 - Design reflects completed Real-time Data Visualization and VE Table Editor with Professional Keybinds
**Status**: Major milestones completed, ready for next phase of development
