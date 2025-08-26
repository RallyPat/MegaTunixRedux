#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <cmath>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <imgui.h>

#include "../../../include/plugin/plugin_interface.h"

// Chart data structures
typedef struct {
    float x, y;
    uint64_t timestamp;
} DataPoint;

typedef struct {
    char name[64];
    char color[16];
    std::vector<DataPoint> points;
    bool visible;
    int max_points;
} DataSeries;

typedef struct {
    char id[64];
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
    std::vector<DataSeries> series;
    uint64_t last_update;
    pthread_mutex_t data_mutex;
    
    // Rendering properties
    float viewport_x_min, viewport_x_max;
    float viewport_y_min, viewport_y_max;
    bool animations_enabled;
    int style_preset;
    char theme_name[32];
    std::vector<std::pair<std::pair<float, float>, std::string>> annotations;
} Chart;

// Global plugin context
static std::map<std::string, Chart> g_charts;
static pthread_mutex_t g_charts_mutex;
static bool g_plugin_initialized = false;
static int g_next_chart_id = 1;

// Forward declarations
static bool chart_plugin_create_chart(const char* chart_id, const char* title, int chart_type);
static bool chart_plugin_destroy_chart(const char* chart_id);
static bool chart_plugin_clear_chart_data(const char* chart_id);
static bool chart_plugin_add_data_point(const char* chart_id, float x_value, float y_value, const char* series_name);
static bool chart_plugin_add_data_series(const char* chart_id, const char* series_name, const char* color);
static bool chart_plugin_update_chart(const char* chart_id);
static bool chart_plugin_set_chart_title(const char* chart_id, const char* title);
static bool chart_plugin_set_axis_labels(const char* chart_id, const char* x_label, const char* y_label);
static bool chart_plugin_set_chart_range(const char* chart_id, float x_min, float x_max, float y_min, float y_max);
static bool chart_plugin_set_chart_type(const char* chart_id, int chart_type);
static bool chart_plugin_enable_zoom(const char* chart_id, bool enable);
static bool chart_plugin_enable_pan(const char* chart_id, bool enable);
static bool chart_plugin_enable_legend(const char* chart_id, bool enable);
static bool chart_plugin_enable_grid(const char* chart_id, bool enable);
static bool chart_plugin_export_chart_data(const char* chart_id, const char* file_path);
static bool chart_plugin_export_chart_image(const char* chart_id, const char* file_path);
static bool chart_plugin_start_streaming(const char* chart_id, const char* ecu_plugin_name, const char* data_source);
static bool chart_plugin_stop_streaming(const char* chart_id);
static bool chart_plugin_set_update_rate(const char* chart_id, int updates_per_second);
static const char* chart_plugin_get_chart_info(const char* chart_id);
static int chart_plugin_get_chart_data_count(const char* chart_id);
static bool chart_plugin_is_chart_streaming(const char* chart_id);

// Chart rendering functions
static bool chart_plugin_render_chart(const char* chart_id, float width, float height);
static bool chart_plugin_get_chart_bounds(const char* chart_id, float* x_min, float* x_max, float* y_min, float* y_max);
static bool chart_plugin_set_chart_viewport(const char* chart_id, float x_min, float x_max, float y_min, float y_max);
static bool chart_plugin_handle_mouse_input(const char* chart_id, float mouse_x, float mouse_y, int button, bool pressed);
static bool chart_plugin_get_chart_tooltip(const char* chart_id, float mouse_x, float mouse_y, char* tooltip, int max_len);

// Advanced features
static bool chart_plugin_add_annotation(const char* chart_id, float x, float y, const char* text, const char* color);
static bool chart_plugin_set_chart_style(const char* chart_id, int style_preset);
static bool chart_plugin_enable_animations(const char* chart_id, bool enable);
static bool chart_plugin_set_chart_theme(const char* chart_id, const char* theme_name);

