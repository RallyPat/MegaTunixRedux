#include "../../include/ui/imgui_runtime_display.h"
#include "../../external/imgui/imgui.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL.h>

// Create ImGui Runtime Display
ImGuiRuntimeDisplay* imgui_runtime_display_create(ECUContext* ecu_ctx) {
    ImGuiRuntimeDisplay* display = (ImGuiRuntimeDisplay*)malloc(sizeof(ImGuiRuntimeDisplay));
    if (!display) return NULL;
    
    // Initialize display state
    memset(display, 0, sizeof(ImGuiRuntimeDisplay));
    
    // Store ECU context directly
    display->ecu_ctx = ecu_ctx;
    
    // Initialize display settings
    display->show_gauges = true;
    display->show_charts = true;
    display->show_digital_readouts = true;
    display->show_alerts = true;
    display->use_round_gauges = true;
    display->show_gauge_config = false;
    display->show_chart_config = false;
    display->show_alert_config = false;
    display->selected_layout = 3; // 3x3 Grid
    display->refresh_rate_ms = 16; // 60 FPS (16.67ms)
    display->gauge_size = 120.0f;
    display->auto_refresh = true;
    display->last_update = 0;
    
    // Performance tracking
    display->frame_count = 0;
    display->last_fps_update = 0;
    display->current_fps = 0.0f;
    display->data_points_received = 0;
    
    // UI state
    display->show_performance_stats = false;
    display->show_data_history = false;
    display->show_advanced_controls = false;
    display->selected_chart = 0;
    display->selected_gauge = 0;
    
    // Demo mode (disabled by default)
    display->demo_mode_enabled = false;
    
    display->initialized = true;
    
    // Initialize gauge configurations
    display->gauge_config_count = 8;
    imgui_load_default_gauge_configs(display->gauge_configs);
    
    // Initialize charts
    display->chart_count = 2;
    imgui_load_default_charts(display->charts, &display->chart_count);
    
    // Initialize alerts
    display->alert_count = 8;
    imgui_load_default_alerts(display->alerts, &display->alert_count);
    
    // Initialize data history
    imgui_clear_data_series(&display->rpm_history);
    imgui_clear_data_series(&display->map_history);
    imgui_clear_data_series(&display->tps_history);
    imgui_clear_data_series(&display->afr_history);
    imgui_clear_data_series(&display->boost_history);
    imgui_clear_data_series(&display->temp_history);
    imgui_clear_data_series(&display->voltage_history);
    imgui_clear_data_series(&display->timing_history);
    
    strcpy(display->rpm_history.name, "RPM");
    strcpy(display->map_history.name, "MAP");
    strcpy(display->tps_history.name, "TPS");
    strcpy(display->afr_history.name, "AFR");
    strcpy(display->boost_history.name, "Boost");
    strcpy(display->temp_history.name, "Temp");
    strcpy(display->voltage_history.name, "Voltage");
    strcpy(display->timing_history.name, "Timing");
    
    display->rpm_history.enabled = true;
    display->map_history.enabled = true;
    display->afr_history.enabled = true;
    display->boost_history.enabled = true;
    
    return display;
}

// Destroy ImGui Runtime Display
void imgui_runtime_display_destroy(ImGuiRuntimeDisplay* display) {
    if (!display) return;
    
    free(display);
}

// Update runtime display
void imgui_runtime_display_update(ImGuiRuntimeDisplay* display) {
    if (!display || !display->initialized) return;
    
    uint32_t current_time = SDL_GetTicks();
    
    // Always update performance stats for accurate FPS measurement
    imgui_update_performance_stats(display);
    
    // Update data more frequently for better responsiveness
    if (display->auto_refresh && (current_time - display->last_update) >= display->refresh_rate_ms) {
        display->last_update = current_time;
        
        // Update data history if we have ECU data or demo mode is enabled
        const ECUData* data = ecu_get_data(display->ecu_ctx);
        if (data) {
            // Throttle data updates when ECU is connected to maintain performance
            static int data_update_counter = 0;
            if (++data_update_counter >= 2) { // Only update every 2 frames when ECU connected
                imgui_update_data_history(display, data);
                data_update_counter = 0;
            }
            
            // Only check alerts every few updates to reduce CPU usage
            static int alert_check_counter = 0;
            if (++alert_check_counter >= 10) { // Reduced frequency for better performance
                imgui_check_alerts(display->alerts, display->alert_count, data);
                alert_check_counter = 0;
            }
        } else if (display->demo_mode_enabled) {
            // Demo data - update every frame since it's lightweight
            imgui_update_data_history(display, NULL); // Pass NULL to trigger demo data generation
        }
    }
}

// Add data point to series
void imgui_add_data_point(DataSeries* series, float value, uint32_t timestamp) {
    if (!series || !series->enabled) return;
    
    // Add new point
    if (series->point_count < series->max_points) {
        series->points[series->point_count].value = value;
        series->points[series->point_count].timestamp = timestamp;
        series->point_count++;
    } else {
        // Shift all points left and add new one at end
        // Use memmove for better performance
        memmove(&series->points[0], &series->points[1], (series->max_points - 1) * sizeof(DataPoint));
        series->points[series->max_points - 1].value = value;
        series->points[series->max_points - 1].timestamp = timestamp;
    }
    
    // Update min/max more efficiently
    if (series->point_count == 1) {
        series->min_value = value;
        series->max_value = value;
    } else {
        // Only update if the new value is outside current range
        if (value < series->min_value) series->min_value = value;
        if (value > series->max_value) series->max_value = value;
    }
}

// Clear data series
void imgui_clear_data_series(DataSeries* series) {
    if (!series) return;
    
    series->point_count = 0;
    series->max_points = 1000;
    series->min_value = 0.0f;
    series->max_value = 0.0f;
    series->enabled = false;
    series->color = new ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
}

