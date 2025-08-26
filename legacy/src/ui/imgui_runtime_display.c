#include "../../include/ui/imgui_runtime_display.h"
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
    
    // Create dashboard
    display->dashboard = dashboard_init(ecu_ctx);
    if (!display->dashboard) {
        free(display);
        return NULL;
    }
    
    // Set default configuration
    display->show_gauges = true;
    display->show_charts = true;
    display->show_digital_readouts = true;
    display->show_alerts = true;
    display->selected_layout = DASHBOARD_LAYOUT_GRID_3X3;
    display->refresh_rate_ms = 100; // 10 FPS
    display->gauge_size = 120.0f;
    display->auto_refresh = true;
    display->last_update = 0;
    display->initialized = true;
    
    // Configure dashboard with default layout
    DashboardConfig config = dashboard_config_full_monitoring();
    dashboard_configure(display->dashboard, &config);
    dashboard_set_active(display->dashboard, true);
    
    return display;
}

// Destroy ImGui Runtime Display
void imgui_runtime_display_destroy(ImGuiRuntimeDisplay* display) {
    if (!display) return;
    
    if (display->dashboard) {
        dashboard_cleanup(display->dashboard);
    }
    
    free(display);
}

// Update runtime display
void imgui_runtime_display_update(ImGuiRuntimeDisplay* display) {
    if (!display || !display->initialized) return;
    
    uint32_t current_time = SDL_GetTicks();
    
    // Check if it's time to update
    if (display->auto_refresh && (current_time - display->last_update) >= display->refresh_rate_ms) {
        dashboard_update(display->dashboard);
        display->last_update = current_time;
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

// Main render function for runtime display
void imgui_runtime_display_render(ImGuiRuntimeDisplay* display) {
    if (!display || !display->initialized || !display->dashboard) return;
    
    // Update display
    imgui_runtime_display_update(display);
    
    // Get current ECU data
    const ECUData* data = NULL;
    if (display->dashboard->ecu_ctx) {
        data = ecu_get_data(display->dashboard->ecu_ctx);
    }
    
    // Display controls
    ImGui::BeginGroup();
    ImGui::Text("Runtime Display Controls");
    ImGui::Separator();
    
    ImGui::Checkbox("Show Gauges", &display->show_gauges);
    ImGui::Checkbox("Show Digital Readouts", &display->show_digital_readouts);
    ImGui::Checkbox("Show Charts", &display->show_charts);
    ImGui::Checkbox("Show Alerts", &display->show_alerts);
    ImGui::Checkbox("Auto Refresh", &display->auto_refresh);
    
    ImGui::SliderInt("Refresh Rate (ms)", &display->refresh_rate_ms, 50, 1000);
    ImGui::SliderFloat("Gauge Size", &display->gauge_size, 80.0f, 200.0f);
    
    // Layout selection
    const char* layouts[] = {"Single Gauge", "Dual Gauge", "Quad Gauge", "3x3 Grid", "4x4 Grid"};
    ImGui::Combo("Layout", &display->selected_layout, layouts, IM_ARRAYSIZE(layouts));
    
    ImGui::EndGroup();
    
    ImGui::SameLine();
    
    // Main display area
    ImGui::BeginGroup();
    ImGui::Text("Real-Time Engine Data");
    ImGui::Separator();
    
    if (!data) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No ECU data available");
        ImGui::EndGroup();
        return;
    }
    
    // Display gauges and readouts based on layout
    switch (display->selected_layout) {
        case DASHBOARD_LAYOUT_SINGLE_GAUGE:
            if (display->show_gauges) {
                imgui_render_gauge("RPM", data->rpm, 0, 8000, 7000, 7500, "RPM", display->gauge_size, true);
            }
            break;
            
        case DASHBOARD_LAYOUT_DUAL_GAUGE:
            if (display->show_gauges) {
                ImGui::Columns(2, NULL, false);
                imgui_render_gauge("RPM", data->rpm, 0, 8000, 7000, 7500, "RPM", display->gauge_size * 0.8f, true);
                ImGui::NextColumn();
                imgui_render_gauge("MAP", data->map, 0, 300, 250, 280, "kPa", display->gauge_size * 0.8f, true);
                ImGui::Columns(1);
            }
            break;
            
        case DASHBOARD_LAYOUT_QUAD_GAUGE:
            if (display->show_gauges) {
                ImGui::Columns(2, NULL, false);
                imgui_render_gauge("RPM", data->rpm, 0, 8000, 7000, 7500, "RPM", display->gauge_size * 0.6f, true);
                ImGui::NextColumn();
                imgui_render_gauge("MAP", data->map, 0, 300, 250, 280, "kPa", display->gauge_size * 0.6f, true);
                ImGui::NextColumn();
                imgui_render_gauge("TPS", data->tps, 0, 100, 80, 90, "%", display->gauge_size * 0.6f, true);
                ImGui::NextColumn();
                imgui_render_gauge("AFR", data->afr, 10, 20, 12, 15, "", display->gauge_size * 0.6f, true);
                ImGui::Columns(1);
            }
            break;
            
        case DASHBOARD_LAYOUT_GRID_3X3:
            if (display->show_gauges) {
                ImGui::Columns(3, NULL, false);
                imgui_render_gauge("RPM", data->rpm, 0, 8000, 7000, 7500, "RPM", display->gauge_size * 0.5f, true);
                ImGui::NextColumn();
                imgui_render_gauge("MAP", data->map, 0, 300, 250, 280, "kPa", display->gauge_size * 0.5f, true);
                ImGui::NextColumn();
                imgui_render_gauge("TPS", data->tps, 0, 100, 80, 90, "%", display->gauge_size * 0.5f, true);
                ImGui::NextColumn();
                imgui_render_gauge("AFR", data->afr, 10, 20, 12, 15, "", display->gauge_size * 0.5f, true);
                ImGui::NextColumn();
                imgui_render_gauge("Boost", data->boost, -20, 30, 25, 28, "PSI", display->gauge_size * 0.5f, true);
                ImGui::NextColumn();
                imgui_render_gauge("Temp", data->coolant_temp, 0, 120, 100, 110, "°C", display->gauge_size * 0.5f, true);
                ImGui::NextColumn();
                imgui_render_gauge("Voltage", data->battery_voltage, 10, 16, 12, 11, "V", display->gauge_size * 0.5f, true);
                ImGui::NextColumn();
                imgui_render_gauge("Timing", data->timing, -20, 50, 40, 45, "°", display->gauge_size * 0.5f, true);
                ImGui::NextColumn();
                imgui_render_gauge("Fuel PW", data->fuel_pw1, 0, 20, 15, 18, "ms", display->gauge_size * 0.5f, true);
                ImGui::Columns(1);
            }
            break;
            
        case DASHBOARD_LAYOUT_GRID_4X4:
            if (display->show_gauges) {
                ImGui::Columns(4, NULL, false);
                imgui_render_gauge("RPM", data->rpm, 0, 8000, 7000, 7500, "RPM", display->gauge_size * 0.4f, true);
                ImGui::NextColumn();
                imgui_render_gauge("MAP", data->map, 0, 300, 250, 280, "kPa", display->gauge_size * 0.4f, true);
                ImGui::NextColumn();
                imgui_render_gauge("TPS", data->tps, 0, 100, 80, 90, "%", display->gauge_size * 0.4f, true);
                ImGui::NextColumn();
                imgui_render_gauge("AFR", data->afr, 10, 20, 12, 15, "", display->gauge_size * 0.4f, true);
                ImGui::NextColumn();
                imgui_render_gauge("Boost", data->boost, -20, 30, 25, 28, "PSI", display->gauge_size * 0.4f, true);
                ImGui::NextColumn();
                imgui_render_gauge("Temp", data->coolant_temp, 0, 120, 100, 110, "°C", display->gauge_size * 0.4f, true);
                ImGui::NextColumn();
                imgui_render_gauge("Voltage", data->battery_voltage, 10, 16, 12, 11, "V", display->gauge_size * 0.4f, true);
                ImGui::NextColumn();
                imgui_render_gauge("Timing", data->timing, -20, 50, 40, 45, "°", display->gauge_size * 0.4f, true);
                ImGui::NextColumn();
                imgui_render_gauge("Fuel PW", data->fuel_pw1, 0, 20, 15, 18, "ms", display->gauge_size * 0.4f, true);
                ImGui::NextColumn();
                imgui_render_gauge("Fuel Duty", data->fuel_duty, 0, 100, 80, 90, "%", display->gauge_size * 0.4f, true);
                ImGui::NextColumn();
                imgui_render_gauge("Knock", data->knock_count, 0, 100, 10, 20, "", display->gauge_size * 0.4f, true);
                ImGui::NextColumn();
                imgui_render_gauge("Knock Retard", data->knock_retard, 0, 10, 5, 8, "°", display->gauge_size * 0.4f, true);
                ImGui::Columns(1);
            }
            break;
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
    
    // Alerts
    if (display->show_alerts) {
        ImGui::Separator();
        ImGui::Text("Alerts:");
        
        bool has_alerts = false;
        
        if (data->rpm > 7500) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "⚠ HIGH RPM: %.0f", data->rpm);
            has_alerts = true;
        }
        
        if (data->coolant_temp > 110) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "⚠ HIGH TEMP: %.1f°C", data->coolant_temp);
            has_alerts = true;
        }
        
        if (data->battery_voltage < 11.0f) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "⚠ LOW VOLTAGE: %.1fV", data->battery_voltage);
            has_alerts = true;
        }
        
        if (data->knock_count > 10) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "⚠ KNOCK DETECTED: %d", data->knock_count);
            has_alerts = true;
        }
        
        if (!has_alerts) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ All systems normal");
        }
    }
    
    ImGui::EndGroup();
} 