// Helper rendering functions
static void draw_line_chart(Chart* chart, const ImVec2& pos, const ImVec2& size);
static void draw_scatter_chart(Chart* chart, const ImVec2& pos, const ImVec2& size);
static void draw_bar_chart(Chart* chart, const ImVec2& pos, const ImVec2& size);
static void draw_grid(Chart* chart, const ImVec2& pos, const ImVec2& size);
static void draw_axes(Chart* chart, const ImVec2& pos, const ImVec2& size);
static void draw_legend(Chart* chart, const ImVec2& pos, const ImVec2& size);
static ImVec2 world_to_screen(Chart* chart, float x, float y, const ImVec2& pos, const ImVec2& size);
static bool screen_to_world(Chart* chart, float screen_x, float screen_y, float* world_x, float* world_y, const ImVec2& pos, const ImVec2& size);

// Helper functions
static std::string generate_chart_id() {
    char id[32];
    snprintf(id, sizeof(id), "chart_%d", g_next_chart_id++);
    return std::string(id);
}

static bool is_valid_chart_type(int chart_type) {
    return chart_type >= 0 && chart_type < CHART_TYPE_COUNT;
}

static bool is_valid_chart_id(const char* chart_id) {
    return chart_id && strlen(chart_id) > 0;
}

static Chart* find_chart(const char* chart_id) {
    if (!is_valid_chart_id(chart_id)) return nullptr;
    
    auto it = g_charts.find(chart_id);
    return (it != g_charts.end()) ? &it->second : nullptr;
}

static DataSeries* find_series(Chart* chart, const char* series_name) {
    if (!chart || !series_name) return nullptr;
    
    for (auto& series : chart->series) {
        if (strcmp(series.name, series_name) == 0) {
            return &series;
        }
    }
    return nullptr;
}

// Chart management implementation
static bool chart_plugin_create_chart(const char* chart_id, const char* title, int chart_type) {
    if (!is_valid_chart_id(chart_id) || !title || !is_valid_chart_type(chart_type)) {
        return false;
    }
    
    pthread_mutex_lock(&g_charts_mutex);
    
    // Check if chart already exists
    if (g_charts.find(chart_id) != g_charts.end()) {
        pthread_mutex_unlock(&g_charts_mutex);
        return false;
    }
    
    // Create new chart
    Chart& chart = g_charts[chart_id];
    strncpy(chart.id, chart_id, sizeof(chart.id) - 1);
    strncpy(chart.title, title, sizeof(chart.title) - 1);
    chart.type = (ChartType)chart_type;
    chart.x_min = 0.0f;
    chart.x_max = 100.0f;
    chart.y_min = 0.0f;
    chart.y_max = 100.0f;
    chart.zoom_enabled = true;
    chart.pan_enabled = true;
    chart.legend_enabled = true;
    chart.grid_enabled = true;
    chart.update_rate = 10;
    chart.streaming = false;
    chart.last_update = 0;
    
    // Initialize rendering properties
    chart.viewport_x_min = 0.0f;
    chart.viewport_x_max = 100.0f;
    chart.viewport_y_min = 0.0f;
    chart.viewport_y_max = 100.0f;
    chart.animations_enabled = true;
    chart.style_preset = 0;
    strncpy(chart.theme_name, "default", sizeof(chart.theme_name) - 1);
    
    // Initialize mutex
    if (pthread_mutex_init(&chart.data_mutex, NULL) != 0) {
        g_charts.erase(chart_id);
        pthread_mutex_unlock(&g_charts_mutex);
        return false;
    }
    
    pthread_mutex_unlock(&g_charts_mutex);
    return true;
}

static bool chart_plugin_destroy_chart(const char* chart_id) {
    if (!is_valid_chart_id(chart_id)) return false;
    
    pthread_mutex_lock(&g_charts_mutex);
    
    auto it = g_charts.find(chart_id);
    if (it != g_charts.end()) {
        pthread_mutex_destroy(&it->second.data_mutex);
        g_charts.erase(it);
        pthread_mutex_unlock(&g_charts_mutex);
        return true;
    }
    
    pthread_mutex_unlock(&g_charts_mutex);
    return false;
}