// Update data history
void imgui_update_data_history(ImGuiRuntimeDisplay* display, const ECUData* data) {
    if (!display) return;
    
    uint32_t current_time = SDL_GetTicks();
    
    // If we have real ECU data, use it; otherwise check if demo mode is enabled
    if (data) {
        imgui_add_data_point(&display->rpm_history, data->rpm, current_time);
        imgui_add_data_point(&display->map_history, data->map, current_time);
        imgui_add_data_point(&display->tps_history, data->tps, current_time);
        imgui_add_data_point(&display->afr_history, data->afr, current_time);
        imgui_add_data_point(&display->boost_history, data->boost, current_time);
        imgui_add_data_point(&display->temp_history, data->coolant_temp, current_time);
        imgui_add_data_point(&display->voltage_history, data->battery_voltage, current_time);
        imgui_add_data_point(&display->timing_history, data->timing, current_time);
    } else if (display->demo_mode_enabled) {
        // Generate demo data only when demo mode is explicitly enabled
        static float demo_time = 0.0f;
        demo_time += 0.1f; // Increment time for demo
        
        // Generate realistic demo values
        float demo_rpm = 800.0f + 2000.0f * sin(demo_time * 0.5f) + 500.0f * sin(demo_time * 2.0f);
        float demo_map = 30.0f + 50.0f * sin(demo_time * 0.3f);
        float demo_tps = 10.0f + 30.0f * sin(demo_time * 0.7f);
        float demo_afr = 14.7f + 2.0f * sin(demo_time * 0.4f);
        float demo_boost = 5.0f + 8.0f * sin(demo_time * 0.6f);
        float demo_temp = 90.0f + 10.0f * sin(demo_time * 0.2f);
        float demo_voltage = 13.5f + 0.5f * sin(demo_time * 0.8f);
        float demo_timing = 15.0f + 10.0f * sin(demo_time * 0.9f);
        
        imgui_add_data_point(&display->rpm_history, demo_rpm, current_time);
        imgui_add_data_point(&display->map_history, demo_map, current_time);
        imgui_add_data_point(&display->tps_history, demo_tps, current_time);
        imgui_add_data_point(&display->afr_history, demo_afr, current_time);
        imgui_add_data_point(&display->boost_history, demo_boost, current_time);
        imgui_add_data_point(&display->temp_history, demo_temp, current_time);
        imgui_add_data_point(&display->voltage_history, demo_voltage, current_time);
        imgui_add_data_point(&display->timing_history, demo_timing, current_time);
    }
    
    // Update chart series with data from individual history series
    for (int c = 0; c < display->chart_count; c++) {
        RealTimeChart* chart = &display->charts[c];
        if (!chart->enabled) continue;
        
        for (int s = 0; s < chart->series_count; s++) {
            DataSeries* chart_series = &chart->series[s];
            if (!chart_series->enabled) continue;
            
            // Link chart series to corresponding data history
            if (strcmp(chart_series->name, "RPM") == 0) {
                // Copy data from rpm_history to chart series
                chart_series->point_count = display->rpm_history.point_count;
                chart_series->min_value = display->rpm_history.min_value;
                chart_series->max_value = display->rpm_history.max_value;
                memcpy(chart_series->points, display->rpm_history.points, 
                       sizeof(DataPoint) * display->rpm_history.point_count);
            } else if (strcmp(chart_series->name, "MAP") == 0) {
                chart_series->point_count = display->map_history.point_count;
                chart_series->min_value = display->map_history.min_value;
                chart_series->max_value = display->map_history.max_value;
                memcpy(chart_series->points, display->map_history.points, 
                       sizeof(DataPoint) * display->map_history.point_count);
            } else if (strcmp(chart_series->name, "TPS") == 0) {
                chart_series->point_count = display->tps_history.point_count;
                chart_series->min_value = display->tps_history.min_value;
                chart_series->max_value = display->tps_history.max_value;
                memcpy(chart_series->points, display->tps_history.points, 
                       sizeof(DataPoint) * display->tps_history.point_count);
            } else if (strcmp(chart_series->name, "AFR") == 0) {
                chart_series->point_count = display->afr_history.point_count;
                chart_series->min_value = display->afr_history.min_value;
                chart_series->max_value = display->afr_history.max_value;
                memcpy(chart_series->points, display->afr_history.points, 
                       sizeof(DataPoint) * display->afr_history.point_count);
            } else if (strcmp(chart_series->name, "AFR Target") == 0) {
                // Use AFR data for target (you can modify this for actual target data)
                chart_series->point_count = display->afr_history.point_count;
                chart_series->min_value = display->afr_history.min_value;
                chart_series->max_value = display->afr_history.max_value;
                memcpy(chart_series->points, display->afr_history.points, 
                       sizeof(DataPoint) * display->afr_history.point_count);
            }
        }
    }
    
    display->data_points_received++;
}

// Update performance stats
void imgui_update_performance_stats(ImGuiRuntimeDisplay* display) {
    if (!display) return;
    
    display->frame_count++;
    uint32_t current_time = SDL_GetTicks();
    
    // Update FPS every second
    if (current_time - display->last_fps_update >= 1000) {
        display->current_fps = (float)display->frame_count * 1000.0f / (current_time - display->last_fps_update);
        display->frame_count = 0;
        display->last_fps_update = current_time;
    }
}

// Render a gauge using ImGui
void imgui_render_gauge(const char* label, float value, float min_val, float max_val, 
                       float warning_threshold, float danger_threshold, 
                       const char* unit, float size, bool show_value) {
    if (!label) return;
    
    // Calculate percentage
    float percentage = 0.0f;
    if (max_val > min_val) {
        percentage = (value - min_val) / (max_val - min_val);
        percentage = (percentage < 0.0f) ? 0.0f : (percentage > 1.0f) ? 1.0f : percentage;
    }
    
    // Determine color based on thresholds
    ImVec4 color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f); // Green (normal)
    if (danger_threshold > 0 && value >= danger_threshold) {
        color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f); // Red (danger)
    } else if (warning_threshold > 0 && value >= warning_threshold) {
        color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f); // Yellow (warning)
    }
    
    // Create gauge container
    ImGui::BeginGroup();
    
    // Gauge label
    ImGui::Text("%s", label);
    
    // Gauge progress bar
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
    ImGui::ProgressBar(percentage, ImVec2(size, 20.0f), "");
    ImGui::PopStyleColor();
    
    // Value display
    if (show_value) {
        ImGui::TextColored(color, "%.1f %s", value, unit ? unit : "");
    }
    
    // Min/Max labels
    ImGui::Text("Min: %.0f  Max: %.0f", min_val, max_val);
    
    ImGui::EndGroup();
}

// Render a round gauge using ImGui
void imgui_render_round_gauge(const char* label, float value, float min_val, float max_val,
                             float warning_threshold, float danger_threshold,
                             const char* unit, float size, bool show_value) {
    if (!label) return;
    
    // Calculate percentage
    float percentage = 0.0f;
    if (max_val > min_val) {
        percentage = (value - min_val) / (max_val - min_val);
        percentage = (percentage < 0.0f) ? 0.0f : (percentage > 1.0f) ? 1.0f : percentage;
    }
    
    // Determine color based on thresholds
    ImVec4 color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f); // Green (normal)
    if (danger_threshold > 0 && value >= danger_threshold) {
        color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f); // Red (danger)
    } else if (warning_threshold > 0 && value >= warning_threshold) {
        color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f); // Yellow (warning)
    }
    
    ImGui::BeginGroup();
    
    // Gauge label
    ImGui::Text("%s", label);
    
    // Calculate gauge dimensions
    float gauge_radius = size * 0.4f;
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    ImVec2 center;
    center.x = cursor_pos.x + gauge_radius + 10;
    center.y = cursor_pos.y + gauge_radius + 10;
    
    // Draw gauge background circle
    ImGui::GetWindowDrawList()->AddCircle(
        center, gauge_radius, 
        IM_COL32(50, 50, 50, 255), 0, 3.0f
    );
    
    // Draw gauge arc using multiple line segments (270 degrees, from -135 to +135)
    float start_angle = -135.0f * 3.14159f / 180.0f;
    float end_angle = start_angle + (270.0f * 3.14159f / 180.0f) * percentage;
    
    // Draw arc segments
    int segments = 20;
    for (int i = 0; i < segments; i++) {
        float angle1 = start_angle + (end_angle - start_angle) * i / (float)segments;
        float angle2 = start_angle + (end_angle - start_angle) * (i + 1) / (float)segments;
        
        ImVec2 p1, p2;
        p1.x = center.x + cos(angle1) * gauge_radius;
        p1.y = center.y + sin(angle1) * gauge_radius;
        p2.x = center.x + cos(angle2) * gauge_radius;
        p2.y = center.y + sin(angle2) * gauge_radius;
        
        ImGui::GetWindowDrawList()->AddLine(
            p1, p2,
            IM_COL32((int)(color.x * 255), (int)(color.y * 255), (int)(color.z * 255), 255), 4.0f
        );
    }
    
    // Draw gauge needle
    float needle_angle = start_angle + (270.0f * 3.14159f / 180.0f) * percentage;
    ImVec2 needle_end;
    needle_end.x = center.x + cos(needle_angle) * (gauge_radius - 5);
    needle_end.y = center.y + sin(needle_angle) * (gauge_radius - 5);
    
    ImGui::GetWindowDrawList()->AddLine(
        center, needle_end,
        IM_COL32(255, 255, 255, 255), 2.0f
    );
    
    // Draw center dot
    ImGui::GetWindowDrawList()->AddCircleFilled(
        center, 3.0f, IM_COL32(255, 255, 255, 255)
    );
    
    // Draw value text in center
    if (show_value) {
        char value_text[32];
        snprintf(value_text, sizeof(value_text), "%.1f", value);
        
        ImVec2 text_size = ImGui::CalcTextSize(value_text);
        ImVec2 text_pos;
        text_pos.x = center.x - text_size.x * 0.5f;
        text_pos.y = center.y - text_size.y * 0.5f;
        
        ImGui::GetWindowDrawList()->AddText(
            text_pos, IM_COL32(255, 255, 255, 255), value_text
        );
        
        // Draw unit below value
        if (unit && strlen(unit) > 0) {
            ImVec2 unit_size = ImGui::CalcTextSize(unit);
            ImVec2 unit_pos;
            unit_pos.x = center.x - unit_size.x * 0.5f;
            unit_pos.y = center.y + text_size.y + 5;
            ImGui::GetWindowDrawList()->AddText(
                unit_pos, IM_COL32(200, 200, 200, 255), unit
            );
        }
    }
    
    // Draw min/max labels
    char min_text[16], max_text[16];
    snprintf(min_text, sizeof(min_text), "%.0f", min_val);
    snprintf(max_text, sizeof(max_text), "%.0f", max_val);
    
    ImVec2 min_pos, max_pos;
    min_pos.x = center.x - gauge_radius - 5;
    min_pos.y = center.y + gauge_radius * 0.7f;
    max_pos.x = center.x + gauge_radius + 5;
    max_pos.y = center.y + gauge_radius * 0.7f;
    
    ImGui::GetWindowDrawList()->AddText(min_pos, IM_COL32(150, 150, 150, 255), min_text);
    ImGui::GetWindowDrawList()->AddText(max_pos, IM_COL32(150, 150, 150, 255), max_text);
    
    // Move cursor to make space for gauge and submit a dummy item
    ImVec2 current_pos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(current_pos.x, current_pos.y + gauge_radius * 2 + 20));
    ImGui::Dummy(ImVec2(1, 1)); // Submit a dummy item to satisfy ImGui requirements
    
    ImGui::EndGroup();
}

