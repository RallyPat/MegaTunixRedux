# MegaTunix Redux - Design Document

## Project Overview
**Project Name:** MegaTunix Redux
**Version:** 2.0.0
**Date:** July 2025
**Original Author:** David J. Andruczyk
**Redux Developer:** Pat Burke with GitHub Copilot assistance

### Purpose
Fork and modernize the existing MegaTunix application to create MegaTunix Redux, a contemporary, cross-platform tuning solution for MegaSquirt and LibreEMS ECU systems. The primary goal is to achieve feature parity with TunerStudio MS (the professional paid version) while maintaining open-source availability. This includes implementing advanced autotuning capabilities, leveraging AI where beneficial, and preserving all existing MegaTunix functionality while dramatically improving user experience and platform support.

### Legacy Application Analysis
**Current MegaTunix (v1.x):**
- Written in C using GTK+2
- Supports MS1, MS1-Extra, MS2, MS2-Extra firmwares
- Plugin architecture for firmware extensibility
- Uses Glade for GUI design
- Cross-platform (Linux, macOS, Windows)
- GPL v2 licensed
- Native application (no interpreted languages)

### Scope
**Included:**
- Complete UI modernization with contemporary design
- **TunerStudio MS Feature Parity:** All professional features including autotuning
- **Advanced Autotuning Engine:** AI-enhanced automatic fuel and ignition optimization
- **Smart Learning Algorithms:** Machine learning for tuning suggestions and optimization
- Enhanced cross-platform support (Linux, Windows, macOS, potentially mobile)
- Improved plugin architecture
- Modern development toolchain and build system
- Enhanced data visualization and logging with predictive analytics
- Real-time tuning capabilities with safety monitoring
- Web-based dashboard option
- Better documentation and user onboarding
- **Professional-Grade Features:** VE analysis, knock detection integration, closed-loop tuning

**Not Included:**
- Changes to core ECU communication protocols
- Breaking compatibility with existing firmware definitions
- Removal of any existing functionality

## Requirements

### Functional Requirements
1. **ECU Communication:** Maintain compatibility with all currently supported MegaSquirt variants
2. **Real-time Data Display:** Live gauges, graphs, and parameter monitoring
3. **Firmware Loading:** Upload and manage ECU firmware
4. **Parameter Tuning:** Modify ECU parameters with validation and safety checks
5. **Data Logging:** Record and analyze ECU data over time with export capabilities
6. **Map Editing:** 3D fuel and ignition map editing with interpolation
7. **Plugin System:** Extensible architecture for new firmware support
8. **Import/Export:** Support existing MegaTunix and TunerStudio configuration formats
9. **Multi-ECU Support:** Handle multiple ECU connections simultaneously
10. **Offline Mode:** Work with saved configurations when ECU not connected

#### TunerStudio MS Feature Parity Requirements
11. **Autotuning Engine:** Automated VE (Volumetric Efficiency) table optimization
12. **Advanced Data Analysis:** Statistical analysis of logged data with trend identification
13. **Closed-Loop Tuning:** Real-time fuel delivery optimization using wideband O2 feedback
14. **Knock Detection Integration:** Automatic ignition timing optimization based on knock sensors
15. **Target AFR Tables:** Sophisticated air-fuel ratio targeting with load-based adjustments
16. **Idle Speed Control:** Automated idle tuning with IAC (Idle Air Control) optimization
17. **Acceleration Enrichment:** Automated pump shot and accel enrichment tuning
18. **Rev Limiter & Launch Control:** Advanced engine protection and launch system tuning
19. **Boost Control:** Automated wastegate and boost controller tuning
20. **Advanced Logging:** High-resolution data logging with trigger-based recording

#### AI-Enhanced Features
21. **Predictive Tuning:** Machine learning models to suggest optimal parameter changes
22. **Anomaly Detection:** AI-powered detection of unusual engine behavior or sensor issues
23. **Pattern Recognition:** Identify common tuning scenarios and recommend base maps
24. **Adaptive Learning:** System learns from successful tunes to improve future suggestions
25. **Safety Intelligence:** AI monitoring for potentially dangerous parameter combinations

