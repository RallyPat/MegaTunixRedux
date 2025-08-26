#include "../../include/core/data_bridge.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <cmath>

// Global data bridge instance
DataBridge g_data_bridge;

// Performance statistics
static DataBridgePerformance g_performance_stats;

// Forward declarations
static void* data_bridge_thread_func(void* arg);
static uint64_t get_timestamp_us(void);
static void update_performance_stats(uint64_t start_time, bool success);

bool data_bridge_init(void) {
    if (g_data_bridge.initialized) return false;
    
    if (pthread_mutex_init(&g_data_bridge.bridge_mutex, NULL) != 0) {
        return false;
    }
    
    g_data_bridge.initialized = true;
    g_data_bridge.thread_running = false;
    
    // Initialize performance stats
    memset(&g_performance_stats, 0, sizeof(g_performance_stats));
    
    printf("[DataBridge] Data bridge system initialized\n");
    return true;
}

void data_bridge_cleanup(void) {
    if (!g_data_bridge.initialized) return;
    
    // Stop bridge thread
    if (g_data_bridge.thread_running) {
        g_data_bridge.thread_running = false;
        pthread_join(g_data_bridge.bridge_thread, NULL);
    }
    
    pthread_mutex_lock(&g_data_bridge.bridge_mutex);
    
    // Clear all connections
    g_data_bridge.connections.clear();
    g_data_bridge.ecu_plugins.clear();
    g_data_bridge.visualization_plugins.clear();
    
    pthread_mutex_unlock(&g_data_bridge.bridge_mutex);
    pthread_mutex_destroy(&g_data_bridge.bridge_mutex);
    
    g_data_bridge.initialized = false;
    printf("[DataBridge] Data bridge system cleaned up\n");
}

bool data_bridge_register_ecu_plugin(PluginInterface* plugin) {
    if (!plugin || plugin->type != PLUGIN_TYPE_ECU) return false;
    
    pthread_mutex_lock(&g_data_bridge.bridge_mutex);
    
    // Check if already registered
    for (auto* existing : g_data_bridge.ecu_plugins) {
        if (existing == plugin) {
            pthread_mutex_unlock(&g_data_bridge.bridge_mutex);
            return false;
        }
    }
    
    g_data_bridge.ecu_plugins.push_back(plugin);
    pthread_mutex_unlock(&g_data_bridge.bridge_mutex);
    
    printf("[DataBridge] Registered ECU plugin: %s\n", plugin->name);
    return true;
}

bool data_bridge_register_visualization_plugin(PluginInterface* plugin) {
    if (!plugin || plugin->type != PLUGIN_TYPE_DATA) return false;
    
    pthread_mutex_lock(&g_data_bridge.bridge_mutex);
    
    // Check if already registered
    for (auto* existing : g_data_bridge.visualization_plugins) {
        if (existing == plugin) {
            pthread_mutex_unlock(&g_data_bridge.bridge_mutex);
            return false;
        }
    }
    
    g_data_bridge.visualization_plugins.push_back(plugin);
    pthread_mutex_unlock(&g_data_bridge.bridge_mutex);
    
    printf("[DataBridge] Registered visualization plugin: %s\n", plugin->name);
    return true;
}

bool data_bridge_create_connection(const char* connection_id,
                                   const char* ecu_plugin_name,
                                   const char* chart_plugin_name,
                                   const char* chart_id,
                                   const char* data_source,
                                   const char* series_name,
                                   float update_rate) {
    if (!connection_id || !ecu_plugin_name || !chart_plugin_name || 
        !chart_id || !data_source || !series_name) {
        return false;
    }
    
    pthread_mutex_lock(&g_data_bridge.bridge_mutex);
    
    // Check if connection already exists
    if (g_data_bridge.connections.find(connection_id) != g_data_bridge.connections.end()) {
        pthread_mutex_unlock(&g_data_bridge.bridge_mutex);
        return false;
    }
    
    // Create new connection
    DataConnection& conn = g_data_bridge.connections[connection_id];
    strncpy(conn.ecu_plugin_name, ecu_plugin_name, sizeof(conn.ecu_plugin_name) - 1);
    strncpy(conn.chart_plugin_name, chart_plugin_name, sizeof(conn.chart_plugin_name) - 1);
    strncpy(conn.chart_id, chart_id, sizeof(conn.chart_id) - 1);
    strncpy(conn.data_source, data_source, sizeof(conn.data_source) - 1);
    strncpy(conn.series_name, series_name, sizeof(conn.series_name) - 1);
    conn.active = false;
    conn.update_rate = update_rate;
    conn.last_update = 0;
    
    pthread_mutex_unlock(&g_data_bridge.bridge_mutex);
    
    printf("[DataBridge] Created connection: %s (%s -> %s)\n", 
           connection_id, ecu_plugin_name, chart_plugin_name);
    return true;
}

