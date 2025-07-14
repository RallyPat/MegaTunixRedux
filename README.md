# MegaTunix Redux - Modern Automotive Tuning Software

[![Build Status](https://github.com/RallyPat/MegaTunixRedux/workflows/Build%20and%20Test%20MegaTunix%20Red### Advanced Configuration
See the [Configuration Guide](docs/configuration.md) for detailed options.

## 📚 Documentationge.svg)](https://github.com/RallyPat/MegaTunixRedux/actions)
[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![C Standard](https://img.shields.io/badge/C-C17-blue.svg)](https://en.wikipedia.org/wiki/C17_(C_standard_revision))
[![GTK Version](https://img.shields.io/badge/GTK-4.12+-green.svg)](https://gtk.org/)

**MegaTunix Redux** is a comprehensive, cross-platform tuning application for automotive Engine Control Units (ECUs), with native support for MegaSquirt, LibreEMS, and Speeduino systems. This is a modernized fork completely rewritten with modern GTK4, enhanced security, and improved performance.

## Key Features

### Multi-Platform ECU Support
- **MegaSquirt**: Complete support for MS1, MS2, and MS3 systems
- **LibreEMS**: Native support for LibreEMS firmware
- **Speeduino**: Full integration with Speeduino open-source ECUs
- **Extensible Plugin System**: Easy addition of new ECU protocols

### Speeduino Integration 🚗
MegaTunix Redux provides **industry-leading Speeduino support** with comprehensive TunerStudio compatibility plus modern enhancements:

#### Core Features
- **🔄 TunerStudio-Compatible Protocol**: 100% compatible with Speeduino's serial protocol
- **📊 Real-time Monitoring**: 85+ engine parameters at 10Hz refresh rate
- **⚙️ Configuration Management**: Read/write all 15 configuration pages with CRC validation
- **🔍 Advanced Diagnostics**: Tooth logging, composite logging, and error detection
- **🔌 Auto-Detection**: Automatic ECU discovery with multiple device support
- **📈 Enhanced Features**: Modern UI, security, and performance improvements

#### Supported Parameters
- **Engine Essentials**: RPM, MAP, TPS, coolant/intake temperatures, battery voltage
- **Fuel System**: Multi-channel injection, fuel pressure, AFR control and monitoring
- **Ignition System**: Timing advance, dwell control, individual cylinder management
- **Advanced Features**: VVT control (2 channels), boost control, flex fuel with ethanol sensing
- **Diagnostics**: Error flags, engine protection, performance monitoring, 16-channel CAN input

#### Modern Enhancements Over TunerStudio
- **🎨 Modern GTK4 Interface**: Responsive design with HiDPI support
- **🔒 Enhanced Security**: Input validation and secure communication
- **🚀 Better Performance**: Hardware-accelerated graphics and optimized data handling
- **🌐 Cross-Platform**: Native Linux, Windows, macOS support
- **🔧 Plugin Architecture**: Extensible for custom features and integrations

## ✨ What's New in Redux

- **🚀 Modern GTK4 Interface** - Complete UI overhaul with contemporary design
- **🔒 Enhanced Security** - Input validation, secure memory handling, and rate limiting
- **⚡ Improved Performance** - Optimized rendering and reduced memory footprint
- **🛠️ Better Development Experience** - Meson build system, CI/CD, Docker support
- **🌐 Cross-Platform** - Native support for Linux, Windows, and macOS
- **📱 HiDPI Ready** - Perfect scaling on high-resolution displays

## 🎯 Features

### Core Functionality
- **Real-time ECU Communication** - Live data monitoring and tuning
- **3D Visualization** - Interactive 3D fuel and ignition maps with OpenGL
- **Dashboard Designer** - Create custom gauge layouts
- **Data Logging** - Comprehensive logging with analysis tools
- **Multi-ECU Support** - MegaSquirt, LibreEMS, and extensible plugin system

### Modern Enhancements
- **Modern Widget System** - Custom GTK4 widgets with CSS theming
- **Secure Networking** - Encrypted multi-user tuning sessions
- **Advanced Analytics** - Built-in performance analysis tools
- **Plugin Architecture** - Easy integration of new ECU protocols

## 🚀 Quick Start

### Prerequisites

**Ubuntu/Debian:**
```bash
sudo apt install meson ninja-build pkg-config libgtk-4-dev libglib2.0-dev \
                 libxml2-dev libgl1-mesa-dev libglu1-mesa-dev flex bison
```

**Fedora/RHEL:**
```bash
sudo dnf install meson ninja-build pkg-config gtk4-devel glib2-devel \
                 libxml2-devel mesa-libGL-devel mesa-libGLU-devel flex bison
```

**Windows (MSYS2):**
```bash
pacman -S mingw-w64-x86_64-meson mingw-w64-x86_64-gtk4 \
          mingw-w64-x86_64-libxml2 flex bison
```

**macOS:**
```bash
brew install meson ninja pkg-config gtk4 libxml2 flex bison
```

### Building

```bash
# Clone the repository
git clone https://github.com/RallyPat/MegaTunixRedux.git
cd MegaTunixRedux

# Configure build
meson setup builddir --buildtype=release

# Build
meson compile -C builddir

# Install (optional)
sudo meson install -C builddir
```

### Docker Development

```bash
# Build development environment
docker-compose build megatunix-dev

# Start development container
docker-compose run --rm megatunix-dev

# Build inside container
meson setup builddir && meson compile -C builddir
```

## 🎮 Usage

### Basic Operation

1. **Connect ECU** - Use serial/USB connection to your ECU
2. **Interrogate** - Auto-detect ECU firmware and capabilities  
3. **Tune** - Modify tables and parameters in real-time
4. **Monitor** - View live data on customizable dashboards
5. **Log** - Record sessions for later analysis

### Advanced Features

- **Multi-User Tuning** - Collaborate with remote users
- **Scripting** - Automate tuning procedures
- **Custom Gauges** - Design specialized instruments
- **Data Export** - Integration with analysis software

## 🔧 Configuration

### Basic Settings
```ini
# ~/.config/megatunix-redux/settings.conf
[Communication]
port = /dev/ttyUSB0
baud_rate = 115200
timeout = 1000

[Display]
theme = dark
font_scale = 1.0
enable_animations = true

[Security]
enable_network = false
max_connections = 4
rate_limit = 10
```

### Advanced Configuration
See the [Configuration Guide](docs/configuration.md) for detailed options.

## � Documentation

## 📚 Documentation

- [User Manual](docs/user-manual.md) - Complete usage guide
- [Developer Guide](docs/developer.md) - API reference and architecture
- [Hardware Compatibility](docs/hardware.md) - Supported ECUs and interfaces
- [Troubleshooting](docs/troubleshooting.md) - Common issues and solutions

## ⚖️ License

MegaTunix Redux is licensed under the [GNU General Public License v2.0](LICENSE).

## 🏆 Acknowledgments

- **David J. Andruczyk** - Original author and maintainer
- **Bruce Bowling & Al Grippo** - MegaSquirt ECU designers  
- **The GTK Team** - Excellent UI toolkit
- **Contributors** - Everyone who has helped improve MegaTunix Redux

---

*Built with ❤️ for the automotive tuning community*
