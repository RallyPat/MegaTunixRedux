/*
 * Plugin Interface - MegaTunix Redux
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * Modern plugin system leveraging the modular architecture foundation
 */

#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct LoggingSystem;
struct SettingsManager;
struct UIThemeManager;
struct ECUIntegration;
struct ImGuiContext;
struct EventSystem;
struct PluginManager;
struct PluginConfig;

// Log level constants (if not already defined)
#ifndef LOG_LEVEL_DEBUG
#define LOG_LEVEL_DEBUG 0
#endif
#ifndef LOG_LEVEL_INFO
#define LOG_LEVEL_INFO 1
#endif
#ifndef LOG_LEVEL_WARNING
#define LOG_LEVEL_WARNING 2
#endif
#ifndef LOG_LEVEL_ERROR
#define LOG_LEVEL_ERROR 3
#endif

// Plugin types
typedef enum {
    PLUGIN_TYPE_ECU,           // ECU protocol plugins
    PLUGIN_TYPE_UI,            // UI component plugins
    PLUGIN_TYPE_DATA,          // Data processing plugins
    PLUGIN_TYPE_INTEGRATION,   // Integration plugins
    PLUGIN_TYPE_COUNT
} PluginType;

// Plugin status
typedef enum {
    PLUGIN_STATUS_UNLOADED,    // Plugin not loaded
    PLUGIN_STATUS_LOADED,      // Plugin loaded but not initialized
    PLUGIN_STATUS_INITIALIZED, // Plugin initialized and running
    PLUGIN_STATUS_ERROR,       // Plugin in error state
    PLUGIN_STATUS_DISABLED     // Plugin disabled by user
} PluginStatus;

// ECU real-time data structure
typedef struct {
    float rpm;
    float map;
    float coolant_temp;
    float air_temp;
    float throttle;
    float afr;
    float timing;
    float fuel_pressure;
    float oil_pressure;
    float battery_voltage;
    uint64_t timestamp;
} ECURealtimeData;

// ECU connection status
typedef struct {
    bool connected;
    const char* port;
    int baudrate;
    const char* protocol;
    const char* ecu_type;
    const char* firmware_version;
    uint32_t rx_packets;
    uint32_t tx_packets;
    uint32_t errors;
    uint64_t last_activity;
} ECUConnectionStatus;

// Enhanced ECU Plugin Interface for real hardware communication
typedef struct {
    // Connection management
    bool (*connect)(const char* port, int baud_rate, const char* protocol);
    bool (*disconnect)(void);
    bool (*is_connected)(void);
    const char* (*get_connection_status)(void);
    
    // Data operations
    bool (*read_table)(int table_id, float* data, int* rows, int* cols);
    bool (*write_table)(int table_id, const float* data, int rows, int cols);
    bool (*read_realtime_data)(ECURealtimeData* data);
    bool (*write_parameter)(int param_id, float value);
    bool (*read_parameter)(int param_id, float* value);
    
    // Protocol-specific operations
    bool (*send_command)(const char* command, char* response, int max_response_len);
    bool (*upload_firmware)(const char* firmware_path);
    bool (*download_config)(const char* config_path);
    
    // Configuration
    bool (*set_protocol_settings)(const char* settings_json);
    const char* (*get_protocol_info)(void);
    bool (*validate_connection)(void);
    
    // Advanced features
    bool (*start_logging)(const char* log_path);
    bool (*stop_logging)(void);
    bool (*get_log_status)(char* status, int max_len);
} ECUPluginInterface;

// UI plugin interface
typedef struct {
    // UI registration
    const char* panel_name;
    const char* menu_path;
    
    // Rendering
    void (*render_panel)(void);
    void (*render_menu_item)(void);
    
    // Event handling
    bool (*handle_input)(const void* event);
    void (*on_focus)(bool focused);
    
    // State management
    void (*save_state)(void);
    void (*load_state)(void);
    
    // UI-specific data
    void* ui_data;
} UIPluginInterface;

