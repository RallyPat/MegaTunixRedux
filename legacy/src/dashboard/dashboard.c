/*
 * Dashboard System - Real-time ECU Data Display Implementation
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * Implements flexible dashboard layouts for displaying ECU data.
 */

#include "../../include/dashboard/dashboard.h"
// Clay removed - will be replaced with ImGui
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Dashboard implementation
DashboardState* dashboard_init(ECUContext* ecu_ctx) {
    DashboardState* dashboard = malloc(sizeof(DashboardState));
    if (!dashboard) {
        return NULL;
    }
    
    // Initialize dashboard state
    memset(dashboard, 0, sizeof(DashboardState));
    dashboard->ecu_ctx = ecu_ctx;
    dashboard->active = false;
    dashboard->last_update = 0;
    dashboard->frame_count = 0;
    
    // Initialize gauge values and alerts arrays
    dashboard->gauge_values = malloc(GAUGE_TYPE_COUNT * sizeof(float));
    dashboard->gauge_alerts = malloc(GAUGE_TYPE_COUNT * sizeof(bool));
    
    if (!dashboard->gauge_values || !dashboard->gauge_alerts) {
        dashboard_cleanup(dashboard);
        return NULL;
    }
    
    // Initialize arrays
    memset(dashboard->gauge_values, 0, GAUGE_TYPE_COUNT * sizeof(float));
    memset(dashboard->gauge_alerts, 0, GAUGE_TYPE_COUNT * sizeof(bool));
    
    // Set default configuration
    dashboard->config = dashboard_config_default();
    
    // UI context will be initialized with ImGui
    
    return dashboard;
}

void dashboard_cleanup(DashboardState* dashboard) {
    if (!dashboard) {
        return;
    }
    
    if (dashboard->gauge_values) {
        free(dashboard->gauge_values);
        dashboard->gauge_values = NULL;
    }
    
    if (dashboard->gauge_alerts) {
        free(dashboard->gauge_alerts);
        dashboard->gauge_alerts = NULL;
    }
    
    // UI context cleanup will be handled by ImGui
    
    free(dashboard);
}

bool dashboard_configure(DashboardState* dashboard, const DashboardConfig* config) {
    if (!dashboard || !config) {
        return false;
    }
    
    dashboard->config = *config;
    return true;
}

bool dashboard_update(DashboardState* dashboard) {
    if (!dashboard || !dashboard->active) {
        return false;
    }
    
    // Update ECU data if available
    if (dashboard->ecu_ctx && ecu_is_connected(dashboard->ecu_ctx)) {
        const ECUData* data = ecu_get_data(dashboard->ecu_ctx);
        if (data) {
            // Update gauge values from ECU data
            dashboard->gauge_values[GAUGE_TYPE_RPM] = data->rpm;
            dashboard->gauge_values[GAUGE_TYPE_MAP] = data->map;
            dashboard->gauge_values[GAUGE_TYPE_TPS] = data->tps;
            dashboard->gauge_values[GAUGE_TYPE_AFR] = data->afr;
            dashboard->gauge_values[GAUGE_TYPE_BOOST] = data->boost;
            dashboard->gauge_values[GAUGE_TYPE_TEMP] = data->coolant_temp;
            dashboard->gauge_values[GAUGE_TYPE_VOLTAGE] = data->battery_voltage;
            dashboard->gauge_values[GAUGE_TYPE_TIMING] = data->timing;
            dashboard->gauge_values[GAUGE_TYPE_FUEL_PRESSURE] = data->fuel_pressure;
            dashboard->gauge_values[GAUGE_TYPE_OIL_PRESSURE] = data->oil_pressure;
        }
    }
    
    dashboard->frame_count++;
    dashboard->last_update = SDL_GetTicks();
    
    return true;
}

bool dashboard_render(DashboardState* dashboard, SDL_Renderer* renderer, TTF_Font* font) {
    if (!dashboard || !dashboard->active || !renderer) {
        return false;
    }
    
    // Placeholder for ImGui rendering
    // This will be implemented with ImGui
    
    return true;
}