// Render digital readout
void imgui_render_digital_readout(const char* label, float value, const char* unit) {
    if (!label) return;
    
    ImGui::BeginGroup();
    ImGui::Text("%s:", label);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.1f %s", value, unit ? unit : "");
    ImGui::EndGroup();
}

// Render simple chart
void imgui_render_chart(const char* label, float* values, int count, float min_val, float max_val) {
    if (!label || !values || count <= 0) return;
    
    ImGui::BeginGroup();
    ImGui::Text("%s", label);
    
    // Simple line chart using ImGui plot lines
    ImGui::PlotLines("", values, count, 0, NULL, min_val, max_val, ImVec2(200, 80));
    
    ImGui::EndGroup();
}

// Render digital gauge
void imgui_render_digital_gauge(const char* label, float value, const char* unit, float size) {
    if (!label) return;
    
    ImGui::BeginGroup();
    
    // Gauge label
    ImGui::Text("%s", label);
    
    // Large digital value
    char value_text[64];
    snprintf(value_text, sizeof(value_text), "%.1f", value);
    
    ImVec2 text_size = ImGui::CalcTextSize(value_text);
    float scale = size / text_size.x;
    if (scale > 2.0f) scale = 2.0f; // Limit maximum scale
    
    ImGui::SetWindowFontScale(scale);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", value_text);
    ImGui::SetWindowFontScale(1.0f);
    
    // Unit
    if (unit && strlen(unit) > 0) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", unit);
    }
    
    ImGui::EndGroup();
}

// Render linear gauge
void imgui_render_linear_gauge(const char* label, float value, float min_val, float max_val,
                              float warning_threshold, float danger_threshold,
                              const char* unit, float size, bool show_value) {
    if (!label) return;
    
    // Calculate percentage
    float percentage = 0.0f;
    if (max_val > min_val) {
        percentage = (value - min_val) / (max_val - min_val);
        percentage = (percentage < 0.0f) ? 0.0f : (percentage > 1.0f) ? 1.0f : percentage;
    }
    
    // Determine color based on thresholds
    ImVec4 color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f); // Green (normal)
    if (danger_threshold > 0 && value >= danger_threshold) {
        color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f); // Red (danger)
    } else if (warning_threshold > 0 && value >= warning_threshold) {
        color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f); // Yellow (warning)
    }
    
    ImGui::BeginGroup();
    
    // Gauge label
    ImGui::Text("%s", label);
    
    // Linear gauge (vertical)
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    float gauge_width = 20.0f;
    float gauge_height = size;
    
    // Draw gauge background
    ImGui::GetWindowDrawList()->AddRectFilled(
        cursor_pos, 
        ImVec2(cursor_pos.x + gauge_width, cursor_pos.y + gauge_height),
        IM_COL32(50, 50, 50, 255)
    );
    
    // Draw gauge fill
    float fill_height = gauge_height * percentage;
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(cursor_pos.x, cursor_pos.y + gauge_height - fill_height),
        ImVec2(cursor_pos.x + gauge_width, cursor_pos.y + gauge_height),
        IM_COL32((int)(color.x * 255), (int)(color.y * 255), (int)(color.z * 255), 255)
    );
    
    // Draw gauge border
    ImGui::GetWindowDrawList()->AddRect(
        cursor_pos,
        ImVec2(cursor_pos.x + gauge_width, cursor_pos.y + gauge_height),
        IM_COL32(100, 100, 100, 255)
    );
    
    // Value display
    if (show_value) {
        ImGui::SameLine();
        ImGui::TextColored(color, "%.1f %s", value, unit ? unit : "");
    }
    
    // Min/Max labels
    ImGui::Text("Min: %.0f  Max: %.0f", min_val, max_val);
    
    ImGui::EndGroup();
}

// Render real-time chart
void imgui_render_real_time_chart(RealTimeChart* chart, uint32_t current_time) {
    if (!chart || !chart->enabled) return;
    
    ImGui::BeginGroup();
    ImGui::Text("%s", chart->title);
    // Add subtitle for Engine Performance chart
    if (strcmp(chart->title, "Engine Performance") == 0) {
        ImGui::Text("Y: RPM, MAP, TPS   X: Time (s)");
    } else if (strcmp(chart->title, "Air/Fuel Ratio") == 0) {
        ImGui::Text("Y: AFR   X: Time (s)");
    } else {
        // Generic label for other charts
        ImGui::Text("Y: Value   X: Time (s)");
    }
    
    // Calculate time window
    uint32_t window_start = current_time - (uint32_t)(chart->time_window_seconds * 1000.0f);
    
    // Prepare data for plotting
    static float plot_data[1000];
    static float plot_times[1000];
    int plot_count = 0;
    
    // Get data from first enabled series
    for (int s = 0; s < chart->series_count && plot_count < 1000; s++) {
        DataSeries* series = &chart->series[s];
        if (!series->enabled) continue;
        
        // Limit the number of points to process for better performance
        int max_points_to_process = (series->point_count > 500) ? 500 : series->point_count;
        int start_index = (series->point_count > 500) ? series->point_count - 500 : 0;
        
        for (int i = start_index; i < start_index + max_points_to_process && plot_count < 1000; i++) {
            if (i >= series->point_count) break;
            if (series->points[i].timestamp >= window_start) {
                plot_data[plot_count] = series->points[i].value;
                plot_times[plot_count] = (float)(series->points[i].timestamp - window_start) / 1000.0f;
                plot_count++;
            }
        }
        break; // Only plot first enabled series for now
    }
    
    // Render chart
    if (plot_count > 1) {
        // Calculate min/max for auto-scaling
        float min_val = plot_data[0];
        float max_val = plot_data[0];
        for (int i = 1; i < plot_count; i++) {
            if (plot_data[i] < min_val) min_val = plot_data[i];
            if (plot_data[i] > max_val) max_val = plot_data[i];
        }
        
        // Add some padding for better visualization
        float range = max_val - min_val;
        if (range < 0.1f) range = 0.1f; // Avoid division by zero
        min_val -= range * 0.1f;
        max_val += range * 0.1f;
        
        // Create unique chart ID based on title
        char chart_id[256];
        snprintf(chart_id, sizeof(chart_id), "##chart_%s", chart->title);
        
        // Calculate dynamic chart size based on available space
        ImVec2 display_size = ImGui::GetIO().DisplaySize;
        float log_height = 180.0f;
        float available_width = display_size.x - 40.0f; // Account for margins
        float available_height = display_size.y - log_height - 100.0f; // Account for UI elements
        float chart_width = (available_width - 20.0f) / 2.0f;
        float chart_height = 220.0f;
        if (chart_width < 200.0f) chart_width = 200.0f;
        if (chart_height < 150.0f) chart_height = 150.0f;
        
        // Y-axis label (drawn to the left of the chart)
        ImGui::Text("Y");
        ImGui::SameLine();
        ImGui::PlotLines(chart_id, plot_data, plot_count, 0, NULL, 
                        chart->auto_scale ? min_val : chart->min_scale,
                        chart->auto_scale ? max_val : chart->max_scale,
                        ImVec2(chart_width, chart_height));
        // X-axis label (drawn below the chart)
        ImGui::Text("X: Time (s)");
        
        // Show current value with unique ID
        if (plot_count > 0) {
            char current_id[256];
            snprintf(current_id, sizeof(current_id), "Current: %.1f##current_%s", plot_data[plot_count - 1], chart->title);
            ImGui::Text("%s", current_id);
        }
    } else {
        ImGui::Text("No data available");
        ImGui::Text("(Connect ECU or wait for data)");
    }
    
    ImGui::EndGroup();
}