// Data processing plugin interface
typedef struct {
    // Data processing
    const char* processor_name;
    const char* input_format;
    const char* output_format;
    
    // Processing functions
    bool (*process_data)(const void* input_data, size_t input_size, void** output_data, size_t* output_size);
    bool (*validate_data)(const void* data, size_t size);
    void (*cleanup_data)(void* data);
    
    // Configuration
    bool (*set_parameter)(const char* param_name, const char* param_value);
    const char* (*get_parameter)(const char* param_name);
    
    // Data-specific information
    void* processor_data;
} DataPluginInterface;

// Integration plugin interface
typedef struct {
    // Integration details
    const char* service_name;
    const char* service_url;
    const char* api_version;
    
    // Connection management
    bool (*connect_service)(const char* credentials);
    void (*disconnect_service)(void);
    bool (*is_connected)(void);
    
    // Data operations
    bool (*send_data)(const char* endpoint, const void* data, size_t size);
    bool (*receive_data)(const char* endpoint, void** data, size_t* size);
    
    // Service-specific functions
    void* service_data;
} IntegrationPluginInterface;

// Data visualization plugin interface
typedef struct {
    // Chart management
    bool (*create_chart)(const char* chart_id, const char* title, int chart_type);
    bool (*destroy_chart)(const char* chart_id);
    bool (*clear_chart_data)(const char* chart_id);
    
    // Data operations
    bool (*add_data_point)(const char* chart_id, float x_value, float y_value, const char* series_name);
    bool (*add_data_series)(const char* chart_id, const char* series_name, const char* color);
    bool (*update_chart)(const char* chart_id);
    
    // Chart configuration
    bool (*set_chart_title)(const char* chart_id, const char* title);
    bool (*set_axis_labels)(const char* chart_id, const char* x_label, const char* y_label);
    bool (*set_chart_range)(const char* chart_id, float x_min, float x_max, float y_min, float y_max);
    bool (*set_chart_type)(const char* chart_id, int chart_type);
    
    // Interactive features
    bool (*enable_zoom)(const char* chart_id, bool enable);
    bool (*enable_pan)(const char* chart_id, bool enable);
    bool (*enable_legend)(const char* chart_id, bool enable);
    bool (*enable_grid)(const char* chart_id, bool enable);
    
    // Data export
    bool (*export_chart_data)(const char* chart_id, const char* file_path);
    bool (*export_chart_image)(const char* chart_id, const char* file_path);
    
    // Real-time streaming
    bool (*start_streaming)(const char* chart_id, const char* ecu_plugin_name, const char* data_source);
    bool (*stop_streaming)(const char* chart_id);
    bool (*set_update_rate)(const char* chart_id, int updates_per_second);
    
    // Chart information
    const char* (*get_chart_info)(const char* chart_id);
    int (*get_chart_data_count)(const char* chart_id);
    bool (*is_chart_streaming)(const char* chart_id);
    
    // Chart rendering
    bool (*render_chart)(const char* chart_id, float width, float height);
    bool (*get_chart_bounds)(const char* chart_id, float* x_min, float* x_max, float* y_min, float* y_max);
    bool (*set_chart_viewport)(const char* chart_id, float x_min, float x_max, float y_min, float y_max);
    bool (*handle_mouse_input)(const char* chart_id, float mouse_x, float mouse_y, int button, bool pressed);
    bool (*get_chart_tooltip)(const char* chart_id, float mouse_x, float mouse_y, char* tooltip, int max_len);
    
    // Advanced features
    bool (*add_annotation)(const char* chart_id, float x, float y, const char* text, const char* color);
    bool (*set_chart_style)(const char* chart_id, int style_preset);
    bool (*enable_animations)(const char* chart_id, bool enable);
    bool (*set_chart_theme)(const char* chart_id, const char* theme_name);
} DataVisualizationPluginInterface;