void dashboard_set_active(DashboardState* dashboard, bool active) {
    if (dashboard) {
        dashboard->active = active;
    }
}

// Configuration helpers
DashboardConfig dashboard_config_default(void) {
    DashboardConfig config = {0};
    config.layout = DASHBOARD_LAYOUT_QUAD_GAUGE;
    strcpy(config.title, "MegaTunix Redux Dashboard");
    config.width = 800;
    config.height = 600;
    config.gauge_count = 4;
    config.background_color = (Color){20, 20, 20, 255};
    config.border_color = (Color){60, 60, 60, 255};
    config.show_title = true;
    config.show_status_bar = true;
    config.auto_refresh = true;
    config.refresh_rate_ms = 100;
    
    // Set default gauges
    config.gauges[0] = gauge_config_rpm();
    config.gauges[1] = gauge_config_boost();
    config.gauges[2] = gauge_config_afr();
    config.gauges[3] = gauge_config_temp();
    
    return config;
}

DashboardConfig dashboard_config_rpm_boost(void) {
    DashboardConfig config = dashboard_config_default();
    config.layout = DASHBOARD_LAYOUT_DUAL_GAUGE;
    strcpy(config.title, "RPM & Boost Monitor");
    config.gauge_count = 2;
    config.gauges[0] = gauge_config_rpm();
    config.gauges[1] = gauge_config_boost();
    return config;
}

DashboardConfig dashboard_config_full_monitoring(void) {
    DashboardConfig config = dashboard_config_default();
    config.layout = DASHBOARD_LAYOUT_GRID_4X4;
    strcpy(config.title, "Full Engine Monitor");
    config.gauge_count = 9;
    config.gauges[0] = gauge_config_rpm();
    config.gauges[1] = gauge_config_map();
    config.gauges[2] = gauge_config_tps();
    config.gauges[3] = gauge_config_afr();
    config.gauges[4] = gauge_config_boost();
    config.gauges[5] = gauge_config_temp();
    config.gauges[6] = gauge_config_voltage();
    config.gauges[7] = gauge_config_timing();
    config.gauges[8] = gauge_config_fuel_pressure();
    return config;
}

DashboardConfig dashboard_config_tuning(void) {
    DashboardConfig config = dashboard_config_default();
    config.layout = DASHBOARD_LAYOUT_GRID_3X3;
    strcpy(config.title, "Tuning Dashboard");
    config.gauge_count = 6;
    config.gauges[0] = gauge_config_rpm();
    config.gauges[1] = gauge_config_afr();
    config.gauges[2] = gauge_config_boost();
    config.gauges[3] = gauge_config_timing();
    config.gauges[4] = gauge_config_fuel_pressure();
    config.gauges[5] = gauge_config_temp();
    return config;
}

DashboardConfig dashboard_config_diagnostic(void) {
    DashboardConfig config = dashboard_config_default();
    config.layout = DASHBOARD_LAYOUT_GRID_4X4;
    strcpy(config.title, "Diagnostic Dashboard");
    config.gauge_count = 8;
    config.gauges[0] = gauge_config_rpm();
    config.gauges[1] = gauge_config_map();
    config.gauges[2] = gauge_config_tps();
    config.gauges[3] = gauge_config_afr();
    config.gauges[4] = gauge_config_voltage();
    config.gauges[5] = gauge_config_temp();
    config.gauges[6] = gauge_config_fuel_pressure();
    config.gauges[7] = gauge_config_oil_pressure();
    return config;
}

