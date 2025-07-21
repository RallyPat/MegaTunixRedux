# MegaTunix Redux

**Modern ECU Tuning Software - Next Generation Architecture**

*Originally created by David J. Andruczyk*  
*Redux version by Pat Burke with GitHub Copilot assistance (2025)*

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]() [![SDL2](https://img.shields.io/badge/renderer-SDL2-blue.svg)]() [![Clay UI](https://img.shields.io/badge/ui-Clay-orange.svg)]() [![License](https://img.shields.io/badge/license-GPL--2.0-red.svg)]()

## 🚀 Overview

MegaTunix Redux is a complete ground-up rewrite of the classic MegaTunix ECU tuning software, built with modern technologies and designed for the next generation of engine management. This Redux version delivers professional-grade tuning capabilities with a focus on real-time performance, cross-platform compatibility, and TunerStudio feature parity.

**🎯 Design Goals:**
- **Professional UI**: Modern immediate-mode interface using Clay UI framework
- **High Performance**: SDL2-based hardware-accelerated rendering
- **Real-time Everything**: Sub-millisecond data streaming and visualization
- **AI-Enhanced**: Neural network autotuning and pattern recognition
- **Cross-platform**: Native Linux, Windows, and macOS support
- **TunerStudio Parity**: Match and exceed commercial tuning software capabilities

## 🏗️ Modern Architecture

### Rendering System
- **Clay UI Framework**: Immediate-mode GUI with professional aesthetics
- **SDL2 Renderer**: Cross-platform hardware acceleration
- **Modern Text**: SDL2_ttf integration with font fallback support
- **Efficient Drawing**: Render command array pattern for optimal performance

### ECU Communication Engine
- **Multi-Protocol**: MegaSquirt, Speeduino, LibreEMS, and more
- **Real-time Streaming**: Advanced data acquisition with microsecond timing
- **Safety First**: Comprehensive connection monitoring and fault detection
- **Plugin Architecture**: Extensible protocol support system

### Advanced Features
- **AI Autotuning**: Neural network VE table optimization
- **Closed-loop Control**: Real-time AFR and ignition management
- **Pattern Recognition**: Intelligent knock detection and analysis
- **Data Analytics**: Statistical processing and trend analysis
- **Professional Logging**: High-speed data capture and replay

## 🛠️ Quick Start

### Prerequisites

**Linux (Ubuntu/Debian):**
```bash
sudo apt install build-essential cmake libsdl2-dev libsdl2-ttf-dev pkg-config
```

**macOS (with Homebrew):**
```bash
brew install cmake sdl2 sdl2_ttf pkg-config
```

**Windows (MSYS2/MinGW):**
```bash
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_ttf
```

### Building

```bash
git clone https://github.com/RallyPat/MegaTunixRedux.git
cd MegaTunixRedux
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Running

```bash
# Standard mode
./megatunix-redux

# Demo mode (no hardware required)
./megatunix-redux --demo-mode

# Debug mode
./megatunix-redux --debug

# Show all options
./megatunix-redux --help
```

## 🎮 Key Features

### Real-time Dashboard
- High-resolution gauge displays
- Customizable layouts and themes
- Real-time graphing and trending
- Warning and alarm systems

### Tuning Capabilities
- VE table editing with live preview
- Ignition timing maps
- AFR target control
- Acceleration enrichment
- Rev limiter and safety systems

### Data Logging
- High-speed data acquisition
- Custom channel configuration
- Export to popular formats
- Replay and analysis tools

### Advanced Tuning
- AI-powered autotuning algorithms
- Closed-loop AFR control
- Knock detection and retard
- Statistical analysis tools
- Pattern recognition systems

## 🔧 Supported ECUs

### Fully Supported
- **Speeduino**: All firmware versions
- **MegaSquirt MS1/MS2**: Including Extra firmwares
- **LibreEMS**: Modern open-source ECU platform

### Planned Support
- MegaSquirt MS3/MS4
- VEMS ECUs
- Generic OBD-II systems
- Custom protocol implementations

## 📊 Performance

MegaTunix Redux is designed for performance:
- **Native C Implementation**: No interpreted languages, minimal overhead
- **Hardware Acceleration**: GPU-accelerated rendering where available
- **Optimized Communication**: Custom protocol stacks for maximum throughput
- **Real-time Scheduling**: Priority-based task management
- **Memory Efficient**: Minimal RAM footprint for embedded systems

## 🔄 Migration from Classic MegaTunix

MegaTunix Redux maintains compatibility with classic MegaTunix configurations while offering enhanced capabilities:

- **Configuration Import**: Automatic migration of tune files
- **Familiar Interface**: Similar workflow with modern improvements
- **Enhanced Features**: All classic features plus new capabilities
- **Performance Boost**: Significantly faster operation

## 📚 Documentation

- **[Documentation Index](docs/README.md)**: Complete documentation overview
- **[Design Document](docs/design/DESIGN_DOCUMENT.md)**: Complete architecture overview
- **[Renderer Architecture](docs/design/RENDERER_ARCHITECTURE.md)**: Technical rendering details
- **[Test Suite](tests/README.md)**: Testing documentation and test organization
- **[Project Status Reports](docs/status/)**: Development status and completion reports
- **[Changelog](CHANGELOG)**: Version history and changes

## 🤝 Contributing

We welcome contributions! Please read our contributing guidelines and:

1. Fork the repository
2. Create a feature branch
3. Make your changes with proper tests
4. Submit a pull request

See `CONTRIBUTING.md` for detailed guidelines.

## 📜 License

MegaTunix Redux is licensed under the GNU General Public License v2.0.
See `LICENSE` for full license text.

## 🙏 Acknowledgments

- **David J. Andruczyk**: Creator of the original MegaTunix
- **Clay UI Team**: For the excellent immediate-mode UI framework
- **SDL2 Community**: For cross-platform graphics foundation
- **ECU Communities**: MegaSquirt, Speeduino, and LibreEMS developers
- **GitHub Copilot**: AI assistance in Redux development

## 📈 Project Status

**Current Version**: 2.0.0  
**Status**: Active Development  
**Stability**: Production Ready  
**Platform Support**: Linux ✅ | Windows ✅ | macOS ✅

---

*MegaTunix Redux - Bringing ECU tuning into the modern era*

