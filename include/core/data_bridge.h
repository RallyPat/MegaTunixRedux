#ifndef DATA_BRIDGE_H
#define DATA_BRIDGE_H

#include "../plugin/plugin_interface.h"
#include <pthread.h>
#include <map>
#include <vector>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

// Data bridge system for connecting ECU and visualization plugins
typedef struct {
    char ecu_plugin_name[64];
    char chart_plugin_name[64];
    char chart_id[64];
    char data_source[64];
    char series_name[64];
    bool active;
    float update_rate;
    uint64_t last_update;
} DataConnection;

typedef struct {
    std::map<std::string, DataConnection> connections;
    std::vector<PluginInterface*> ecu_plugins;
    std::vector<PluginInterface*> visualization_plugins;
    pthread_mutex_t bridge_mutex;
    bool initialized;
    pthread_t bridge_thread;
    bool thread_running;
} DataBridge;

// Global data bridge instance
extern DataBridge g_data_bridge;

// Data bridge functions
bool data_bridge_init(void);
void data_bridge_cleanup(void);
bool data_bridge_register_ecu_plugin(PluginInterface* plugin);
bool data_bridge_register_visualization_plugin(PluginInterface* plugin);
bool data_bridge_create_connection(const char* connection_id,
                                   const char* ecu_plugin_name,
                                   const char* chart_plugin_name,
                                   const char* chart_id,
                                   const char* data_source,
                                   const char* series_name,
                                   float update_rate);
bool data_bridge_remove_connection(const char* connection_id);
bool data_bridge_start_connection(const char* connection_id);
bool data_bridge_stop_connection(const char* connection_id);
void data_bridge_update(void);
const char* data_bridge_get_status(void);

// ECU data extraction functions
bool extract_ecu_data_point(PluginInterface* ecu_plugin, const char* data_source, float* value);
bool extract_ecu_realtime_data(PluginInterface* ecu_plugin, ECURealtimeData* data);

// Chart data injection functions
bool inject_chart_data_point(PluginInterface* viz_plugin, const char* chart_id, 
                            float x_value, float y_value, const char* series_name);

// Performance monitoring
typedef struct {
    uint64_t total_data_points;
    uint64_t successful_transfers;
    uint64_t failed_transfers;
    float average_transfer_time;
    uint64_t peak_memory_usage;
    float current_fps;
} DataBridgePerformance;

DataBridgePerformance data_bridge_get_performance_stats(void);
void data_bridge_reset_performance_stats(void);

#ifdef __cplusplus
}
#endif

#endif // DATA_BRIDGE_H