// Check alerts
void imgui_check_alerts(AlertConfig* alerts, int count, const ECUData* data) {
    if (!alerts || !data) return;
    
    uint32_t current_time = SDL_GetTicks();
    
    for (int i = 0; i < count; i++) {
        AlertConfig* alert = &alerts[i];
        if (!alert->enabled) continue;
        
        bool triggered = false;
        
        // Check conditions based on alert name
        if (strstr(alert->name, "RPM") && data->rpm > alert->threshold) {
            triggered = true;
        } else if (strstr(alert->name, "TEMP") && data->coolant_temp > alert->threshold) {
            triggered = true;
        } else if (strstr(alert->name, "VOLTAGE") && data->battery_voltage < alert->threshold) {
            triggered = true;
        } else if (strstr(alert->name, "KNOCK") && data->knock_count > alert->threshold) {
            triggered = true;
        } else if (strstr(alert->name, "BOOST") && data->boost > alert->threshold) {
            triggered = true;
        }
        
        if (triggered && !alert->triggered) {
            alert->triggered = true;
            alert->trigger_time = current_time;
        } else if (!triggered && alert->triggered) {
            alert->triggered = false;
        }
        
        alert->last_check = current_time;
    }
}

// Render alerts panel
void imgui_render_alerts_panel(AlertConfig* alerts, int count, const ECUData* data) {
    if (!alerts || count <= 0) return;
    
    ImGui::BeginGroup();
    ImGui::Text("Alerts");
    ImGui::Separator();
    
    bool has_active_alerts = false;
    
    for (int i = 0; i < count; i++) {
        AlertConfig* alert = &alerts[i];
        if (!alert->enabled) continue;
        
        if (alert->triggered) {
            has_active_alerts = true;
            ImVec4* color = static_cast<ImVec4*>(alert->color);
            ImGui::TextColored(*color, "⚠ %s", alert->name);
        }
    }
    
    if (!has_active_alerts) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ All systems normal");
    }
    
    ImGui::EndGroup();
}

// Render performance panel
void imgui_render_performance_panel(ImGuiRuntimeDisplay* display) {
    if (!display) return;
    
    ImGui::BeginGroup();
    ImGui::Text("Performance");
    ImGui::Separator();
    
    ImGui::Text("FPS: %.1f", display->current_fps);
    ImGui::Text("Data Points: %u", display->data_points_received);
    ImGui::Text("Refresh Rate: %d ms", display->refresh_rate_ms);
    
    // Memory usage (simplified)
    ImGui::Text("Memory: ~%.1f MB", 
                (float)(sizeof(ImGuiRuntimeDisplay) + 
                       display->rpm_history.point_count * sizeof(DataPoint) * 8) / (1024 * 1024));
    
    ImGui::EndGroup();
}

// Load default gauge configurations
void imgui_load_default_gauge_configs(RuntimeGaugeConfig* configs) {
    if (!configs) return;
    
    // RPM Gauge
    strcpy(configs[0].label, "RPM");
    strcpy(configs[0].unit, "RPM");
    configs[0].min_value = 0.0f;
    configs[0].max_value = 8000.0f;
    configs[0].warning_threshold = 7000.0f;
    configs[0].danger_threshold = 7500.0f;
    configs[0].enabled = true;
    
    // MAP Gauge
    strcpy(configs[1].label, "MAP");
    strcpy(configs[1].unit, "kPa");
    configs[1].min_value = 0.0f;
    configs[1].max_value = 300.0f;
    configs[1].warning_threshold = 250.0f;
    configs[1].danger_threshold = 280.0f;
    configs[1].enabled = true;
    
    // TPS Gauge
    strcpy(configs[2].label, "TPS");
    strcpy(configs[2].unit, "%");
    configs[2].min_value = 0.0f;
    configs[2].max_value = 100.0f;
    configs[2].warning_threshold = 80.0f;
    configs[2].danger_threshold = 90.0f;
    configs[2].enabled = true;
    
    // Coolant Temp Gauge
    strcpy(configs[3].label, "Coolant");
    strcpy(configs[3].unit, "°C");
    configs[3].min_value = 0.0f;
    configs[3].max_value = 120.0f;
    configs[3].warning_threshold = 100.0f;
    configs[3].danger_threshold = 110.0f;
    configs[3].enabled = true;
    
    // Battery Voltage Gauge
    strcpy(configs[4].label, "Voltage");
    strcpy(configs[4].unit, "V");
    configs[4].min_value = 10.0f;
    configs[4].max_value = 16.0f;
    configs[4].warning_threshold = 12.0f;
    configs[4].danger_threshold = 11.0f;
    configs[4].enabled = true;
    
    // AFR Gauge
    strcpy(configs[5].label, "AFR");
    strcpy(configs[5].unit, "");
    configs[5].min_value = 10.0f;
    configs[5].max_value = 20.0f;
    configs[5].warning_threshold = 12.0f;
    configs[5].danger_threshold = 15.0f;
    configs[5].enabled = true;
    
    // Boost Gauge
    strcpy(configs[6].label, "Boost");
    strcpy(configs[6].unit, "PSI");
    configs[6].min_value = -20.0f;
    configs[6].max_value = 30.0f;
    configs[6].warning_threshold = 25.0f;
    configs[6].danger_threshold = 28.0f;
    configs[6].enabled = true;
    
    // Timing Gauge
    strcpy(configs[7].label, "Timing");
    strcpy(configs[7].unit, "°");
    configs[7].min_value = -20.0f;
    configs[7].max_value = 50.0f;
    configs[7].warning_threshold = 40.0f;
    configs[7].danger_threshold = 45.0f;
    configs[7].enabled = true;
}

// Load default charts
void imgui_load_default_charts(RealTimeChart* charts, int* count) {
    if (!charts || !count || *count < 2) return;
    
    // Engine Performance Chart
    strcpy(charts[0].title, "Engine Performance");
    charts[0].series_count = 3;
    charts[0].time_window_seconds = 30.0f;
    charts[0].show_grid = true;
    charts[0].show_legend = true;
    charts[0].auto_scale = true;
    charts[0].enabled = true;
    
    strcpy(charts[0].series[0].name, "RPM");
    charts[0].series[0].enabled = true;
    charts[0].series[0].color = new ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    
    strcpy(charts[0].series[1].name, "MAP");
    charts[0].series[1].enabled = true;
    charts[0].series[1].color = new ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    
    strcpy(charts[0].series[2].name, "TPS");
    charts[0].series[2].enabled = true;
    charts[0].series[2].color = new ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
    
    // AFR Chart
    strcpy(charts[1].title, "Air/Fuel Ratio");
    charts[1].series_count = 2;
    charts[1].time_window_seconds = 30.0f;
    charts[1].show_grid = true;
    charts[1].show_legend = true;
    charts[1].auto_scale = false;
    charts[1].min_scale = 10.0f;
    charts[1].max_scale = 20.0f;
    charts[1].enabled = true;
    
    strcpy(charts[1].series[0].name, "AFR");
    charts[1].series[0].enabled = true;
    charts[1].series[0].color = new ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
    
    strcpy(charts[1].series[1].name, "AFR Target");
    charts[1].series[1].enabled = true;
    charts[1].series[1].color = new ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
}