// Chart types
typedef enum {
    CHART_TYPE_LINE,           // Line chart
    CHART_TYPE_SCATTER,        // Scatter plot
    CHART_TYPE_BAR,            // Bar chart
    CHART_TYPE_AREA,           // Area chart
    CHART_TYPE_3D_SURFACE,     // 3D surface plot
    CHART_TYPE_HEATMAP,        // Heat map
    CHART_TYPE_GAUGE,          // Gauge display
    CHART_TYPE_DIGITAL,        // Digital readout
    CHART_TYPE_COUNT
} ChartType;

// Chart configuration
typedef struct {
    char chart_id[64];
    char title[128];
    char x_label[64];
    char y_label[64];
    ChartType type;
    float x_min, x_max;
    float y_min, y_max;
    bool zoom_enabled;
    bool pan_enabled;
    bool legend_enabled;
    bool grid_enabled;
    int update_rate;
    bool streaming;
    char ecu_source[64];
    char data_source[64];
} ChartConfig;

// Plugin context for accessing core functionality
typedef struct {
    // Core system access
    struct LoggingSystem* logging;
    struct SettingsManager* settings;
    struct UIThemeManager* themes;
    
    // ECU communication
    struct ECUIntegration* ecu;
    
    // UI system
    struct ImGuiContext* imgui;
    
    // Event system
    struct EventSystem* events;
    
    // Plugin management
    struct PluginManager* plugin_mgr;
    
    // Configuration
    struct PluginConfig* config;
} PluginContext;

// Core plugin interface
typedef struct {
    const char* name;           // Plugin name
    const char* version;        // Plugin version
    const char* author;         // Plugin author
    const char* description;    // Plugin description
    PluginType type;            // Plugin type
    PluginStatus status;        // Current plugin status
    
    // Lifecycle functions
    bool (*init)(PluginContext* ctx);
    void (*cleanup)(void);
    void (*update)(void);
    
    // Plugin-specific interfaces
    union {
        ECUPluginInterface ecu;
        UIPluginInterface ui;
        DataPluginInterface data;
        DataVisualizationPluginInterface visualization;
        IntegrationPluginInterface integration;
    } interface;
    
    // Internal plugin data
    void* internal_data;
    void* library_handle;  // Dynamic library handle
} PluginInterface;

// Plugin event structure
typedef struct {
    const char* event_name;
    const char* source_plugin;
    void* data;
    size_t data_size;
    uint64_t timestamp;
} PluginEvent;

// Event callback function type
typedef void (*EventCallback)(const PluginEvent* event);

// Event system interface
struct EventSystem {
    // Event registration
    bool (*subscribe)(const char* event_name, EventCallback callback);
    bool (*unsubscribe)(const char* event_name, EventCallback callback);
    
    // Event publishing
    bool (*publish)(const char* event_name, void* data, size_t size);
    bool (*publish_to_plugin)(const char* plugin_name, const char* event_name, void* data, size_t size);
    
    // Event handling
    void (*process_events)(void);
    void (*clear_events)(void);
};

// Plugin manager interface
struct PluginManager {
    PluginInterface* plugins;
    int count;
    int capacity;
    
    // Plugin loading
    bool (*load_plugin)(const char* plugin_path);
    bool (*unload_plugin)(const char* plugin_name);
    
    // Plugin discovery
    void (*scan_plugin_directory)(const char* directory);
    PluginInterface* (*find_plugin)(const char* name);
    
    // Plugin lifecycle
    bool (*init_all_plugins)(void);
    void (*cleanup_all_plugins)(void);
    void (*update_all_plugins)(void);
    
    // Plugin communication
    bool (*send_event)(const char* plugin_name, const char* event, void* data);
    bool (*broadcast_event)(const char* event, void* data);
};

// Plugin configuration
struct PluginConfig {
    char name[64];
    bool enabled;
    char config_file[256];
    void* user_data;
};

// Plugin system initialization
bool plugin_system_init(void);
void plugin_system_cleanup(void);

// Plugin interface functions
PluginInterface* get_plugin_interface(void);

#ifdef __cplusplus
}
#endif

#endif // PLUGIN_INTERFACE_H
