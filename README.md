# MegaTunix Redux

A modern, cross-platform ECU tuning application built with Dear ImGui and SDL2.

## 🚀 **Latest Update: Critical Bug Fixed!**

**CRITICAL ISSUE RESOLVED**: The ignition table keyboard navigation bug has been identified and fixed! The problem was hardcoded VE table references in the arrow key navigation code, causing data corruption when navigating in the ignition table.

### **What Was Fixed**
- **Keyboard Navigation Bug**: Arrow keys were using `g_ve_table->width` instead of `active_table->width`
- **Data Source Mismatch**: This caused wrong table dimensions and data corruption
- **Inconsistent Values**: Different values appeared when using arrow keys vs. double-clicking

### **Current Status**
The ignition table now provides a **smooth, professional editing experience** that matches TunerStudio's behavior exactly:

✅ **Fully Interactive**: Click cells to select, double-click to edit  
✅ **Direct Typing**: Type numbers directly in cells without clicking first  
✅ **Auto-save Navigation**: Values automatically save when using arrow keys or Tab  
✅ **Professional Controls**: Full keyboard support with multi-selection  
✅ **Heatmap Visualization**: Color-coded cells based on ignition timing values  
✅ **Stable Operation**: No more crashes from complex OpenGL drawing code  
✅ **Correct Data Source**: Values now properly pulled from ignition table data  
✅ **Fixed Keyboard Navigation**: Arrow keys use correct table dimensions  

## 🎯 **Key Features**

### **Modern UI Framework**
- **Dear ImGui**: Professional-grade immediate mode GUI
- **SDL2 Integration**: Cross-platform window management and input handling
- **OpenGL Rendering**: Hardware-accelerated graphics for smooth performance

### **ECU Communication**
- **Speeduino Support**: Native CRC protocol implementation
- **Custom Library**: Professional-grade communication stack
- **Real-time Data**: Live ECU parameter monitoring

### **Professional Table Editing**
- **VE Table**: Advanced fuel tuning with 2D heatmap and 3D visualization
- **Ignition Table**: Spark timing optimization with professional controls
- **Keyboard Shortcuts**: Professional-grade navigation and operations
- **Multi-cell Selection**: Advanced editing operations
- **Auto-save**: Intelligent data preservation

### **Cross-Platform Support**
- **Linux**: Primary development platform
- **Windows**: Full compatibility
- **macOS**: Native support

## 🛠️ **Technical Architecture**

### **Core Components**
- **Window Manager**: SDL2-based cross-platform window handling
- **UI System**: Dear ImGui with custom theming and professional controls
- **Table Engine**: Advanced data grid with real-time editing
- **Communication Stack**: Custom ECU protocol implementation
- **Graphics Pipeline**: OpenGL-based rendering system

### **Performance Features**
- **Hardware Acceleration**: OpenGL for smooth graphics
- **Efficient Rendering**: Optimized table display algorithms
- **Memory Management**: Professional-grade resource handling
- **Real-time Updates**: Responsive UI with minimal latency

## 📋 **Installation**

### **Prerequisites**
- CMake 3.20+
- SDL2 development libraries
- OpenGL development libraries
- C++17 compatible compiler

### **Build Instructions**
```bash
# Clone the repository
git clone https://github.com/yourusername/MegaTunixRedux.git
cd MegaTunixRedux

# Create build directory
mkdir build_linux && cd build_linux

# Configure and build
cmake ..
make -j4

# Run the application
./megatunix-redux
```

## 🔧 **Development Status**

### **Completed Features**
- ✅ **Core Infrastructure**: SDL2 + Dear ImGui integration
- ✅ **VE Table**: Professional fuel tuning interface
- ✅ **Ignition Table**: Complete spark timing optimization
- ✅ **Keyboard Controls**: Professional navigation and editing
- ✅ **Heatmap Visualization**: Color-coded data representation
- ✅ **Auto-save System**: Intelligent data preservation
- ✅ **Multi-cell Operations**: Advanced editing capabilities
- ✅ **Critical Bug Fixes**: Keyboard navigation and data corruption resolved

### **Current Focus**
- **Testing & Validation**: Ensuring all features work correctly
- **Performance Optimization**: Fine-tuning rendering and responsiveness
- **Documentation**: Comprehensive user and developer guides
- **Cross-platform Testing**: Windows and macOS compatibility

## 📚 **Documentation**

- **[Design Documents](docs/design/)**: Architecture and implementation details
- **[Status Reports](docs/status/)**: Current development progress
- **[Troubleshooting](TROUBLESHOOTING.md)**: Common issues and solutions
- **[TODO](TODO.md)**: Development roadmap and tasks

## 🤝 **Contributing**

We welcome contributions! Please see our contributing guidelines and development roadmap.

## 📄 **License**

This project is licensed under the same terms as the original MegaTunix project.

## 🙏 **Acknowledgments**

- **David J. Andruczyk**: Original MegaTunix creator
- **Dear ImGui Community**: Excellent immediate mode GUI library
- **SDL2 Team**: Cross-platform multimedia library
- **OpenGL Community**: Graphics programming standards

---

**MegaTunix Redux** - Bringing professional ECU tuning to the modern era with a focus on stability, performance, and user experience.