// Load default alerts
void imgui_load_default_alerts(AlertConfig* alerts, int* count) {
    if (!alerts || !count || *count < 8) return;
    
    // High RPM Alert
    strcpy(alerts[0].name, "High RPM");
    strcpy(alerts[0].condition, "RPM > 7500");
    alerts[0].threshold = 7500.0f;
    alerts[0].enabled = true;
    alerts[0].priority = 2; // Danger
    alerts[0].color = new ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    
    // High Temperature Alert
    strcpy(alerts[1].name, "High Temperature");
    strcpy(alerts[1].condition, "Coolant Temp > 110°C");
    alerts[1].threshold = 110.0f;
    alerts[1].enabled = true;
    alerts[1].priority = 2; // Danger
    alerts[1].color = new ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    
    // Low Voltage Alert
    strcpy(alerts[2].name, "Low Voltage");
    strcpy(alerts[2].condition, "Battery Voltage < 11.0V");
    alerts[2].threshold = 11.0f;
    alerts[2].enabled = true;
    alerts[2].priority = 2; // Danger
    alerts[2].color = new ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    
    // Knock Alert
    strcpy(alerts[3].name, "Knock Detected");
    strcpy(alerts[3].condition, "Knock Count > 10");
    alerts[3].threshold = 10.0f;
    alerts[3].enabled = true;
    alerts[3].priority = 2; // Danger
    alerts[3].color = new ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    
    // High Boost Alert
    strcpy(alerts[4].name, "High Boost");
    strcpy(alerts[4].condition, "Boost > 25 PSI");
    alerts[4].threshold = 25.0f;
    alerts[4].enabled = true;
    alerts[4].priority = 1; // Warning
    alerts[4].color = new ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
    
    // High TPS Alert
    strcpy(alerts[5].name, "High TPS");
    strcpy(alerts[5].condition, "TPS > 90%");
    alerts[5].threshold = 90.0f;
    alerts[5].enabled = true;
    alerts[5].priority = 1; // Warning
    alerts[5].color = new ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
    
    // High MAP Alert
    strcpy(alerts[6].name, "High MAP");
    strcpy(alerts[6].condition, "MAP > 250 kPa");
    alerts[6].threshold = 250.0f;
    alerts[6].enabled = true;
    alerts[6].priority = 1; // Warning
    alerts[6].color = new ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
    
    // High Fuel Pressure Alert
    strcpy(alerts[7].name, "High Fuel Pressure");
    strcpy(alerts[7].condition, "Fuel Pressure > 80 PSI");
    alerts[7].threshold = 80.0f;
    alerts[7].enabled = true;
    alerts[7].priority = 1; // Warning
    alerts[7].color = new ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
}