### Non-Functional Requirements
- **Performance:** Maintain <100ms response time for parameter updates
- **Security:** Secure communication with ECU, protect against malformed data
- **Scalability:** Support 100+ simultaneous parameters monitoring
- **Compatibility:** 
  - Windows 10/11 (x64)
  - macOS 10.15+ (Intel & Apple Silicon)
  - Linux (major distributions)
  - Potentially iOS/Android for monitoring
- **Maintainability:** Modern codebase with comprehensive testing
- **Usability:** Intuitive interface suitable for both beginners and experts

## Technical Specifications

### Technology Stack (Native C/C++ with Clay UI)

**Core Application:**
- **Language:** C/C++ for maximum performance and real-time capabilities
- **UI Framework:** Clay - Immediate mode UI library for native applications
- **Graphics Backend:** OpenGL for hardware-accelerated rendering
- **Build System:** CMake for cross-platform builds
- **Package Manager:** vcpkg or Conan for C++ dependencies

**System Libraries:**
- **Serial Communication:** libserialport for cross-platform serial ECU communication
- **Database:** SQLite3 C API for data logging and configuration storage
- **File I/O:** Standard C library with JSON parsing (cJSON or nlohmann/json)
- **Math Operations:** GSL (GNU Scientific Library) for advanced calculations
- **Threading:** C++11 std::thread for concurrent operations

**Autotuning & AI Components:**
- **Machine Learning:** ONNX Runtime C++ API for AI inference
- **Optimization Algorithms:** Custom C++ implementations of genetic algorithms, PID controllers
- **Signal Processing:** FFTW library for digital signal processing
- **Control Systems:** Real-time PID and adaptive control implementations
- **Statistical Analysis:** Custom C++ statistical computation modules

**Platform Integration:**
- **Windows:** Win32 API for system integration, WinUSB for device access
- **Linux:** GTK+ or Qt integration layer (optional), udev for device management
- **macOS:** Cocoa integration layer, IOKit for device access

**Development Tools:**
- **Build System:** CMake with cross-platform toolchains
- **Testing:** Google Test (gtest) for unit testing, custom integration tests
- **Debugging:** GDB, LLDB, Visual Studio debugger support
- **Documentation:** Doxygen for API documentation
- **Version Control:** Git with conventional commits

### Architecture Overview (Native C/C++ with Clay UI)
```
┌─────────────────────────────────────────────────────────────────┐
│                   MEGATUNIX REDUX NATIVE APP                   │
├─────────────────────────────────────────────────────────────────┤
│                      CLAY UI LAYER                             │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────┐  │
│  │   Clay Context  │    │  Render Engine  │    │ Input Mgmt  │  │
│  │                 │    │                 │    │             │  │
│  │ • UI Elements   │◄──►│ • OpenGL/Vulkan │◄──►│ • Mouse     │  │
│  │ • Layout Engine │    │ • Text Rendering│    │ • Keyboard  │  │
│  │ • Style System  │    │ • GPU Accel     │    │ • Touch     │  │
│  │ • Animation     │    │ • Frame Buffer  │    │ • Gestures  │  │
│  │ • Responsiveness│    │ • VSync         │    │ • Events    │  │
│  └─────────────────┘    └─────────────────┘    └─────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                    APPLICATION CORE                             │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────┐  │
│  │  ECU Interface  │    │  Data Manager   │    │  App State  │  │
│  │                 │    │                 │    │             │  │
│  │ • Serial Comm   │◄──►│ • SQLite DB     │◄──►│ • Config    │  │
│  │ • Protocol Stack│    │ • File I/O      │    │ • Sessions  │  │
│  │ • Firmware Ops  │    │ • Data Logging  │    │ • UI State  │  │
│  │ • Safety Checks │    │ • Cache Mgmt    │    │ • Preferences│  │
│  │ • Device Detect │    │ • Backup/Restore│    │ • History   │  │
│  └─────────────────┘    └─────────────────┘    └─────────────┘  │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────┐  │
│  │ Autotuning Core │    │   AI/ML Engine  │    │ Analysis    │  │
│  │                 │    │                 │    │             │  │
│  │ • VE Optimizer  │◄──►│ • ONNX Runtime  │◄──►│ • Statistics│  │
│  │ • AFR Controller│    │ • Neural Networks│    │ • FFT/DSP   │  │
│  │ • Ignition Tuner│    │ • Genetic Alg   │    │ • Pattern Rec│  │
│  │ • Closed Loop   │    │ • Model Inference│    │ • Anomaly Det│  │
│  │ • Safety Monitor│    │ • Real-time ML  │    │ • Reports   │  │
│  └─────────────────┘    └─────────────────┘    └─────────────┘  │
├─────────────────────────────────────────────────────────────────┤
│                    HARDWARE INTERFACE                           │
│    Serial/USB ECU ◄── libserialport ──► MegaSquirt + Sensors   │
│         ▲                                         ▲            │
│    Wideband O2                              Knock Sensors      │
└─────────────────────────────────────────────────────────────────┘
```

