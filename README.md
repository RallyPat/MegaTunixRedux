# MegaTunix Redux

A modern, cross-platform ECU tuning application built with Dear ImGui, SDL2, and OpenGL. Designed for Speeduino and other ECU platforms with professional-grade features.

**Author**: Patrick Burke  
**Based on**: Original MegaTunix by David J. Andruczyk

## 🚀 **Current Status: PROFESSIONAL ECU TUNING SOFTWARE PARITY ACHIEVED**

### ✅ **Completed Major Features**

#### **🏆 Real-time Data Visualization** ✅ **FULLY OPERATIONAL**
- **Live Gauges**: Professional round, bar, digital, and linear gauges
- **Real-time Charts**: Historical data tracking with configurable time ranges
- **Advanced Alerting**: Configurable thresholds with visual and audio alerts
- **Performance Monitoring**: FPS tracking and data point statistics
- **Scrollable Interface**: Responsive layout with proper padding
- **Demo Data**: Test functionality without ECU connection

#### **🏆 VE Table Editor with Professional Keybindings** ✅ **FULLY OPERATIONAL**
- **2D Heatmap**: Color-coded visualization with direct cell editing
- **3D View**: Interactive wireframe with rotation, zoom, and pan controls
- **Table Editor**: Direct spreadsheet-style editing with Excel navigation
- **Professional Keybindings**: +, -, *, I keys for value operations and interpolation
- **Copy/Paste**: Ctrl+C/Ctrl+V for cell operations
- **Visual Feedback**: Real-time status indicators and interpolation mode display
- **Professional Legend**: Integrated help panel with all keybindings and status
- **Configuration**: Adjustable increment amounts and scaling percentages
- **Input System**: Reliable direct number entry with visual feedback

#### **🏆 ECU Communication** ✅ **FULLY OPERATIONAL**
- **Speeduino Protocol**: Full CRC binary protocol implementation
- **Asynchronous Connection**: Non-blocking UI with real-time feedback
- **Adaptive Timing**: Self-optimizing communication timing
- **Real-time Streaming**: Continuous data flow with live statistics
- **In-UI Logging**: Real-time application and communication logs

## 🎨 **Features**

### **Professional UI Framework**
- **Dear ImGui Integration**: Modern, responsive interface
- **Cross-platform**: Linux, Windows, macOS support
- **Dark Theme**: Professional appearance with consistent styling
- **12 Functional Tabs**: Complete MegaTunix interface

### **Real-time Data Visualization**
- **Multiple Gauge Types**: Round, bar, digital, linear with customization
- **Historical Charts**: Circular buffer-based data tracking
- **Alert System**: Configurable thresholds with visual feedback
- **Performance Metrics**: FPS monitoring and data point tracking
- **Responsive Layout**: Scrollable content with proper padding

### **VE Table Editor**
- **2D Heatmap**: Color-coded VE value visualization
- **3D Wireframe**: Interactive 3D view with mouse controls
- **Direct Editing**: Click-to-edit cells with keyboard input
- **Excel Navigation**: Tab/Shift+Tab for cell navigation
- **Plus/Minus Controls**: Real-time value adjustment
- **Professional Features**: Axis labels, color legends, status info

### **ECU Communication**
- **Speeduino Support**: Full CRC binary protocol
- **Asynchronous Operation**: Non-blocking connection handling
- **Adaptive Timing**: Self-learning timeout optimization
- **Real-time Statistics**: Live monitoring of communication
- **Error Handling**: Comprehensive error tracking and recovery

## 🛠️ **Building**

### **Prerequisites**
- CMake 3.10 or higher
- SDL2 development libraries
- OpenGL development libraries
- TTF development libraries

### **Linux Build**
```bash
cd /home/pat/GitHubRepos/MegaTunixRedux
mkdir -p build_linux
cd build_linux
cmake ..
make
```

### **Running**
```bash
cd build_linux
./megatunix-redux
```

## 📋 **Development Status**