// Render gauge configuration panel
void imgui_render_gauge_config_panel(RuntimeGaugeConfig* configs, int count) {
    if (!configs || count <= 0) return;
    
    ImGui::BeginGroup();
    ImGui::Text("Gauge Configuration");
    ImGui::Separator();
    
    for (int i = 0; i < count; i++) {
        RuntimeGaugeConfig* config = &configs[i];
        
        ImGui::PushID(i);
        
        // Gauge header
        ImGui::Text("%s", config->label);
        
        // Enable/disable checkbox
        ImGui::SameLine();
        ImGui::Checkbox("Enabled", &config->enabled);
        
        if (config->enabled) {
            // Min/Max values
            ImGui::SetNextItemWidth(80);
            ImGui::DragFloat("Min", &config->min_value, 1.0f, -1000.0f, 1000.0f, "%.0f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            ImGui::DragFloat("Max", &config->max_value, 1.0f, -1000.0f, 1000.0f, "%.0f");
            
            // Warning/Danger thresholds
            ImGui::SetNextItemWidth(80);
            ImGui::DragFloat("Warn", &config->warning_threshold, 1.0f, -1000.0f, 1000.0f, "%.0f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            ImGui::DragFloat("Danger", &config->danger_threshold, 1.0f, -1000.0f, 1000.0f, "%.0f");
            
            // Unit
            ImGui::SetNextItemWidth(60);
            ImGui::InputText("Unit", config->unit, sizeof(config->unit));
        }
        
        ImGui::PopID();
        ImGui::Separator();
    }
    
    ImGui::EndGroup();
}

// Render chart configuration panel
void imgui_render_chart_config_panel(RealTimeChart* charts, int count) {
    if (!charts || count <= 0) return;
    
    ImGui::BeginGroup();
    ImGui::Text("Chart Configuration");
    ImGui::Separator();
    
    for (int i = 0; i < count; i++) {
        RealTimeChart* chart = &charts[i];
        
        ImGui::PushID(i);
        
        // Chart header
        ImGui::Text("%s", chart->title);
        
        // Enable/disable checkbox
        ImGui::SameLine();
        ImGui::Checkbox("Enabled", &chart->enabled);
        
        if (chart->enabled) {
            // Time window
            ImGui::SetNextItemWidth(120);
            ImGui::DragFloat("Time Window (s)", &chart->time_window_seconds, 1.0f, 5.0f, 300.0f, "%.0f");
            
            // Auto scale
            ImGui::Checkbox("Auto Scale", &chart->auto_scale);
            
            if (!chart->auto_scale) {
                ImGui::SetNextItemWidth(80);
                ImGui::DragFloat("Min Scale", &chart->min_scale, 1.0f, -1000.0f, 1000.0f, "%.0f");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80);
                ImGui::DragFloat("Max Scale", &chart->max_scale, 1.0f, -1000.0f, 1000.0f, "%.0f");
            }
            
            // Display options
            ImGui::Checkbox("Show Grid", &chart->show_grid);
            ImGui::SameLine();
            ImGui::Checkbox("Show Legend", &chart->show_legend);
        }
        
        ImGui::PopID();
        ImGui::Separator();
    }
    
    ImGui::EndGroup();
}

// Render alert configuration panel
void imgui_render_alert_config_panel(AlertConfig* alerts, int count) {
    if (!alerts || count <= 0) return;
    
    ImGui::BeginGroup();
    ImGui::Text("Alert Configuration");
    ImGui::Separator();
    
    for (int i = 0; i < count; i++) {
        AlertConfig* alert = &alerts[i];
        
        ImGui::PushID(i);
        
        // Alert header
        ImGui::Text("%s", alert->name);
        
        // Enable/disable checkbox
        ImGui::SameLine();
        ImGui::Checkbox("Enabled", &alert->enabled);
        
        if (alert->enabled) {
            // Threshold
            ImGui::SetNextItemWidth(100);
            ImGui::DragFloat("Threshold", &alert->threshold, 1.0f, -1000.0f, 1000.0f, "%.1f");
            
            // Priority
            ImGui::SetNextItemWidth(100);
            const char* priorities[] = {"Info", "Warning", "Danger", "Critical"};
            ImGui::Combo("Priority", &alert->priority, priorities, IM_ARRAYSIZE(priorities));
            
            // Condition display
            ImGui::Text("Condition: %s", alert->condition);
            
            // Status
            if (alert->triggered) {
                ImVec4* color = static_cast<ImVec4*>(alert->color);
                ImGui::TextColored(*color, "Status: TRIGGERED");
            } else {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: Normal");
            }
        }
        
        ImGui::PopID();
        ImGui::Separator();
    }
    
    ImGui::EndGroup();
}

// Main render function for runtime display
void imgui_runtime_display_render(ImGuiRuntimeDisplay* display) {
    if (!display || !display->initialized || !display->ecu_ctx) return;
    
    // Update display
    imgui_runtime_display_update(display);
    
    // Get current ECU data - use global data if demo mode is enabled
    const ECUData* data = ecu_get_data(display->ecu_ctx);
    uint32_t current_time = SDL_GetTicks();
    
    // If demo mode is enabled, always generate demo data (override ECU data if needed)
    if (display->demo_mode_enabled) {
        // We need to pass the global ECU data to the gauge rendering
        // For now, we'll use a static ECUData structure with demo values
        static ECUData demo_data;
        static float demo_time = 0.0f;
        demo_time += 0.1f; // Increment time for demo
        
        // Generate realistic demo values with proper clamping - wider RPM range
        demo_data.rpm = fmax(800.0f, 800.0f + 7200.0f * sin(demo_time * 0.3f) + 1000.0f * sin(demo_time * 1.5f));
        demo_data.map = fmax(20.0f, 30.0f + 50.0f * sin(demo_time * 0.3f));
        demo_data.tps = fmax(0.0f, 10.0f + 30.0f * sin(demo_time * 0.7f));
        demo_data.afr = fmax(10.0f, fmin(20.0f, 14.7f + 2.0f * sin(demo_time * 0.4f)));
        demo_data.boost = fmax(-5.0f, 5.0f + 8.0f * sin(demo_time * 0.6f));
        demo_data.coolant_temp = fmax(60.0f, fmin(120.0f, 90.0f + 10.0f * sin(demo_time * 0.2f)));
        demo_data.intake_temp = fmax(60.0f, fmin(120.0f, 85.0f + 8.0f * sin(demo_time * 0.3f)));
        demo_data.oil_temp = fmax(60.0f, fmin(130.0f, 95.0f + 12.0f * sin(demo_time * 0.4f)));
        demo_data.battery_voltage = fmax(10.0f, fmin(16.0f, 13.5f + 0.5f * sin(demo_time * 0.8f)));
        demo_data.timing = fmax(-10.0f, fmin(40.0f, 15.0f + 10.0f * sin(demo_time * 0.9f)));
        demo_data.oil_pressure = fmax(50.0f, 300.0f + 100.0f * sin(demo_time * 0.5f));
        demo_data.fuel_pressure = fmax(100.0f, 250.0f + 50.0f * sin(demo_time * 0.6f));
        demo_data.fuel_pw1 = fmax(1.0f, 2.5f + 1.5f * sin(demo_time * 0.7f));
        demo_data.fuel_duty = fmax(0.0f, fmin(100.0f, 15.0f + 10.0f * sin(demo_time * 0.8f)));
        demo_data.knock_count = 0.0f; // Usually 0 in normal operation
        demo_data.knock_retard = 0.0f; // Usually 0 in normal operation
        
        data = &demo_data;
    }
    
    // Enhanced Display Controls - Compact layout with advanced options
    ImGui::BeginGroup();
    ImGui::Text("Display Controls");
    ImGui::Separator();
    
    // First row of controls
    ImGui::Checkbox("Gauges", &display->show_gauges);
    ImGui::SameLine();
    ImGui::Checkbox("Charts", &display->show_charts);
    ImGui::SameLine();
    ImGui::Checkbox("Digital", &display->show_digital_readouts);
    ImGui::SameLine();
    ImGui::Checkbox("Alerts", &display->show_alerts);
    
    // Second row of controls
    ImGui::Checkbox("Round Gauges", &display->use_round_gauges);
    ImGui::SameLine();
    ImGui::Checkbox("Auto Refresh", &display->auto_refresh);
    ImGui::SameLine();
    ImGui::Checkbox("Performance", &display->show_performance_stats);
    ImGui::SameLine();
    ImGui::Checkbox("Advanced", &display->show_advanced_controls);
    
    // Demo mode toggle (prominent placement)
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Demo Mode:");
    ImGui::SameLine();
    if (ImGui::Checkbox("Enable Demo Data", &display->demo_mode_enabled)) {
        // This will be synced with global demo mode in main.cpp
    }
    if (display->demo_mode_enabled) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Demo data active");
        ImGui::Text("Simulated engine data is being displayed");
    }
    ImGui::Separator();
    
    // Third row - compact sliders and layout
    ImGui::Text("Refresh Rate:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::SliderInt("##refresh", &display->refresh_rate_ms, 16, 1000, "%d ms");
    ImGui::SameLine();
    ImGui::Text("Size:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("##size", &display->gauge_size, 80.0f, 200.0f, "%.0f");
    
    // Layout selection
    ImGui::Text("Layout:");
    ImGui::SameLine();
    const char* layouts[] = {"Single", "Dual", "Quad", "3x3 Grid", "4x4 Grid"};
    ImGui::SetNextItemWidth(120);
    ImGui::Combo("##layout", &display->selected_layout, layouts, IM_ARRAYSIZE(layouts));
    
    ImGui::EndGroup();
    
    ImGui::SameLine();
    
    // Configuration panels (if enabled)
    if (display->show_gauge_config) {
        ImGui::SameLine();
        imgui_render_gauge_config_panel(display->gauge_configs, display->gauge_config_count);
    }
    
    if (display->show_chart_config) {
        ImGui::SameLine();
        imgui_render_chart_config_panel(display->charts, display->chart_count);
    }
    
    if (display->show_alert_config) {
        ImGui::SameLine();
        imgui_render_alert_config_panel(display->alerts, display->alert_count);
    }
    
    if (display->show_performance_stats) {
        ImGui::SameLine();
        imgui_render_performance_panel(display);
    }
    
    // Advanced controls panel (when "Advanced" is enabled)
    if (display->show_advanced_controls) {
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::Text("Advanced Controls");
        ImGui::Separator();
        
        // Chart configuration shortcuts
        ImGui::Text("Chart Config:");
        ImGui::SameLine();
        if (ImGui::Button("Show Chart Config")) {
            display->show_chart_config = !display->show_chart_config;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Show Alert Config")) {
            display->show_alert_config = !display->show_alert_config;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Show Gauge Config")) {
            display->show_gauge_config = !display->show_gauge_config;
        }
        
        // Data history controls
        ImGui::Text("Data History:");
        ImGui::SameLine();
        if (ImGui::Button("Clear History")) {
            imgui_clear_data_series(&display->rpm_history);
            imgui_clear_data_series(&display->map_history);
            imgui_clear_data_series(&display->tps_history);
            imgui_clear_data_series(&display->afr_history);
            imgui_clear_data_series(&display->boost_history);
            imgui_clear_data_series(&display->temp_history);
            imgui_clear_data_series(&display->voltage_history);
            imgui_clear_data_series(&display->timing_history);
            display->data_points_received = 0;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Reset Stats")) {
            display->frame_count = 0;
            display->last_fps_update = current_time;
            display->current_fps = 0.0f;
        }
        
        // Data series enable/disable
        ImGui::Text("Data Series:");
        ImGui::Checkbox("RPM History", &display->rpm_history.enabled);
        ImGui::SameLine();
        ImGui::Checkbox("MAP History", &display->map_history.enabled);
        ImGui::SameLine();
        ImGui::Checkbox("AFR History", &display->afr_history.enabled);
        ImGui::SameLine();
        ImGui::Checkbox("Boost History", &display->boost_history.enabled);
        
        ImGui::EndGroup();
    }
    
    // Main display area
    ImGui::Separator();
    
    // Calculate available space for scrollable content
    ImVec2 display_size = ImGui::GetIO().DisplaySize;
    float log_height = 180.0f;
    float log_y = display_size.y - log_height - 10.0f;
    float available_height = log_y - ImGui::GetCursorPosY() - 20.0f; // 20px margin
    
    // Create scrollable area for main content
    ImGui::BeginChild("MainContent", ImVec2(0, available_height), false, 
                      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);
    
    // Gauge layouts based on selection
    switch (display->selected_layout) {
        case 0: // Single Gauge
            if (display->show_gauges) {
                RuntimeGaugeConfig* rpm_config = &display->gauge_configs[0]; // RPM
                if (display->use_round_gauges) {
                    imgui_render_round_gauge(rpm_config->label, data->rpm, 
                                           rpm_config->min_value, rpm_config->max_value,
                                           rpm_config->warning_threshold, rpm_config->danger_threshold,
                                           rpm_config->unit, display->gauge_size, true);
                } else {
                    imgui_render_gauge(rpm_config->label, data->rpm, 
                                     rpm_config->min_value, rpm_config->max_value,
                                     rpm_config->warning_threshold, rpm_config->danger_threshold,
                                     rpm_config->unit, display->gauge_size, true);
                }
            }
            break;
            
        case 1: // Dual Gauge
            if (display->show_gauges) {
                ImGui::Columns(2, NULL, false);
                RuntimeGaugeConfig* rpm_config = &display->gauge_configs[0]; // RPM
                RuntimeGaugeConfig* map_config = &display->gauge_configs[1]; // MAP
                
                if (display->use_round_gauges) {
                    imgui_render_round_gauge(rpm_config->label, data->rpm, 
                                           rpm_config->min_value, rpm_config->max_value,
                                           rpm_config->warning_threshold, rpm_config->danger_threshold,
                                           rpm_config->unit, display->gauge_size * 0.8f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(map_config->label, data->map, 
                                           map_config->min_value, map_config->max_value,
                                           map_config->warning_threshold, map_config->danger_threshold,
                                           map_config->unit, display->gauge_size * 0.8f, true);
                } else {
                    imgui_render_gauge(rpm_config->label, data->rpm, 
                                     rpm_config->min_value, rpm_config->max_value,
                                     rpm_config->warning_threshold, rpm_config->danger_threshold,
                                     rpm_config->unit, display->gauge_size * 0.8f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(map_config->label, data->map, 
                                     map_config->min_value, map_config->max_value,
                                     map_config->warning_threshold, map_config->danger_threshold,
                                     map_config->unit, display->gauge_size * 0.8f, true);
                }
                ImGui::Columns(1);
            }
            break;
            
        case 2: // Quad Gauge
            if (display->show_gauges) {
                ImGui::Columns(2, NULL, false);
                RuntimeGaugeConfig* rpm_config = &display->gauge_configs[0]; // RPM
                RuntimeGaugeConfig* map_config = &display->gauge_configs[1]; // MAP
                RuntimeGaugeConfig* tps_config = &display->gauge_configs[2]; // TPS
                RuntimeGaugeConfig* afr_config = &display->gauge_configs[5]; // AFR
                
                if (display->use_round_gauges) {
                    imgui_render_round_gauge(rpm_config->label, data->rpm, 
                                           rpm_config->min_value, rpm_config->max_value,
                                           rpm_config->warning_threshold, rpm_config->danger_threshold,
                                           rpm_config->unit, display->gauge_size * 0.6f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(map_config->label, data->map, 
                                           map_config->min_value, map_config->max_value,
                                           map_config->warning_threshold, map_config->danger_threshold,
                                           map_config->unit, display->gauge_size * 0.6f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(tps_config->label, data->tps, 
                                           tps_config->min_value, tps_config->max_value,
                                           tps_config->warning_threshold, tps_config->danger_threshold,
                                           tps_config->unit, display->gauge_size * 0.6f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(afr_config->label, data->afr, 
                                           afr_config->min_value, afr_config->max_value,
                                           afr_config->warning_threshold, afr_config->danger_threshold,
                                           afr_config->unit, display->gauge_size * 0.6f, true);
                } else {
                    imgui_render_gauge(rpm_config->label, data->rpm, 
                                     rpm_config->min_value, rpm_config->max_value,
                                     rpm_config->warning_threshold, rpm_config->danger_threshold,
                                     rpm_config->unit, display->gauge_size * 0.6f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(map_config->label, data->map, 
                                     map_config->min_value, map_config->max_value,
                                     map_config->warning_threshold, map_config->danger_threshold,
                                     map_config->unit, display->gauge_size * 0.6f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(tps_config->label, data->tps, 
                                     tps_config->min_value, tps_config->max_value,
                                     tps_config->warning_threshold, tps_config->danger_threshold,
                                     tps_config->unit, display->gauge_size * 0.6f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(afr_config->label, data->afr, 
                                     afr_config->min_value, afr_config->max_value,
                                     afr_config->warning_threshold, afr_config->danger_threshold,
                                     afr_config->unit, display->gauge_size * 0.6f, true);
                }
                ImGui::Columns(1);
            }
            break;
            
        case 3: // 3x3 Grid
            if (display->show_gauges) {
                ImGui::Columns(3, NULL, false);
                RuntimeGaugeConfig* rpm_config = &display->gauge_configs[0]; // RPM
                RuntimeGaugeConfig* map_config = &display->gauge_configs[1]; // MAP
                RuntimeGaugeConfig* tps_config = &display->gauge_configs[2]; // TPS
                RuntimeGaugeConfig* afr_config = &display->gauge_configs[5]; // AFR
                RuntimeGaugeConfig* boost_config = &display->gauge_configs[6]; // Boost
                RuntimeGaugeConfig* temp_config = &display->gauge_configs[3]; // Coolant Temp
                RuntimeGaugeConfig* voltage_config = &display->gauge_configs[4]; // Battery Voltage
                RuntimeGaugeConfig* timing_config = &display->gauge_configs[7]; // Timing
                
                if (display->use_round_gauges) {
                    imgui_render_round_gauge(rpm_config->label, data->rpm, 
                                           rpm_config->min_value, rpm_config->max_value,
                                           rpm_config->warning_threshold, rpm_config->danger_threshold,
                                           rpm_config->unit, display->gauge_size * 0.5f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(map_config->label, data->map, 
                                           map_config->min_value, map_config->max_value,
                                           map_config->warning_threshold, map_config->danger_threshold,
                                           map_config->unit, display->gauge_size * 0.5f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(tps_config->label, data->tps, 
                                           tps_config->min_value, tps_config->max_value,
                                           tps_config->warning_threshold, tps_config->danger_threshold,
                                           tps_config->unit, display->gauge_size * 0.5f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(afr_config->label, data->afr, 
                                           afr_config->min_value, afr_config->max_value,
                                           afr_config->warning_threshold, afr_config->danger_threshold,
                                           afr_config->unit, display->gauge_size * 0.5f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(boost_config->label, data->boost, 
                                           boost_config->min_value, boost_config->max_value,
                                           boost_config->warning_threshold, boost_config->danger_threshold,
                                           boost_config->unit, display->gauge_size * 0.5f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(temp_config->label, data->coolant_temp, 
                                           temp_config->min_value, temp_config->max_value,
                                           temp_config->warning_threshold, temp_config->danger_threshold,
                                           temp_config->unit, display->gauge_size * 0.5f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(voltage_config->label, data->battery_voltage, 
                                           voltage_config->min_value, voltage_config->max_value,
                                           voltage_config->warning_threshold, voltage_config->danger_threshold,
                                           voltage_config->unit, display->gauge_size * 0.5f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(timing_config->label, data->timing, 
                                           timing_config->min_value, timing_config->max_value,
                                           timing_config->warning_threshold, timing_config->danger_threshold,
                                           timing_config->unit, display->gauge_size * 0.5f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge("Fuel PW", data->fuel_pw1, 0, 20, 15, 18, "ms", display->gauge_size * 0.5f, true);
                } else {
                    imgui_render_gauge(rpm_config->label, data->rpm, 
                                     rpm_config->min_value, rpm_config->max_value,
                                     rpm_config->warning_threshold, rpm_config->danger_threshold,
                                     rpm_config->unit, display->gauge_size * 0.5f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(map_config->label, data->map, 
                                     map_config->min_value, map_config->max_value,
                                     map_config->warning_threshold, map_config->danger_threshold,
                                     map_config->unit, display->gauge_size * 0.5f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(tps_config->label, data->tps, 
                                     tps_config->min_value, tps_config->max_value,
                                     tps_config->warning_threshold, tps_config->danger_threshold,
                                     tps_config->unit, display->gauge_size * 0.5f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(afr_config->label, data->afr, 
                                     afr_config->min_value, afr_config->max_value,
                                     afr_config->warning_threshold, afr_config->danger_threshold,
                                     afr_config->unit, display->gauge_size * 0.5f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(boost_config->label, data->boost, 
                                     boost_config->min_value, boost_config->max_value,
                                     boost_config->warning_threshold, boost_config->danger_threshold,
                                     boost_config->unit, display->gauge_size * 0.5f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(temp_config->label, data->coolant_temp, 
                                     temp_config->min_value, temp_config->max_value,
                                     temp_config->warning_threshold, temp_config->danger_threshold,
                                     temp_config->unit, display->gauge_size * 0.5f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(voltage_config->label, data->battery_voltage, 
                                     voltage_config->min_value, voltage_config->max_value,
                                     voltage_config->warning_threshold, voltage_config->danger_threshold,
                                     voltage_config->unit, display->gauge_size * 0.5f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(timing_config->label, data->timing, 
                                     timing_config->min_value, timing_config->max_value,
                                     timing_config->warning_threshold, timing_config->danger_threshold,
                                     timing_config->unit, display->gauge_size * 0.5f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge("Fuel PW", data->fuel_pw1, 0, 20, 15, 18, "ms", display->gauge_size * 0.5f, true);
                }
                ImGui::Columns(1);
            }
            break;
            
        case 4: // 4x4 Grid
            if (display->show_gauges) {
                ImGui::Columns(4, NULL, false);
                RuntimeGaugeConfig* rpm_config = &display->gauge_configs[0]; // RPM
                RuntimeGaugeConfig* map_config = &display->gauge_configs[1]; // MAP
                RuntimeGaugeConfig* tps_config = &display->gauge_configs[2]; // TPS
                RuntimeGaugeConfig* afr_config = &display->gauge_configs[5]; // AFR
                RuntimeGaugeConfig* boost_config = &display->gauge_configs[6]; // Boost
                RuntimeGaugeConfig* temp_config = &display->gauge_configs[3]; // Coolant Temp
                RuntimeGaugeConfig* voltage_config = &display->gauge_configs[4]; // Battery Voltage
                RuntimeGaugeConfig* timing_config = &display->gauge_configs[7]; // Timing
                
                if (display->use_round_gauges) {
                    imgui_render_round_gauge(rpm_config->label, data->rpm, 
                                           rpm_config->min_value, rpm_config->max_value,
                                           rpm_config->warning_threshold, rpm_config->danger_threshold,
                                           rpm_config->unit, display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(map_config->label, data->map, 
                                           map_config->min_value, map_config->max_value,
                                           map_config->warning_threshold, map_config->danger_threshold,
                                           map_config->unit, display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(tps_config->label, data->tps, 
                                           tps_config->min_value, tps_config->max_value,
                                           tps_config->warning_threshold, tps_config->danger_threshold,
                                           tps_config->unit, display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(afr_config->label, data->afr, 
                                           afr_config->min_value, afr_config->max_value,
                                           afr_config->warning_threshold, afr_config->danger_threshold,
                                           afr_config->unit, display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(boost_config->label, data->boost, 
                                           boost_config->min_value, boost_config->max_value,
                                           boost_config->warning_threshold, boost_config->danger_threshold,
                                           boost_config->unit, display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(temp_config->label, data->coolant_temp, 
                                           temp_config->min_value, temp_config->max_value,
                                           temp_config->warning_threshold, temp_config->danger_threshold,
                                           temp_config->unit, display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(voltage_config->label, data->battery_voltage, 
                                           voltage_config->min_value, voltage_config->max_value,
                                           voltage_config->warning_threshold, voltage_config->danger_threshold,
                                           voltage_config->unit, display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge(timing_config->label, data->timing, 
                                           timing_config->min_value, timing_config->max_value,
                                           timing_config->warning_threshold, timing_config->danger_threshold,
                                           timing_config->unit, display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge("Fuel PW", data->fuel_pw1, 0, 20, 15, 18, "ms", display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge("Fuel Duty", data->fuel_duty, 0, 100, 80, 90, "%", display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge("Knock", data->knock_count, 0, 100, 10, 20, "", display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_round_gauge("Knock Retard", data->knock_retard, 0, 10, 5, 8, "°", display->gauge_size * 0.4f, true);
                } else {
                    imgui_render_gauge(rpm_config->label, data->rpm, 
                                     rpm_config->min_value, rpm_config->max_value,
                                     rpm_config->warning_threshold, rpm_config->danger_threshold,
                                     rpm_config->unit, display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(map_config->label, data->map, 
                                     map_config->min_value, map_config->max_value,
                                     map_config->warning_threshold, map_config->danger_threshold,
                                     map_config->unit, display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(tps_config->label, data->tps, 
                                     tps_config->min_value, tps_config->max_value,
                                     tps_config->warning_threshold, tps_config->danger_threshold,
                                     tps_config->unit, display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(afr_config->label, data->afr, 
                                     afr_config->min_value, afr_config->max_value,
                                     afr_config->warning_threshold, afr_config->danger_threshold,
                                     afr_config->unit, display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(boost_config->label, data->boost, 
                                     boost_config->min_value, boost_config->max_value,
                                     boost_config->warning_threshold, boost_config->danger_threshold,
                                     boost_config->unit, display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(temp_config->label, data->coolant_temp, 
                                     temp_config->min_value, temp_config->max_value,
                                     temp_config->warning_threshold, temp_config->danger_threshold,
                                     temp_config->unit, display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(voltage_config->label, data->battery_voltage, 
                                     voltage_config->min_value, voltage_config->max_value,
                                     voltage_config->warning_threshold, voltage_config->danger_threshold,
                                     voltage_config->unit, display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge(timing_config->label, data->timing, 
                                     timing_config->min_value, timing_config->max_value,
                                     timing_config->warning_threshold, timing_config->danger_threshold,
                                     timing_config->unit, display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge("Fuel PW", data->fuel_pw1, 0, 20, 15, 18, "ms", display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge("Fuel Duty", data->fuel_duty, 0, 100, 80, 90, "%", display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge("Knock", data->knock_count, 0, 100, 10, 20, "", display->gauge_size * 0.4f, true);
                    ImGui::NextColumn();
                    imgui_render_gauge("Knock Retard", data->knock_retard, 0, 10, 5, 8, "°", display->gauge_size * 0.4f, true);
                }
                ImGui::Columns(1);
            }
            break;
    }
    
    // Real-time charts section
    if (display->show_charts && display->chart_count > 0) {
        ImGui::Separator();
        ImGui::Text("Real-Time Charts");
        
        // Calculate available space above the UI Log
        ImVec2 display_size = ImGui::GetIO().DisplaySize;
        float log_height = 180.0f;
        float log_y = display_size.y - log_height - 10.0f;
        float available_height = log_y - ImGui::GetCursorPosY() - 20.0f; // 20px margin
        
        // Use horizontal layout to maximize space usage
        if (display->chart_count == 1) {
            // Single chart - use full width
            imgui_render_real_time_chart(&display->charts[0], current_time);
        } else if (display->chart_count == 2) {
            // Two charts side by side
            ImGui::Columns(2, NULL, false);
            imgui_render_real_time_chart(&display->charts[0], current_time);
            ImGui::NextColumn();
            imgui_render_real_time_chart(&display->charts[1], current_time);
            ImGui::Columns(1);
        } else {
            // Multiple charts in a grid - adjust based on available space
            int cols = (display->chart_count >= 4) ? 2 : display->chart_count;
            ImGui::Columns(cols, NULL, false);
            for (int i = 0; i < display->chart_count; i++) {
                imgui_render_real_time_chart(&display->charts[i], current_time);
                if ((i + 1) % cols == 0 && i < display->chart_count - 1) {
                    ImGui::NextColumn();
                }
            }
            ImGui::Columns(1);
        }
    }
    
    // Digital readouts
    if (display->show_digital_readouts) {
        ImGui::Separator();
        ImGui::Text("Digital Readouts:");
        
        ImGui::Columns(4, NULL, false);
        imgui_render_digital_readout("RPM", data->rpm, "RPM");
        ImGui::NextColumn();
        imgui_render_digital_readout("MAP", data->map, "kPa");
        ImGui::NextColumn();
        imgui_render_digital_readout("TPS", data->tps, "%");
        ImGui::NextColumn();
        imgui_render_digital_readout("AFR", data->afr, "");
        ImGui::NextColumn();
        imgui_render_digital_readout("Boost", data->boost, "PSI");
        ImGui::NextColumn();
        imgui_render_digital_readout("Coolant", data->coolant_temp, "°C");
        ImGui::NextColumn();
        imgui_render_digital_readout("Intake", data->intake_temp, "°C");
        ImGui::NextColumn();
        imgui_render_digital_readout("Voltage", data->battery_voltage, "V");
        ImGui::Columns(1);
    }
    
    // Enhanced alerts section
    if (display->show_alerts) {
        ImGui::Separator();
        imgui_render_alerts_panel(display->alerts, display->alert_count, data);
    }
    
    // Add padding at the bottom of scrollable area to ensure all content is visible
    for (int i = 0; i < 20; ++i) ImGui::Spacing();
    
    ImGui::EndChild();
}