// Gauge configuration helpers
GaugeConfig gauge_config_rpm(void) {
    GaugeConfig config = {0};
    config.type = GAUGE_TYPE_RPM;
    strcpy(config.label, "RPM");
    strcpy(config.unit, "rpm");
    config.min_value = 0.0f;
    config.max_value = 8000.0f;
    config.warning_threshold = 0.8f;
    config.danger_threshold = 0.9f;
    config.show_value = true;
    config.show_percentage = false;
    config.normal_color = (Color){0, 255, 0, 255};
    config.warning_color = (Color){255, 255, 0, 255};
    config.danger_color = (Color){255, 0, 0, 255};
    config.background_color = (Color){40, 40, 40, 255};
    config.text_color = (Color){255, 255, 255, 255};
    return config;
}

GaugeConfig gauge_config_map(void) {
    GaugeConfig config = {0};
    config.type = GAUGE_TYPE_MAP;
    strcpy(config.label, "MAP");
    strcpy(config.unit, "kPa");
    config.min_value = 0.0f;
    config.max_value = 300.0f;
    config.warning_threshold = 0.7f;
    config.danger_threshold = 0.85f;
    config.show_value = true;
    config.show_percentage = false;
    config.normal_color = (Color){0, 255, 0, 255};
    config.warning_color = (Color){255, 255, 0, 255};
    config.danger_color = (Color){255, 0, 0, 255};
    config.background_color = (Color){40, 40, 40, 255};
    config.text_color = (Color){255, 255, 255, 255};
    return config;
}

GaugeConfig gauge_config_tps(void) {
    GaugeConfig config = {0};
    config.type = GAUGE_TYPE_TPS;
    strcpy(config.label, "TPS");
    strcpy(config.unit, "%");
    config.min_value = 0.0f;
    config.max_value = 100.0f;
    config.warning_threshold = 0.8f;
    config.danger_threshold = 0.95f;
    config.show_value = true;
    config.show_percentage = true;
    config.normal_color = (Color){0, 255, 0, 255};
    config.warning_color = (Color){255, 255, 0, 255};
    config.danger_color = (Color){255, 0, 0, 255};
    config.background_color = (Color){40, 40, 40, 255};
    config.text_color = (Color){255, 255, 255, 255};
    return config;
}

GaugeConfig gauge_config_afr(void) {
    GaugeConfig config = {0};
    config.type = GAUGE_TYPE_AFR;
    strcpy(config.label, "AFR");
    strcpy(config.unit, "");
    config.min_value = 10.0f;
    config.max_value = 20.0f;
    config.warning_threshold = 0.3f;
    config.danger_threshold = 0.5f;
    config.show_value = true;
    config.show_percentage = false;
    config.normal_color = (Color){0, 255, 0, 255};
    config.warning_color = (Color){255, 255, 0, 255};
    config.danger_color = (Color){255, 0, 0, 255};
    config.background_color = (Color){40, 40, 40, 255};
    config.text_color = (Color){255, 255, 255, 255};
    return config;
}

GaugeConfig gauge_config_boost(void) {
    GaugeConfig config = {0};
    config.type = GAUGE_TYPE_BOOST;
    strcpy(config.label, "Boost");
    strcpy(config.unit, "psi");
    config.min_value = -20.0f;
    config.max_value = 30.0f;
    config.warning_threshold = 0.8f;
    config.danger_threshold = 0.9f;
    config.show_value = true;
    config.show_percentage = false;
    config.normal_color = (Color){0, 255, 0, 255};
    config.warning_color = (Color){255, 255, 0, 255};
    config.danger_color = (Color){255, 0, 0, 255};
    config.background_color = (Color){40, 40, 40, 255};
    config.text_color = (Color){255, 255, 255, 255};
    return config;
}

GaugeConfig gauge_config_temp(void) {
    GaugeConfig config = {0};
    config.type = GAUGE_TYPE_TEMP;
    strcpy(config.label, "Temp");
    strcpy(config.unit, "°C");
    config.min_value = 0.0f;
    config.max_value = 120.0f;
    config.warning_threshold = 0.7f;
    config.danger_threshold = 0.85f;
    config.show_value = true;
    config.show_percentage = false;
    config.normal_color = (Color){0, 255, 0, 255};
    config.warning_color = (Color){255, 255, 0, 255};
    config.danger_color = (Color){255, 0, 0, 255};
    config.background_color = (Color){40, 40, 40, 255};
    config.text_color = (Color){255, 255, 255, 255};
    return config;
}