static bool chart_plugin_clear_chart_data(const char* chart_id) {
    Chart* chart = find_chart(chart_id);
    if (!chart) return false;
    
    pthread_mutex_lock(&chart->data_mutex);
    
    for (auto& series : chart->series) {
        series.points.clear();
    }
    
    pthread_mutex_unlock(&chart->data_mutex);
    return true;
}

// Data operations implementation
static bool chart_plugin_add_data_point(const char* chart_id, float x_value, float y_value, const char* series_name) {
    Chart* chart = find_chart(chart_id);
    if (!chart || !series_name) return false;
    
    pthread_mutex_lock(&chart->data_mutex);
    
    // Find or create series
    DataSeries* series = find_series(chart, series_name);
    if (!series) {
        // Create new series with default color
        DataSeries new_series;
        strncpy(new_series.name, series_name, sizeof(new_series.name) - 1);
        strncpy(new_series.color, "#FF0000", sizeof(new_series.color) - 1);
        new_series.visible = true;
        new_series.max_points = 1000;
        chart->series.push_back(new_series);
        series = &chart->series.back();
    }
    
    // Add data point
    DataPoint point;
    point.x = x_value;
    point.y = y_value;
    point.timestamp = time(NULL);
    
    series->points.push_back(point);
    
    // Limit data points for performance
    if (series->points.size() > series->max_points) {
        series->points.erase(series->points.begin());
    }
    
    // Update chart bounds
    if (x_value < chart->x_min) chart->x_min = x_value;
    if (x_value > chart->x_max) chart->x_max = x_value;
    if (y_value < chart->y_min) chart->y_min = y_value;
    if (y_value > chart->y_max) chart->y_max = y_value;
    
    chart->last_update = point.timestamp;
    
    pthread_mutex_unlock(&chart->data_mutex);
    return true;
}

static bool chart_plugin_add_data_series(const char* chart_id, const char* series_name, const char* color) {
    Chart* chart = find_chart(chart_id);
    if (!chart || !series_name) return false;
    
    pthread_mutex_lock(&chart->data_mutex);
    
    // Check if series already exists
    if (find_series(chart, series_name)) {
        pthread_mutex_unlock(&chart->data_mutex);
        return false;
    }
    
    // Create new series
    DataSeries series;
    strncpy(series.name, series_name, sizeof(series.name) - 1);
    strncpy(series.color, color ? color : "#FF0000", sizeof(series.color) - 1);
    series.visible = true;
    series.max_points = 1000;
    
    chart->series.push_back(series);
    
    pthread_mutex_unlock(&chart->data_mutex);
    return true;
}

static bool chart_plugin_update_chart(const char* chart_id) {
    Chart* chart = find_chart(chart_id);
    if (!chart) return false;
    
    // Chart is automatically updated when data is added
    // This function can be used for manual refresh if needed
    return true;
}

// Chart configuration implementation
static bool chart_plugin_set_chart_title(const char* chart_id, const char* title) {
    Chart* chart = find_chart(chart_id);
    if (!chart || !title) return false;
    
    strncpy(chart->title, title, sizeof(chart->title) - 1);
    return true;
}

static bool chart_plugin_set_axis_labels(const char* chart_id, const char* x_label, const char* y_label) {
    Chart* chart = find_chart(chart_id);
    if (!chart) return false;
    
    if (x_label) strncpy(chart->x_label, x_label, sizeof(chart->x_label) - 1);
    if (y_label) strncpy(chart->y_label, y_label, sizeof(chart->y_label) - 1);
    
    return true;
}

static bool chart_plugin_set_chart_range(const char* chart_id, float x_min, float x_max, float y_min, float y_max) {
    Chart* chart = find_chart(chart_id);
    if (!chart) return false;
    
    chart->x_min = x_min;
    chart->x_max = x_max;
    chart->y_min = y_min;
    chart->y_max = y_max;
    
    return true;
}

static bool chart_plugin_set_chart_type(const char* chart_id, int chart_type) {
    Chart* chart = find_chart(chart_id);
    if (!chart || !is_valid_chart_type(chart_type)) return false;
    
    chart->type = (ChartType)chart_type;
    return true;
}