## Data Model

### Configuration Schema
```typescript
interface ECUConfiguration {
  id: string;
  name: string;
  firmwareType: 'MS1' | 'MS1-Extra' | 'MS2' | 'MS2-Extra' | 'LibreEMS';
  firmwareVersion: string;
  connectionType: 'Serial' | 'USB' | 'Bluetooth';
  connectionParams: ConnectionParams;
  parameters: Parameter[];
  maps: Map3D[];
  datalogging: DatalogConfig;
  autotuneConfig: AutotuneConfiguration;
  aiSettings: AIConfiguration;
  createdAt: Date;
  updatedAt: Date;
}

interface AutotuneConfiguration {
  enabled: boolean;
  targetAFR: AFRTarget[];
  veTableOptimization: VEOptimizationConfig;
  ignitionAdvanceOptimization: IgnitionConfig;
  closedLoopSettings: ClosedLoopConfig;
  safetyLimits: SafetyConfiguration;
  learningRate: number;
  convergenceThreshold: number;
}

interface AFRTarget {
  rpmRange: [number, number];
  loadRange: [number, number];
  targetAFR: number;
  tolerance: number;
  priority: 'high' | 'medium' | 'low';
}

interface VEOptimizationConfig {
  enabled: boolean;
  algorithm: 'gradient_descent' | 'genetic' | 'neural_network';
  maxIterations: number;
  learningRate: number;
  targetAccuracy: number;
  regionWeighting: RegionWeight[];
}

interface AIConfiguration {
  enabled: boolean;
  modelType: 'neural_network' | 'decision_tree' | 'ensemble';
  predictionConfidence: number;
  trainingDataSize: number;
  autoRetraining: boolean;
  anomalyDetectionThreshold: number;
  suggestionLevel: 'conservative' | 'moderate' | 'aggressive';
}

interface Parameter {
  id: string;
  name: string;
  description: string;
  type: 'number' | 'boolean' | 'enum';
  unit?: string;
  min?: number;
  max?: number;
  precision?: number;
  category: string;
  address: number;
  readOnly: boolean;
  currentValue?: any;
}

interface Map3D {
  id: string;
  name: string;
  type: 'fuel' | 'ignition' | 'target_afr' | 'boost' | 'custom';
  xAxis: AxisDefinition;
  yAxis: AxisDefinition;
  data: number[][];
  interpolation: 'linear' | 'cubic' | 'spline';
  autotuneHistory: AutotuneSession[];
  aiOptimized: boolean;
  lastOptimization: Date;
}

interface AutotuneSession {
  id: string;
  timestamp: Date;
  algorithm: string;
  iterations: number;
  convergenceScore: number;
  beforeData: number[][];
  afterData: number[][];
  performanceMetrics: {
    avgAFRError: number;
    maxAFRError: number;
    stabilityIndex: number;
    fuelEconomyImprovement: number;
  };
}
```

### Data Storage
- **Configuration Files:** JSON format (backward compatible with legacy and TunerStudio formats)
- **Data Logs:** SQLite database for efficient querying with full-text search
- **Plugin Definitions:** JSON/YAML firmware definition files
- **User Preferences:** Local storage/config files
- **AI Training Data:** Separate SQLite database for machine learning datasets
- **Autotune Sessions:** Compressed historical data with before/after comparisons
- **Performance Baselines:** Statistical baselines for comparison and improvement tracking

## User Interface Design