GaugeConfig gauge_config_voltage(void) {
    GaugeConfig config = {0};
    config.type = GAUGE_TYPE_VOLTAGE;
    strcpy(config.label, "Voltage");
    strcpy(config.unit, "V");
    config.min_value = 10.0f;
    config.max_value = 15.0f;
    config.warning_threshold = 0.3f;
    config.danger_threshold = 0.5f;
    config.show_value = true;
    config.show_percentage = false;
    config.normal_color = (Color){0, 255, 0, 255};
    config.warning_color = (Color){255, 255, 0, 255};
    config.danger_color = (Color){255, 0, 0, 255};
    config.background_color = (Color){40, 40, 40, 255};
    config.text_color = (Color){255, 255, 255, 255};
    return config;
}

GaugeConfig gauge_config_timing(void) {
    GaugeConfig config = {0};
    config.type = GAUGE_TYPE_TIMING;
    strcpy(config.label, "Timing");
    strcpy(config.unit, "°");
    config.min_value = -20.0f;
    config.max_value = 50.0f;
    config.warning_threshold = 0.8f;
    config.danger_threshold = 0.9f;
    config.show_value = true;
    config.show_percentage = false;
    config.normal_color = (Color){0, 255, 0, 255};
    config.warning_color = (Color){255, 255, 0, 255};
    config.danger_color = (Color){255, 0, 0, 255};
    config.background_color = (Color){40, 40, 40, 255};
    config.text_color = (Color){255, 255, 255, 255};
    return config;
}

GaugeConfig gauge_config_fuel_pressure(void) {
    GaugeConfig config = {0};
    config.type = GAUGE_TYPE_FUEL_PRESSURE;
    strcpy(config.label, "Fuel Press");
    strcpy(config.unit, "psi");
    config.min_value = 0.0f;
    config.max_value = 100.0f;
    config.warning_threshold = 0.7f;
    config.danger_threshold = 0.85f;
    config.show_value = true;
    config.show_percentage = false;
    config.normal_color = (Color){0, 255, 0, 255};
    config.warning_color = (Color){255, 255, 0, 255};
    config.danger_color = (Color){255, 0, 0, 255};
    config.background_color = (Color){40, 40, 40, 255};
    config.text_color = (Color){255, 255, 255, 255};
    return config;
}

GaugeConfig gauge_config_oil_pressure(void) {
    GaugeConfig config = {0};
    config.type = GAUGE_TYPE_OIL_PRESSURE;
    strcpy(config.label, "Oil Press");
    strcpy(config.unit, "psi");
    config.min_value = 0.0f;
    config.max_value = 100.0f;
    config.warning_threshold = 0.3f;
    config.danger_threshold = 0.5f;
    config.show_value = true;
    config.show_percentage = false;
    config.normal_color = (Color){0, 255, 0, 255};
    config.warning_color = (Color){255, 255, 0, 255};
    config.danger_color = (Color){255, 0, 0, 255};
    config.background_color = (Color){40, 40, 40, 255};
    config.text_color = (Color){255, 255, 255, 255};
    return config;
}

// Placeholder gauge rendering functions for ImGui
void dashboard_render_single_gauge(DashboardState* dashboard, SDL_Renderer* renderer, TTF_Font* font) {
    // Placeholder for ImGui rendering
    // This will be implemented with ImGui
}

void dashboard_render_dual_gauge(DashboardState* dashboard, SDL_Renderer* renderer, TTF_Font* font) {
    // Placeholder for ImGui rendering
    // This will be implemented with ImGui
}