// Interactive features implementation
static bool chart_plugin_enable_zoom(const char* chart_id, bool enable) {
    Chart* chart = find_chart(chart_id);
    if (!chart) return false;
    
    chart->zoom_enabled = enable;
    return true;
}

static bool chart_plugin_enable_pan(const char* chart_id, bool enable) {
    Chart* chart = find_chart(chart_id);
    if (!chart) return false;
    
    chart->pan_enabled = enable;
    return true;
}

static bool chart_plugin_enable_legend(const char* chart_id, bool enable) {
    Chart* chart = find_chart(chart_id);
    if (!chart) return false;
    
    chart->legend_enabled = enable;
    return true;
}

static bool chart_plugin_enable_grid(const char* chart_id, bool enable) {
    Chart* chart = find_chart(chart_id);
    if (!chart) return false;
    
    chart->grid_enabled = enable;
    return true;
}

// Data export implementation
static bool chart_plugin_export_chart_data(const char* chart_id, const char* file_path) {
    Chart* chart = find_chart(chart_id);
    if (!chart || !file_path) return false;
    
    FILE* file = fopen(file_path, "w");
    if (!file) return false;
    
    pthread_mutex_lock(&chart->data_mutex);
    
    // Write header
    fprintf(file, "Chart: %s\n", chart->title);
    fprintf(file, "X-Axis: %s\n", chart->x_label);
    fprintf(file, "Y-Axis: %s\n", chart->y_label);
    fprintf(file, "Timestamp,Series,X,Y\n");
    
    // Write data points
    for (const auto& series : chart->series) {
        for (const auto& point : series.points) {
            fprintf(file, "%lu,%s,%.6f,%.6f\n", 
                    point.timestamp, series.name, point.x, point.y);
        }
    }
    
    pthread_mutex_unlock(&chart->data_mutex);
    fclose(file);
    
    return true;
}

static bool chart_plugin_export_chart_image(const char* chart_id, const char* file_path) {
    // TODO: Implement image export (PNG, SVG, etc.)
    // For now, just return success
    return true;
}

// Real-time streaming implementation
static bool chart_plugin_start_streaming(const char* chart_id, const char* ecu_plugin_name, const char* data_source) {
    Chart* chart = find_chart(chart_id);
    if (!chart || !ecu_plugin_name || !data_source) return false;
    
    chart->streaming = true;
    strncpy(chart->ecu_source, ecu_plugin_name, sizeof(chart->ecu_source) - 1);
    strncpy(chart->data_source, data_source, sizeof(chart->data_source) - 1);
    
    return true;
}

static bool chart_plugin_stop_streaming(const char* chart_id) {
    Chart* chart = find_chart(chart_id);
    if (!chart) return false;
    
    chart->streaming = false;
    return true;
}

static bool chart_plugin_set_update_rate(const char* chart_id, int updates_per_second) {
    Chart* chart = find_chart(chart_id);
    if (!chart || updates_per_second <= 0) return false;
    
    chart->update_rate = updates_per_second;
    return true;
}

// Chart information implementation
static const char* chart_plugin_get_chart_info(const char* chart_id) {
    static char info_buffer[512];
    Chart* chart = find_chart(chart_id);
    if (!chart) return "Chart not found";
    
    snprintf(info_buffer, sizeof(info_buffer),
             "Chart: %s\nType: %d\nSeries: %zu\nData Points: %d\nStreaming: %s\nUpdate Rate: %d Hz",
             chart->title, chart->type, chart->series.size(),
             chart_plugin_get_chart_data_count(chart_id),
             chart->streaming ? "Yes" : "No", chart->update_rate);
    
    return info_buffer;
}

static int chart_plugin_get_chart_data_count(const char* chart_id) {
    Chart* chart = find_chart(chart_id);
    if (!chart) return 0;
    
    int total_points = 0;
    pthread_mutex_lock(&chart->data_mutex);
    
    for (const auto& series : chart->series) {
        total_points += series.points.size();
    }
    
    pthread_mutex_unlock(&chart->data_mutex);
    return total_points;
}