### **Completed Milestones**
- ✅ **UI Framework**: Complete Dear ImGui integration with professional styling
- ✅ **ECU Communication**: Robust Speeduino CRC protocol implementation
- ✅ **Real-time Visualization**: Professional gauges, charts, and alerting system
- ✅ **VE Table Editor**: Complete 3D visualization with professional keybindings
- ✅ **Professional Keybindings**: Professional ECU tuning software-style keybindings with legend
- ✅ **Input System**: Reliable direct number entry with visual feedback
- ✅ **Cross-platform**: Linux-focused with Windows/Mac support

### **Next Priorities**

#### **🔥 Immediate Next Steps**
1. **Multi-cell Selection System**: Click and drag selection for rectangular areas in VE table
2. **Data Logging System**: Comprehensive data logging to files with viewer
3. **Advanced VE Table Operations**: Set To Value, Reset, Undo/Redo, Smoothing algorithms

#### **📋 Planned Features**
- **Advanced ECU Features**: Firmware management, configuration backup/restore
- **Enhanced Visualization**: Dashboard designer, custom gauge types
- **Configuration Management**: Settings persistence, user preferences, themes
- **Agentic Autotuning**: AI-driven optimization (lowest priority)

## 📄 **License**

MegaTunix Redux is based on the original MegaTunix project and is licensed under the **GNU General Public License v2** (GPL v2). See the `LICENSE` file for the complete license text.

### **Third-Party Dependencies**

This project uses several third-party libraries:

- **Dear ImGui v1.92.2**: MIT License - Copyright (c) 2014-2025 Omar Cornut
- **SDL2**: zlib License - Copyright (c) 1997-2024 Sam Lantinga  
- **SDL2_ttf**: zlib License - Copyright (c) 2001-2024 Sam Lantinga
- **libserialport**: LGPL v3 - Copyright (c) 2013-2014 Uwe Hermann
- **zlib**: zlib License - Copyright (c) 1995-2024 Jean-loup Gailly and Mark Adler

For complete license attribution and compliance information, see `LICENSE_ATTRIBUTION.md`.

## 🎯 **Technical Architecture**

### **Core Technologies**
- **Language**: C/C++ with C++17 features
- **UI Framework**: Dear ImGui with SDL2
- **Graphics**: OpenGL for rendering
- **Communication**: Custom ECU library
- **Build System**: CMake for cross-platform compilation

### **Key Components**
- **Main Application**: `src/main.cpp` - Application entry point and UI orchestration
- **UI Components**: `src/ui/` - ImGui-based interface components
- **ECU Communication**: `src/ecu/` - ECU protocol implementation
- **Runtime Display**: `src/ui/imgui_runtime_display.cpp` - Real-time visualization
- **VE Table**: `src/ui/imgui_ve_table.c` - Table editing and visualization

## 🚀 **Getting Started**

1. **Clone the repository**
2. **Install dependencies** (SDL2, OpenGL, TTF)
3. **Build the project** using CMake
4. **Run the application** and connect to your ECU
5. **Explore the features**:
   - Use the Runtime Display tab for real-time monitoring
   - Use the VE Table Editor for fuel map tuning
   - Use the Communications tab for connection management

## 📖 **Documentation**

- **CURRENT_STATUS_ANALYSIS.md**: Comprehensive current status overview
- **NEXT_AGENT_GUIDE.md**: Development guide for contributors
- **TODO_LIST.md**: Detailed task list and priorities
- **CROSS_PLATFORM_IMPLEMENTATION_SUMMARY.md**: Technical implementation details
- **CLEANUP_SUMMARY.md**: Code organization and cleanup status
- **LICENSE_ATTRIBUTION.md**: Third-party license compliance

## 🤝 **Contributing**

This project follows a structured development approach with clear milestones and documentation. See `NEXT_AGENT_GUIDE.md` for development guidelines and current priorities.

## 📄 **License**

[License information to be added]

---

**MegaTunix Redux** - Professional ECU tuning software with modern UI and robust communication capabilities.