void dashboard_render_quad_gauge(DashboardState* dashboard, SDL_Renderer* renderer, TTF_Font* font) {
    // Placeholder for ImGui rendering
    // This will be implemented with ImGui
}

void dashboard_render_grid_3x3(DashboardState* dashboard, SDL_Renderer* renderer, TTF_Font* font) {
    // Placeholder for ImGui rendering
    // This will be implemented with ImGui
}

void dashboard_render_grid_4x4(DashboardState* dashboard, SDL_Renderer* renderer, TTF_Font* font) {
    // Placeholder for ImGui rendering
    // This will be implemented with ImGui
}

// Utility functions
float dashboard_get_gauge_value(DashboardState* dashboard, GaugeType type) {
    if (!dashboard || type < 0 || type >= GAUGE_TYPE_COUNT) {
        return 0.0f;
    }
    return dashboard->gauge_values[type];
}

bool dashboard_is_gauge_alert(DashboardState* dashboard, GaugeType type) {
    if (!dashboard || type < 0 || type >= GAUGE_TYPE_COUNT) {
        return false;
    }
    return dashboard->gauge_alerts[type];
}

Color dashboard_get_gauge_color(DashboardState* dashboard, GaugeType type) {
    if (!dashboard || type < 0 || type >= GAUGE_TYPE_COUNT) {
        return (Color){255, 255, 255, 255};
    }
    
    const GaugeConfig* gauge = &dashboard->config.gauges[type];
    float value = dashboard_get_gauge_value(dashboard, type);
    float percentage = (value - gauge->min_value) / (gauge->max_value - gauge->min_value);
    
    if (percentage > gauge->danger_threshold) {
        return gauge->danger_color;
    } else if (percentage > gauge->warning_threshold) {
        return gauge->warning_color;
    } else {
        return gauge->normal_color;
    }
}

void dashboard_update_status_text(DashboardState* dashboard) {
    if (!dashboard) {
        return;
    }
    
    if (dashboard->ecu_ctx && ecu_is_connected(dashboard->ecu_ctx)) {
        strcpy(dashboard->status_text, "Connected");
    } else {
        strcpy(dashboard->status_text, "Disconnected");
    }
}

const char* dashboard_get_layout_name(DashboardLayout layout) {
    switch (layout) {
        case DASHBOARD_LAYOUT_SINGLE_GAUGE: return "Single Gauge";
        case DASHBOARD_LAYOUT_DUAL_GAUGE: return "Dual Gauge";
        case DASHBOARD_LAYOUT_QUAD_GAUGE: return "Quad Gauge";
        case DASHBOARD_LAYOUT_GRID_3X3: return "3x3 Grid";
        case DASHBOARD_LAYOUT_GRID_4X4: return "4x4 Grid";
        case DASHBOARD_LAYOUT_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

const char* dashboard_get_gauge_type_name(GaugeType type) {
    switch (type) {
        case GAUGE_TYPE_RPM: return "RPM";
        case GAUGE_TYPE_MAP: return "MAP";
        case GAUGE_TYPE_TPS: return "TPS";
        case GAUGE_TYPE_AFR: return "AFR";
        case GAUGE_TYPE_BOOST: return "Boost";
        case GAUGE_TYPE_TEMP: return "Temperature";
        case GAUGE_TYPE_VOLTAGE: return "Voltage";
        case GAUGE_TYPE_TIMING: return "Timing";
        case GAUGE_TYPE_FUEL_PRESSURE: return "Fuel Pressure";
        case GAUGE_TYPE_OIL_PRESSURE: return "Oil Pressure";
        case GAUGE_TYPE_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

// Event handling
bool dashboard_handle_mouse_click(DashboardState* dashboard, int x, int y) {
    // Placeholder for ImGui event handling
    // This will be implemented with ImGui
    return false;
}

bool dashboard_handle_key_press(DashboardState* dashboard, SDL_Keycode key) {
    // Placeholder for ImGui event handling
    // This will be implemented with ImGui
    return false;
} 