static bool chart_plugin_is_chart_streaming(const char* chart_id) {
    Chart* chart = find_chart(chart_id);
    return chart ? chart->streaming : false;
}

// Chart rendering implementation
static bool chart_plugin_render_chart(const char* chart_id, float width, float height) {
    Chart* chart = find_chart(chart_id);
    if (!chart) return false;
    
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size(width, height);
    
    // Draw chart background
    ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), 
                                            IM_COL32(30, 30, 30, 255));
    
    // Draw grid if enabled
    if (chart->grid_enabled) {
        draw_grid(chart, pos, size);
    }
    
    // Draw axes
    draw_axes(chart, pos, size);
    
    // Draw chart based on type
    switch (chart->type) {
        case CHART_TYPE_LINE:
            draw_line_chart(chart, pos, size);
            break;
        case CHART_TYPE_SCATTER:
            draw_scatter_chart(chart, pos, size);
            break;
        case CHART_TYPE_BAR:
            draw_bar_chart(chart, pos, size);
            break;
        default:
            draw_line_chart(chart, pos, size); // Default to line chart
            break;
    }
    
    // Draw legend if enabled
    if (chart->legend_enabled) {
        draw_legend(chart, pos, size);
    }
    
    // Draw annotations
    pthread_mutex_lock(&chart->data_mutex);
    for (const auto& annotation : chart->annotations) {
        ImVec2 screen_pos = world_to_screen(chart, annotation.first.first, annotation.first.second, pos, size);
        ImGui::GetWindowDrawList()->AddText(screen_pos, IM_COL32(255, 255, 0, 255), 
                                          annotation.second.c_str());
    }
    pthread_mutex_unlock(&chart->data_mutex);
    
    return true;
}

static bool chart_plugin_get_chart_bounds(const char* chart_id, float* x_min, float* x_max, float* y_min, float* y_max) {
    Chart* chart = find_chart(chart_id);
    if (!chart || !x_min || !x_max || !y_min || !y_max) return false;
    
    *x_min = chart->x_min;
    *x_max = chart->x_max;
    *y_min = chart->y_min;
    *y_max = chart->y_max;
    
    return true;
}

static bool chart_plugin_set_chart_viewport(const char* chart_id, float x_min, float x_max, float y_min, float y_max) {
    Chart* chart = find_chart(chart_id);
    if (!chart) return false;
    
    chart->viewport_x_min = x_min;
    chart->viewport_x_max = x_max;
    chart->viewport_y_min = y_min;
    chart->viewport_y_max = y_max;
    
    return true;
}

static bool chart_plugin_handle_mouse_input(const char* chart_id, float mouse_x, float mouse_y, int button, bool pressed) {
    Chart* chart = find_chart(chart_id);
    if (!chart) return false;
    
    // Handle zoom and pan based on mouse input
    if (chart->zoom_enabled && button == 0 && pressed) { // Left click
        // Zoom in/out logic could be implemented here
        return true;
    }
    
    if (chart->pan_enabled && button == 1 && pressed) { // Right click
        // Pan logic could be implemented here
        return true;
    }
    
    return false;
}

static bool chart_plugin_get_chart_tooltip(const char* chart_id, float mouse_x, float mouse_y, char* tooltip, int max_len) {
    Chart* chart = find_chart(chart_id);
    if (!chart || !tooltip || max_len <= 0) return false;
    
    // Convert screen coordinates to world coordinates
    float world_x, world_y;
    if (screen_to_world(chart, mouse_x, mouse_y, &world_x, &world_y, ImVec2(0, 0), ImVec2(1, 1))) {
        snprintf(tooltip, max_len, "X: %.2f, Y: %.2f", world_x, world_y);
        return true;
    }
    
    return false;
}

// Advanced features implementation
static bool chart_plugin_add_annotation(const char* chart_id, float x, float y, const char* text, const char* color) {
    Chart* chart = find_chart(chart_id);
    if (!chart || !text) return false;
    
    chart->annotations.push_back({{x, y}, std::string(text)});
    return true;
}