### Modern UI Principles
- **Dark/Light Mode:** System preference detection with manual override
- **Responsive Design:** Adaptive layout for different screen sizes
- **Accessibility:** WCAG 2.1 AA compliance
- **Progressive Disclosure:** Show complexity as needed
- **Contextual Help:** Inline documentation and tooltips

### Key Screens/Views

#### 1. Dashboard View
- Connection status and ECU info
- Critical parameter monitoring
- Quick access to common functions
- Real-time data visualization

#### 2. Parameter Editor
- Categorized parameter tree
- Search and filter capabilities
- Bulk operations
- Change validation and warnings

#### 3. Map Editor
- 3D visualization of fuel/ignition maps
- Interactive editing with drag/brush tools
- Compare mode for before/after
- Import/export individual maps

#### 4. Data Logger
- Real-time graphing
- Historical data analysis
- Export to CSV/other formats
- Trigger-based recording

#### 5. Firmware Manager
- Firmware upload/download
- Backup/restore ECU settings
- Firmware comparison tools

### User Experience Flow
1. **Connection Setup:** Auto-detect ECU or manual configuration
2. **Parameter Loading:** Automatic firmware detection and parameter loading
3. **Real-time Monitoring:** Live data display with customizable layouts
4. **Parameter Modification:** Safe editing with validation and confirmation
5. **Data Analysis:** Historical trending and comparison tools

### Electron-Specific Architecture

#### Process Communication (IPC)
```typescript
// Main Process API
interface MainProcessAPI {
  // ECU Communication
  ecuConnect(params: ConnectionParams): Promise<boolean>;
  ecuDisconnect(): Promise<void>;
  ecuReadParameter(address: number): Promise<number>;
  ecuWriteParameter(address: number, value: number): Promise<void>;
  
  // File Operations
  loadConfiguration(path: string): Promise<ECUConfiguration>;
  saveConfiguration(config: ECUConfiguration, path: string): Promise<void>;
  
  // Data Logging
  startDataLogging(config: DatalogConfig): Promise<void>;
  stopDataLogging(): Promise<DataLogSummary>;
  getLogData(query: LogQuery): Promise<LogData[]>;
}

// Renderer Process Events
interface RendererEvents {
  'ecu-data-update': (data: ECUDataPacket) => void;
  'ecu-connection-status': (status: ConnectionStatus) => void;
  'ecu-error': (error: ECUError) => void;
  'firmware-upload-progress': (progress: number) => void;
}
```

#### Window Management
- **Main Window:** Primary application interface
- **Map Editor Window:** Dedicated 3D map editing (optional separate window)
- **Data Logger Window:** Real-time charting and analysis
- **Settings Window:** Application and ECU configuration
- **About/Help Windows:** Documentation and support

## Plugin Architecture

### Electron Plugin System
```typescript
// Plugin runs in isolated context
interface ElectronPlugin {
  id: string;
  name: string;
  version: string;
  author: string;
  
  // Plugin lifecycle
  activate(api: PluginAPI): Promise<void>;
  deactivate(): Promise<void>;
  
  // Firmware support
  getFirmwareDefinition(): FirmwareDefinition;
  
  // UI contributions
  getUIComponents?(): PluginUIComponents;
  getMenuItems?(): MenuItem[];
}

interface PluginAPI {
  // ECU Communication (proxied through main process)
  sendECUCommand(command: ECUCommand): Promise<ECUResponse>;
  subscribeToECUData(callback: DataCallback): () => void;
  
  // UI Integration
  registerView(id: string, component: React.Component): void;
  registerMenuItem(item: MenuItem): void;
  
  // Configuration
  getPluginConfig(key: string): any;
  setPluginConfig(key: string, value: any): Promise<void>;
  
  // File System (sandboxed)
  readPluginFile(path: string): Promise<string>;
  writePluginFile(path: string, data: string): Promise<void>;
}
```
```typescript
interface PluginAPI {
  registerFirmware(definition: FirmwareDefinition): void;
  registerParameterType(type: ParameterType): void;
  registerVisualization(component: React.Component): void;
  getECUData(): ECUData;
  sendECUCommand(command: ECUCommand): Promise<Response>;
}

interface FirmwareDefinition {
  id: string;
  name: string;
  version: string;
  parameters: ParameterDefinition[];
  maps: MapDefinition[];
  communication: CommunicationProtocol;
}
```

