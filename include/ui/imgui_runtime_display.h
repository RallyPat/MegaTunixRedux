#ifndef IMGUI_RUNTIME_DISPLAY_H
#define IMGUI_RUNTIME_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../ecu/ecu_communication.h"
#include <stdbool.h>

// Historical data point
typedef struct {
    float value;
    uint32_t timestamp;
} DataPoint;

// Historical data series
typedef struct {
    char name[32];
    DataPoint points[1000];  // 1000 data points for history
    int point_count;
    int max_points;
    float min_value;
    float max_value;
    bool enabled;
    void* color;  // ImVec4* in C++ implementation
} DataSeries;

// Real-time chart configuration
typedef struct {
    char title[64];
    DataSeries series[8];  // Up to 8 data series per chart
    int series_count;
    float time_window_seconds;  // How much history to show
    bool show_grid;
    bool show_legend;
    bool auto_scale;
    float min_scale;
    float max_scale;
    bool enabled;
} RealTimeChart;

// Enhanced gauge configuration
typedef struct {
    char label[32];
    char unit[16];
    float min_value;
    float max_value;
    float warning_threshold;
    float danger_threshold;
    bool enabled;
    void* normal_color;   // ImVec4* in C++ implementation
    void* warning_color;  // ImVec4* in C++ implementation
    void* danger_color;   // ImVec4* in C++ implementation
    bool show_value;
    bool show_min_max;
    bool show_thresholds;
    int gauge_style;  // 0=bar, 1=round, 2=digital, 3=linear
} RuntimeGaugeConfig;

// Alert configuration
typedef struct {
    char name[64];
    char condition[128];
    float threshold;
    bool enabled;
    bool triggered;
    uint32_t trigger_time;
    uint32_t last_check;
    void* color;  // ImVec4* in C++ implementation
    int priority;  // 0=info, 1=warning, 2=danger, 3=critical
} AlertConfig;

// ImGui Runtime Display state
typedef struct {
    ECUContext* ecu_ctx;
    bool initialized;
    bool show_gauges;
    bool show_charts;
    bool show_digital_readouts;
    bool show_alerts;
    bool use_round_gauges;
    bool show_gauge_config;
    bool show_chart_config;
    bool show_alert_config;
    int selected_layout;
    int refresh_rate_ms;
    float gauge_size;
    bool auto_refresh;
    uint32_t last_update;
    
    // Enhanced features
    RuntimeGaugeConfig gauge_configs[16];
    int gauge_config_count;
    
    RealTimeChart charts[4];  // Up to 4 charts
    int chart_count;
    
    AlertConfig alerts[16];  // Up to 16 alerts
    int alert_count;
    
    // Data history
    DataSeries rpm_history;
    DataSeries map_history;
    DataSeries tps_history;
    DataSeries afr_history;
    DataSeries boost_history;
    DataSeries temp_history;
    DataSeries voltage_history;
    DataSeries timing_history;
    
    // Performance tracking
    uint32_t frame_count;
    uint32_t last_fps_update;
    float current_fps;
    uint32_t data_points_received;
    
    // UI state
    bool show_performance_stats;
    bool show_data_history;
    bool show_advanced_controls;
    int selected_chart;
    int selected_gauge;
    
    // Demo mode
    bool demo_mode_enabled;
} ImGuiRuntimeDisplay;

// Function declarations
ImGuiRuntimeDisplay* imgui_runtime_display_create(ECUContext* ecu_ctx);
void imgui_runtime_display_destroy(ImGuiRuntimeDisplay* display);
void imgui_runtime_display_update(ImGuiRuntimeDisplay* display);
void imgui_runtime_display_render(ImGuiRuntimeDisplay* display);

// Enhanced gauge rendering functions
void imgui_render_gauge(const char* label, float value, float min_val, float max_val, 
                       float warning_threshold, float danger_threshold, 
                       const char* unit, float size, bool show_value);
void imgui_render_round_gauge(const char* label, float value, float min_val, float max_val,
                             float warning_threshold, float danger_threshold,
                             const char* unit, float size, bool show_value);
void imgui_render_digital_gauge(const char* label, float value, const char* unit, float size);
void imgui_render_linear_gauge(const char* label, float value, float min_val, float max_val,
                              float warning_threshold, float danger_threshold,
                              const char* unit, float size, bool show_value);
void imgui_render_digital_readout(const char* label, float value, const char* unit);

// Real-time chart functions
void imgui_render_real_time_chart(RealTimeChart* chart, uint32_t current_time);
void imgui_render_chart_config_panel(RealTimeChart* charts, int count);
void imgui_add_data_point(DataSeries* series, float value, uint32_t timestamp);
void imgui_clear_data_series(DataSeries* series);

// Alert system functions
void imgui_render_alerts_panel(AlertConfig* alerts, int count, const ECUData* data);
void imgui_check_alerts(AlertConfig* alerts, int count, const ECUData* data);
void imgui_render_alert_config_panel(AlertConfig* alerts, int count);

// Gauge configuration functions
void imgui_render_gauge_config_panel(RuntimeGaugeConfig* configs, int count);
void imgui_load_default_gauge_configs(RuntimeGaugeConfig* configs);
void imgui_load_default_charts(RealTimeChart* charts, int* count);
void imgui_load_default_alerts(AlertConfig* alerts, int* count);

// Data history functions
void imgui_update_data_history(ImGuiRuntimeDisplay* display, const ECUData* data);
void imgui_render_data_history_panel(ImGuiRuntimeDisplay* display);

// Performance monitoring
void imgui_update_performance_stats(ImGuiRuntimeDisplay* display);
void imgui_render_performance_panel(ImGuiRuntimeDisplay* display);

#ifdef __cplusplus
}
#endif

#endif // IMGUI_RUNTIME_DISPLAY_H 