static bool chart_plugin_set_chart_style(const char* chart_id, int style_preset) {
    Chart* chart = find_chart(chart_id);
    if (!chart) return false;
    
    chart->style_preset = style_preset;
    return true;
}

static bool chart_plugin_enable_animations(const char* chart_id, bool enable) {
    Chart* chart = find_chart(chart_id);
    if (!chart) return false;
    
    chart->animations_enabled = enable;
    return true;
}

static bool chart_plugin_set_chart_theme(const char* chart_id, const char* theme_name) {
    Chart* chart = find_chart(chart_id);
    if (!chart || !theme_name) return false;
    
    strncpy(chart->theme_name, theme_name, sizeof(chart->theme_name) - 1);
    return true;
}

// Plugin lifecycle functions
static bool chart_plugin_init(PluginContext* ctx) {
    if (g_plugin_initialized) return false;
    
    if (pthread_mutex_init(&g_charts_mutex, NULL) != 0) {
        return false;
    }
    
    g_plugin_initialized = true;
    return true;
}

static void chart_plugin_cleanup(void) {
    if (!g_plugin_initialized) return;
    
    pthread_mutex_lock(&g_charts_mutex);
    
    // Clean up all charts
    for (auto& pair : g_charts) {
        pthread_mutex_destroy(&pair.second.data_mutex);
    }
    g_charts.clear();
    
    pthread_mutex_unlock(&g_charts_mutex);
    pthread_mutex_destroy(&g_charts_mutex);
    
    g_plugin_initialized = false;
}

// Helper rendering functions
static ImVec2 world_to_screen(Chart* chart, float x, float y, const ImVec2& pos, const ImVec2& size) {
    float normalized_x = (x - chart->viewport_x_min) / (chart->viewport_x_max - chart->viewport_x_min);
    float normalized_y = (y - chart->viewport_y_min) / (chart->viewport_y_max - chart->viewport_y_min);
    
    return ImVec2(pos.x + normalized_x * size.x, pos.y + (1.0f - normalized_y) * size.y);
}

static bool screen_to_world(Chart* chart, float screen_x, float screen_y, float* world_x, float* world_y, const ImVec2& pos, const ImVec2& size) {
    if (!world_x || !world_y) return false;
    
    float normalized_x = (screen_x - pos.x) / size.x;
    float normalized_y = 1.0f - (screen_y - pos.y) / size.y;
    
    *world_x = chart->viewport_x_min + normalized_x * (chart->viewport_x_max - chart->viewport_x_min);
    *world_y = chart->viewport_y_min + normalized_y * (chart->viewport_y_max - chart->viewport_y_min);
    
    return true;
}

static void draw_grid(Chart* chart, const ImVec2& pos, const ImVec2& size) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    uint32_t grid_color = IM_COL32(60, 60, 60, 255);
    
    // Draw vertical grid lines
    for (int i = 0; i <= 10; i++) {
        float x = pos.x + (size.x * i) / 10.0f;
        draw_list->AddLine(ImVec2(x, pos.y), ImVec2(x, pos.y + size.y), grid_color, 1.0f);
    }
    
    // Draw horizontal grid lines
    for (int i = 0; i <= 10; i++) {
        float y = pos.y + (size.y * i) / 10.0f;
        draw_list->AddLine(ImVec2(pos.x, y), ImVec2(pos.x + size.x, y), grid_color, 1.0f);
    }
}

static void draw_axes(Chart* chart, const ImVec2& pos, const ImVec2& size) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    uint32_t axis_color = IM_COL32(200, 200, 200, 255);
    
    // X-axis
    draw_list->AddLine(ImVec2(pos.x, pos.y + size.y), ImVec2(pos.x + size.x, pos.y + size.y), axis_color, 2.0f);
    
    // Y-axis
    draw_list->AddLine(ImVec2(pos.x, pos.y), ImVec2(pos.x, pos.y + size.y), axis_color, 2.0f);
    
    // Axis labels
    ImGui::SetCursorScreenPos(ImVec2(pos.x + size.x/2, pos.y + size.y + 20));
    ImGui::Text("%s", chart->x_label[0] ? chart->x_label : "X");
    
    ImGui::SetCursorScreenPos(ImVec2(pos.x - 30, pos.y + size.y/2));
    ImGui::Text("%s", chart->y_label[0] ? chart->y_label : "Y");
}

