/*
 * Dashboard System - Real-time ECU Data Display
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * Provides flexible dashboard layouts for displaying ECU data.
 */

#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "../ecu/ecu_communication.h"
#include "../megatunix_redux.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
// Clay removed - will be replaced with ImGui
#include <stdbool.h>
#include <stdint.h>

// Dashboard layout types
typedef enum {
    DASHBOARD_LAYOUT_SINGLE_GAUGE = 0,
    DASHBOARD_LAYOUT_DUAL_GAUGE,
    DASHBOARD_LAYOUT_QUAD_GAUGE,
    DASHBOARD_LAYOUT_GRID_3X3,
    DASHBOARD_LAYOUT_GRID_4X4,
    DASHBOARD_LAYOUT_CUSTOM,
    DASHBOARD_LAYOUT_COUNT
} DashboardLayout;

// Gauge types
typedef enum {
    GAUGE_TYPE_RPM = 0,
    GAUGE_TYPE_MAP,
    GAUGE_TYPE_TPS,
    GAUGE_TYPE_AFR,
    GAUGE_TYPE_BOOST,
    GAUGE_TYPE_TEMP,
    GAUGE_TYPE_VOLTAGE,
    GAUGE_TYPE_TIMING,
    GAUGE_TYPE_FUEL_PRESSURE,
    GAUGE_TYPE_OIL_PRESSURE,
    GAUGE_TYPE_CUSTOM,
    GAUGE_TYPE_COUNT
} GaugeType;

// Gauge configuration
typedef struct {
    GaugeType type;
    char label[64];
    char unit[16];
    float min_value;
    float max_value;
    float warning_threshold;
    float danger_threshold;
    bool show_value;
    bool show_percentage;
    Color normal_color;
    Color warning_color;
    Color danger_color;
    Color background_color;
    Color text_color;
} GaugeConfig;

// Dashboard configuration
typedef struct {
    DashboardLayout layout;
    char title[128];
    int width;
    int height;
    int gauge_count;
    GaugeConfig gauges[16];
    Color background_color;
    Color border_color;
    bool show_title;
    bool show_status_bar;
    bool auto_refresh;
    int refresh_rate_ms;
} DashboardConfig;

// Dashboard state
typedef struct {
    DashboardConfig config;
    ECUContext* ecu_ctx;
    // ClayUIContext* ui_ctx; // Clay removed - will be replaced with ImGui
    bool active;
    uint32_t last_update;
    uint32_t frame_count;
    float* gauge_values;
    bool* gauge_alerts;
    char status_text[256];
} DashboardState;

// Function declarations
DashboardState* dashboard_init(ECUContext* ecu_ctx);
void dashboard_cleanup(DashboardState* dashboard);
bool dashboard_configure(DashboardState* dashboard, const DashboardConfig* config);
bool dashboard_update(DashboardState* dashboard);
bool dashboard_render(DashboardState* dashboard, SDL_Renderer* renderer, TTF_Font* font);
void dashboard_set_active(DashboardState* dashboard, bool active);

// Configuration helpers
DashboardConfig dashboard_config_default(void);
DashboardConfig dashboard_config_rpm_boost(void);
DashboardConfig dashboard_config_full_monitoring(void);
DashboardConfig dashboard_config_tuning(void);
DashboardConfig dashboard_config_diagnostic(void);

// Gauge configuration helpers
GaugeConfig gauge_config_rpm(void);
GaugeConfig gauge_config_map(void);
GaugeConfig gauge_config_tps(void);
GaugeConfig gauge_config_afr(void);
GaugeConfig gauge_config_boost(void);
GaugeConfig gauge_config_temp(void);
GaugeConfig gauge_config_voltage(void);
GaugeConfig gauge_config_timing(void);
GaugeConfig gauge_config_fuel_pressure(void);
GaugeConfig gauge_config_oil_pressure(void);

// Layout rendering functions
void dashboard_render_single_gauge(DashboardState* dashboard, SDL_Renderer* renderer, TTF_Font* font);
void dashboard_render_dual_gauge(DashboardState* dashboard, SDL_Renderer* renderer, TTF_Font* font);
void dashboard_render_quad_gauge(DashboardState* dashboard, SDL_Renderer* renderer, TTF_Font* font);
void dashboard_render_grid_3x3(DashboardState* dashboard, SDL_Renderer* renderer, TTF_Font* font);
void dashboard_render_grid_4x4(DashboardState* dashboard, SDL_Renderer* renderer, TTF_Font* font);

// Utility functions
float dashboard_get_gauge_value(DashboardState* dashboard, GaugeType type);
bool dashboard_is_gauge_alert(DashboardState* dashboard, GaugeType type);
Color dashboard_get_gauge_color(DashboardState* dashboard, GaugeType type);
void dashboard_update_status_text(DashboardState* dashboard);
const char* dashboard_get_layout_name(DashboardLayout layout);
const char* dashboard_get_gauge_type_name(GaugeType type);

// Event handling
bool dashboard_handle_mouse_click(DashboardState* dashboard, int x, int y);
bool dashboard_handle_key_press(DashboardState* dashboard, SDL_Keycode key);

#endif // DASHBOARD_H 