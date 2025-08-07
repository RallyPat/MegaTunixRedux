#ifndef IMGUI_COMMUNICATIONS_H
#define IMGUI_COMMUNICATIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../ecu/ecu_communication.h"
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

// Communications statistics
typedef struct {
    uint32_t bytes_received;
    uint32_t bytes_sent;
    uint32_t packets_received;
    uint32_t packets_sent;
    uint32_t errors_count;
    uint32_t timeouts_count;
    uint32_t last_activity;
    float data_rate_rx;  // bytes per second
    float data_rate_tx;  // bytes per second
    float packet_rate_rx; // packets per second
    float packet_rate_tx; // packets per second
} CommunicationsStats;

// Callback function types
typedef void (*LogCallback)(int level, const char* format, ...);
typedef bool (*VETableResizeCallback)(int width, int height);
typedef void (*VETableSetAxisRangesCallback)(float x_min, float x_max, float y_min, float y_max);
typedef void (*VETableSetAxisNamesCallback)(const char* x_name, const char* y_name, const char* x_units, const char* y_units);
typedef void (*VETableSetMetadataCallback)(float scale, float min_value, float max_value);

// Communications UI state
typedef struct {
    ECUContext* ecu_ctx;
    bool initialized;
    bool show_connection_dialog;
    bool show_port_scan;
    bool show_statistics;
    bool show_protocol_info;
    bool auto_connect_enabled;
    bool auto_reconnect_enabled;
    int selected_protocol;
    int selected_port;
    int selected_baud_rate;
    int timeout_ms;
    int reconnect_interval;
    
    // Port scanning
    SerialPortList detected_ports;
    bool scanning_ports;
    uint32_t scan_start_time;
    
    // Statistics
    CommunicationsStats stats;
    uint32_t stats_last_update;
    
    // Error tracking
    char last_error[256];
    bool error_occurred;
    uint32_t error_time;
    
    // Connection state tracking
    bool connecting;
    uint32_t connect_start_time;
    char connecting_message[256];
    
    // Asynchronous connection support
    SDL_Thread* connection_thread;
    bool connection_thread_running;
    ECUConfig pending_connection_config;
    bool connection_result;
    bool connection_completed;
    
    // Connection history
    char connection_history[10][256];
    int history_count;
    int history_index;
    
    // INI file support
    bool use_ini_file;
    char ini_file_path[512];
    char detected_ecu_name[64];
    char detected_ecu_version[64];
    bool ini_file_valid;
    char ini_error_message[256];
    
    // File dialog support
    void* file_dialog;
    bool show_file_dialog;
    
    // Dynamic protocol support
    void* dynamic_protocol_manager;
    bool show_protocol_manager;
    char import_ini_path[512];
    bool show_import_dialog;
    
    // Demo mode support
    bool demo_mode_enabled;
    char demo_ini_file_path[256];
    bool demo_ini_file_valid;
    char demo_ecu_name[64];
    char demo_ecu_version[64];
    char demo_error_message[256];
    
    // VE table configuration callbacks
    VETableResizeCallback ve_table_resize_callback;
    VETableSetAxisRangesCallback ve_table_set_axis_ranges_callback;
    VETableSetAxisNamesCallback ve_table_set_axis_names_callback;
    VETableSetMetadataCallback ve_table_set_metadata_callback;
    
    // Log callback
    LogCallback log_callback;
} ImGuiCommunications;

// Function declarations
ImGuiCommunications* imgui_communications_create(ECUContext* ecu_ctx);
void imgui_communications_destroy(ImGuiCommunications* comms);
void imgui_communications_update(ImGuiCommunications* comms);
void imgui_communications_render(ImGuiCommunications* comms);

// Set log callback for communications module
void imgui_communications_set_log_callback(ImGuiCommunications* comms, LogCallback callback);

// Set VE table configuration callbacks
void imgui_communications_set_ve_table_callbacks(ImGuiCommunications* comms,
                                                VETableResizeCallback resize_callback,
                                                VETableSetAxisRangesCallback axis_ranges_callback,
                                                VETableSetAxisNamesCallback axis_names_callback,
                                                VETableSetMetadataCallback metadata_callback);

// Helper functions
void imgui_render_connection_dialog(ImGuiCommunications* comms);
void imgui_render_port_scan(ImGuiCommunications* comms);
void imgui_render_statistics(ImGuiCommunications* comms);
void imgui_render_protocol_info(ImGuiCommunications* comms);
void imgui_render_connection_history(ImGuiCommunications* comms);
void imgui_render_protocol_manager(ImGuiCommunications* comms);
void imgui_render_import_dialog(ImGuiCommunications* comms);

#ifdef __cplusplus
}
#endif

#endif // IMGUI_COMMUNICATIONS_H 