static void draw_line_chart(Chart* chart, const ImVec2& pos, const ImVec2& size) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    pthread_mutex_lock(&chart->data_mutex);
    
    for (const auto& series : chart->series) {
        if (series.points.size() < 2) continue;
        
        // Parse color (simple hex parsing)
        uint32_t color = IM_COL32(255, 0, 0, 255); // Default red
        if (series.color[0] == '#') {
            int r, g, b;
            sscanf(series.color + 1, "%02x%02x%02x", &r, &g, &b);
            color = IM_COL32(r, g, b, 255);
        }
        
        // Draw line segments
        for (size_t i = 1; i < series.points.size(); i++) {
            ImVec2 p1 = world_to_screen(chart, series.points[i-1].x, series.points[i-1].y, pos, size);
            ImVec2 p2 = world_to_screen(chart, series.points[i].x, series.points[i].y, pos, size);
            draw_list->AddLine(p1, p2, color, 2.0f);
        }
        
        // Draw data points
        for (const auto& point : series.points) {
            ImVec2 screen_pos = world_to_screen(chart, point.x, point.y, pos, size);
            draw_list->AddCircleFilled(screen_pos, 3.0f, color);
        }
    }
    
    pthread_mutex_unlock(&chart->data_mutex);
}

static void draw_scatter_chart(Chart* chart, const ImVec2& pos, const ImVec2& size) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    pthread_mutex_lock(&chart->data_mutex);
    
    for (const auto& series : chart->series) {
        // Parse color
        uint32_t color = IM_COL32(255, 0, 0, 255);
        if (series.color[0] == '#') {
            int r, g, b;
            sscanf(series.color + 1, "%02x%02x%02x", &r, &g, &b);
            color = IM_COL32(r, g, b, 255);
        }
        
        // Draw data points
        for (const auto& point : series.points) {
            ImVec2 screen_pos = world_to_screen(chart, point.x, point.y, pos, size);
            draw_list->AddCircleFilled(screen_pos, 4.0f, color);
        }
    }
    
    pthread_mutex_unlock(&chart->data_mutex);
}

static void draw_bar_chart(Chart* chart, const ImVec2& pos, const ImVec2& size) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    pthread_mutex_lock(&chart->data_mutex);
    
    for (const auto& series : chart->series) {
        if (series.points.empty()) continue;
        
        // Parse color
        uint32_t color = IM_COL32(255, 0, 0, 255);
        if (series.color[0] == '#') {
            int r, g, b;
            sscanf(series.color + 1, "%02x%02x%02x", &r, &g, &b);
            color = IM_COL32(r, g, b, 255);
        }
        
        // Draw bars
        float bar_width = size.x / series.points.size();
        for (size_t i = 0; i < series.points.size(); i++) {
            const auto& point = series.points[i];
            ImVec2 bar_pos = world_to_screen(chart, point.x, point.y, pos, size);
            ImVec2 bar_size(bar_width * 0.8f, pos.y + size.y - bar_pos.y);
            
            draw_list->AddRectFilled(bar_pos, ImVec2(bar_pos.x + bar_size.x, bar_pos.y + bar_size.y), color);
        }
    }
    
    pthread_mutex_unlock(&chart->data_mutex);
}

static void draw_legend(Chart* chart, const ImVec2& pos, const ImVec2& size) {
    ImGui::SetCursorScreenPos(ImVec2(pos.x + size.x - 150, pos.y + 10));
    
    pthread_mutex_lock(&chart->data_mutex);
    
    for (const auto& series : chart->series) {
        // Parse color
        uint32_t color = IM_COL32(255, 0, 0, 255);
        if (series.color[0] == '#') {
            int r, g, b;
            sscanf(series.color + 1, "%02x%02x%02x", &r, &g, &b);
            color = IM_COL32(r, g, b, 255);
        }
        
        // Draw color indicator
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(cursor_pos.x + 8, cursor_pos.y + 8), 6, color);
        ImGui::SameLine();
        ImGui::Text("%s", series.name);
    }
    
    pthread_mutex_unlock(&chart->data_mutex);
}