### Legacy Compatibility
- Import existing Glade GUI definitions
- Convert legacy parameter definitions to new format
- Maintain support for existing plugins through adapter layer

## ECU Communication Protocol

### Serial Communication
- **Baud Rates:** 9600, 19200, 38400, 57600, 115200
- **Data Format:** 8N1 (8 data bits, no parity, 1 stop bit)
- **Flow Control:** Hardware/software configurable
- **Timeout Handling:** Configurable retry logic

### Protocol Implementation
```typescript
interface ECUProtocol {
  connect(params: ConnectionParams): Promise<void>;
  disconnect(): Promise<void>;
  readParameter(address: number): Promise<number>;
  writeParameter(address: number, value: number): Promise<void>;
  uploadFirmware(firmware: Uint8Array): Promise<void>;
  getVersion(): Promise<string>;
}
```

## Security Considerations
- **Firmware Validation:** Checksum verification before upload
- **Parameter Bounds Checking:** Prevent dangerous values
- **Communication Encryption:** For network-based connections
- **Audit Logging:** Track all parameter changes
- **Backup Creation:** Automatic ECU backup before modifications

## Testing Strategy

### Unit Tests
- Parameter validation logic
- Protocol communication handlers
- Data transformation functions
- Plugin loading and registration

### Integration Tests
- End-to-end ECU communication
- Firmware upload/download processes
- Data logging and retrieval
- Plugin compatibility

### Hardware Testing
- Multiple MegaSquirt variants
- Different operating systems
- Various connection types (Serial, USB)
- Performance under high data rates

## Development Guidelines

### Code Standards
- **Language:** TypeScript for type safety
- **Linting:** ESLint + Prettier
- **Testing:** Jest for unit tests, Cypress for E2E
- **Documentation:** JSDoc for API documentation
- **Git Workflow:** GitFlow with feature branches
- **Commit Messages:** Conventional commits format

### File Structure (Electron Project)
```
megatunix-redux/
├── src/
│   ├── main/                    # Main process (Node.js)
│   │   ├── index.ts            # App entry point
│   │   ├── window-manager.ts   # Window creation/management
│   │   ├── ecu/                # ECU communication
│   │   │   ├── protocol.ts     # Communication protocols
│   │   │   ├── serial.ts       # Serial port handling
│   │   │   └── firmware.ts     # Firmware operations
│   │   ├── data/               # Data management
│   │   │   ├── database.ts     # SQLite operations
│   │   │   ├── config.ts       # Configuration management
│   │   │   └── logging.ts      # Data logging
│   │   └── plugins/            # Plugin system
│   │       ├── loader.ts       # Plugin loading
│   │       └── api.ts          # Plugin API implementation
│   ├── renderer/               # Renderer process (React)
│   │   ├── index.tsx          # React entry point
│   │   ├── components/        # UI components
│   │   │   ├── Dashboard/     # Main dashboard
│   │   │   ├── MapEditor/     # 3D map editing
│   │   │   ├── Parameters/    # Parameter management
│   │   │   ├── DataLogger/    # Real-time charts
│   │   │   └── Settings/      # Configuration UI
│   │   ├── store/             # Redux store
│   │   ├── hooks/             # Custom React hooks
│   │   └── utils/             # Utility functions
│   ├── shared/                # Shared code
│   │   ├── types.ts          # TypeScript interfaces
│   │   ├── constants.ts      # App constants
│   │   └── protocols/        # ECU protocol definitions
│   └── preload/              # Preload scripts
│       └── index.ts          # Main preload script
├── assets/                   # Static assets
│   ├── icons/               # Application icons
│   ├── images/              # UI images
│   └── firmware/            # Default firmware definitions
├── plugins/                 # Plugin directory
│   ├── megasquirt-ms1/     # MS1 plugin
│   ├── megasquirt-ms2/     # MS2 plugin
│   └── libreems/           # LibreEMS plugin
├── tests/                  # Test files
│   ├── main/               # Main process tests
│   ├── renderer/           # Renderer tests
│   └── e2e/                # End-to-end tests
├── docs/                   # Documentation
├── build/                  # Build configuration
│   ├── webpack.config.js   # Webpack configuration
│   └── electron-builder.json # Packaging config
└── dist/                   # Built application
```