bool data_bridge_remove_connection(const char* connection_id) {
    if (!connection_id) return false;
    
    pthread_mutex_lock(&g_data_bridge.bridge_mutex);
    
    auto it = g_data_bridge.connections.find(connection_id);
    if (it != g_data_bridge.connections.end()) {
        g_data_bridge.connections.erase(it);
        pthread_mutex_unlock(&g_data_bridge.bridge_mutex);
        printf("[DataBridge] Removed connection: %s\n", connection_id);
        return true;
    }
    
    pthread_mutex_unlock(&g_data_bridge.bridge_mutex);
    return false;
}

bool data_bridge_start_connection(const char* connection_id) {
    if (!connection_id) return false;
    
    pthread_mutex_lock(&g_data_bridge.bridge_mutex);
    
    auto it = g_data_bridge.connections.find(connection_id);
    if (it != g_data_bridge.connections.end()) {
        it->second.active = true;
        
        // Start bridge thread if not running
        if (!g_data_bridge.thread_running) {
            g_data_bridge.thread_running = true;
            if (pthread_create(&g_data_bridge.bridge_thread, NULL, data_bridge_thread_func, NULL) != 0) {
                g_data_bridge.thread_running = false;
                pthread_mutex_unlock(&g_data_bridge.bridge_mutex);
                return false;
            }
        }
        
        pthread_mutex_unlock(&g_data_bridge.bridge_mutex);
        printf("[DataBridge] Started connection: %s\n", connection_id);
        return true;
    }
    
    pthread_mutex_unlock(&g_data_bridge.bridge_mutex);
    return false;
}

bool data_bridge_stop_connection(const char* connection_id) {
    if (!connection_id) return false;
    
    pthread_mutex_lock(&g_data_bridge.bridge_mutex);
    
    auto it = g_data_bridge.connections.find(connection_id);
    if (it != g_data_bridge.connections.end()) {
        it->second.active = false;
        pthread_mutex_unlock(&g_data_bridge.bridge_mutex);
        printf("[DataBridge] Stopped connection: %s\n", connection_id);
        return true;
    }
    
    pthread_mutex_unlock(&g_data_bridge.bridge_mutex);
    return false;
}

void data_bridge_update(void) {
    if (!g_data_bridge.initialized) return;
    
    uint64_t current_time = get_timestamp_us();
    
    pthread_mutex_lock(&g_data_bridge.bridge_mutex);
    
    for (auto& pair : g_data_bridge.connections) {
        DataConnection& conn = pair.second;
        
        if (!conn.active) continue;
        
        // Check if it's time to update
        uint64_t interval_us = (uint64_t)(1000000.0f / conn.update_rate);
        if (current_time - conn.last_update < interval_us) continue;
        
        // Find ECU plugin
        PluginInterface* ecu_plugin = nullptr;
        for (auto* plugin : g_data_bridge.ecu_plugins) {
            if (strcmp(plugin->name, conn.ecu_plugin_name) == 0) {
                ecu_plugin = plugin;
                break;
            }
        }
        
        // Find visualization plugin
        PluginInterface* viz_plugin = nullptr;
        for (auto* plugin : g_data_bridge.visualization_plugins) {
            if (strcmp(plugin->name, conn.chart_plugin_name) == 0) {
                viz_plugin = plugin;
                break;
            }
        }
        
        if (!ecu_plugin || !viz_plugin) continue;
        
        uint64_t start_time = get_timestamp_us();
        
        // Extract data from ECU plugin
        float value;
        bool success = extract_ecu_data_point(ecu_plugin, conn.data_source, &value);
        
        if (success) {
            // Inject data into visualization plugin
            float x_value = current_time / 1000000.0f; // Convert to seconds
            success = inject_chart_data_point(viz_plugin, conn.chart_id, x_value, value, conn.series_name);
        }
        
        conn.last_update = current_time;
        update_performance_stats(start_time, success);
    }
    
    pthread_mutex_unlock(&g_data_bridge.bridge_mutex);
}