static void chart_plugin_update(void) {
    if (!g_plugin_initialized) return;
    
    // Update streaming charts with simulated data
    pthread_mutex_lock(&g_charts_mutex);
    
    for (auto& pair : g_charts) {
        Chart& chart = pair.second;
        
        if (chart.streaming && chart.series.size() > 0) {
            // Simulate real-time data for demonstration
            static float time_counter = 0.0f;
            time_counter += 0.1f;
            
            // Add sample data to first series
            if (!chart.series.empty()) {
                DataSeries& series = chart.series[0];
                
                pthread_mutex_lock(&chart.data_mutex);
                
                DataPoint point;
                point.x = time_counter;
                point.y = sin(time_counter) * 50.0f + 50.0f; // Sine wave
                point.timestamp = time(NULL);
                
                series.points.push_back(point);
                if (series.points.size() > series.max_points) {
                    series.points.erase(series.points.begin());
                }
                
                pthread_mutex_unlock(&chart.data_mutex);
            }
        }
    }
    
    pthread_mutex_unlock(&g_charts_mutex);
}

// Plugin interface structure
static DataVisualizationPluginInterface g_chart_interface = {
    .create_chart = chart_plugin_create_chart,
    .destroy_chart = chart_plugin_destroy_chart,
    .clear_chart_data = chart_plugin_clear_chart_data,
    .add_data_point = chart_plugin_add_data_point,
    .add_data_series = chart_plugin_add_data_series,
    .update_chart = chart_plugin_update_chart,
    .set_chart_title = chart_plugin_set_chart_title,
    .set_axis_labels = chart_plugin_set_axis_labels,
    .set_chart_range = chart_plugin_set_chart_range,
    .set_chart_type = chart_plugin_set_chart_type,
    .enable_zoom = chart_plugin_enable_zoom,
    .enable_pan = chart_plugin_enable_pan,
    .enable_legend = chart_plugin_enable_legend,
    .enable_grid = chart_plugin_enable_grid,
    .export_chart_data = chart_plugin_export_chart_data,
    .export_chart_image = chart_plugin_export_chart_image,
    .start_streaming = chart_plugin_start_streaming,
    .stop_streaming = chart_plugin_stop_streaming,
    .set_update_rate = chart_plugin_set_update_rate,
    .get_chart_info = chart_plugin_get_chart_info,
    .get_chart_data_count = chart_plugin_get_chart_data_count,
    .is_chart_streaming = chart_plugin_is_chart_streaming,
    .render_chart = chart_plugin_render_chart,
    .get_chart_bounds = chart_plugin_get_chart_bounds,
    .set_chart_viewport = chart_plugin_set_chart_viewport,
    .handle_mouse_input = chart_plugin_handle_mouse_input,
    .get_chart_tooltip = chart_plugin_get_chart_tooltip,
    .add_annotation = chart_plugin_add_annotation,
    .set_chart_style = chart_plugin_set_chart_style,
    .enable_animations = chart_plugin_enable_animations,
    .set_chart_theme = chart_plugin_set_chart_theme
};

// Plugin interface descriptor
static PluginInterface g_chart_plugin_interface = {
    .name = "Advanced Chart Plugin",
    .version = "1.0.0",
    .author = "MegaTunix Redux Team",
    .description = "Professional data visualization with real-time charts and graphs",
    .type = PLUGIN_TYPE_DATA,
    .status = PLUGIN_STATUS_LOADED,
    .init = chart_plugin_init,
    .cleanup = chart_plugin_cleanup,
    .update = chart_plugin_update,
    .interface = {
        .visualization = g_chart_interface
    },
    .internal_data = nullptr,
    .library_handle = nullptr
};

// Plugin entry point
extern "C" PluginInterface* get_plugin_interface(void) {
    return &g_chart_plugin_interface;
}