## Migration Strategy

### Phase 1: Core Infrastructure (Months 1-2)
- Set up development environment
- Implement basic ECU communication
- Create parameter management system
- Basic UI shell

### Phase 2: Feature Parity (Months 3-4)
- Port all existing functionality
- Implement plugin system
- Create configuration import tools
- Basic testing suite

### Phase 3: Modern Features (Months 5-6)
- Enhanced visualizations
- Improved user experience
- Mobile companion app
- Comprehensive documentation

### Phase 4: Polish and Release (Months 7-8)
- Performance optimization
- Extensive testing
- User beta testing
- Release preparation

## Deployment and Distribution

### Deployment and Distribution

#### Electron Build Process
```json
// package.json scripts
{
  "scripts": {
    "dev": "concurrently \"npm run dev:main\" \"npm run dev:renderer\"",
    "dev:main": "tsc -w --project src/main/tsconfig.json",
    "dev:renderer": "vite serve src/renderer",
    "build": "npm run build:main && npm run build:renderer",
    "build:main": "tsc --project src/main/tsconfig.json",
    "build:renderer": "vite build src/renderer",
    "dist": "electron-builder",
    "dist:win": "electron-builder --win",
    "dist:mac": "electron-builder --mac", 
    "dist:linux": "electron-builder --linux"
  }
}
```

#### Distribution Targets
- **Windows:** 
  - Installer (.exe) for Windows 10/11 x64
  - Portable version (.zip)
  - Microsoft Store (MSIX package)
- **macOS:**
  - DMG installer for Intel and Apple Silicon
  - App Store submission (mas build)
- **Linux:**
  - AppImage (universal)
  - Debian package (.deb)
  - RPM package (.rpm)
  - Snap package
  - Flatpak

#### Auto-Updates with Electron
```typescript
// Auto-updater configuration
import { autoUpdater } from 'electron-updater';

autoUpdater.checkForUpdatesAndNotify();
autoUpdater.on('update-available', () => {
  // Notify user of available update
});
autoUpdater.on('update-downloaded', () => {
  // Prompt user to restart and install
});
```

## Success Criteria
1. **Functionality:** 100% feature parity with legacy MegaTunix
2. **Performance:** Sub-100ms parameter update response time
3. **Compatibility:** Support all currently supported ECU variants
4. **User Adoption:** Positive feedback from beta testers
5. **Maintainability:** Comprehensive test coverage (>90%)
6. **Documentation:** Complete user and developer documentation

## Legacy Considerations
- **Data Migration:** Tools to import existing MegaTunix configurations
- **Parallel Operation:** Run alongside legacy MegaTunix during transition
- **Community Support:** Maintain compatibility with existing forums/resources
- **Plugin Migration:** Adapter for existing plugin ecosystem

## Future Roadmap
- **Mobile Apps:** iOS/Android monitoring applications
- **Cloud Sync:** Configuration backup and sharing
- **Advanced Analytics:** Machine learning for tuning suggestions
- **Community Features:** Sharing and collaboration tools
- **IoT Integration:** Remote monitoring capabilities

---

## Implementation Instructions

When implementing this Electron-based modernization:

1. **Start with Electron Boilerplate:** Use a modern Electron + React + TypeScript template
2. **Implement Main Process First:** Focus on ECU communication and data management in the main process
3. **Secure IPC Communication:** Use contextIsolation and disable node integration in renderer
4. **Handle Serial Permissions:** Properly request and manage serial port access
5. **Implement Safety Checks:** All ECU operations must include validation and error handling
6. **Use Modern React Patterns:** Hooks, context, and functional components throughout
7. **Optimize for Performance:** Minimize main-renderer IPC calls, use efficient data structures
8. **Plan for Cross-Platform:** Test serial communication on Windows, macOS, and Linux
9. **Include Comprehensive Logging:** Debug logging for development, production logging for support
10. **Engage the Community:** Work with existing MegaTunix users throughout development

**Electron-Specific Development Priority:**
1. Main process ECU communication and safety systems
2. IPC layer between main and renderer processes  
3. React UI with real-time data updates
4. Parameter management and validation UI
5. Plugin system with secure sandboxing
6. Cross-platform builds and distribution