const char* data_bridge_get_status(void) {
    static char status_buffer[512];
    
    pthread_mutex_lock(&g_data_bridge.bridge_mutex);
    
    int active_connections = 0;
    for (const auto& pair : g_data_bridge.connections) {
        if (pair.second.active) active_connections++;
    }
    
    snprintf(status_buffer, sizeof(status_buffer),
             "Data Bridge Status:\n"
             "ECU Plugins: %zu\n"
             "Visualization Plugins: %zu\n"
             "Total Connections: %zu\n"
             "Active Connections: %d\n"
             "Thread Running: %s\n"
             "Data Points Transferred: %lu\n"
             "Success Rate: %.1f%%",
             g_data_bridge.ecu_plugins.size(),
             g_data_bridge.visualization_plugins.size(),
             g_data_bridge.connections.size(),
             active_connections,
             g_data_bridge.thread_running ? "Yes" : "No",
             g_performance_stats.total_data_points,
             g_performance_stats.total_data_points > 0 ? 
                (100.0f * g_performance_stats.successful_transfers / g_performance_stats.total_data_points) : 0.0f);
    
    pthread_mutex_unlock(&g_data_bridge.bridge_mutex);
    
    return status_buffer;
}

// ECU data extraction functions
bool extract_ecu_data_point(PluginInterface* ecu_plugin, const char* data_source, float* value) {
    if (!ecu_plugin || !data_source || !value) return false;
    
    // Get real-time data from ECU plugin
    ECURealtimeData ecu_data;
    if (!ecu_plugin->interface.ecu.read_realtime_data || 
        !ecu_plugin->interface.ecu.read_realtime_data(&ecu_data)) {
        return false;
    }
    
    // Extract specific data source
    if (strcmp(data_source, "rpm") == 0) {
        *value = ecu_data.rpm;
    } else if (strcmp(data_source, "map") == 0) {
        *value = ecu_data.map;
    } else if (strcmp(data_source, "air_temp") == 0) {
        *value = ecu_data.air_temp;
    } else if (strcmp(data_source, "timing") == 0) {
        *value = ecu_data.timing;
    } else if (strcmp(data_source, "battery_voltage") == 0) {
        *value = ecu_data.battery_voltage;
    } else {
        return false; // Unknown data source
    }
    
    return true;
}

bool extract_ecu_realtime_data(PluginInterface* ecu_plugin, ECURealtimeData* data) {
    if (!ecu_plugin || !data) return false;
    
    return ecu_plugin->interface.ecu.read_realtime_data && 
           ecu_plugin->interface.ecu.read_realtime_data(data);
}

// Chart data injection functions
bool inject_chart_data_point(PluginInterface* viz_plugin, const char* chart_id, 
                            float x_value, float y_value, const char* series_name) {
    if (!viz_plugin || !chart_id || !series_name) return false;
    
    return viz_plugin->interface.visualization.add_data_point &&
           viz_plugin->interface.visualization.add_data_point(chart_id, x_value, y_value, series_name);
}

// Performance monitoring
DataBridgePerformance data_bridge_get_performance_stats(void) {
    return g_performance_stats;
}

void data_bridge_reset_performance_stats(void) {
    memset(&g_performance_stats, 0, sizeof(g_performance_stats));
}

// Helper functions
static void* data_bridge_thread_func(void* arg) {
    (void)arg; // Suppress unused parameter warning
    
    printf("[DataBridge] Bridge thread started\n");
    
    while (g_data_bridge.thread_running) {
        data_bridge_update();
        usleep(10000); // 10ms sleep for 100Hz update rate
    }
    
    printf("[DataBridge] Bridge thread stopped\n");
    return NULL;
}

static uint64_t get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static void update_performance_stats(uint64_t start_time, bool success) {
    uint64_t end_time = get_timestamp_us();
    float transfer_time = (end_time - start_time) / 1000.0f; // Convert to milliseconds
    
    g_performance_stats.total_data_points++;
    if (success) {
        g_performance_stats.successful_transfers++;
    } else {
        g_performance_stats.failed_transfers++;
    }
    
    // Update rolling average of transfer time
    if (g_performance_stats.total_data_points == 1) {
        g_performance_stats.average_transfer_time = transfer_time;
    } else {
        g_performance_stats.average_transfer_time = 
            (g_performance_stats.average_transfer_time * 0.9f) + (transfer_time * 0.1f);
    }
    
    // Simple FPS calculation (updates per second)
    static uint64_t last_fps_update = 0;
    static uint64_t fps_counter = 0;
    fps_counter++;
    
    if (end_time - last_fps_update >= 1000000) { // 1 second
        g_performance_stats.current_fps = fps_counter;
        fps_counter = 0;
        last_fps_update = end_time;
    }
}

