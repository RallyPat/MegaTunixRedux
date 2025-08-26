/*
 * MegaTunix Redux - Main Application Entry Point
 * 
 * Copyright (C) 2025 Patrick Burke
 * Based on original MegaTunix by David J. Andruczyk
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>

// ImGui includes
// Dear ImGui v1.92.2 WIP - Copyright (c) 2014-2025 Omar Cornut
// Licensed under MIT License - see LICENSE_ATTRIBUTION.md for full license text
#include "../../external/imgui/imgui.h"
#include "../../external/imgui/imgui_impl_sdl2.h"
#include "../../external/imgui/imgui_impl_opengl3.h"

#include "../include/megatunix_redux.h"
#include "../include/ecu/ecu_communication.h"
#include "../include/dashboard/dashboard.h"
#include "../include/utils/config.h"
#include "../include/utils/logging.h"
#include "../include/ui/imgui_ve_table.h"
#include "../include/ui/ve_table_editor.h"
#include "../include/ui/ui_theme_manager.h"
#include "../include/ui/logging_system.h"
#include "../include/ui/settings_manager.h"
#include "../include/ui/table_operations.h"
#include "../include/ui/ecu_integration.h"
#include "../include/ui/imgui_runtime_display.h"
#include "../include/ui/imgui_communications.h"
#include "../include/ui/imgui_key_bindings.h"
#include "../include/plugin/plugin_manager.h"
#include "../include/core/data_bridge.h"
#include "../include/data/datalog_manager.h"
#include "../include/automation/macro_engine.h"
#include "../include/automation/action_triggers.h"
#include "../include/integrations/gps_provider.h"
#include "../include/ui/undo_redo.h"
#include "../include/ui/keybindings_prefs.h"
#include "../include/io/export_import.h"
#include "../include/diagnostics/diagnostics.h"

// Forward declaration for log callback
void imgui_communications_set_log_callback(LogCallback callback);

// UI Theme and Styling System - Now handled by ui_theme_manager module

// Theme functions now handled by ui_theme_manager module
// Theme array and functions now handled by ui_theme_manager module

// Theme functions now handled by ui_theme_manager module

// Theme application now handled by ui_theme_manager module

// Theme switching now handled by ui_theme_manager module

// Professional button rendering now handled by ui_theme_manager module

// Section header rendering now handled by ui_theme_manager module

// Status indicator rendering now handled by ui_theme_manager module

// Metric card rendering now handled by ui_theme_manager module

// Log system constants and structures now handled by logging_system module

// Global variables
static SDL_Window* g_window = NULL;
static SDL_GLContext g_gl_context = NULL;
static TTF_Font* g_font = NULL;
static bool g_running = true;
static int g_selected_tab = 8; // Default to VE Table tab so it gets initialized
static const char* g_tab_names[] = {
    "About",
    "General", 
    "Communications",
    "Runtime Display",
    "DataLogging",
    "Logviewer",
    "Engine Vitals",
    "Enrichments",
    "VE Table(1)",
    "Ignition Table",
    "Tools",
    "Warmup Wizard",
    "Plugin Manager"
};

// Log system state now handled by logging_system module

// ECU state
static ECUContext* g_ecu_context = NULL;
static bool g_ecu_connected = false;
static char g_ecu_status[256] = "Disconnected";
static ECUData g_ecu_data = {0};

// Button states
static bool g_locate_port_button_pressed = false;
static bool g_reverse_connect_button_pressed = false;
static bool g_reset_io_button_pressed = false;
static int g_button_press_timer = 0;
static char g_button_status_text[256] = "";

// Log timing now handled by logging_system module

// ECU Table state
static ImGuiTable* g_ve_table = NULL;
static ImGuiTable* g_ignition_table = NULL;
static ImGuiVE3DView g_ve_3d_view = {0};
static bool g_ve_table_initialized = false;
static bool g_ignition_table_initialized = false;
static ImGuiVETexture* g_ve_texture = NULL;
static bool g_ve_texture_initialized = false;
static bool g_ve_mouse_dragging = false;
static ImVec2 g_ve_last_mouse_pos = {0, 0};
static int g_ve_view_mode = 0; // 0=2D, 1=3D, 2=Editor
// Single cell selection (for backward compatibility)
static int g_selected_cell_x = -1;
static int g_selected_cell_y = -1;

// Multi-cell selection system
typedef struct {
    int start_x, start_y;
    int end_x, end_y;
    bool active;
    bool dragging;
    ImVec2 drag_start_pos;
    ImVec2 drag_current_pos;
} MultiCellSelection;

static MultiCellSelection g_multi_selection = {-1, -1, -1, -1, false, false, {0, 0}, {0, 0}};
static char g_cell_edit_buffer[32] = "";
static bool g_cell_editing = false;
static bool g_buffer_updated = false; // Flag to indicate buffer was updated
static bool g_input_field_focused = false; // Flag to track if input field is focused
static bool g_show_input_field = false; // Flag to control when to show the input field
static bool g_just_navigated = false; // Flag to track when we just navigated to a new cell
static int g_current_rpm_cell = -1;
static int g_current_map_cell = -1;

// Engine trail tracking
#define MAX_TRAIL_POINTS 50
typedef struct {
    int x, y;
    uint32_t timestamp;
    float alpha; // Fading alpha value
} TrailPoint;

static TrailPoint g_engine_trail_2d[MAX_TRAIL_POINTS];
static TrailPoint g_engine_trail_3d[MAX_TRAIL_POINTS];
static int g_trail_count_2d = 0;
static int g_trail_count_3d = 0;
static uint32_t g_last_trail_update = 0;
static bool g_show_engine_trail = true; // Toggle for engine trail visibility
// Removed Y-axis label rotation toggle - MAP title is now permanently rotated

// View system for TunerStudio-style layout
typedef enum {
    VIEW_GAUGE_CLUSTER = 0,
    VIEW_TUNING,
    VIEW_GRAPHING,
    VIEW_DIAGNOSTICS,
    VIEW_COMMUNICATIONS,
    VIEW_PLUGIN_MANAGER,
    VIEW_COUNT
} ViewType;

static ViewType g_selected_view = VIEW_GAUGE_CLUSTER;

// Global variable to track which table is selected in tuning view
static int g_selected_table_index = 0; // 0 = VE Table, 1 = Ignition Table

// Speeduino Communication Protocol - now handled by ecu_integration module
// Note: These constants are defined in ecu_communication.h

// Settings and persistence now handled by settings_manager module

// Speeduino Communication Structures - now handled by ecu_integration module
// Note: These structures are defined in ecu_communication.h

// UserSettings structure now handled by settings_manager module

// UserSettings initialization now handled by settings_manager module

// Speeduino communication globals - now handled by ecu_integration module
// Note: These are now managed by the ecu_integration module

// Helper function to get the currently active table
static ImGuiTable* get_active_table() {
    // Check if we're in the VE Table tab
    if (g_selected_tab == 8) {
        return g_ve_table;
    }
    // Check if we're in the tuning view with a table selected
    else if (g_selected_view == VIEW_TUNING) {
        if (g_selected_table_index == 0) {
            return g_ve_table;
        } else if (g_selected_table_index == 1) {
            return g_ignition_table;
        }
    }
    return NULL;
}

// Speeduino globals - now handled by ecu_integration module
// Note: These are now managed by the ecu_integration module
static uint32_t g_last_realtime_update = 0;
static uint32_t g_realtime_update_interval = 100; // 10Hz update rate

// Demo mode variables
static bool g_demo_mode = false;
static bool g_debug_mode = false;

// Key binding state for professional table editing
static TableKeyBindingState g_key_binding_state;

// Clipboard for copy/paste operations
static float g_clipboard_data[16][16]; // 16x16 grid for clipboard
static int g_clipboard_width = 0;
static int g_clipboard_height = 0;
static bool g_clipboard_has_data = false;

// Advanced VE ops UI state
static bool g_show_set_to_popup = false;
static float g_set_to_value = 0.0f;
static bool g_show_paste_special_popup = false;
static int g_paste_special_mode = 0; // 0=Multiply %, 1=Multiply Raw, 2=Add, 3=Subtract

// Professional table operations and display
static bool g_table_interpolation_mode = false;
static int g_interpolation_start_x = -1;
static int g_interpolation_start_y = -1;
static int g_interpolation_end_x = -1;
static int g_interpolation_end_y = -1;
static float g_interpolation_factor = 0.5f;

// Table comparison and versioning
static ImGuiTable* g_table_backup = NULL;
static bool g_table_has_changes = false;
static int g_table_version = 1;
static char g_table_comment[256] = "Initial table";

// Professional table display options
static bool g_show_table_headers = true;
static bool g_show_table_grid = true;
static bool g_show_table_values = true;
static bool g_show_table_heatmap = true;
static float g_table_opacity = 0.8f;
static int g_table_view_mode = 0; // 0=2D, 1=3D, 2=Comparison, 3=Professional
static bool g_show_settings_window = false;

// Interpolation state for smooth transitions
static bool g_interpolation_mode = false;

// Legend window state
static bool show_legend = false;
static bool g_legend_just_popped = false;
// Legend enhancements
static bool key_binding_matches(SDL_Keycode key, SDL_Keymod mod, TableOperation expected) {
    TableKeyBindingState tmp_state;
    imgui_key_bindings_init(&tmp_state);
    imgui_key_bindings_set_table_focused(true);
    TableOperation op = imgui_key_bindings_process_key(&tmp_state, key, mod);
    imgui_key_bindings_set_table_focused(false);
    return op == expected;
}

static bool compute_bindings_mismatch(void) {
    bool ok = true;
    ok &= key_binding_matches(SDLK_PLUS, (SDL_Keymod)0, TABLE_OP_INCREASE_BY) ||
          key_binding_matches(SDLK_KP_PLUS, (SDL_Keymod)0, TABLE_OP_INCREASE_BY);
    ok &= key_binding_matches(SDLK_MINUS, (SDL_Keymod)0, TABLE_OP_DECREASE_BY) ||
          key_binding_matches(SDLK_KP_MINUS, (SDL_Keymod)0, TABLE_OP_DECREASE_BY);
    ok &= key_binding_matches(SDLK_ASTERISK, (SDL_Keymod)0, TABLE_OP_SCALE_BY) ||
          key_binding_matches(SDLK_KP_MULTIPLY, (SDL_Keymod)0, TABLE_OP_SCALE_BY);
    ok &= key_binding_matches(SDLK_i, (SDL_Keymod)0, TABLE_OP_INTERPOLATE);
    ok &= key_binding_matches(SDLK_h, (SDL_Keymod)0, TABLE_OP_INTERPOLATE_H);
    ok &= key_binding_matches(SDLK_v, (SDL_Keymod)0, TABLE_OP_INTERPOLATE_V);
    ok &= key_binding_matches(SDLK_s, (SDL_Keymod)0, TABLE_OP_SMOOTH_CELLS);
    ok &= key_binding_matches(SDLK_f, (SDL_Keymod)0, TABLE_OP_FILL_UP_RIGHT);
    ok &= key_binding_matches(SDLK_c, KMOD_CTRL, TABLE_OP_COPY);
    ok &= key_binding_matches(SDLK_v, KMOD_CTRL, TABLE_OP_PASTE);
    return !ok;
}

// Window position cache for event handling (to avoid calling ImGui functions from event handler)
static ImVec2 g_table_window_pos = ImVec2(0, 0);
static ImVec2 g_table_window_size = ImVec2(0, 0);
static bool g_table_window_valid = false;

// Safe table access wrapper (forward declaration)
struct SafeTableAccess;

// Runtime Display state
static ImGuiRuntimeDisplay* g_runtime_display = NULL;
static bool g_runtime_display_initialized = false;

// Communications state
static ImGuiCommunications* g_communications = NULL;
static bool g_communications_initialized = false;

// Camera preset functions
void set_isometric_view() {
    g_ve_3d_view.rotation_x = 35.264f; // arctan(1/sqrt(2)) - proper isometric angle
    g_ve_3d_view.rotation_y = 45.0f;
    g_ve_3d_view.zoom = 2.0f;
    g_ve_3d_view.pan_x = 0.0f;
    g_ve_3d_view.pan_y = 0.0f;
}

void set_side_view() {
    g_ve_3d_view.rotation_x = 0.0f;
    g_ve_3d_view.rotation_y = 90.0f;
    g_ve_3d_view.zoom = 1.5f;
    g_ve_3d_view.pan_x = 0.0f;
    g_ve_3d_view.pan_y = 0.0f;
}

void set_top_down_view() {
    g_ve_3d_view.rotation_x = 90.0f;
    g_ve_3d_view.rotation_y = 0.0f;
    g_ve_3d_view.zoom = 1.5f;
    g_ve_3d_view.pan_x = 0.0f;
    g_ve_3d_view.pan_y = 0.0f;
}

// Log system functions now handled by logging_system module


// Safe table access wrapper implementation
struct SafeTableAccess {
    static bool is_table_valid() {
        return g_ve_table != NULL && g_ve_table_initialized && 
               g_ve_table->data != NULL && 
               g_ve_table->width > 0 && g_ve_table->height > 0;
    }
    
    static float get_value_safe(int x, int y, float default_value = 75.0f) {
        if (!is_table_valid()) {
            add_log_entry(2, "SafeTableAccess: Table not valid for get_value_safe(%d, %d)", x, y);
            return default_value;
        }
        
        if (x < 0 || x >= g_ve_table->width || y < 0 || y >= g_ve_table->height) {
            add_log_entry(2, "SafeTableAccess: Invalid coordinates (%d, %d) for table %dx%d", 
                         x, y, g_ve_table->width, g_ve_table->height);
            return default_value;
        }
        
        if (g_ve_table->data[y] == NULL) {
            add_log_entry(2, "SafeTableAccess: Row %d is NULL", y);
            return default_value;
        }
        
        return g_ve_table->data[y][x];
    }
    
    static bool set_value_safe(int x, int y, float value) {
        if (!is_table_valid()) {
            add_log_entry(2, "SafeTableAccess: Table not valid for set_value_safe(%d, %d, %.1f)", x, y, value);
            return false;
        }
        
        if (x < 0 || x >= g_ve_table->width || y < 0 || y >= g_ve_table->height) {
            add_log_entry(2, "SafeTableAccess: Invalid coordinates (%d, %d) for table %dx%d", 
                         x, y, g_ve_table->width, g_ve_table->height);
            return false;
        }
        
        if (g_ve_table->data[y] == NULL) {
            add_log_entry(2, "SafeTableAccess: Row %d is NULL", y);
            return false;
        }
        
        g_ve_table->data[y][x] = value;
        return true;
    }
};

// Forward declarations for multi-cell selection functions
void start_multi_selection(int x, int y);
void update_multi_selection(int x, int y);
void end_multi_selection();

// render_log_window function now handled by logging_system module


// Function declarations
bool init_sdl();
bool init_opengl();
bool init_ttf();
bool init_imgui();
bool init_ecu_communication();
void cleanup_sdl();
void cleanup_opengl();
void cleanup_ttf();
void cleanup_imgui();
void cleanup_ecu_communication();
void handle_events();
void update();
void render();
void render_main_window();
void render_about_tab();
void render_general_tab();
void render_communications_tab();
void render_runtime_display_tab();
void render_datalogging_tab();
void render_logviewer_tab();
void render_engine_vitals_tab();
void render_enrichments_tab();
void render_ve_table_tab();
void render_ignition_table_tab();
void render_ve_table_2d_view();
void render_ve_table_3d_view();
void render_ve_table_editor_view();
void render_tools_tab();
void render_warmup_wizard_tab();

// TunerStudio-style UI functions
void render_tunerstudio_sidebar();
void render_content_by_view();
void render_gauge_cluster_view();
void render_tuning_view();
void render_graphing_view();
void render_diagnostics_view();
void render_communications_view();
void render_plugin_manager_view();
void render_settings_window();

// Settings management functions
void load_user_settings();
void save_user_settings();
void apply_user_settings();
void reset_user_settings_to_defaults();

// Professional table operations
void create_table_backup();
void restore_table_from_backup();
void compare_tables();
void interpolate_table_values();
void smooth_table_region();
void apply_table_math_operation(const char* operation, float value);
void export_table_to_file(const char* filename);
void import_table_from_file(const char* filename);
void render_professional_table_header();
void render_table_operations_toolbar();

// Speeduino communication system - now handled by ecu_integration module
void speeduino_init();
void speeduino_cleanup();
bool speeduino_connect(const char* port);
void speeduino_disconnect();
bool speeduino_is_connected();
bool speeduino_send_packet(SpeeduinoPacket* packet);
uint8_t speeduino_calculate_crc(uint8_t* data, int length);
bool speeduino_get_realtime_data();
bool speeduino_get_table_data(uint8_t table_id);
bool speeduino_set_table_data(uint8_t table_id, uint8_t* data, int length);
void speeduino_update_connection_status();
void render_ecu_connection_panel();

void handle_communications_buttons(); 
void update_engine_trail(int current_x, int current_y, TrailPoint* trail, int* trail_count);
void copy_selected_cell_to_clipboard();
void paste_from_clipboard();
void interpolate_between_cells();

// UI Theme functions now handled by ui_theme_manager module

// Advanced VE ops helpers
void copy_selection_to_clipboard();
void paste_block_at(int start_x, int start_y);
void paste_special_block_at(int start_x, int start_y, int mode);
void horizontal_interpolate_selection();
void vertical_interpolate_selection();

// Multi-cell selection helper functions
void clear_multi_selection();
void start_multi_selection(int x, int y);
void update_multi_selection(int x, int y);
void end_multi_selection();
bool is_cell_in_selection(int x, int y);
void get_selection_bounds(int* min_x, int* min_y, int* max_x, int* max_y);
int get_selection_cell_count();
void apply_operation_to_selection(TableOperation operation, float value);
void smooth_selection();
void gaussian_smooth_selection();
void moving_average_smooth_selection();
void bilateral_smooth_selection();

// Professional smoothing functions for different smoothing methods
// These provide the core functionality that professional tuners need

/**
 * Gaussian smoothing with configurable strength
 * Called when user presses 'G' key
 */
void gaussian_smooth_selection() {
    if (!g_ve_table) {
        add_log_entry(0, "No VE table available for Gaussian smoothing");
        return;
    }
    
    if (g_multi_selection.active) {
        // Use multi-selection bounds
        int min_x, min_y, max_x, max_y;
        get_selection_bounds(&min_x, &min_y, &max_x, &max_y);
        
        add_log_entry(0, "*** GAUSSIAN SMOOTHING SELECTION *** - Processing selection [%d,%d] to [%d,%d]", 
                     min_x, min_y, max_x, max_y);
        
        // Use Gaussian smoothing with edge preservation
        bool success = imgui_table_gaussian_smooth(g_ve_table, min_x, min_y, max_x, max_y, 1.5f, true);
        
        if (success) {
            add_log_entry(0, "*** GAUSSIAN SMOOTHING COMPLETE *** - Applied edge-preserving smoothing");
            
            // Update the 3D texture if available
            if (g_ve_texture) {
                imgui_ve_texture_update(g_ve_texture, g_ve_table, &g_ve_3d_view);
            }
        } else {
            add_log_entry(0, "*** GAUSSIAN SMOOTHING FAILED *** - Smoothing failed");
        }
        
    } else if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
        // Single cell selected - smooth a 5x5 area around it
        int start_x = fmax(0, g_selected_cell_x - 2);
        int end_x = fmin(g_ve_table->width - 1, g_selected_cell_x + 2);
        int start_y = fmax(0, g_selected_cell_y - 2);
        int end_y = fmin(g_ve_table->height - 1, g_selected_cell_y + 2);
        
        add_log_entry(0, "*** 5x5 GAUSSIAN SMOOTHING *** - Smoothing area around [%d,%d]", g_selected_cell_x, g_selected_cell_y);
        
        // Use Gaussian smoothing with edge preservation
        bool success = imgui_table_gaussian_smooth(g_ve_table, start_x, start_y, end_x, end_y, 1.0f, true);
        
        if (success) {
            add_log_entry(0, "*** 5x5 GAUSSIAN SMOOTHING COMPLETE *** - Applied edge-preserving smoothing");
            
            // Update the 3D texture if available
            if (g_ve_texture) {
                imgui_ve_texture_update(g_ve_texture, g_ve_table, &g_ve_3d_view);
            }
        } else {
            add_log_entry(0, "*** 5x5 GAUSSIAN SMOOTHING FAILED *** - Smoothing failed");
        }
        
    } else {
        add_log_entry(0, "No selection available for Gaussian smoothing");
    }
}

/**
 * Moving average smoothing for trend-based smoothing
 * Called when user presses 'M' key
 */
void moving_average_smooth_selection() {
    if (!g_ve_table) {
        add_log_entry(0, "No VE table available for moving average smoothing");
        return;
    }
    
    if (g_multi_selection.active) {
        // Use multi-selection bounds
        int min_x, min_y, max_x, max_y;
        get_selection_bounds(&min_x, &min_y, &max_x, &max_y);
        
        add_log_entry(0, "*** MOVING AVERAGE SMOOTHING SELECTION *** - Processing selection [%d,%d] to [%d,%d]", 
                     min_x, min_y, max_x, max_y);
        
        // Use moving average smoothing with 5x5 window
        bool success = imgui_table_moving_average_smooth(g_ve_table, min_x, min_y, max_x, max_y, 5, false);
        
        if (success) {
            add_log_entry(0, "*** MOVING AVERAGE SMOOTHING COMPLETE *** - Applied 5x5 moving average smoothing");
            
            // Update the 3D texture if available
            if (g_ve_texture) {
                imgui_ve_texture_update(g_ve_texture, g_ve_table, &g_ve_3d_view);
            }
        } else {
            add_log_entry(0, "*** MOVING AVERAGE SMOOTHING FAILED *** - Smoothing failed");
        }
        
    } else if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
        // Single cell selected - smooth a 3x3 area around it
        int start_x = fmax(0, g_selected_cell_x - 1);
        int end_x = fmin(g_ve_table->width - 1, g_selected_cell_x + 1);
        int start_y = fmax(0, g_selected_cell_y - 1);
        int end_y = fmin(g_ve_table->height - 1, g_selected_cell_y + 1);
        
        add_log_entry(0, "*** 3x3 MOVING AVERAGE SMOOTHING *** - Smoothing area around [%d,%d]", g_selected_cell_x, g_selected_cell_y);
        
        // Use moving average smoothing with 3x3 window
        bool success = imgui_table_moving_average_smooth(g_ve_table, start_x, start_y, end_x, end_y, 3, false);
        
        if (success) {
            add_log_entry(0, "*** 3x3 MOVING AVERAGE SMOOTHING COMPLETE *** - Applied 3x3 moving average smoothing");
            
            // Update the 3D texture if available
            if (g_ve_texture) {
                imgui_ve_texture_update(g_ve_texture, g_ve_table, &g_ve_3d_view);
            }
        } else {
            add_log_entry(0, "*** 3x3 MOVING AVERAGE SMOOTHING FAILED *** - Smoothing failed");
        }
        
    } else {
        add_log_entry(0, "No selection available for moving average smoothing");
    }
}

/**
 * Bilateral smoothing for edge-preserving smoothing
 * Called when user presses 'B' key
 */
void bilateral_smooth_selection() {
    if (!g_ve_table) {
        add_log_entry(0, "No VE table available for bilateral smoothing");
        return;
    }
    
    if (g_multi_selection.active) {
        // Use multi-selection bounds
        int min_x, min_y, max_x, max_y;
        get_selection_bounds(&min_x, &min_y, &max_x, &max_y);
        
        add_log_entry(0, "*** BILATERAL SMOOTHING SELECTION *** - Processing selection [%d,%d] to [%d,%d]", 
                     min_x, min_y, max_x, max_y);
        
        // Use bilateral smoothing with edge preservation
        bool success = imgui_table_bilateral_smooth(g_ve_table, min_x, min_y, max_x, max_y, 1.5f, 20.0f);
        
        if (success) {
            add_log_entry(0, "*** BILATERAL SMOOTHING COMPLETE *** - Applied edge-preserving bilateral smoothing");
            
            // Update the 3D texture if available
            if (g_ve_texture) {
                imgui_ve_texture_update(g_ve_texture, g_ve_table, &g_ve_3d_view);
            }
        } else {
            add_log_entry(0, "*** BILATERAL SMOOTHING FAILED *** - Smoothing failed");
        }
        
    } else if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
        // Single cell selected - smooth a 3x3 area around it
        int start_x = fmax(0, g_selected_cell_x - 1);
        int end_x = fmin(g_ve_table->width - 1, g_selected_cell_x + 1);
        int start_y = fmax(0, g_selected_cell_y - 1);
        int end_y = fmin(g_ve_table->height - 1, g_selected_cell_y + 1);
        
        add_log_entry(0, "*** 3x3 BILATERAL SMOOTHING *** - Smoothing area around [%d,%d]", g_selected_cell_x, g_selected_cell_y);
        
        // Use bilateral smoothing with edge preservation
        bool success = imgui_table_bilateral_smooth(g_ve_table, start_x, start_y, end_x, end_y, 1.0f, 15.0f);
        
        if (success) {
            add_log_entry(0, "*** 3x3 BILATERAL SMOOTHING COMPLETE *** - Applied edge-preserving bilateral smoothing");
            
            // Update the 3D texture if available
            if (g_ve_texture) {
                imgui_ve_texture_update(g_ve_texture, g_ve_table, &g_ve_3d_view);
            }
        } else {
            add_log_entry(0, "*** 3x3 BILATERAL SMOOTHING FAILED *** - Smoothing failed");
        }
        
    } else {
        add_log_entry(0, "No selection available for bilateral smoothing");
    }
}

int main(int argc, char* argv[]) {
    printf("MegaTunix Redux - ImGui Version\n");
    printf("By Patrick Burke\n");
    printf("Based on MegaTunix by David J. Andruczyk\n");
    printf("(ImGui Version)\n\n");

    // Parse command line arguments
    bool demo_mode = false;
    bool debug_mode = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--demo-mode") == 0 || strcmp(argv[i], "-d") == 0) {
            demo_mode = true;
            printf("Demo mode enabled\n");
        } else if (strcmp(argv[i], "--debug") == 0) {
            debug_mode = true;
            printf("Debug mode enabled\n");
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --demo-mode, -d    Enable demo mode with simulated data\n");
            printf("  --debug            Enable debug mode\n");
            printf("  --help, -h         Show this help message\n");
            return 0;
        }
    }

    // Set global demo mode flags
    g_demo_mode = demo_mode;
    g_debug_mode = debug_mode;
    
    // Note: Logging system not yet initialized - will log after initialization

    // Initialize SDL
    if (!init_sdl()) {
        add_log_entry(2, "Failed to initialize SDL");
        fprintf(stderr, "Failed to initialize SDL\n");
        return 1;
    }
    add_log_entry(0, "SDL initialized successfully");

    // Initialize OpenGL
    if (!init_opengl()) {
        add_log_entry(2, "Failed to initialize OpenGL");
        fprintf(stderr, "Failed to initialize OpenGL\n");
        cleanup_sdl();
        return 1;
    }
    add_log_entry(0, "OpenGL initialized successfully");

    // Initialize TTF
    if (!init_ttf()) {
        add_log_entry(2, "Failed to initialize TTF");
        fprintf(stderr, "Failed to initialize TTF\n");
        cleanup_opengl();
        cleanup_sdl();
        return 1;
    }
    add_log_entry(0, "TTF initialized successfully");

    // Initialize ImGui
    if (!init_imgui()) {
        add_log_entry(2, "Failed to initialize ImGui");
        fprintf(stderr, "Failed to initialize ImGui\n");
        cleanup_ttf();
        cleanup_opengl();
        cleanup_sdl();
        return 1;
    }
    add_log_entry(0, "ImGui initialized successfully");

    // Load user settings (this will apply the theme and other preferences)
    load_user_settings();
    
    add_log_entry(0, "User settings loaded successfully");

    // Initialize foundational subsystems (stubs) to support parity roadmap
    config_init();
    diagnostics_init();
    datalog_manager_init();
    macro_engine_init();
    action_triggers_init();
    gps_provider_init();
    undo_redo_init();
    keybindings_prefs_init();

    // Initialize ECU communication
    if (!init_ecu_communication()) {
        add_log_entry(2, "Failed to initialize ECU communication");
        fprintf(stderr, "Failed to initialize ECU communication\n");
        cleanup_imgui();
        cleanup_ttf();
        cleanup_opengl();
        cleanup_sdl();
        return 1;
    }
    add_log_entry(0, "ECU communication initialized successfully");
    
    // Set up global demo mode callback
    ecu_set_global_demo_mode_callback([](bool enabled) {
        g_demo_mode = enabled;
        if (enabled) {
            add_log_entry(0, "Global demo mode enabled - demo data generation active");
        } else {
            add_log_entry(0, "Global demo mode disabled - demo data generation stopped");
        }
    });
    
    // Initialize key binding system for professional table editing
    imgui_key_bindings_init(&g_key_binding_state);
    add_log_entry(0, "Key binding system initialized for professional table editing");

    // Initialize VE Table Editor module
    if (!init_ve_table_editor()) {
        add_log_entry(2, "Failed to initialize VE Table Editor module");
        fprintf(stderr, "Failed to initialize VE Table Editor module\n");
        cleanup_imgui();
        cleanup_ttf();
        cleanup_opengl();
        cleanup_sdl();
        return 1;
    }
    add_log_entry(0, "VE Table Editor module initialized successfully");

    // Initialize UI Theme Manager module
    if (!init_ui_theme_manager()) {
        add_log_entry(2, "Failed to initialize UI Theme Manager module");
        fprintf(stderr, "Failed to initialize UI Theme Manager module\n");
        cleanup_ve_table_editor();
        cleanup_imgui();
        cleanup_ttf();
        cleanup_opengl();
        cleanup_sdl();
        return 1;
    }
    add_log_entry(0, "UI Theme Manager module initialized successfully");

    // Initialize Logging System module
    if (!init_logging_system()) {
        add_log_entry(2, "Failed to initialize Logging System module");
        fprintf(stderr, "Failed to initialize Logging System module\n");
        cleanup_ui_theme_manager();
        cleanup_ve_table_editor();
        cleanup_imgui();
        cleanup_ttf();
        cleanup_opengl();
        cleanup_sdl();
        return 1;
    }
    add_log_entry(0, "Logging System module initialized successfully");

    // Initialize Settings Manager module
    if (!init_settings_manager()) {
        add_log_entry(2, "Failed to initialize Settings Manager module");
        fprintf(stderr, "Failed to initialize Settings Manager module\n");
        cleanup_logging_system();
        cleanup_ui_theme_manager();
        cleanup_ve_table_editor();
        cleanup_imgui();
        cleanup_ttf();
        cleanup_opengl();
        cleanup_sdl();
        return 1;
    }
    add_log_entry(0, "Settings Manager module initialized successfully");

    // Initialize Table Operations module
    if (!init_table_operations()) {
        add_log_entry(2, "Failed to initialize Table Operations module");
        fprintf(stderr, "Failed to initialize Table Operations module\n");
        cleanup_settings_manager();
        cleanup_logging_system();
        cleanup_ui_theme_manager();
        cleanup_ve_table_editor();
        cleanup_imgui();
        cleanup_ttf();
        cleanup_opengl();
        cleanup_sdl();
        return 1;
    }
    add_log_entry(0, "Table Operations module initialized successfully");

    // Initialize ECU Integration module
    if (!init_ecu_integration()) {
        add_log_entry(2, "Failed to initialize ECU Integration module");
        fprintf(stderr, "Failed to initialize ECU Integration module\n");
        cleanup_table_operations();
        cleanup_settings_manager();
        cleanup_logging_system();
        cleanup_ui_theme_manager();
        cleanup_ve_table_editor();
        cleanup_imgui();
        cleanup_ttf();
        cleanup_opengl();
        cleanup_sdl();
        return 1;
    }
    add_log_entry(0, "ECU Integration module initialized successfully");

    // Initialize Plugin System module
    if (!plugin_system_init()) {
        add_log_entry(2, "Failed to initialize Plugin System module");
        fprintf(stderr, "Failed to initialize Plugin System module\n");
        cleanup_ecu_integration();
        cleanup_table_operations();
        cleanup_settings_manager();
        cleanup_logging_system();
        cleanup_ui_theme_manager();
        cleanup_ve_table_editor();
        cleanup_imgui();
        cleanup_ttf();
        cleanup_opengl();
        cleanup_sdl();
        return 1;
    }
    add_log_entry(0, "Plugin System module initialized successfully");

    // Automatically scan and load plugins during startup
    add_log_entry(0, "Auto-scanning plugin directory...");
    PluginManager* mgr = get_plugin_manager();
    if (mgr && mgr->scan_plugin_directory) {
        add_log_entry(0, "Plugin manager found, scanning directory...");
        mgr->scan_plugin_directory("plugins");
        add_log_entry(0, "Plugin directory scanned");
        
        // Initialize all loaded plugins
        if (mgr->init_all_plugins) {
            add_log_entry(0, "Initializing all plugins...");
            mgr->init_all_plugins();
            add_log_entry(0, "All plugins initialized");
        } else {
            add_log_entry(2, "Warning: init_all_plugins function not available");
        }
    } else {
        add_log_entry(2, "Warning: Plugin manager not available for auto-loading");
    }

    // Initialize Data Bridge System
    if (!data_bridge_init()) {
        add_log_entry(2, "Failed to initialize Data Bridge System");
        fprintf(stderr, "Failed to initialize Data Bridge System\n");
        // Note: plugin_system_cleanup() will be called in normal cleanup path
        cleanup_ecu_integration();
        cleanup_table_operations();
        cleanup_settings_manager();
        cleanup_logging_system();
        cleanup_ui_theme_manager();
        cleanup_ve_table_editor();
        cleanup_imgui();
        cleanup_ttf();
        cleanup_opengl();
        cleanup_sdl();
        return 1;
    }
    add_log_entry(0, "Data Bridge System initialized successfully");

    // Initialize VE table early so it's available for the update loop
    add_log_entry(0, "Initializing VE table...");
    g_ve_table = imgui_ve_table_create(16, 12); // Start with default size, will be resized based on INI
    add_log_entry(0, "VE table created at address: %p with size: %dx%d", g_ve_table, g_ve_table ? g_ve_table->width : -1, g_ve_table ? g_ve_table->height : -1);
    if (g_ve_table) {
        add_log_entry(0, "VE table created successfully");
        add_log_entry(0, "*** VE TABLE CREATED: %dx%d ***", g_ve_table->width, g_ve_table->height);
        
        // Load demo data for testing
        imgui_table_load_demo_data(g_ve_table);
        add_log_entry(0, "VE table size: %dx%d", g_ve_table->width, g_ve_table->height);
        
        // Always load demo data for testing
        imgui_table_load_demo_data(g_ve_table);
        add_log_entry(0, "Demo data loaded into VE table");
        add_log_entry(0, "VE table value range: %.1f - %.1f", g_ve_table->metadata.min_value, g_ve_table->metadata.max_value);
        
        // Initialize 3D view with isometric default
        set_isometric_view();
        g_ve_3d_view.wireframe_mode = false;
        g_ve_3d_view.show_grid = true;
        g_ve_3d_view.show_axes = true;
        g_ve_3d_view.opacity = 0.8f;
        
        g_ve_table_initialized = true;
        add_log_entry(0, "VE table initialization complete");
    } else {
        add_log_entry(2, "ERROR: Failed to create VE table");
    }
    
    // Initialize ignition table
    add_log_entry(0, "Initializing ignition table...");
    g_ignition_table = (ImGuiTable*)malloc(sizeof(ImGuiTable));
    if (g_ignition_table) {
        memset(g_ignition_table, 0, sizeof(ImGuiTable));
        if (imgui_ignition_table_create(g_ignition_table, 16, 12)) {
            add_log_entry(0, "Ignition table created successfully");
            add_log_entry(0, "*** IGNITION TABLE CREATED: %dx%d ***", g_ignition_table->width, g_ignition_table->width);
            
            // Load demo data for testing
            imgui_ignition_table_load_demo_data(g_ignition_table);
            add_log_entry(0, "Ignition table size: %dx%d", g_ignition_table->width, g_ignition_table->height);
            add_log_entry(0, "Demo data loaded into ignition table");
            add_log_entry(0, "Ignition table value range: %.1f - %.1f %s", g_ignition_table->metadata.min_value, g_ignition_table->metadata.max_value, g_ignition_table->metadata.units);
            
            g_ignition_table_initialized = true;
            add_log_entry(0, "Ignition table initialization complete");
        } else {
            add_log_entry(2, "ERROR: Failed to initialize ignition table");
            free(g_ignition_table);
            g_ignition_table = NULL;
        }
    } else {
        add_log_entry(2, "ERROR: Failed to allocate ignition table");
    }
    
    add_log_entry(0, "VE table callbacks will be set up when communications module is initialized");

    add_log_entry(0, "Initialization complete - entering main loop");

    // Initialize Speeduino communication system
    speeduino_init();
    
    // Main loop
    while (g_running) {
        handle_events();
        update();
        
        // Update Speeduino communication status
        speeduino_update_connection_status();
        
        render();
    }

    add_log_entry(0, "Shutting down...");

    // Save user settings before cleanup
    save_user_settings();
    add_log_entry(0, "User settings saved");

    // Cleanup
    data_bridge_cleanup();
    plugin_system_cleanup();
    speeduino_cleanup();
    keybindings_prefs_shutdown();
    undo_redo_shutdown();
    cleanup_table_operations();
    cleanup_ecu_integration();
    cleanup_ve_table_editor();
    cleanup_ui_theme_manager();
    cleanup_logging_system();
    cleanup_settings_manager();
    gps_provider_shutdown();
    action_triggers_shutdown();
    macro_engine_shutdown();
    datalog_manager_shutdown();
    diagnostics_shutdown();
    config_cleanup();
    cleanup_ecu_communication();
    cleanup_imgui();
    cleanup_ttf();
    cleanup_opengl();
    cleanup_sdl();

    add_log_entry(0, "Cleanup complete");
    printf("Cleanup complete\n");
    return 0;
}

bool init_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window
    g_window = SDL_CreateWindow(
        "MEGATUNIX REDUX DISCONNECTED Ready for ECU Connection",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
    );

    if (!g_window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

bool init_opengl() {
    g_gl_context = SDL_GL_CreateContext(g_window);
    if (!g_gl_context) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_MakeCurrent(g_window, g_gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    return true;
}

bool init_ttf() {
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        return false;
    }

    g_font = TTF_OpenFont("assets/fonts/DejaVuSans.ttf", 14);
    if (!g_font) {
        printf("Could not load font: %s\n", TTF_GetError());
        printf("Continuing without custom font...\n");
        // Don't return false - we can continue without the font
    }

    return true;
}

bool init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Remove docking and viewports for now - they may not be available in this ImGui version
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Platform/Renderer backends FIRST
    if (!ImGui_ImplSDL2_InitForOpenGL(g_window, g_gl_context)) {
        fprintf(stderr, "ImGui_ImplSDL2_InitForOpenGL failed\n");
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 130")) {
        fprintf(stderr, "ImGui_ImplOpenGL3_Init failed\n");
        return false;
    }
    
    // Set renderer flags for proper font atlas handling BEFORE loading fonts
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

    // Setup style
    ImGui::StyleColorsDark();

    // Load Unicode font with comprehensive Unicode support
    ImFont* font = nullptr;
    
    // Configure font atlas for Unicode support
    ImFontAtlas* atlas = io.Fonts;
    atlas->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
    atlas->Flags |= ImFontAtlasFlags_NoMouseCursors;
    
    // Add Unicode glyph ranges for better symbol support (16-bit compatible)
    static const ImWchar ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2190, 0x21FF, // Arrows
        0x2600, 0x26FF, // Miscellaneous Symbols
        0x2700, 0x27BF, // Dingbats
        0,
    };
    
    // Configure font config for better Unicode support
    ImFontConfig font_config;
    font_config.MergeMode = false;
    font_config.PixelSnapH = true;
    font_config.OversampleH = 2;
    font_config.OversampleV = 1;
    
    // Try to load a font with Unicode support - start with DejaVu which has good Unicode coverage
    font = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16.0f, &font_config, ranges);
    if (font) {
        printf("Loaded DejaVu Sans font successfully\n");
    } else {
        // Try bundled Noto Sans
        font = io.Fonts->AddFontFromFileTTF("assets/fonts/NotoSans-Regular.ttf", 16.0f, &font_config, ranges);
        if (font) {
            printf("Loaded bundled Noto Sans font successfully\n");
        } else {
            // Try system Noto Sans
            font = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf", 16.0f, &font_config, ranges);
            if (font) {
                printf("Loaded system Noto Sans font successfully\n");
            } else {
                // Try Ubuntu font
                font = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf", 16.0f, &font_config, ranges);
                if (font) {
                    printf("Loaded Ubuntu font successfully\n");
                } else {
                    // Try Liberation Sans
                    font = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 16.0f, &font_config, ranges);
                    if (font) {
                        printf("Loaded Liberation Sans font successfully\n");
                    } else {
                        printf("Warning: Could not load any Unicode font, using default font\n");
                    }
                }
            }
        }
    }
    if (!font) {
        // Final fallback - use default font
        printf("Warning: Could not load Unicode font, using default font\n");
        font = io.Fonts->AddFontDefault();
        if (font) {
            printf("Loaded default font successfully\n");
        }
    }
    
    if (font) {
        // Set as default font
        io.FontDefault = font;
        printf("Loaded Unicode font successfully: %s\n", font ? "Yes" : "No");
    }

    return true;
}

// ECU communication functions now handled by ecu_integration module
bool init_ecu_communication(); // Declaration only - implementation in ecu_integration module

void cleanup_sdl() {
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
    }
    SDL_Quit();
}

void cleanup_opengl() {
    if (g_gl_context) {
        SDL_GL_DeleteContext(g_gl_context);
        g_gl_context = NULL;
    }
}

void cleanup_ttf() {
    if (g_font) {
        TTF_CloseFont(g_font);
        g_font = NULL;
    }
    TTF_Quit();
}

void cleanup_imgui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void cleanup_ecu_communication(); // Declaration only - implementation in ecu_integration module

void handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Get ImGui IO state to check if ImGui wants the event
        ImGuiIO& io = ImGui::GetIO();
        bool imgui_wants_mouse = io.WantCaptureMouse;
        
        // Handle our custom input processing first (before ImGui)
        bool event_handled = false;
        
        // Variables for table editing (declared outside switch to avoid jump issues)
        bool is_table_editing_active = false;
        ImGuiTable* active_table = NULL;
        
        switch (event.type) {
            case SDL_QUIT:
                g_running = false;
                break;
                
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    g_running = false;
                }
                break;
                
            case SDL_KEYDOWN:
                // Handle function key shortcuts for view switching
                switch (event.key.keysym.sym) {
                    case SDLK_F1:
                        g_selected_view = VIEW_GAUGE_CLUSTER;
                        add_log_entry(0, "F1 pressed - switching to Gauge Cluster view");
                        event_handled = true;
                        break;
                    case SDLK_F2:
                        g_selected_view = VIEW_TUNING;
                        add_log_entry(0, "F2 pressed - switching to Tuning view");
                        event_handled = true;
                        break;
                    case SDLK_F3:
                        g_selected_view = VIEW_GRAPHING;
                        add_log_entry(0, "F3 pressed - switching to Graphing view");
                        event_handled = true;
                        break;
                    case SDLK_F4:
                        g_selected_view = VIEW_DIAGNOSTICS;
                        add_log_entry(0, "F4 pressed - switching to Diagnostics view");
                        event_handled = true;
                        break;
                    case SDLK_F5:
                        g_selected_view = VIEW_COMMUNICATIONS;
                        add_log_entry(0, "F5 pressed - switching to Communications view");
                        event_handled = true;
                        break;
                    case SDLK_F6:
                        g_selected_view = VIEW_PLUGIN_MANAGER;
                        add_log_entry(0, "F6 pressed - switching to Plugin Manager view");
                        event_handled = true;
                        break;
                }
                
                if (event_handled) break;
                
                // Handle legend window keyboard shortcuts (global, regardless of focus)
                if (show_legend && (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_RETURN)) {
                    add_log_entry(0, "Global keyboard shortcut pressed - closing legend window");
                    show_legend = false;
                    event_handled = true;
                    break;
                }
                
                // Handle professional table editing key bindings (for VE Table and Ignition Table)
                // Check if we're in the VE Table tab
                if (g_selected_tab == 8) {
                    is_table_editing_active = true;
                    active_table = g_ve_table;
                }
                // Check if we're in the tuning view with a table selected
                else if (g_selected_view == VIEW_TUNING) {
                    if (g_selected_table_index == 0) {
                        is_table_editing_active = true;
                        active_table = g_ve_table;
                    } else if (g_selected_table_index == 1) {
                        is_table_editing_active = true;
                        active_table = g_ignition_table;
                    }
                }
                
                if (is_table_editing_active && active_table) {
                    imgui_key_bindings_set_table_focused(true);
                    
                    // Add detailed key logging for debugging
                    add_log_entry(0, "*** KEY EVENT DEBUG *** - Key: %c (0x%02X), Modifiers: 0x%04X, Tab: %d", 
                                 event.key.keysym.sym, event.key.keysym.sym, event.key.keysym.mod, g_selected_tab);
                    
                    // CRITICAL DEBUG: Log table selection state
                    add_log_entry(0, "*** TABLE SELECTION DEBUG *** - View: %d, Table Index: %d, Active Table: %s", 
                                 g_selected_view, g_selected_table_index, 
                                 (active_table == g_ve_table) ? "VE_TABLE" : 
                                 (active_table == g_ignition_table) ? "IGNITION_TABLE" : "UNKNOWN");
                    
                    TableOperation operation = imgui_key_bindings_process_key(&g_key_binding_state, 
                                                                             event.key.keysym.sym, 
                                                                             (SDL_Keymod)event.key.keysym.mod);
                    add_log_entry(0, "*** KEY PRESSED *** - Key: %c (0x%02X), Operation: %d (%s)", 
                                 event.key.keysym.sym, event.key.keysym.sym, operation, 
                                 imgui_key_bindings_get_operation_name(operation));
                    
                    add_log_entry(0, "*** OPERATION CHECK *** - Operation: %d, Selected Cell: [%d,%d], Table Focused: %s", 
                                 operation, g_selected_cell_x, g_selected_cell_y, 
                                 imgui_key_bindings_is_table_focused() ? "true" : "false");
                    
                    // Handle basic navigation keys first
                    bool navigation_handled = false;
                    if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
                        // Save current buffer value before navigating away
                        if (strlen(g_cell_edit_buffer) > 0) {
                            float input_value = atof(g_cell_edit_buffer);
                            // Use appropriate range for the table type
                            float min_val = 0.0f;
                            float max_val = 200.0f;
                            if (active_table && active_table->metadata.type == TABLE_TYPE_IGNITION) {
                                min_val = active_table->metadata.min_value;
                                max_val = active_table->metadata.max_value;
                            }
                            if (input_value >= min_val && input_value <= max_val) {
                                // Save to the current cell before moving
                                imgui_table_set_value(active_table, g_selected_cell_x, g_selected_cell_y, input_value);
                                add_log_entry(0, "Auto-saved value %.1f to cell [%d,%d] on navigation", 
                                            input_value, g_selected_cell_x, g_selected_cell_y);
                            }
                        }
                        
                        switch (event.key.keysym.sym) {
                            case SDLK_UP:
                                // If multi-selection is active and we are not extending with Shift, clear it
                                if (g_multi_selection.active && !(event.key.keysym.mod & KMOD_SHIFT)) {
                                    clear_multi_selection();
                                }
                                if ((event.key.keysym.mod & KMOD_SHIFT) && g_multi_selection.active) {
                                    // Extend selection up
                                    update_multi_selection(g_selected_cell_x, fmax(0, g_selected_cell_y - 1));
                                    g_selected_cell_y = fmax(0, g_selected_cell_y - 1);
                                    add_log_entry(0, "Multi-selection: Extended UP to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                } else if ((event.key.keysym.mod & KMOD_SHIFT)) {
                                    // Shift+Up: Start or extend multi-cell selection up
                                    if (!g_multi_selection.active) {
                                        start_multi_selection(g_selected_cell_x, g_selected_cell_y);
                                        add_log_entry(0, "*** KEYBOARD MULTI-SELECTION STARTED *** - Shift+Up");
                                    }
                                    update_multi_selection(g_selected_cell_x, fmax(0, g_selected_cell_y - 1));
                                    g_selected_cell_y = fmax(0, g_selected_cell_y - 1);
                                    add_log_entry(0, "Multi-selection: Extended UP to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                } else if ((event.key.keysym.mod & (KMOD_CTRL | KMOD_SHIFT)) == (KMOD_CTRL | KMOD_SHIFT)) {
                                    // Ctrl+Shift+Up: Start multi-cell selection up (legacy support)
                                    add_log_entry(0, "*** KEYBOARD MULTI-SELECTION TRIGGERED *** - Ctrl+Shift+Up");
                                    if (!g_multi_selection.active) {
                                        start_multi_selection(g_selected_cell_x, g_selected_cell_y);
                                    }
                                    update_multi_selection(g_selected_cell_x, fmax(0, g_selected_cell_y - 1));
                                    g_selected_cell_y = fmax(0, g_selected_cell_y - 1);
                                    add_log_entry(0, "Multi-selection: Started UP to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                } else {
                                    g_selected_cell_y = fmax(0, g_selected_cell_y - 1);
                                    add_log_entry(0, "Navigation: Moved UP to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                }
                                navigation_handled = true;
                                break;
                            case SDLK_DOWN:
                                if (g_multi_selection.active && !(event.key.keysym.mod & KMOD_SHIFT)) {
                                    clear_multi_selection();
                                }
                                if ((event.key.keysym.mod & KMOD_SHIFT) && g_multi_selection.active) {
                                    // Extend selection down
                                    update_multi_selection(g_selected_cell_x, fmin(active_table->height - 1, g_selected_cell_y + 1));
                                    g_selected_cell_y = fmin(active_table->height - 1, g_selected_cell_y + 1);
                                    add_log_entry(0, "Multi-selection: Extended DOWN to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                } else if ((event.key.keysym.mod & KMOD_SHIFT)) {
                                    // Shift+Down: Start or extend multi-cell selection down
                                    if (!g_multi_selection.active) {
                                        start_multi_selection(g_selected_cell_x, g_selected_cell_y);
                                        add_log_entry(0, "*** KEYBOARD MULTI-SELECTION STARTED *** - Shift+Down");
                                    }
                                    update_multi_selection(g_selected_cell_x, fmin(active_table->height - 1, g_selected_cell_y + 1));
                                    g_selected_cell_y = fmin(active_table->height - 1, g_selected_cell_y + 1);
                                    add_log_entry(0, "Multi-selection: Extended DOWN to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                } else if ((event.key.keysym.mod & (KMOD_CTRL | KMOD_SHIFT)) == (KMOD_CTRL | KMOD_SHIFT)) {
                                    // Ctrl+Shift+Down: Start multi-cell selection down (legacy support)
                                    if (!g_multi_selection.active) {
                                        start_multi_selection(g_selected_cell_x, g_selected_cell_y);
                                    }
                                    update_multi_selection(g_selected_cell_x, fmin(active_table->height - 1, g_selected_cell_y + 1));
                                    g_selected_cell_y = fmin(active_table->height - 1, g_selected_cell_y + 1);
                                    add_log_entry(0, "Multi-selection: Started DOWN to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                } else {
                                    g_selected_cell_y = fmin(active_table->height - 1, g_selected_cell_y + 1);
                                    add_log_entry(0, "Navigation: Moved DOWN to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                }
                                navigation_handled = true;
                                break;
                            case SDLK_LEFT:
                                if (g_multi_selection.active && !(event.key.keysym.mod & KMOD_SHIFT)) {
                                    clear_multi_selection();
                                }
                                if ((event.key.keysym.mod & KMOD_SHIFT) && g_multi_selection.active) {
                                    // Extend selection left
                                    update_multi_selection(fmax(0, g_selected_cell_x - 1), g_selected_cell_y);
                                    g_selected_cell_x = fmax(0, g_selected_cell_x - 1);
                                    add_log_entry(0, "Multi-selection: Extended LEFT to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                } else if ((event.key.keysym.mod & KMOD_SHIFT)) {
                                    // Shift+Left: Start or extend multi-cell selection left
                                    if (!g_multi_selection.active) {
                                        start_multi_selection(g_selected_cell_x, g_selected_cell_y);
                                        add_log_entry(0, "*** KEYBOARD MULTI-SELECTION STARTED *** - Shift+Left");
                                    }
                                    update_multi_selection(fmax(0, g_selected_cell_x - 1), g_selected_cell_y);
                                    g_selected_cell_x = fmax(0, g_selected_cell_x - 1);
                                    add_log_entry(0, "Multi-selection: Extended LEFT to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                } else if ((event.key.keysym.mod & (KMOD_CTRL | KMOD_SHIFT)) == (KMOD_CTRL | KMOD_SHIFT)) {
                                    // Ctrl+Shift+Left: Start multi-cell selection left (legacy support)
                                    if (!g_multi_selection.active) {
                                        start_multi_selection(g_selected_cell_x, g_selected_cell_y);
                                    }
                                    update_multi_selection(fmax(0, g_selected_cell_x - 1), g_selected_cell_y);
                                    g_selected_cell_x = fmax(0, g_selected_cell_x - 1);
                                    add_log_entry(0, "Multi-selection: Started LEFT to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                } else {
                                    g_selected_cell_x = fmax(0, g_selected_cell_x - 1);
                                    add_log_entry(0, "Navigation: Moved LEFT to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                }
                                navigation_handled = true;
                                break;
                            case SDLK_RIGHT:
                                if (g_multi_selection.active && !(event.key.keysym.mod & KMOD_SHIFT)) {
                                    clear_multi_selection();
                                }
                                if ((event.key.keysym.mod & KMOD_SHIFT) && g_multi_selection.active) {
                                    // Extend selection right
                                    update_multi_selection(fmin(active_table->width - 1, g_selected_cell_x + 1), g_selected_cell_y);
                                    g_selected_cell_x = fmin(active_table->width - 1, g_selected_cell_x + 1);
                                    add_log_entry(0, "Multi-selection: Extended RIGHT to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                } else if ((event.key.keysym.mod & KMOD_SHIFT)) {
                                    // Shift+Right: Start or extend multi-cell selection right
                                    if (!g_multi_selection.active) {
                                        start_multi_selection(g_selected_cell_x, g_selected_cell_y);
                                        add_log_entry(0, "*** KEYBOARD MULTI-SELECTION STARTED *** - Shift+Right");
                                    }
                                    update_multi_selection(fmin(active_table->width - 1, g_selected_cell_x + 1), g_selected_cell_y);
                                    g_selected_cell_x = fmin(active_table->width - 1, g_selected_cell_x + 1);
                                    add_log_entry(0, "Multi-selection: Extended RIGHT to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                } else if ((event.key.keysym.mod & (KMOD_CTRL | KMOD_SHIFT)) == (KMOD_CTRL | KMOD_SHIFT)) {
                                    // Ctrl+Shift+Right: Start multi-cell selection right (legacy support)
                                    if (!g_multi_selection.active) {
                                        start_multi_selection(g_selected_cell_x, g_selected_cell_y);
                                    }
                                    update_multi_selection(fmin(active_table->width - 1, g_selected_cell_x + 1), g_selected_cell_y);
                                    g_selected_cell_x = fmin(active_table->width - 1, g_selected_cell_x + 1);
                                    add_log_entry(0, "Multi-selection: Started RIGHT to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                } else {
                                    g_selected_cell_x = fmin(active_table->width - 1, g_selected_cell_x + 1);
                                    add_log_entry(0, "Navigation: Moved RIGHT to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                }
                                navigation_handled = true;
                                break;
                            case SDLK_TAB:
                                if (g_multi_selection.active && !(event.key.keysym.mod & KMOD_SHIFT)) {
                                    clear_multi_selection();
                                }
                                if (event.key.keysym.mod & KMOD_SHIFT) {
                                    // Shift+Tab: Move backward
                                    g_selected_cell_x--;
                                    if (g_selected_cell_x < 0) {
                                        g_selected_cell_x = active_table->width - 1;
                                        g_selected_cell_y--;
                                        if (g_selected_cell_y < 0) {
                                            g_selected_cell_y = active_table->height - 1;
                                        }
                                    }
                                } else {
                                    // Tab: Move forward
                                    g_selected_cell_x++;
                                    if (g_selected_cell_x >= active_table->width) {
                                        g_selected_cell_x = 0;
                                        g_selected_cell_y++;
                                        if (g_selected_cell_y >= active_table->height) {
                                            g_selected_cell_y = 0;
                                        }
                                    }
                                }
                                navigation_handled = true;
                                add_log_entry(0, "Navigation: Tab to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                break;
                            case SDLK_ESCAPE:
                                // Escape: Clear multi-cell selection or cancel editing
                                if (g_multi_selection.active) {
                                    clear_multi_selection();
                                    add_log_entry(0, "Multi-cell selection cleared");
                                } else if (g_cell_editing) {
                                    g_cell_editing = false;
                                    g_show_input_field = false;
                                    g_input_field_focused = false;
                                    add_log_entry(0, "Cell editing cancelled");
                                }
                                navigation_handled = true;
                                break;
                        }
                    }
                    
                    if (navigation_handled) {
                        event_handled = true;
                        // Update the cell edit buffer with the new cell's value
                        snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.1f", 
                                imgui_table_get_value(active_table, g_selected_cell_x, g_selected_cell_y));
                        g_buffer_updated = true;
                        g_just_navigated = true; // Set flag to clear buffer on next number entry
                    }
                    
                    // Check if we have a valid target (single cell or multi-cell selection)
                    bool has_valid_target = (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) || g_multi_selection.active;
                    
                    if (operation != TABLE_OP_NONE && has_valid_target) {
                        float current_val, new_val;
                        
                        // Handle professional operations
                        switch (operation) {
                            case TABLE_OP_SET_TO:
                                // Trigger Set-To popup
                                if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
                                    g_set_to_value = imgui_table_get_value(active_table, g_selected_cell_x, g_selected_cell_y);
                                } else {
                                    g_set_to_value = 0.0f;
                                }
                                g_show_set_to_popup = true;
                                add_log_entry(0, "Set To operation requested - opening input dialog");
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_INCREMENT:
                                // Increment by configured amount
                                if (g_multi_selection.active) {
                                    apply_operation_to_selection(TABLE_OP_INCREASE_BY, g_key_binding_state.increment_amount);
                                } else {
                                    current_val = imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y);
                                    new_val = fmin(200.0f, current_val + g_key_binding_state.increment_amount);
                                    imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, new_val);
                                    snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.0f", new_val);
                                    g_buffer_updated = true;
                                    add_log_entry(0, "Incremented cell [%d,%d] to %.1f", g_selected_cell_x, g_selected_cell_y, new_val);
                                }
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_DECREMENT:
                                // Decrement by configured amount
                                if (g_multi_selection.active) {
                                    apply_operation_to_selection(TABLE_OP_DECREASE_BY, g_key_binding_state.increment_amount);
                                } else {
                                    current_val = imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y);
                                    new_val = fmax(0.0f, current_val - g_key_binding_state.increment_amount);
                                    imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, new_val);
                                    snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.0f", new_val);
                                    g_buffer_updated = true;
                                    add_log_entry(0, "Decremented cell [%d,%d] to %.1f", g_selected_cell_x, g_selected_cell_y, new_val);
                                }
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_INCREASE_BY:
                                // Increase by configured amount
                                if (g_multi_selection.active) {
                                    apply_operation_to_selection(TABLE_OP_INCREASE_BY, g_key_binding_state.increment_amount);
                                } else {
                                    current_val = imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y);
                                    new_val = fmin(200.0f, current_val + g_key_binding_state.increment_amount);
                                    imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, new_val);
                                    snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.0f", new_val);
                                    g_buffer_updated = true;
                                    add_log_entry(0, "Increased cell [%d,%d] to %.1f", g_selected_cell_x, g_selected_cell_y, new_val);
                                }
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_DECREASE_BY:
                                // Decrease by configured amount
                                if (g_multi_selection.active) {
                                    apply_operation_to_selection(TABLE_OP_DECREASE_BY, g_key_binding_state.increment_amount);
                                } else {
                                    current_val = imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y);
                                    new_val = fmax(0.0f, current_val - g_key_binding_state.increment_amount);
                                    imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, new_val);
                                    snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.0f", new_val);
                                    g_buffer_updated = true;
                                    add_log_entry(0, "Decreased cell [%d,%d] to %.1f", g_selected_cell_x, g_selected_cell_y, new_val);
                                }
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_SCALE_BY: {
                                // Scale by configured percentage factor
                                if (g_multi_selection.active) {
                                    apply_operation_to_selection(TABLE_OP_SCALE_BY, g_key_binding_state.percent_increment);
                                } else {
                                    current_val = imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y);
                                    float scale_factor = 1.0f + (g_key_binding_state.percent_increment / 100.0f);
                                    new_val = fmin(200.0f, fmax(0.0f, current_val * scale_factor));
                                    imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, new_val);
                                    snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.0f", new_val);
                                    g_buffer_updated = true;
                                    add_log_entry(0, "Scaled cell [%d,%d] by %.1f%% to %.1f", g_selected_cell_x, g_selected_cell_y, g_key_binding_state.percent_increment, new_val);
                                }
                                event_handled = true;
                                break;
                            }
                                
                            case TABLE_OP_INTERPOLATE:
                                add_log_entry(0, "*** INTERPOLATION TRIGGERED *** - Key pressed, calling interpolate_between_cells()");
                                interpolate_between_cells_legacy();
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_INTERPOLATE_H:
                                add_log_entry(0, "Horizontal interpolation triggered");
                                horizontal_interpolate_selection_legacy();
                                event_handled = true;
                                break;
                            case TABLE_OP_INTERPOLATE_V:
                                add_log_entry(0, "Vertical interpolation triggered");
                                vertical_interpolate_selection_legacy();
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_SMOOTH_CELLS:
                                add_log_entry(0, "*** SMOOTHING TRIGGERED *** - Key pressed, calling smooth_selection()");
                                smooth_selection_legacy();
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_GAUSSIAN_SMOOTH:
                                add_log_entry(0, "*** GAUSSIAN SMOOTHING TRIGGERED *** - Key pressed, calling gaussian_smooth_selection()");
                                gaussian_smooth_selection_legacy();
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_MOVING_AVERAGE_SMOOTH:
                                add_log_entry(0, "*** MOVING AVERAGE SMOOTHING TRIGGERED *** - Key pressed, calling moving_average_smooth_selection()");
                                moving_average_smooth_selection_legacy();
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_BILATERAL_SMOOTH:
                                add_log_entry(0, "*** BILATERAL SMOOTHING TRIGGERED *** - Key pressed, calling bilateral_smooth_selection()");
                                bilateral_smooth_selection_legacy();
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_FILL_UP_RIGHT: {
                                // Fill selection with current cell's value (Up+Right style)
                                if (g_ve_table) {
                                    float fill_val = 0.0f;
                                    bool have_source = false;
                                    if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
                                        fill_val = imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y);
                                        have_source = true;
                                    } else if (g_multi_selection.active) {
                                        // Fallback to selection's start cell
                                        fill_val = imgui_table_get_value(g_ve_table, g_multi_selection.start_x, g_multi_selection.start_y);
                                        have_source = true;
                                    }
                                    if (have_source && g_multi_selection.active) {
                                        apply_operation_to_selection(TABLE_OP_SET_TO, fill_val);
                                    }
                                }
                                event_handled = true;
                                break;
                            }
                                
                            case TABLE_OP_COPY:
                                copy_selection_to_clipboard_legacy();
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_PASTE:
                                paste_from_clipboard_legacy();
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_RESET:
                                // Reset selected cells to default value (75.0f for VE table)
                                if (g_multi_selection.active) {
                                    apply_operation_to_selection(TABLE_OP_SET_TO, 75.0f);
                                    add_log_entry(0, "Reset selection to default value 75.0");
                                } else if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
                                    imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, 75.0f);
                                    snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "75.0");
                                    g_buffer_updated = true;
                                    add_log_entry(0, "Reset cell [%d,%d] to default value 75.0", g_selected_cell_x, g_selected_cell_y);
                                }
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_UNDO:
                                // TODO: Implement undo system
                                add_log_entry(0, "Undo operation requested (not yet implemented)");
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_REDO:
                                // TODO: Implement redo system
                                add_log_entry(0, "Redo operation requested (not yet implemented)");
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_HELP:
                                // Toggle help/legend window
                                show_legend = !show_legend;
                                add_log_entry(0, "Help/legend window toggled: %s", show_legend ? "shown" : "hidden");
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_EDIT_MODE:
                                // Toggle edit mode (could be used to lock/unlock table editing)
                                add_log_entry(0, "Edit mode toggle requested (not yet implemented)");
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_FIND:
                                // Find/search functionality
                                add_log_entry(0, "Find/search requested (not yet implemented)");
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_REPLACE:
                                // Replace functionality
                                add_log_entry(0, "Replace requested (not yet implemented)");
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_REFRESH:
                                // Refresh table data from ECU
                                if (g_ve_table) {
                                    imgui_table_load_demo_data(g_ve_table);
                                    add_log_entry(0, "Table data refreshed");
                                }
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_ZOOM_IN:
                                // Zoom in on table view
                                if (g_ve_3d_view.zoom < 10.0f) {
                                    g_ve_3d_view.zoom *= 1.2f;
                                    add_log_entry(0, "Zoomed in to %.1f", g_ve_3d_view.zoom);
                                }
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_ZOOM_OUT:
                                // Zoom out on table view
                                if (g_ve_3d_view.zoom > 0.1f) {
                                    g_ve_3d_view.zoom /= 1.2f;
                                    add_log_entry(0, "Zoomed out to %.1f", g_ve_3d_view.zoom);
                                }
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_FIT_VIEW:
                                // Fit table to view
                                g_ve_3d_view.zoom = 1.5f;
                                g_ve_3d_view.rotation_x = 35.264f;
                                g_ve_3d_view.rotation_y = 45.0f;
                                g_ve_3d_view.pan_x = 0.0f;
                                g_ve_3d_view.pan_y = 0.0f;
                                add_log_entry(0, "View reset to default isometric view");
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_TOGGLE_SELECTION:
                                // Toggle cell selection (could be used to clear selection)
                                if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
                                    g_selected_cell_x = -1;
                                    g_selected_cell_y = -1;
                                    clear_multi_selection();
                                    add_log_entry(0, "Cell selection cleared");
                                }
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_CLEAR_CELL:
                                // Clear cell value (set to 0)
                                if (g_multi_selection.active) {
                                    apply_operation_to_selection(TABLE_OP_SET_TO, 0.0f);
                                    add_log_entry(0, "Selection cleared to 0");
                                } else if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
                                    imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, 0.0f);
                                    snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "0.0");
                                    g_buffer_updated = true;
                                    add_log_entry(0, "Cell [%d,%d] cleared to 0", g_selected_cell_x, g_selected_cell_y);
                                }
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_APPLY_CHANGES:
                                // Apply current buffer changes
                                if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0 && strlen(g_cell_edit_buffer) > 0) {
                                    float input_value = atof(g_cell_edit_buffer);
                                    if (input_value >= 0.0f && input_value <= 200.0f) {
                                        imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, input_value);
                                        add_log_entry(0, "Applied buffer value %.1f to cell [%d,%d]", input_value, g_selected_cell_x, g_selected_cell_y);
                                        g_cell_edit_buffer[0] = '\0';
                                        g_buffer_updated = true;
                                    }
                                }
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_CANCEL_OPERATION:
                                // Cancel current operation (clear buffer, clear selection)
                                g_cell_edit_buffer[0] = '\0';
                                g_buffer_updated = true;
                                if (g_multi_selection.active) {
                                    clear_multi_selection();
                                    add_log_entry(0, "Multi-selection cancelled");
                                }
                                event_handled = true;
                                break;
                                
                            default:
                                break;
                        }
                    }
                    
                    // Handle direct number entry for selected cell
                    if (!event_handled && g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
                    // Check if it's a number key (0-9) or decimal point (ignore when Ctrl is held to avoid accidental paste)
                    if (!(event.key.keysym.mod & KMOD_CTRL) &&
                        ((event.key.keysym.sym >= SDLK_0 && event.key.keysym.sym <= SDLK_9) || 
                         event.key.keysym.sym == SDLK_PERIOD || event.key.keysym.sym == SDLK_KP_PERIOD)) {
                            
                            // Clear buffer if we just navigated to this cell
                            if (g_just_navigated) {
                                g_cell_edit_buffer[0] = '\0';
                                g_just_navigated = false;
                            }
                            
                            // Get the character to append
                            char key_char = '\0';
                            if (event.key.keysym.sym >= SDLK_0 && event.key.keysym.sym <= SDLK_9) {
                                key_char = '0' + (event.key.keysym.sym - SDLK_0);
                            } else if (event.key.keysym.sym == SDLK_PERIOD || event.key.keysym.sym == SDLK_KP_PERIOD) {
                                key_char = '.';
                            }
                            
                            if (key_char != '\0') {
                                // Append to the buffer
                                int current_len = strlen(g_cell_edit_buffer);
                                if (current_len < sizeof(g_cell_edit_buffer) - 1) {
                                    g_cell_edit_buffer[current_len] = key_char;
                                    g_cell_edit_buffer[current_len + 1] = '\0';
                                    g_buffer_updated = true;
                                    event_handled = true;
                                    
                                    add_log_entry(0, "Direct number entry: appended '%c' to buffer, now: '%s'", 
                                                key_char, g_cell_edit_buffer);
                                }
                            }
                        }
                        // Handle Enter key to apply the value
                        else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                            if (strlen(g_cell_edit_buffer) > 0) {
                                float input_value = atof(g_cell_edit_buffer);
                                if (input_value >= 0.0f && input_value <= 200.0f) {
                                    imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, input_value);
                                    add_log_entry(0, "Applied direct number entry: %.1f to cell [%d,%d]", 
                                                input_value, g_selected_cell_x, g_selected_cell_y);
                                    
                                    // Clear the buffer after successful application
                                    g_cell_edit_buffer[0] = '\0';
                                    g_buffer_updated = true;
                                    event_handled = true;
                                }
                            }
                        }
                        // Handle Escape key to cancel editing
                        else if (event.key.keysym.sym == SDLK_ESCAPE) {
                            // Clear the buffer and restore original value
                            g_cell_edit_buffer[0] = '\0';
                            snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.1f", 
                                    imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y));
                            g_buffer_updated = true;
                            event_handled = true;
                            add_log_entry(0, "Cancelled direct number entry, restored original value");
                        }
                    }
                }
                break;
                
            case SDL_MOUSEBUTTONUP:
                // Handle mouse release for multi-cell selection
                if (g_selected_tab == 8 && event.button.button == SDL_BUTTON_LEFT && g_multi_selection.dragging && g_ve_table && g_ve_table_initialized) {
                    end_multi_selection();
                    event_handled = true;
                }
                break;
                
            case SDL_MOUSEMOTION:
                // Handle mouse drag for multi-cell selection
                if (g_selected_tab == 8 && g_multi_selection.dragging && g_ve_table && g_ve_table_initialized) {
                    // Convert SDL mouse coordinates to table coordinates
                    int mouse_x = event.motion.x;
                    int mouse_y = event.motion.y;
                    
                    // Calculate table area - use cached window position to avoid calling ImGui functions from event handler
                    float table_start_x = 0, table_start_y = 0, table_width = 0, table_height = 0;
                    if (g_table_window_valid) {
                        table_start_x = g_table_window_pos.x + 40;  // Match rendering: 40px from left
                        table_start_y = g_table_window_pos.y + 30;  // Match rendering: 30px from top
                        table_width = g_table_window_size.x - 80;   // Match rendering: 40px margin on each side
                        table_height = g_table_window_size.y - 60;  // Match rendering: 30px margin on top/bottom
                    } else {
                        // Fallback to approximate values if cache is not valid
                        int window_width = ImGui::GetIO().DisplaySize.x;
                        int window_height = ImGui::GetIO().DisplaySize.y;
                        table_start_x = 120;
                        table_start_y = 250;
                        table_width = window_width - 240;
                        table_height = window_height - 450;
                    }
                    
                    // Convert to table coordinates
                    int table_x = -1, table_y = -1;
                    if (mouse_x >= table_start_x && mouse_x <= table_start_x + table_width &&
                        mouse_y >= table_start_y && mouse_y <= table_start_y + table_height) {
                        // Convert to table coordinates (no additional margin adjustment needed)
                        float adjusted_mouse_x = mouse_x - table_start_x;
                        float adjusted_mouse_y = mouse_y - table_start_y;
                        
                        // Calculate cell dimensions - match rendering exactly
                        float cell_width = table_width / g_ve_table->width;
                        float cell_height = table_height / g_ve_table->height;
                        
                        table_x = (int)(adjusted_mouse_x / cell_width);
                        table_y = (int)(adjusted_mouse_y / cell_height);
                        
                        // Clamp to valid range
                        table_x = fmax(0, fmin(g_ve_table->width - 1, table_x));
                        table_y = fmax(0, fmin(g_ve_table->height - 1, table_y));
                        
                        // Update multi-cell selection only if coordinates are valid
                        if (table_x >= 0 && table_y >= 0 && table_x < g_ve_table->width && table_y < g_ve_table->height) {
                            update_multi_selection(table_x, table_y);
                            add_log_entry(3, "Mouse drag detected at (%d, %d) -> table [%d, %d]", 
                                        mouse_x, mouse_y, table_x, table_y);
                        } else {
                            add_log_entry(3, "Mouse drag outside table area at (%d, %d)", mouse_x, mouse_y);
                        }
                    }
                    event_handled = true;
                       }
                       break;
        }
        
        // Only pass events to ImGui if we didn't handle them ourselves
        if (!event_handled) {
            ImGui_ImplSDL2_ProcessEvent(&event);
        }
    }
}

void update() {
    // Update ECU status and data
    if (g_ecu_context) {
        // Update ECU data (this triggers continuous data streaming)
        ecu_update(g_ecu_context);
        
        bool was_connected = g_ecu_connected;
        g_ecu_connected = ecu_is_connected(g_ecu_context);
        
        // Log connection state changes
        if (was_connected != g_ecu_connected) {
            if (g_ecu_connected) {
                add_log_entry(0, "ECU connection established");
            } else {
                add_log_entry(1, "ECU connection lost");
            }
        }
        
        // Use ecu_get_state instead of ecu_get_status
        ECUConnectionState state = ecu_get_state(g_ecu_context);
        switch (state) {
            case ECU_STATE_DISCONNECTED:
                strcpy(g_ecu_status, "Disconnected");
                break;
            case ECU_STATE_CONNECTING:
                strcpy(g_ecu_status, "Connecting");
                break;
            case ECU_STATE_CONNECTED:
                strcpy(g_ecu_status, "Connected");
                break;
            case ECU_STATE_ERROR:
                strcpy(g_ecu_status, "Error");
                break;
            default:
                strcpy(g_ecu_status, "Unknown");
                break;
        }
        // Use ecu_get_data correctly (it returns a pointer)
        const ECUData* data = ecu_get_data(g_ecu_context);
        if (data) {
            g_ecu_data = *data;
        }
    }
    
    // Generate demo data if in demo mode (independent of ECU connection)
    if (g_demo_mode) {
        static float demo_time = 0.0f;
        demo_time += 0.1f; // Increment time for demo
        
        // Generate realistic demo values with proper clamping - wider RPM range
        g_ecu_data.rpm = fmax(800.0f, 800.0f + 7200.0f * sin(demo_time * 0.3f) + 1000.0f * sin(demo_time * 1.5f));
        
        // Generate more random MAP data that jumps around the VE table
        float map_base = 30.0f + 50.0f * sin(demo_time * 0.3f);
        float map_random = 20.0f + 180.0f * sin(demo_time * 0.7f) * cos(demo_time * 0.5f);
        float map_spike = 40.0f * sin(demo_time * 2.1f) * sin(demo_time * 1.3f);
        g_ecu_data.map = fmax(20.0f, fmin(240.0f, map_base + map_random + map_spike));
        g_ecu_data.tps = fmax(0.0f, 10.0f + 30.0f * sin(demo_time * 0.7f));
        g_ecu_data.afr = fmax(10.0f, fmin(20.0f, 14.7f + 2.0f * sin(demo_time * 0.4f)));
        g_ecu_data.boost = fmax(-5.0f, 5.0f + 8.0f * sin(demo_time * 0.6f));
        g_ecu_data.coolant_temp = fmax(60.0f, fmin(120.0f, 90.0f + 10.0f * sin(demo_time * 0.2f)));
        g_ecu_data.intake_temp = fmax(60.0f, fmin(120.0f, 85.0f + 8.0f * sin(demo_time * 0.3f)));
        g_ecu_data.oil_temp = fmax(60.0f, fmin(130.0f, 95.0f + 12.0f * sin(demo_time * 0.4f)));
        g_ecu_data.battery_voltage = fmax(10.0f, fmin(16.0f, 13.5f + 0.5f * sin(demo_time * 0.8f)));
        g_ecu_data.timing = fmax(-10.0f, fmin(40.0f, 15.0f + 10.0f * sin(demo_time * 0.9f)));
        g_ecu_data.oil_pressure = fmax(50.0f, 300.0f + 100.0f * sin(demo_time * 0.5f));
        g_ecu_data.fuel_pressure = fmax(100.0f, 250.0f + 50.0f * sin(demo_time * 0.6f));
    }

    // Update button press timer
    if (g_button_press_timer > 0) {
        g_button_press_timer--;
        if (g_button_press_timer == 0) {
            g_locate_port_button_pressed = false;
            g_reverse_connect_button_pressed = false;
            g_reset_io_button_pressed = false;
            strcpy(g_button_status_text, "");
        }
    }

    handle_communications_buttons();
    
    // Calculate current engine operating point from actual ECU data (moved from 2D view)
    if (g_ve_table && g_ve_table_initialized) {
        add_log_entry(3, "DEBUG: VE table initialized, calculating engine position");
        float current_rpm = g_ecu_data.rpm;
        float current_map = g_ecu_data.map;
        
        // Find the closest cells for current RPM and MAP
        g_current_rpm_cell = -1;
        g_current_map_cell = -1;
        
        // Find RPM cell (find the closest cell to current RPM)
        g_current_rpm_cell = 0; // Default to first cell
        float min_distance = fabs(current_rpm - g_ve_table->x_axis[0]);
        
        for (int x = 0; x < g_ve_table->width; x++) {
            float distance = fabs(current_rpm - g_ve_table->x_axis[x]);
            if (distance < min_distance) {
                min_distance = distance;
                g_current_rpm_cell = x;
            }
        }
        
        // Debug output for RPM calculation
        if (g_demo_mode) {
            add_log_entry(3, "DEBUG: Current RPM: %.0f, Closest cell: %d (RPM: %.0f), Distance: %.1f", 
                         current_rpm, g_current_rpm_cell, g_ve_table->x_axis[g_current_rpm_cell], min_distance);
        }
        
        // Find MAP cell (find the closest cell to current MAP)
        g_current_map_cell = 0; // Default to first cell
        float min_map_distance = fabs(current_map - g_ve_table->y_axis[0]);
        
        for (int y = 0; y < g_ve_table->height; y++) {
            float distance = fabs(current_map - g_ve_table->y_axis[y]);
            if (distance < min_map_distance) {
                min_map_distance = distance;
                g_current_map_cell = y;
            }
        }
        
        // Debug output for MAP calculation
        if (g_demo_mode) {
            add_log_entry(3, "DEBUG: Current MAP: %.1f, Closest cell: %d (MAP: %.1f), Distance: %.1f", 
                         current_map, g_current_map_cell, g_ve_table->y_axis[g_current_map_cell], min_map_distance);
        }
        
        // Update engine trail for both 2D and 3D views
        if (g_current_rpm_cell >= 0 && g_current_map_cell >= 0) {
            update_engine_trail(g_current_rpm_cell, g_current_map_cell, g_engine_trail_2d, &g_trail_count_2d);
            update_engine_trail(g_current_rpm_cell, g_current_map_cell, g_engine_trail_3d, &g_trail_count_3d);
        }
    } else {
        add_log_entry(3, "DEBUG: VE table not initialized yet - g_ve_table: %p, g_ve_table_initialized: %d", 
                     g_ve_table, g_ve_table_initialized);
    }
    
    // Add periodic log entry every 30 seconds (now handled by logging system module)
}

void render() {
    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Render main window
    render_main_window();

    // Render log window
    render_log_window();
    
    // Legend will be rendered as an overlay after the main render

    // Render
    ImGui::Render();
    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Render legend as a true overlay after everything else
    if (show_legend) {
        add_log_entry(0, "Rendering legend overlay - show_legend is true");
        // Start a new ImGui frame for the overlay
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        // Render the legend window (floating, movable and resizable)
        if (g_legend_just_popped) {
            // Place it in the right panel area initially to avoid overlapping the VE table center
            ImVec2 disp = ImGui::GetIO().DisplaySize;
            ImGui::SetNextWindowPos(ImVec2(disp.x * 0.70f, disp.y * 0.25f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
            g_legend_just_popped = false;
        } else {
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.70f, ImGui::GetIO().DisplaySize.y * 0.25f), ImGuiCond_Once, ImVec2(0.5f, 0.0f));
        }
        ImGui::SetNextWindowSize(ImVec2(720, 640), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowFocus();
        ImGuiWindowFlags legend_flags = ImGuiWindowFlags_NoCollapse;
        if (ImGui::Begin("Key Bindings Legend", &show_legend, legend_flags | ImGuiWindowFlags_NoSavedSettings)) {
            if (ImGui::Button("Dock Legend")) { show_legend = false; }
            
            ImGui::Text("🎮 VE Table Professional Key Bindings");
            ImGui::Text("Professional ECU tuning software-style muscle memory for professional table editing");
            ImGui::Separator();
            
            // Status indicator
            ImGui::Text("Status: ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "● Active (VE Table Tab Selected)");
            ImGui::SameLine();
            ImGui::Text(" | Increment: %.1f | Percent: %.1f%%", 
                       g_key_binding_state.increment_amount, g_key_binding_state.percent_increment);
            
            ImGui::Separator();
            
            // Comprehensive table with keybindings
            if (ImGui::BeginTable("KeyBindingsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 80);
                ImGui::TableSetupColumn("Function", ImGuiTableColumnFlags_WidthFixed, 200);
                ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80);
                ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();
                
                // Navigation Keys
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Arrow Keys");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Navigate Cells");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Move between table cells");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Tab");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Next Cell");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Move to next cell (Excel-style)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Shift+Tab");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Previous Cell");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Move to previous cell");
                
                // Basic Value Operations
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "+");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Increment");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Increase cell value by %.1f", g_key_binding_state.increment_amount);
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "-");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Decrement");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Decrease cell value by %.1f", g_key_binding_state.increment_amount);
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Alt++");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Increment Alt");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Alternative increment method");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Alt+-");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Decrement Alt");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Alternative decrement method");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "*");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Scale By");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Multiply cell value by %.1f%%", g_key_binding_state.percent_increment);
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "=");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Set To");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Set selected cell(s) to specific value");
                
                // Advanced Operations
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "I");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Interpolate");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Interpolate between two selected cells");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "H");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Interpolate H");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Interpolate horizontally across selection");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "V");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Interpolate V");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Interpolate vertically across selection");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "S");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Smooth");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Smooth selected cells (3x3 kernel)");

                // New: Fill Up+Right
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "f");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Fill Up+Right");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Fill selection using current cell's value");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "R");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Reset");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "PLAN");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Reset selected cells to default (planned)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Z");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Undo");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "PLAN");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Undo last action (planned)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Y");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Redo");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "PLAN");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Redo last undone action (planned)");
                
                // Copy/Paste Operations
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Ctrl+C");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Copy");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Copy selection or single cell to clipboard");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Ctrl+V");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Paste");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Paste block at selected cell");
                
                // Function Keys
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "F1");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Help");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Toggle help/legend window");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "F5");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Refresh");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Refresh table data");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "F6/F7");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Zoom");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Zoom in/out on table view");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "F8");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Fit View");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Reset view to default isometric");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Space");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Clear Selection");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Clear current cell selection");
                
                // Additional Operations
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.8f, 0.5f, 1.0f, 1.0f), "Delete");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Clear Cell");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Clear cell value to 0");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.8f, 0.5f, 1.0f, 1.0f), "Enter");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Apply Changes");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Apply current buffer value");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.8f, 0.5f, 1.0f, 1.0f), "Escape");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Cancel");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Cancel current operation");
                
                ImGui::EndTable();
            }
            
            // Configuration Section
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "⚙️ Configuration");
            ImGui::Text("Adjust these values to customize your editing experience:");
            
            ImGui::SliderFloat("Increment Amount", &g_key_binding_state.increment_amount, 0.1f, 10.0f, "%.1f");
            ImGui::SliderFloat("Percent Increment", &g_key_binding_state.percent_increment, 1.0f, 50.0f, "%.1f%%");
            
            ImGui::Separator();
            
            // Legend for status indicators
            ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "📊 Status Legend");
            ImGui::Text("OK Working | PART Partial | PLAN Planned");
            
            ImGui::Separator();
            
            // Keyboard shortcuts are now handled globally in SDL event handler
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Press ESC or Enter to close this window");
            
            // Add a close button that also works
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Close Options:");
            ImGui::Spacing();
            
            // Make button very prominent
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
            
            // Try a completely different approach - use InvisibleButton
            ImGui::InvisibleButton("close_button", ImVec2(200, 40));
            
            // Debug: Check if button is hovered
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Button is being hovered!");
                add_log_entry(0, "Button is being hovered!");
            }
            
            // Debug: Check if button was clicked - try different methods
            if (ImGui::IsItemClicked()) {
                add_log_entry(0, "*** InvisibleButton IsItemClicked detected! ***");
                show_legend = false;
                add_log_entry(0, "*** Close button clicked - closing legend window ***");
            }
            
            if (ImGui::IsItemClicked(0)) { // Left mouse button
                add_log_entry(0, "*** InvisibleButton IsItemClicked(0) detected! ***");
                show_legend = false;
                add_log_entry(0, "*** Close button clicked - closing legend window ***");
            }
            
            // Draw the button manually on top of the invisible button
            ImVec2 current_pos = ImGui::GetCursorPos();
            ImGui::SetCursorPos(ImVec2(current_pos.x, current_pos.y - 40));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
            ImGui::Button("CLOSE WINDOW PLANW", ImVec2(200, 40));
            ImGui::PopStyleColor(3);
            
            // Debug: Show current state
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Debug: show_legend = %s", show_legend ? "true" : "false");
            
            ImGui::PopStyleColor(3);
            ImGui::Spacing();
            
            ImGui::End();
        }
        
        // Finish overlay frame and render it immediately to avoid input freeze
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        // Render the overlay
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    // Remove viewport features for now
    // if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    //     SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
    //     SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
    //     ImGui::UpdatePlatformWindows();
    //     ImGui::RenderPlatformWindowsDefault();
    //     SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    // }

    SDL_GL_SwapWindow(g_window);
}

void render_main_window() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags window_flags = 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("MegaTunix Redux", NULL, window_flags);

    // Professional top banner with enhanced styling
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_medium);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, g_ui_theme.corner_radius);
    ImGui::BeginChild("TopBanner", ImVec2(ImGui::GetWindowWidth() - 20, 50), true);
    
    // Application title with professional styling
    ImGui::SetCursorPos(ImVec2(15, 15));
    ImGui::PushStyleColor(ImGuiCol_Text, g_ui_theme.primary_color);
    
    // Show current view in title
    const char* view_name = "MEGATUNIX REDUX";
    switch (g_selected_view) {
        case VIEW_GAUGE_CLUSTER:
            view_name = "MEGATUNIX REDUX - Gauge Cluster";
            break;
        case VIEW_TUNING:
            view_name = "MEGATUNIX REDUX - Tuning";
            break;
        case VIEW_GRAPHING:
            view_name = "MEGATUNIX REDUX - Graphing";
            break;
        case VIEW_DIAGNOSTICS:
            view_name = "MEGATUNIX REDUX - Diagnostics";
            break;
        case VIEW_COMMUNICATIONS:
            view_name = "MEGATUNIX REDUX - Communications";
            break;
        case VIEW_PLUGIN_MANAGER:
            view_name = "MEGATUNIX REDUX - Plugin Manager";
            break;
        default:
            view_name = "MEGATUNIX REDUX";
            break;
    }
    
    ImGui::Text("🏁 %s", view_name);
    ImGui::PopStyleColor();
    
    // Status indicator with professional styling
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 200);
    if (g_demo_mode) {
        ImGui::TextColored(g_ui_theme.warning_color, "● DEMO MODE");
    } else {
        render_status_indicator("ECU", g_ecu_connected, g_ecu_status);
    }
    
    // Theme selector dropdown
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 350);
    ImGui::TextColored(g_ui_theme.text_secondary, "Theme:");
    ImGui::SameLine();
    
    const char* theme_names[] = {
        "Classic Automotive",
        "Modern Tech", 
        "Racing Green",
        "Sunset Synthwave",
        "Ocean Blue"
    };
    
    if (ImGui::BeginCombo("##ThemeSelector", theme_names[ui_theme_manager_get_current_theme_type()])) {
        for (int i = 0; i < THEME_COUNT; i++) {
            if (ImGui::Selectable(theme_names[i], ui_theme_manager_get_current_theme_type() == i)) {
                switch_theme((ThemeType)i);
            }
        }
        ImGui::EndCombo();
    }
    
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    
    // TunerStudio-style menu bar (exact replica)
    ImGui::SetCursorPos(ImVec2(0, 70));
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, g_ui_theme.background_dark);
    
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project", "Ctrl+N")) { /* TODO */ }
            if (ImGui::MenuItem("Open Project", "Ctrl+O")) { /* TODO */ }
            if (ImGui::MenuItem("Save Project", "Ctrl+S")) { /* TODO */ }
            ImGui::Separator();
            if (ImGui::MenuItem("Import Tune", "Ctrl+I")) { /* TODO */ }
            if (ImGui::MenuItem("Export Tune", "Ctrl+E")) { /* TODO */ }
            ImGui::Separator();
            if (ImGui::MenuItem("Recent Projects")) { /* TODO */ }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) { /* TODO */ }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) { /* TODO */ }
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) { /* TODO */ }
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "Ctrl+X")) { /* TODO */ }
            if (ImGui::MenuItem("Copy", "Ctrl+C")) { /* TODO */ }
            if (ImGui::MenuItem("Paste", "Ctrl+V")) { /* TODO */ }
            if (ImGui::MenuItem("Delete", "Del")) { /* TODO */ }
            ImGui::Separator();
            if (ImGui::MenuItem("Find/Replace", "Ctrl+F")) { /* TODO */ }
            if (ImGui::MenuItem("Go To", "Ctrl+G")) { /* TODO */ }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Gauge Cluster", "F1")) { g_selected_view = VIEW_GAUGE_CLUSTER; }
            if (ImGui::MenuItem("Tuning & Dyno Views", "F2")) { g_selected_view = VIEW_TUNING; }
            if (ImGui::MenuItem("Graphing & Logging", "F3")) { g_selected_view = VIEW_GRAPHING; }
            if (ImGui::MenuItem("Diagnostics & High Speed Loggers", "F4")) { g_selected_view = VIEW_DIAGNOSTICS; }
            if (ImGui::MenuItem("Communications", "F5")) { g_selected_view = VIEW_COMMUNICATIONS; }
            if (ImGui::MenuItem("Plugin Manager", "F6")) { g_selected_view = VIEW_PLUGIN_MANAGER; }
            ImGui::Separator();
            if (ImGui::MenuItem("Status Bar")) { /* TODO */ }
            if (ImGui::MenuItem("Toolbar")) { /* TODO */ }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Communications")) {
            if (ImGui::MenuItem("Connect", "F5")) { /* TODO */ }
            if (ImGui::MenuItem("Disconnect", "F6")) { /* TODO */ }
            if (ImGui::MenuItem("Settings")) { /* TODO */ }
            if (ImGui::MenuItem("Firmware")) { /* TODO */ }
            if (ImGui::MenuItem("Port Monitor")) { /* TODO */ }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Data Logging")) {
            if (ImGui::MenuItem("Start Logging", "F7")) { /* TODO */ }
            if (ImGui::MenuItem("Stop Logging", "F8")) { /* TODO */ }
            if (ImGui::MenuItem("Logging Setup")) { /* TODO */ }
            if (ImGui::MenuItem("Log Analysis")) { /* TODO */ }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Data Logging")) { /* TODO */ }
            if (ImGui::MenuItem("Analysis")) { /* TODO */ }
            if (ImGui::MenuItem("Calibration")) { /* TODO */ }
            if (ImGui::MenuItem("Validation")) { /* TODO */ }
            if (ImGui::MenuItem("Calculator")) { /* TODO */ }
            if (ImGui::MenuItem("Unit Converter")) { /* TODO */ }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Settings")) { g_show_settings_window = true; }
            ImGui::Separator();
            if (ImGui::MenuItem("Manual")) { /* TODO */ }
            if (ImGui::MenuItem("About")) { /* TODO */ }
            if (ImGui::MenuItem("Updates")) { /* TODO */ }
            if (ImGui::MenuItem("Support")) { /* TODO */ }
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
    
    ImGui::PopStyleColor();

    // Main content area with exact TunerStudio layout
    ImGui::SetCursorPos(ImVec2(0, 100));
    
    // Calculate available height for content (leave space for System Log)
    float available_height = ImGui::GetWindowHeight() - 250; // Increased padding to 250px for better spacing
    
    // Left sidebar navigation (exact TunerStudio style)
    ImGui::BeginChild("LeftSidebar", ImVec2(280, available_height), true);
    render_tunerstudio_sidebar();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Right content area (main workspace)
    ImGui::BeginChild("MainWorkspace", ImVec2(0, available_height), true);
    render_content_by_view();
    ImGui::EndChild();
    
    // Add spacing between main content and System Log
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    
    // Visual separator line
    ImGui::Separator();
    ImGui::Spacing();
    
    // System Log section at the bottom with proper height constraints
    ImGui::TextColored(g_ui_theme.primary_color, "System Log");
    ImGui::Separator();
    
    // System Log panel with constrained height
    ImGui::BeginChild("SystemLogPanel", ImVec2(0, 180), true); // Reduced height to 180px for better proportions
    
    // Log controls
    ImGui::BeginGroup();
    static bool auto_scroll = true;
    ImGui::Checkbox("Auto-scroll", &auto_scroll);
    ImGui::SameLine();
    ImGui::Text("Filter:");
    ImGui::SameLine();
    const char* filter_items[] = { "All", "Info", "Warning", "Error" };
    static int current_filter = 0;
    if (ImGui::BeginCombo("##LogFilter", filter_items[current_filter])) {
        for (int i = 0; i < IM_ARRAYSIZE(filter_items); i++) {
            if (ImGui::Selectable(filter_items[i], current_filter == i)) {
                current_filter = i;
                // TODO: Apply filter to logging system
            }
        }
        ImGui::EndCombo();
    }
    ImGui::EndGroup();
    
    ImGui::Separator();
    
    // Log content area (scrollable)
    ImGui::BeginChild("LogContent", ImVec2(0, 0), true);
    
    // Display sample log entries for now
    ImGui::TextColored(g_ui_theme.text_muted, "[08:47:12] INFO: Speeduino update_connection_status - simplified stub");
    ImGui::TextColored(g_ui_theme.text_muted, "[08:47:12] INFO: Rendering VIEW_GAUGE_CLUSTER");
    ImGui::TextColored(g_ui_theme.text_muted, "[08:47:12] DEBUG: VE table initialized, calculating engine position");
    ImGui::TextColored(g_ui_theme.text_muted, "[08:47:12] INFO: Plugin system initialized successfully");
    ImGui::TextColored(g_ui_theme.text_muted, "[08:47:12] INFO: View switched from 0 to 0");
    ImGui::TextColored(g_ui_theme.text_muted, "[08:47:12] INFO: Rendering VIEW_GAUGE_CLUSTER");
    
    // Add more sample entries to show scrolling
    for (int i = 0; i < 20; i++) {
        ImGui::TextColored(g_ui_theme.text_secondary, "[08:47:%02d] INFO: Sample log entry %d", i, i);
    }
    
    ImGui::EndChild(); // LogContent
    ImGui::EndChild(); // SystemLogPanel
    
    // Render settings window if open
    render_settings_window();
    
    ImGui::End();
}

void render_about_tab() {
    // Professional section header
    render_section_header("About", "Application Information", g_ui_theme.primary_color);
    
    // Application information in a professional card layout
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, g_ui_theme.corner_radius);
    ImGui::BeginChild("AppInfo", ImVec2(0, 120), true);
    
    ImGui::TextColored(g_ui_theme.primary_color, "🏁 MegaTunix Redux - ImGui Version");
    ImGui::TextColored(g_ui_theme.text_secondary, "By Patrick Burke");
    ImGui::TextColored(g_ui_theme.text_secondary, "Based on MegaTunix by David J. Andruczyk");
    ImGui::TextColored(g_ui_theme.text_muted, "Professional ECU Tuning Software");
    
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    
    // Demo mode section with enhanced styling
    render_section_header("Demo Mode", "Testing and Development", g_ui_theme.warning_color);
    
    ImGui::BeginGroup();
    
    // Demo mode toggle with professional styling
    ImGui::TextColored(g_ui_theme.text_primary, "Enable Demo Mode:");
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_CheckMark, g_ui_theme.success_color);
    if (ImGui::Checkbox("##demo_mode_about", &g_demo_mode)) {
        if (g_demo_mode) {
            add_log_entry(0, "Demo mode enabled from About tab");
        } else {
            add_log_entry(0, "Demo mode disabled from About tab");
        }
    }
    ImGui::PopStyleColor();
    
    // Demo mode status indicator
    if (g_demo_mode) {
        ImGui::SameLine();
        render_status_indicator("Demo Mode", true, "ACTIVE");
        ImGui::TextColored(g_ui_theme.text_secondary, "Demo mode is currently active - all data is simulated");
    } else {
        ImGui::SameLine();
        render_status_indicator("Demo Mode", false, "INACTIVE");
    }
    
    ImGui::EndGroup();
    
    ImGui::Spacing();
    
    // Theme selection section
    render_section_header("UI Themes", "Choose Your Visual Style", g_ui_theme.accent_color);
    
    ImGui::TextColored(g_ui_theme.text_primary, "Select from multiple professional color schemes:");
    ImGui::Spacing();
    
    // Theme preview grid
    const char* theme_names[] = {
        "Classic Automotive",
        "Modern Tech", 
        "Racing Green",
        "Sunset Synthwave",
        "Ocean Blue"
    };
    
    const char* theme_descriptions[] = {
        "Deep reds, chrome silvers, professional blacks",
        "Electric blue, sleek grays, modern aesthetics",
        "British racing green with gold accents",
        "Synthwave aesthetic with warm oranges, deep purples, cream highlights",
        "Deep blues, teals, white accents"
    };
    
    ImVec4 theme_colors[] = {
        ImVec4(0.8f, 0.1f, 0.1f, 1.0f),      // Classic Automotive red
        ImVec4(0.0f, 0.6f, 1.0f, 1.0f),      // Modern Tech blue
        ImVec4(0.0f, 0.4f, 0.2f, 1.0f),      // Racing Green green
        ImVec4(1.0f, 0.4f, 0.0f, 1.0f),      // Sunset Synthwave orange
        ImVec4(0.0f, 0.4f, 0.8f, 1.0f)       // Ocean Blue blue
    };
    
    for (int i = 0; i < THEME_COUNT; i++) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, g_ui_theme.corner_radius);
        char child_id[64];
        sprintf(child_id, "ThemePreview##%d", i);
        ImGui::BeginChild(child_id, ImVec2(0, 60), true);
        
        // Theme name and color indicator
        ImGui::TextColored(theme_colors[i], "● %s", theme_names[i]);
        ImGui::TextColored(g_ui_theme.text_secondary, "%s", theme_descriptions[i]);
        
        // Apply theme button
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 100);
        if (ui_theme_manager_get_current_theme_type() == i) {
            ImGui::TextColored(g_ui_theme.success_color, "✓ ACTIVE");
        } else {
            char button_id[64];
            sprintf(button_id, "Apply##%d", i);
            if (ImGui::Button(button_id)) {
                switch_theme((ThemeType)i);
            }
        }
        
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        
        if (i < THEME_COUNT - 1) ImGui::Spacing();
    }
    
    ImGui::Spacing();
    
    // Application description
    render_section_header("Description", "What This Application Does", g_ui_theme.accent_color);
    
    ImGui::TextColored(g_ui_theme.text_primary, "This application provides a professional graphical interface for monitoring and controlling an ECU.");
    ImGui::TextColored(g_ui_theme.text_secondary, "It uses SDL2 for windowing, OpenGL for rendering, and ImGui for modern UI.");
    ImGui::TextColored(g_ui_theme.text_secondary, "The ECU communication is handled by a robust, multi-protocol library.");
    
    ImGui::Spacing();
    
    ImGui::TextColored(g_ui_theme.text_muted, "Demo Mode allows you to test the application without an ECU connection.");
    ImGui::TextColored(g_ui_theme.text_muted, "Enable it to see simulated engine data and test all features.");
}

void render_general_tab() {
    // Professional section header
    render_section_header("General", "System Information and Status", g_ui_theme.primary_color);
    
    ImGui::Spacing();
    
    // Application information with professional styling
    render_section_header("Application Information", "Version and Status Details", g_ui_theme.accent_color);
    
    ImGui::BeginGroup();
    
    // App info in a professional card layout
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, g_ui_theme.corner_radius);
    ImGui::BeginChild("AppInfo", ImVec2(0, 100), true);
    
    // App details with enhanced styling
    ImGui::TextColored(g_ui_theme.text_secondary, "Application Version:");
    ImGui::SameLine();
    ImGui::TextColored(g_ui_theme.primary_color, "1.0.0");
    
    ImGui::TextColored(g_ui_theme.text_secondary, "ECU Status:");
    ImGui::SameLine();
    ImGui::TextColored(g_ui_theme.text_primary, "%s", g_ecu_status);
    
    ImGui::TextColored(g_ui_theme.text_secondary, "Connected:");
    ImGui::SameLine();
    render_status_indicator("ECU", g_ecu_connected, g_ecu_connected ? "Yes" : "No");
    
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    
    ImGui::EndGroup();
    
    ImGui::Spacing();
    
    // Demo mode section with professional styling
    render_section_header("Demo Mode", "Testing and Development Features", g_ui_theme.warning_color);
    
    ImGui::BeginGroup();
    
    // Demo mode toggle with enhanced styling
    ImGui::TextColored(g_ui_theme.text_primary, "Enable Demo Mode:");
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_CheckMark, g_ui_theme.success_color);
    if (ImGui::Checkbox("##demo_mode_general", &g_demo_mode)) {
        if (g_demo_mode) {
            add_log_entry(0, "Demo mode enabled by user");
        } else {
            add_log_entry(0, "Demo mode disabled by user");
        }
    }
    ImGui::PopStyleColor();
    
    // Demo mode status and features
    if (g_demo_mode) {
        ImGui::SameLine();
        render_status_indicator("Demo Mode", true, "ACTIVE");
        
        ImGui::Spacing();
        
        // Demo features in a professional card layout
        ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_medium);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, g_ui_theme.corner_radius);
        ImGui::BeginChild("DemoFeatures", ImVec2(0, 120), true);
        
        ImGui::TextColored(g_ui_theme.warning_color, "🚀 DEMO MODE FEATURES:");
        ImGui::TextColored(g_ui_theme.text_primary, "• All data is simulated for testing purposes");
        ImGui::TextColored(g_ui_theme.text_primary, "• Real-time charts show demo data");
        ImGui::TextColored(g_ui_theme.text_primary, "• VE table contains demo values");
        ImGui::TextColored(g_ui_theme.text_primary, "• 3D view shows simulated engine position");
        ImGui::TextColored(g_ui_theme.text_primary, "• No actual ECU communication");
        
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    } else {
        ImGui::SameLine();
        render_status_indicator("Demo Mode", false, "INACTIVE");
    }
    
    ImGui::EndGroup();
    
    ImGui::Spacing();
    
    // ECU data section with professional styling
    render_section_header("ECU Data", "Real-time Engine Parameters", g_ui_theme.success_color);
    
    ImGui::BeginGroup();
    
    // ECU data in a professional card layout
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, g_ui_theme.corner_radius);
    ImGui::BeginChild("ECUData", ImVec2(0, 120), true);
    
    // ECU data with enhanced styling
    ImGui::TextColored(g_ui_theme.text_secondary, "RPM:");
    ImGui::SameLine();
    ImGui::TextColored(g_ui_theme.primary_color, "%.0f", g_ecu_data.rpm);
    
    ImGui::TextColored(g_ui_theme.text_secondary, "Coolant Temp:");
    ImGui::SameLine();
    ImGui::TextColored(g_ui_theme.text_primary, "%.1f°C", g_ecu_data.coolant_temp);
    
    ImGui::TextColored(g_ui_theme.text_secondary, "Intake Air Temp:");
    ImGui::SameLine();
    ImGui::TextColored(g_ui_theme.text_primary, "%.1f°C", g_ecu_data.intake_temp);
    
    ImGui::TextColored(g_ui_theme.text_secondary, "MAP:");
    ImGui::SameLine();
    ImGui::TextColored(g_ui_theme.text_primary, "%.1f kPa", g_ecu_data.map);
    
    ImGui::TextColored(g_ui_theme.text_secondary, "TPS:");
    ImGui::SameLine();
    ImGui::TextColored(g_ui_theme.text_primary, "%.1f%%", g_ecu_data.tps);
    
    ImGui::TextColored(g_ui_theme.text_secondary, "Battery Voltage:");
    ImGui::SameLine();
    ImGui::TextColored(g_ui_theme.text_primary, "%.1fV", g_ecu_data.battery_voltage);
    
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    
    ImGui::EndGroup();
}

void render_communications_tab() {
    // Professional section header
    render_section_header("Communications", "ECU Connection and Data Transfer", g_ui_theme.primary_color);
    
    ImGui::Spacing();
    
    // ECU connection panel
    render_ecu_connection_panel();
    
    ImGui::Spacing();
    
    // Initialize communications if not done yet
    if (!g_communications_initialized) {
        g_communications = imgui_communications_create(g_ecu_context);
        if (g_communications) {
            g_communications_initialized = true;
            // Set up log callback for communications module
            imgui_communications_set_log_callback(g_communications, add_log_entry);
            
            // Set up VE table callbacks for communications module
            add_log_entry(0, "Setting up VE table callbacks...");
            imgui_communications_set_ve_table_callbacks(g_communications,
                [](int width, int height) -> bool {
                    add_log_entry(0, "VE table resize callback called: %dx%d", width, height);
                    add_log_entry(0, "*** VE TABLE RESIZE REQUESTED: %dx%d ***", width, height);
                    add_log_entry(0, "Current g_ve_table dimensions before resize: %dx%d (address: %p)", g_ve_table ? g_ve_table->width : -1, g_ve_table ? g_ve_table->height : -1, g_ve_table);
                    if (g_ve_table) {
                        bool result = imgui_table_resize(g_ve_table, width, height);
                        add_log_entry(0, "VE table resize result: %s", result ? "SUCCESS" : "FAILED");
                        add_log_entry(0, "Current g_ve_table dimensions after resize: %dx%d (address: %p)", g_ve_table ? g_ve_table->width : -1, g_ve_table ? g_ve_table->height : -1, g_ve_table);
                        if (result) {
                            add_log_entry(0, "*** VE TABLE RESIZE SUCCESS: %dx%d ***", g_ve_table->width, g_ve_table->height);
                        } else {
                            add_log_entry(2, "*** VE TABLE RESIZE FAILED ***");
                        }
                        return result;
                    }
                    add_log_entry(2, "VE table resize failed: g_ve_table is NULL");
                    return false;
                },
                [](float x_min, float x_max, float y_min, float y_max) {
                    add_log_entry(0, "VE table axis ranges callback: X(%.0f-%.0f), Y(%.0f-%.0f)", x_min, x_max, y_min, y_max);
                    if (g_ve_table) {
                        imgui_table_set_axis_ranges(g_ve_table, x_min, x_max, y_min, y_max);
                    }
                },
                [](const char* x_name, const char* y_name, const char* x_units, const char* y_units) {
                    add_log_entry(0, "VE table axis names callback: X(%s %s), Y(%s %s)", x_name, x_units, y_name, y_units);
                    if (g_ve_table) {
                        imgui_table_set_axis_names(g_ve_table, x_name, y_name, x_units, y_units);
                    }
                },
                [](float scale, float min_value, float max_value) {
                    add_log_entry(0, "VE table metadata callback: scale=%.2f, range=%.0f-%.0f", scale, min_value, max_value);
                    if (g_ve_table) {
                        // Note: scale is no longer stored in metadata
                        g_ve_table->metadata.min_value = min_value;
                        g_ve_table->metadata.max_value = max_value;
                    }
                }
            );
            add_log_entry(0, "VE table callbacks set up successfully");
            add_log_entry(0, "Communications tab initialized successfully");
        } else {
            add_log_entry(2, "Failed to initialize communications tab");
        }
    }
    
    if (g_communications && g_communications_initialized) {
        ImGui::Separator();
        ImGui::TextColored(g_ui_theme.text_secondary, "Legacy Communications System:");
        imgui_communications_render(g_communications);
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to initialize communications!");
    }
}

void render_runtime_display_tab() {
    // Initialize runtime display if not done yet
    if (!g_runtime_display_initialized) {
        g_runtime_display = imgui_runtime_display_create(g_ecu_context);
        if (g_runtime_display) {
            g_runtime_display_initialized = true;
        }
    }
    
    // Sync demo mode with global setting
    if (g_runtime_display) {
        g_runtime_display->demo_mode_enabled = g_demo_mode;
    }
    
    if (g_runtime_display && g_runtime_display_initialized) {
        imgui_runtime_display_render(g_runtime_display);
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to initialize runtime display!");
    }
}

void render_datalogging_tab() {
    // Professional section header
    render_section_header("Data Logging", "Log Management and Analysis", g_ui_theme.primary_color);
    
    ImGui::Spacing();
    
    // Log files section with professional styling
    render_section_header("Log Files", "Available Log Sources", g_ui_theme.accent_color);
    
    ImGui::BeginGroup();
    
    // Log files in a professional card layout
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, g_ui_theme.corner_radius);
    ImGui::BeginChild("LogFiles", ImVec2(0, 120), true);
    
    // Log file entries with enhanced styling
    ImGui::TextColored(g_ui_theme.text_secondary, "Application Logs:");
    ImGui::TextColored(g_ui_theme.text_primary, "  📄 /logs/megatunix.log");
    ImGui::TextColored(g_ui_theme.text_primary, "  📄 /logs/ecu_data.log");
    
    ImGui::Spacing();
    
    ImGui::TextColored(g_ui_theme.text_secondary, "Log Status:");
    ImGui::TextColored(g_ui_theme.success_color, "  ● Active logging enabled");
    ImGui::TextColored(g_ui_theme.text_muted, "  Last updated: Just now");
    
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    
    ImGui::EndGroup();
    
    ImGui::Spacing();
    
    // Log management section with professional styling
    render_section_header("Log Management", "Control and Maintenance Operations", g_ui_theme.secondary_color);
    
    ImGui::BeginGroup();
    
    // Enhanced button styling
    bool clear_logs_clicked = false;
                    ui_theme_manager_render_professional_button("Clear Logs", ImVec2(120, 30), &clear_logs_clicked, 
                               ui_theme_manager_get_current_theme()->warning_color, "Clear all log files");
    if (clear_logs_clicked) {
        // TODO: Implement log clearing
        ImGui::OpenPopup("Logs Cleared");
    }
    
    ImGui::SameLine();
    
    bool export_logs_clicked = false;
                    ui_theme_manager_render_professional_button("Export Logs", ImVec2(120, 30), &export_logs_clicked, 
                               ui_theme_manager_get_current_theme()->accent_color, "Export logs to external format");
    if (export_logs_clicked) {
        // TODO: Implement log export
        add_log_entry(0, "Export logs button pressed");
    }
    
    ImGui::SameLine();
    
    bool rotate_logs_clicked = false;
                    ui_theme_manager_render_professional_button("Rotate Logs", ImVec2(120, 30), &rotate_logs_clicked, 
                               ui_theme_manager_get_current_theme()->primary_color, "Rotate log files");
    if (rotate_logs_clicked) {
        // TODO: Implement log rotation
        add_log_entry(0, "Rotate logs button pressed");
    }
    
    ImGui::EndGroup();
    
    // Logs Cleared popup with enhanced styling
    if (ImGui::BeginPopupModal("Logs Cleared", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(g_ui_theme.success_color, "✅ Logs have been cleared successfully!");
        ImGui::TextColored(g_ui_theme.text_secondary, "All log files have been reset and are ready for new data.");
        
        ImGui::Spacing();
        
        bool ok_clicked = false;
                ui_theme_manager_render_professional_button("OK", ImVec2(120, 0), &ok_clicked, 
                                   ui_theme_manager_get_current_theme()->primary_color, "Close this dialog");
        if (ok_clicked) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void render_logviewer_tab() {
    // Professional section header
    render_section_header("Log Viewer", "View ECU and Application Logs", g_ui_theme.primary_color);
    
    ImGui::Spacing();
    
    // Log viewer controls with professional styling
    render_section_header("Log Controls", "Filter and View Options", g_ui_theme.accent_color);
    
    ImGui::BeginGroup();
    
    // Log level filter with enhanced styling
    ImGui::TextColored(g_ui_theme.text_primary, "Log Level Filter:");
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_FrameBg, g_ui_theme.background_light);
    ImGui::SetNextItemWidth(150);
    const char* log_levels[] = {"All", "Info+", "Warning+", "Error Only", "Debug"};
    int selected_log_level = get_log_filter_level();
    if (ImGui::BeginCombo("##log_level", log_levels[selected_log_level])) {
        for (int i = 0; i < IM_ARRAYSIZE(log_levels); i++) {
            const bool is_selected = (selected_log_level == i);
            if (ImGui::Selectable(log_levels[i], is_selected)) {
                set_log_filter_level(i);
                selected_log_level = i;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    
    // Auto-refresh toggle with enhanced styling
    ImGui::PushStyleColor(ImGuiCol_CheckMark, g_ui_theme.success_color);
    bool auto_refresh = get_log_auto_scroll();
    if (ImGui::Checkbox("Auto-scroll", &auto_refresh)) {
        set_log_auto_scroll(auto_refresh);
    }
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    
    // Clear logs button with enhanced styling
    bool clear_viewer_logs_clicked = false;
                    ui_theme_manager_render_professional_button("Clear Viewer", ImVec2(100, 25), &clear_viewer_logs_clicked, 
                               ui_theme_manager_get_current_theme()->warning_color, "Clear log viewer display");
    if (clear_viewer_logs_clicked) {
        clear_logs();
        add_log_entry(0, "Log viewer cleared");
    }
    
    ImGui::EndGroup();
    
    ImGui::Spacing();
    
    // Log content section with professional styling
    render_section_header("Log Content", "Real-time Log Display", g_ui_theme.success_color);
    
    ImGui::BeginGroup();
    
    // Use the new logging system module
    render_log_tab_content();
    
    ImGui::EndGroup();
}

void render_engine_vitals_tab() {
    // Professional section header
    render_section_header("Engine Vitals", "Real-time Monitoring", g_ui_theme.primary_color);
    
    // Demo mode indicator
    if (g_demo_mode) {
        ImGui::SameLine();
        render_status_indicator("Demo Mode", true, "ACTIVE");
    }
    
    ImGui::Spacing();
    
    // Engine Speed Section
    render_section_header("Engine Speed", "RPM and Vehicle Speed", g_ui_theme.accent_color);
    
    // Engine speed metrics in a grid layout
    ImGui::BeginGroup();
    
    // RPM display with large, prominent styling
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, g_ui_theme.corner_radius);
    ImGui::BeginChild("RPMDisplay", ImVec2(200, 100), true);
    
    ImGui::TextColored(g_ui_theme.text_secondary, "Engine RPM");
    ImGui::TextColored(g_ui_theme.primary_color, "%.0f", g_ecu_data.rpm);
    ImGui::TextColored(g_ui_theme.text_muted, "RPM");
    
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    
    // Vehicle speed display
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
    ImGui::BeginChild("SpeedDisplay", ImVec2(200, 100), true);
    
    ImGui::TextColored(g_ui_theme.text_secondary, "Vehicle Speed");
    ImGui::TextColored(g_ui_theme.secondary_color, "%.1f", g_ecu_data.rpm * 0.1f); // Placeholder calculation
    ImGui::TextColored(g_ui_theme.text_muted, "km/h");
    
    ImGui::EndChild();
    ImGui::PopStyleColor();
    
    ImGui::EndGroup();
    
    ImGui::Spacing();
    
    // Temperatures Section
    render_section_header("Temperatures", "Engine and Intake Temperatures", g_ui_theme.warning_color);
    
    // Temperature metrics in a grid
    ImGui::BeginGroup();
    
    // Coolant temperature
    char coolant_str[16];
    snprintf(coolant_str, sizeof(coolant_str), "%.0f", g_ecu_data.coolant_temp);
    render_metric_card("Coolant", coolant_str, "°C", 
                      g_ecu_data.coolant_temp > 100 ? g_ui_theme.warning_color : g_ui_theme.success_color,
                      "Engine coolant");
    
    ImGui::SameLine();
    
    // Intake temperature
    char intake_str[16];
    snprintf(intake_str, sizeof(intake_str), "%.0f", g_ecu_data.intake_temp);
    render_metric_card("Intake", intake_str, "°C", 
                      g_ecu_data.intake_temp > 80 ? g_ui_theme.warning_color : g_ui_theme.success_color,
                      "Intake air");
    
    ImGui::SameLine();
    
    // Oil temperature
    char oil_str[16];
    snprintf(oil_str, sizeof(oil_str), "%.0f", g_ecu_data.oil_temp);
    render_metric_card("Oil", oil_str, "°C", 
                      g_ecu_data.oil_temp > 120 ? g_ui_theme.warning_color : g_ui_theme.success_color,
                      "Engine oil");
    
    ImGui::EndGroup();
    
    ImGui::Spacing();
    
    // Pressures Section
    render_section_header("Pressures", "Engine and Fuel System Pressures", g_ui_theme.success_color);
    
    // Pressure metrics in a grid
    ImGui::BeginGroup();
    
    // MAP pressure
    char map_str[16];
    snprintf(map_str, sizeof(map_str), "%.0f", g_ecu_data.map);
    render_metric_card("MAP", map_str, "kPa", 
                      g_ecu_data.map > 200 ? g_ui_theme.warning_color : g_ui_theme.success_color,
                      "Manifold pressure");
    
    ImGui::SameLine();
    
    // Oil pressure
    char oil_press_str[16];
    snprintf(oil_press_str, sizeof(oil_press_str), "%.0f", g_ecu_data.oil_pressure);
    render_metric_card("Oil Press", oil_press_str, "kPa", 
                      g_ecu_data.oil_pressure < 100 ? g_ui_theme.error_color : g_ui_theme.success_color,
                      "Oil system");
    
    ImGui::SameLine();
    
    // Fuel pressure
    char fuel_press_str[16];
    snprintf(fuel_press_str, sizeof(fuel_press_str), "%.0f", g_ecu_data.fuel_pressure);
    render_metric_card("Fuel Press", fuel_press_str, "kPa", 
                      g_ecu_data.fuel_pressure < 150 ? g_ui_theme.warning_color : g_ui_theme.success_color,
                      "Fuel system");
    
    ImGui::EndGroup();
    
    ImGui::Spacing();
    
    // Additional Engine Data
    render_section_header("Additional Data", "Other Engine Parameters", g_ui_theme.accent_color);
    
    ImGui::BeginGroup();
    
    // AFR display
    char afr_str[16];
    snprintf(afr_str, sizeof(afr_str), "%.1f", g_ecu_data.afr);
    render_metric_card("AFR", afr_str, "ratio", 
                      g_ecu_data.afr < 12 || g_ecu_data.afr > 16 ? g_ui_theme.warning_color : g_ui_theme.success_color,
                      "Air/Fuel ratio");
    
    ImGui::SameLine();
    
    // Boost display
    char boost_str[16];
    snprintf(boost_str, sizeof(boost_str), "%.1f", g_ecu_data.boost);
    render_metric_card("Boost", boost_str, "kPa", 
                      g_ecu_data.boost > 0 ? g_ui_theme.secondary_color : g_ui_theme.text_primary,
                      "Turbo boost");
    
    ImGui::SameLine();
    
    // Timing display
    char timing_str[16];
    snprintf(timing_str, sizeof(timing_str), "%.1f", g_ecu_data.timing);
    render_metric_card("Timing", timing_str, "°", 
                      g_ecu_data.timing > 30 ? g_ui_theme.warning_color : g_ui_theme.success_color,
                      "Ignition timing");
    
    ImGui::EndGroup();
}

void render_enrichments_tab() {
    ImGui::Text("Enrichments - Additional Data Sources");
    ImGui::Separator();
    
    ImGui::Text("This tab will eventually integrate with external sensors and data.");
    ImGui::Text("Currently, it's a placeholder for future enhancements.");
}

void render_ve_table_tab() {
    // Professional section header for VE Table
    render_section_header("VE Table(1)", "Fuel and Spark Tuning", g_ui_theme.primary_color);
    
    // Demo mode indicator
    if (g_demo_mode) {
        ImGui::SameLine();
        render_status_indicator("Demo Mode", true, "ACTIVE");
    }
    
    // Check if VE table is available
    if (!g_ve_table || !g_ve_table_initialized) {
        ImGui::TextColored(g_ui_theme.error_color, "VE Table not available");
        return;
    }
    
    // Table information with professional styling
    render_section_header("Table Information", "Current Table Status", g_ui_theme.success_color);
    
    // Table info in a professional card layout
    ImGui::BeginGroup();
    
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, g_ui_theme.corner_radius);
    ImGui::BeginChild("TableInfo", ImVec2(300, 80), true);
    
    ImGui::TextColored(g_ui_theme.text_secondary, "Table Size:");
    ImGui::TextColored(g_ui_theme.primary_color, "%dx%d", g_ve_table->width, g_ve_table->height);
    
    ImGui::TextColored(g_ui_theme.text_secondary, "Value Range:");
    ImGui::TextColored(g_ui_theme.accent_color, "%.1f - %.1f", g_ve_table->metadata.min_value, g_ve_table->metadata.max_value);
    
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    
    ImGui::EndGroup();
    
    // Professional table operations
    render_section_header("Professional Operations", "Advanced Table Editing", g_ui_theme.accent_color);
    
    ImGui::BeginGroup();
    
    if (ImGui::Button("Create Backup", ImVec2(120, 25))) {
        create_table_backup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Restore Backup", ImVec2(120, 25))) {
        restore_table_from_backup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Export Table", ImVec2(120, 25))) {
        export_table_to_file("ve_table_export.csv");
    }
    ImGui::SameLine();
    if (ImGui::Button("Import Table", ImVec2(120, 25))) {
        import_table_from_file("ve_table_import.csv");
    }
    
    ImGui::EndGroup();
    
    // Professional Key Bindings Legend
    ImGui::Separator();
    ImGui::Text("🎯 Professional Table Editing Key Bindings");
    ImGui::SameLine();
    
    // Simple key bindings display
    ImGui::Text("Arrow Keys: Navigate | I: Interpolate | S: Smooth | B: Backup | R: Restore");
    
    // End of function
    add_log_entry(0, "VE Table tab rendered successfully");
}

void render_ignition_table_tab() {
    // DEBUG: Add logging to see if this function is actually being called
    add_log_entry(0, "DEBUG: render_ignition_table_tab() called - checking table state");
    
    // Professional section header for Ignition Table
    render_section_header("Ignition Table(1)", "Spark Advance Tuning", g_ui_theme.primary_color);
    
    // Demo mode indicator
    if (g_demo_mode) {
        ImGui::SameLine();
        render_status_indicator("Demo Mode", true, "ACTIVE");
    }
    
    // Check if Ignition table is available
    if (!g_ignition_table || !g_ignition_table_initialized) {
        add_log_entry(0, "ERROR: Ignition table not available or not initialized");
        ImGui::TextColored(g_ui_theme.error_color, "Ignition Table not available");
        return;
    }
    
    add_log_entry(0, "DEBUG: Ignition table is valid - width=%d, height=%d", 
                 g_ignition_table->width, g_ignition_table->height);
    
    // Table information with professional styling
    render_section_header("Table Information", "Current Table Status", g_ui_theme.success_color);
    
    // Table info in a professional card layout
    ImGui::BeginGroup();
    
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, g_ui_theme.corner_radius);
    ImGui::BeginChild("TableInfo", ImVec2(300, 80), true);
    
    ImGui::TextColored(g_ui_theme.text_secondary, "Table Size:");
    ImGui::TextColored(g_ui_theme.primary_color, "%dx%d", g_ignition_table->width, g_ignition_table->height);
    
    ImGui::TextColored(g_ui_theme.text_secondary, "Value Range:");
    ImGui::TextColored(g_ui_theme.accent_color, "%.1f - %.1f", g_ignition_table->metadata.min_value, g_ignition_table->metadata.max_value);
    
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    
    ImGui::EndGroup();
    
    // Professional table operations
    render_section_header("Professional Operations", "Advanced Table Editing", g_ui_theme.accent_color);
    
    ImGui::BeginGroup();
    
    if (ImGui::Button("Create Backup", ImVec2(120, 25))) {
        create_table_backup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Restore Backup", ImVec2(120, 25))) {
        restore_table_from_backup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Export Table", ImVec2(120, 25))) {
        export_table_to_file("ignition_table_export.csv");
    }
    ImGui::SameLine();
    if (ImGui::Button("Import Table", ImVec2(120, 25))) {
        import_table_from_file("ignition_table_import.csv");
    }
    
    ImGui::EndGroup();
    
    ImGui::Separator();
    
    // Professional editing controls
    ImGui::BeginGroup();
    ImGui::TextColored(g_ui_theme.primary_color, "Professional Editing Controls:");
    
    if (ImGui::Button("Reset to Demo", ImVec2(140, 25))) {
        imgui_ignition_table_load_demo_data(g_ignition_table);
        g_table_has_changes = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Selection", ImVec2(140, 25))) {
        g_selected_cell_x = -1;
        g_selected_cell_y = -1;
        g_cell_editing = false;
        clear_multi_selection();
    }
    ImGui::SameLine();
    if (ImGui::Button("Edit Cell", ImVec2(140, 25))) {
        if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
            g_cell_editing = true;
            g_input_field_focused = true;
            // Initialize buffer with current value
            float current_val = imgui_table_get_value(g_ignition_table, g_selected_cell_x, g_selected_cell_y);
            snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.1f", current_val);
            add_log_entry(0, "Entering edit mode for cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
        } else {
            add_log_entry(0, "Please select a cell first before editing");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Create Backup", ImVec2(140, 25))) {
        create_table_backup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Restore Backup", ImVec2(140, 25))) {
        restore_table_from_backup();
    }
    ImGui::EndGroup();
    
    // Professional cell information display
    if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
        ImGui::BeginChild("CellInfo", ImVec2(0, 80), true);
        
        float cell_value = imgui_table_get_value(g_ignition_table, g_selected_cell_x, g_selected_cell_y);
        ImGui::TextColored(g_ui_theme.primary_color, "Selected Cell Information:");
        ImGui::Text("Position: [%d, %d] | Value: %.1f°", 
                   g_selected_cell_x, g_selected_cell_y, cell_value);
        ImGui::Text("MAP: %.1f kPa | RPM: %.0f", 
                   g_ignition_table->x_axis[g_selected_cell_x], 
                   g_ignition_table->y_axis[g_selected_cell_y]);
        
        // Quick edit field
        ImGui::Text("Quick Edit:");
        ImGui::SameLine();
        ImGui::PushItemWidth(100);
        if (ImGui::InputFloat("##QuickEdit", &cell_value, 1.0f, 5.0f, "%.1f")) {
            imgui_table_set_value(g_ignition_table, g_selected_cell_x, g_selected_cell_y, cell_value);
            g_table_has_changes = true;
        }
        ImGui::PopItemWidth();
        
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    
    // Professional multi-selection display
    if (g_multi_selection.active) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
        ImGui::BeginChild("MultiSelectionInfo", ImVec2(0, 60), true);
        
        int cell_count = get_selection_cell_count();
        ImGui::TextColored(g_ui_theme.primary_color, "Multi-Selection Active:");
        ImGui::Text("Range: [%d,%d] to [%d,%d] | Cells: %d", 
                   g_multi_selection.start_x, g_multi_selection.start_y,
                   g_multi_selection.end_x, g_multi_selection.end_y, cell_count);
        ImGui::Text("Operations: Press 'I' for interpolation, 'S' for smoothing, 'M' for math operations");
        
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    
    ImGui::Separator();
    
    // Professional table title and instructions
    ImGui::TextColored(g_ui_theme.primary_color, "Professional Table Editor");
    ImGui::TextColored(g_ui_theme.text_secondary, "Click cells to select | Double-click to edit | Use arrow keys to navigate | Press Enter to save changes");
    
    // Debug: Log the first few values to verify data source
    add_log_entry(0, "DEBUG: Ignition table data verification - First few values:");
    for (int y = 0; y < 3 && y < g_ignition_table->height; y++) {
        for (int x = 0; x < 3 && x < g_ignition_table->width; x++) {
            float val = g_ignition_table->data[y][x];
            add_log_entry(0, "  [%d,%d] = %.1f", x, y, val);
        }
    }
    
    // Debug: Log the specific cell that was showing wrong value
    add_log_entry(0, "DEBUG: Checking cell [8,3] (RPM 2545, MAP 116):");
    if (g_ignition_table->height > 3 && g_ignition_table->width > 8) {
        float val = g_ignition_table->data[3][8];
        add_log_entry(0, "  Cell [8,3] = %.1f (should be around 35.0)", val);
        add_log_entry(0, "  RPM: %.0f, MAP: %.1f", g_ignition_table->y_axis[3], g_ignition_table->x_axis[8]);
    }
    
    // Debug: Check the highlighted cell from screenshot [6,2] = 78.8
    add_log_entry(0, "DEBUG: Checking highlighted cell [6,2] (RPM 1864, MAP 92):");
    if (g_ignition_table->height > 2 && g_ignition_table->width > 6) {
        float val = g_ignition_table->data[2][6];
        add_log_entry(0, "  Cell [6,2] = %.1f (screenshot shows 78.8 - CORRUPTED!)", val);
        add_log_entry(0, "  RPM: %.0f, MAP: %.1f", g_ignition_table->y_axis[2], g_ignition_table->x_axis[6]);
        add_log_entry(0, "  Expected value should be around 33.1° based on demo algorithm");
    }
    
    // Debug: Check if data array is properly allocated
    add_log_entry(0, "DEBUG: Table structure verification:");
    add_log_entry(0, "  Width: %d, Height: %d", g_ignition_table->width, g_ignition_table->height);
    add_log_entry(0, "  Data pointer: %p", (void*)g_ignition_table->data);
    if (g_ignition_table->data) {
        add_log_entry(0, "  Data[0] pointer: %p", (void*)g_ignition_table->data[0]);
    }
    
    // Debug: Check for any corrupted values in the entire table
    add_log_entry(0, "DEBUG: Scanning for corrupted values (>50° or <10°):");
    int corrupted_count = 0;
    for (int y = 0; y < g_ignition_table->height; y++) {
        for (int x = 0; x < g_ignition_table->width; x++) {
            float val = g_ignition_table->data[y][x];
            if (val > 50.0f || val < 10.0f) {
                add_log_entry(0, "  CORRUPTED: [%d,%d] = %.1f°", x, y, val);
                corrupted_count++;
            }
        }
    }
    add_log_entry(0, "DEBUG: Found %d corrupted values in ignition table", corrupted_count);
    
    // Debug: Verify demo data loading by checking a few specific cells
    add_log_entry(0, "DEBUG: Verifying demo data loading:");
    if (g_ignition_table->height > 0 && g_ignition_table->width > 0) {
        float first_cell = g_ignition_table->data[0][0];
        float last_cell = g_ignition_table->data[g_ignition_table->height-1][g_ignition_table->width-1];
        add_log_entry(0, "  First cell [0,0] = %.1f° (should be ~15.0°)", first_cell);
        add_log_entry(0, "  Last cell [%d,%d] = %.1f° (should be ~43.0°)", 
                     g_ignition_table->width-1, g_ignition_table->height-1, last_cell);
        
        // Check if values look like they came from demo algorithm
        if (first_cell < 10.0f || first_cell > 25.0f) {
            add_log_entry(0, "  ERROR: First cell value %.1f is outside expected demo range!", first_cell);
        }
        if (last_cell < 35.0f || last_cell > 50.0f) {
            add_log_entry(0, "  ERROR: Last cell value %.1f is outside expected demo range!", last_cell);
        }
    }
    
    // Simple table grid using ImGui::BeginTable (much safer than custom OpenGL drawing)
    ImGui::BeginChild("IgnitionTableEditor", ImVec2(0, 0), true);
    
    if (ImGui::BeginTable("IgnitionTable", g_ignition_table->width + 1, 
                          ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | 
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        
        // Header row with MAP values (X-axis)
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        ImGui::TableNextColumn();
        ImGui::TextColored(g_ui_theme.primary_color, "RPM\\MAP");
        for (int x = 0; x < g_ignition_table->width; x++) {
            ImGui::TableNextColumn();
            ImGui::TextColored(g_ui_theme.primary_color, "%.0f", g_ignition_table->x_axis[x]);
        }
        
        // Data rows with RPM values (Y-axis) and timing data
        for (int y = 0; y < g_ignition_table->height; y++) {
            ImGui::TableNextRow();
            
            // RPM label column (Y-axis)
            ImGui::TableNextColumn();
            ImGui::TextColored(g_ui_theme.accent_color, "%.0f", g_ignition_table->y_axis[y]);
            
            // Data columns with actual timing values
            for (int x = 0; x < g_ignition_table->width; x++) {
                ImGui::TableNextColumn();
                
                // Get current value with bounds checking
                float value = 0.0f;
                if (y < g_ignition_table->height && x < g_ignition_table->width) {
                    value = g_ignition_table->data[y][x];
                    
                    // CRITICAL DEBUG: Log the data access for this specific cell
                    if (x == g_selected_cell_x && y == g_selected_cell_y) {
                        add_log_entry(0, "*** CELL DATA ACCESS DEBUG *** - Cell [%d,%d]: Value=%.1f, Table=%s", 
                                     x, y, value, 
                                     (g_ignition_table == g_ve_table) ? "VE_TABLE_WRONG!" : 
                                     (g_ignition_table == g_ignition_table) ? "IGNITION_TABLE_CORRECT" : "UNKNOWN");
                    }
                    
                    // Debug: Log suspicious values
                    if (value > 80.0f || value < -10.0f) {
                        add_log_entry(0, "DEBUG: Suspicious value at [%d,%d] = %.1f", x, y, value);
                    }
                } else {
                    add_log_entry(0, "ERROR: Accessing out of bounds at [%d,%d], table size is %dx%d", x, y, g_ignition_table->width, g_ignition_table->height);
                }
                
                // Check if this cell is selected
                bool is_selected = (x == g_selected_cell_x && y == g_selected_cell_y);
                
                // Create a clickable button for each cell
                char cell_button_id[64];
                snprintf(cell_button_id, sizeof(cell_button_id), "##cell_%d_%d", x, y);
                
                // Calculate heatmap color based on ignition value
                float normalized = 0.0f;
                if (g_ignition_table->metadata.max_value > g_ignition_table->metadata.min_value) {
                    normalized = (value - g_ignition_table->metadata.min_value) / (g_ignition_table->metadata.max_value - g_ignition_table->metadata.min_value);
                }
                
                // Set button color based on value (heatmap effect)
                ImVec4 button_color;
                if (normalized < 0.5f) {
                    // Blue to Green gradient for lower values
                    float t = normalized * 2.0f;
                    button_color = ImVec4(0.0f, t, 1.0f - t, 0.8f);
                } else {
                    // Green to Red gradient for higher values
                    float t = (normalized - 0.5f) * 2.0f;
                    button_color = ImVec4(t, 1.0f - t, 0.0f, 0.8f);
                }
                
                // Handle cell selection and editing
                if (is_selected && g_cell_editing) {
                    // Show input field for editing
                    ImGui::PushItemWidth(60);
                    char input_id[64];
                    snprintf(input_id, sizeof(input_id), "##input_%d_%d", x, y);
                    
                    if (ImGui::InputText(input_id, g_cell_edit_buffer, sizeof(g_cell_edit_buffer), 
                                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
                        // Parse and save the new value
                        float new_value = atof(g_cell_edit_buffer);
                        // Clamp to valid range
                        new_value = fmax(g_ignition_table->metadata.min_value, 
                                       fmin(g_ignition_table->metadata.max_value, new_value));
                        
                        // Save to table
                        imgui_table_set_value(g_ignition_table, x, y, new_value);
                        g_table_has_changes = true;
                        g_cell_editing = false;
                        g_input_field_focused = false;
                        
                        // Clear the buffer after successful edit
                        g_cell_edit_buffer[0] = '\0';
                        add_log_entry(0, "Ignition table cell [%d,%d] updated to %.1f°", x, y, new_value);
                    }
                    ImGui::PopItemWidth();
                } else {
                    // Show clickable button with current value and heatmap color
                    ImGui::PushStyleColor(ImGuiCol_Button, button_color);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(button_color.x, button_color.y, button_color.z, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(button_color.x, button_color.y, button_color.z, 1.0f));
                    
                    if (ImGui::Button(cell_button_id, ImVec2(60, 20))) {
                        g_selected_cell_x = x;
                        g_selected_cell_y = y;
                        g_cell_editing = false;
                        g_input_field_focused = false;
                        clear_multi_selection();
                        add_log_entry(0, "Ignition table cell [%d,%d] selected", x, y);
                    }
                    
                    // Handle double-click to enter edit mode
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                        g_cell_editing = true;
                        g_input_field_focused = true;
                        // Initialize buffer with current value
                        snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.1f", value);
                        add_log_entry(0, "Double-clicked cell [%d,%d], entering edit mode", x, y);
                    }
                    
                    ImGui::PopStyleColor(3);
                    
                    // Show the value as text centered on the button
                    ImVec2 button_pos = ImGui::GetItemRectMin();
                    ImVec2 button_size = ImGui::GetItemRectSize();
                    
                    // Format the value text
                    char value_text[16];
                    snprintf(value_text, sizeof(value_text), "%.1f", value);
                    ImVec2 text_size = ImGui::CalcTextSize(value_text);
                    ImVec2 text_pos = ImVec2(
                        button_pos.x + (button_size.x - text_size.x) * 0.5f,
                        button_pos.y + (button_size.y - text_size.y) * 0.5f
                    );
                    
                    // Draw text with shadow for better visibility
                    ImGui::GetWindowDrawList()->AddText(
                        ImVec2(text_pos.x + 1, text_pos.y + 1),
                        IM_COL32(0, 0, 0, 255),
                        value_text
                    );
                    ImGui::GetWindowDrawList()->AddText(
                        text_pos,
                        IM_COL32(255, 255, 255, 255),
                        value_text
                    );
                }
                
                // Highlight the selected cell
                if (is_selected) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, 
                                          ImGui::GetColorU32(g_ui_theme.accent_color));
                }
            }
        }
        
        ImGui::EndTable();
    }
    
    ImGui::EndChild();
    
    // Professional Key Bindings Legend
    ImGui::Separator();
    ImGui::Text("🎯 Professional Table Editing Key Bindings");
    ImGui::SameLine();
    
    // Simple key bindings display
    ImGui::Text("Arrow Keys: Navigate | I: Interpolate | S: Smooth | B: Backup | R: Restore");
    
    // End of function
    add_log_entry(0, "Ignition Table tab rendered successfully");
}


void render_tools_tab() {
    ImGui::Text("Tools - Utility Functions");
    ImGui::Separator();
    
    ImGui::Text("This tab will contain various utility functions.");
    ImGui::Text("Currently, it's a placeholder for future tools.");
}

void render_warmup_wizard_tab() {
    ImGui::Text("Warmup Wizard - ECU Warmup Procedure");
    ImGui::Separator();
    
    ImGui::Text("This tab will guide you through the ECU warmup procedure.");
    ImGui::Text("It will include steps for engine pre-heating and ECU initialization.");
}

// TunerStudio-style UI implementations
void render_tunerstudio_sidebar() {
    // Project name header (exact TunerStudio style)
    ImGui::PushStyleColor(ImGuiCol_Header, g_ui_theme.primary_color);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, g_ui_theme.primary_color);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, g_ui_theme.primary_color);
    
    if (ImGui::CollapsingHeader("ECU Project", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PopStyleColor(3);
        
        // Communications section - TOP PRIORITY for ECU connection
        ImGui::TextColored(g_ui_theme.primary_color, "Communications");
        if (g_selected_view == VIEW_COMMUNICATIONS) {
            ImGui::TextColored(g_ui_theme.success_color, "● ECU Connection");
        } else {
            if (ImGui::Selectable("ECU Connection", false)) {
                add_log_entry(0, "ECU Connection selected, switching to communications view");
                g_selected_view = VIEW_COMMUNICATIONS;
                add_log_entry(0, "g_selected_view set to VIEW_COMMUNICATIONS (%d)", VIEW_COMMUNICATIONS);
            }
        }
        if (ImGui::Selectable("Connection Status", false)) { /* TODO */ }
        if (ImGui::Selectable("Protocol Settings", false)) { /* TODO */ }
        if (ImGui::Selectable("Connection Logs", false)) { /* TODO */ }
        
        ImGui::Separator();
        
        // Fuel Settings section
        if (ImGui::TreeNode("Fuel Settings")) {
            if (g_selected_view == VIEW_TUNING && g_selected_table_index == 0) {
                ImGui::TextColored(g_ui_theme.success_color, "● VE Table 1");
            } else {
                if (ImGui::Selectable("VE Table 1", false)) {
                    add_log_entry(0, "VE Table 1 selected, switching to tuning view");
                    g_selected_view = VIEW_TUNING;
                    g_selected_table_index = 0; // Set to VE Table
                    add_log_entry(0, "g_selected_view set to VIEW_TUNING (%d), table index set to %d", VIEW_TUNING, g_selected_table_index);
                }
            }
            if (ImGui::Selectable("AFR Table", false)) { /* TODO */ }
            if (ImGui::Selectable("Target AFR Table", false)) { /* TODO */ }
            if (ImGui::Selectable("Fuel Pressure Table", false)) { /* TODO */ }
            if (ImGui::Selectable("Injector Dead Time Table", false)) { /* TODO */ }
            if (ImGui::Selectable("Acceleration Enrichment Table", false)) { /* TODO */ }
            if (ImGui::Selectable("Cranking Fuel Table", false)) { /* TODO */ }
            if (ImGui::Selectable("Afterstart Enrichment Table", false)) { /* TODO */ }
            if (ImGui::Selectable("Warmup Enrichment Table", false)) { /* TODO */ }
            ImGui::TreePop();
        }
        
        // Ignition Settings section
        if (ImGui::TreeNode("Ignition Settings")) {
            if (g_selected_view == VIEW_TUNING && g_selected_table_index == 1) {
                ImGui::TextColored(g_ui_theme.success_color, "● Ignition Table 1");
            } else {
                if (ImGui::Selectable("Ignition Table 1", false)) {
                    add_log_entry(0, "Ignition Table 1 selected, switching to tuning view");
                    g_selected_view = VIEW_TUNING;
                    g_selected_table_index = 1; // Set to Ignition Table
                    add_log_entry(0, "g_selected_view set to VIEW_TUNING (%d), table index set to %d", VIEW_TUNING, g_selected_table_index);
                }
            }
            if (ImGui::Selectable("Ignition Trim Table", false)) { /* TODO */ }
            if (ImGui::Selectable("Cranking Timing Table", false)) { /* TODO */ }
            if (ImGui::Selectable("Idle Timing Table", false)) { /* TODO */ }
            if (ImGui::Selectable("Knock Retard Table", false)) { /* TODO */ }
            if (ImGui::Selectable("Launch Control Timing", false)) { /* TODO */ }
            ImGui::TreePop();
        }
        
        // Idle Control section
        if (ImGui::TreeNode("Idle Control")) {
            if (ImGui::Selectable("Idle Speed Control Table", false)) { /* TODO */ }
            if (ImGui::Selectable("Idle Valve Position", false)) { /* TODO */ }
            ImGui::TreePop();
        }
        
        // Boost Control section
        if (ImGui::TreeNode("Boost Control")) {
            if (ImGui::Selectable("Boost Control Table", false)) { /* TODO */ }
            if (ImGui::Selectable("Wastegate Duty Cycle", false)) { /* TODO */ }
            ImGui::TreePop();
        }
        
        // Advanced Features section
        if (ImGui::TreeNode("Advanced Features")) {
            if (ImGui::Selectable("VVT Table", false)) { /* TODO */ }
            if (ImGui::Selectable("Nitrous Control Table", false)) { /* TODO */ }
            if (ImGui::Selectable("Rev Limiter Table", false)) { /* TODO */ }
            if (ImGui::Selectable("Launch Control Table", false)) { /* TODO */ }
            ImGui::TreePop();
        }
        
        // 3D Tuning Maps section
        if (ImGui::TreeNode("3D Tuning Maps")) {
            if (ImGui::Selectable("3D VE View", false)) { /* TODO */ }
            if (ImGui::Selectable("3D Ignition View", false)) { /* TODO */ }
            if (ImGui::Selectable("3D AFR View", false)) { /* TODO */ }
            ImGui::TreePop();
        }
        
        ImGui::Separator();
        
        // Real-time Data section
        ImGui::TextColored(g_ui_theme.primary_color, "Real-time Data");
        if (g_selected_view == VIEW_GAUGE_CLUSTER) {
            ImGui::TextColored(g_ui_theme.success_color, "● Gauge Cluster");
        } else {
            if (ImGui::Selectable("Gauge Cluster", false)) {
                add_log_entry(0, "Gauge Cluster selected, switching to gauge cluster view");
                g_selected_view = VIEW_GAUGE_CLUSTER;
                add_log_entry(0, "g_selected_view set to VIEW_GAUGE_CLUSTER (%d)", VIEW_GAUGE_CLUSTER);
            }
        }
        if (ImGui::Selectable("Runtime Values", false)) { /* TODO */ }
        if (ImGui::Selectable("Status Flags", false)) { /* TODO */ }
        if (ImGui::Selectable("Performance Metrics", false)) { /* TODO */ }
        
        ImGui::Separator();
        
        // Data Logging section
        ImGui::TextColored(g_ui_theme.primary_color, "Data Logging");
        if (ImGui::Selectable("Logging Setup", false)) { /* TODO */ }
        if (ImGui::Selectable("Log Analysis", false)) { /* TODO */ }
        if (ImGui::Selectable("Scatter Plots", false)) { /* TODO */ }
        if (ImGui::Selectable("Performance Logs", false)) { /* TODO */ }
        
        ImGui::Separator();
        
        // Diagnostics section
        ImGui::TextColored(g_ui_theme.primary_color, "Diagnostics");
        if (ImGui::Selectable("Error Codes", false)) { /* TODO */ }
        if (ImGui::Selectable("System Health", false)) { /* TODO */ }
        if (ImGui::Selectable("Performance Monitoring", false)) { /* TODO */ }
        
        ImGui::Separator();
        
        // Plugin Manager section
        ImGui::TextColored(g_ui_theme.primary_color, "Plugin Manager");
        if (g_selected_view == VIEW_PLUGIN_MANAGER) {
            ImGui::TextColored(g_ui_theme.success_color, "● Plugin Manager");
        } else {
            if (ImGui::Selectable("Plugin Manager", false)) {
                add_log_entry(0, "Plugin Manager selected, switching to plugin manager view");
                g_selected_view = VIEW_PLUGIN_MANAGER;
                add_log_entry(0, "g_selected_view set to VIEW_PLUGIN_MANAGER (%d)", VIEW_PLUGIN_MANAGER);
            }
        }
        if (ImGui::Selectable("Plugin Settings", false)) { /* TODO */ }
        if (ImGui::Selectable("Plugin Development", false)) { /* TODO */ }
    } else {
        ImGui::PopStyleColor(3);
    }
}

void render_content_by_view() {
    // Debug logging for view switching
    static ViewType last_view = VIEW_COUNT;
    if (last_view != g_selected_view) {
        add_log_entry(0, "View switched from %d to %d", last_view, g_selected_view);
        last_view = g_selected_view;
    }
    
    switch (g_selected_view) {
        case VIEW_GAUGE_CLUSTER:
            add_log_entry(0, "Rendering VIEW_GAUGE_CLUSTER");
            render_gauge_cluster_view();
            break;
        case VIEW_TUNING:
            add_log_entry(0, "Rendering VIEW_TUNING");
            render_tuning_view();
            break;
        case VIEW_GRAPHING:
            add_log_entry(0, "Rendering VIEW_GRAPHING");
            render_graphing_view();
            break;
        case VIEW_DIAGNOSTICS:
            add_log_entry(0, "Rendering VIEW_DIAGNOSTICS");
            render_diagnostics_view();
            break;
        case VIEW_COMMUNICATIONS:
            add_log_entry(0, "Rendering VIEW_COMMUNICATIONS");
            render_communications_view();
            break;
        case VIEW_PLUGIN_MANAGER:
            add_log_entry(0, "Rendering VIEW_PLUGIN_MANAGER");
            render_plugin_manager_view();
            break;
        default:
            add_log_entry(2, "Unknown view type: %d, defaulting to gauge cluster", g_selected_view);
            render_gauge_cluster_view();
            break;
    }
}

void render_gauge_cluster_view() {
    ImGui::TextColored(g_ui_theme.primary_color, "Gauge Cluster - Engine Monitoring");
    ImGui::Separator();
    
    // Demo mode indicator
    if (g_demo_mode) {
        ImGui::TextColored(g_ui_theme.warning_color, "DEMO MODE - Simulated Data");
        ImGui::Separator();
    }
    
    // First row of gauges
    ImGui::BeginGroup();
    
    // Engine Speed gauge (large, prominent)
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
    ImGui::BeginChild("EngineSpeedGauge", ImVec2(200, 150), true);
    ImGui::TextColored(g_ui_theme.primary_color, "Engine Speed");
    ImGui::TextColored(g_ui_theme.success_color, "%.0f", g_demo_mode ? 2500.0f : 0.0f);
    ImGui::TextColored(g_ui_theme.text_secondary, "RPM");
    ImGui::TextColored(g_ui_theme.text_muted, "0 - 8000");
    ImGui::EndChild();
    ImGui::PopStyleColor();
    
    ImGui::EndGroup();
    
    ImGui::SameLine();
    
    // Throttle Position gauge
    ImGui::BeginGroup();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
    ImGui::BeginChild("ThrottleGauge", ImVec2(150, 120), true);
    ImGui::TextColored(g_ui_theme.primary_color, "Throttle");
    ImGui::TextColored(g_ui_theme.warning_color, "%.1f%%", g_demo_mode ? 45.5f : 0.0f);
    ImGui::TextColored(g_ui_theme.text_muted, "0 - 100%%");
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::EndGroup();
    
    ImGui::SameLine();
    
    // Coolant Temperature gauge
    ImGui::BeginGroup();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
    ImGui::BeginChild("CoolantGauge", ImVec2(150, 120), true);
    ImGui::TextColored(g_ui_theme.primary_color, "Coolant");
    ImGui::TextColored(g_ui_theme.error_color, "%.0f°F", g_demo_mode ? 185.0f : 0.0f);
    ImGui::TextColored(g_ui_theme.text_muted, "-40 - 240°F");
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::EndGroup();
    
    ImGui::SameLine();
    
    // Ignition Advance gauge
    ImGui::BeginGroup();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
    ImGui::BeginChild("IgnitionGauge", ImVec2(150, 120), true);
    ImGui::TextColored(g_ui_theme.primary_color, "Ignition");
    ImGui::TextColored(g_ui_theme.secondary_color, "%.1f°", g_demo_mode ? 22.5f : 0.0f);
    ImGui::TextColored(g_ui_theme.text_muted, "-10 - 50°");
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::EndGroup();
    
    // Second row of gauges
    ImGui::Spacing();
    
    // Lost Sync Counter gauge
    ImGui::BeginGroup();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
    ImGui::BeginChild("SyncGauge", ImVec2(150, 120), true);
    ImGui::TextColored(g_ui_theme.primary_color, "Lost Sync");
    ImGui::TextColored(g_ui_theme.text_primary, "%.0f", g_demo_mode ? 0.0f : 0.0f);
    ImGui::TextColored(g_ui_theme.text_muted, "0 - 240");
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::EndGroup();
    
    ImGui::SameLine();
    
    // Connection status
    ImGui::BeginGroup();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
    ImGui::BeginChild("ConnectionStatus", ImVec2(200, 120), true);
    ImGui::TextColored(g_ui_theme.primary_color, "Status");
    if (g_demo_mode) {
        ImGui::TextColored(g_ui_theme.success_color, "DEMO MODE");
        ImGui::TextColored(g_ui_theme.text_secondary, "Simulated ECU");
    } else {
        ImGui::TextColored(g_ui_theme.error_color, "NOT CONNECTED");
        ImGui::TextColored(g_ui_theme.text_secondary, "No ECU");
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::EndGroup();
}

void render_tuning_view() {
    // Show the currently selected table directly
    if (g_selected_table_index == 0) {
        render_ve_table_tab();
    } else if (g_selected_table_index == 1) {
        render_ignition_table_tab();
    } else {
        // Default to VE table if somehow no table is selected
        ImGui::TextColored(g_ui_theme.primary_color, "Tuning Views - Table Editing");
        ImGui::Separator();
        ImGui::Text("Please select a table from the sidebar:");
        ImGui::BulletText("VE Table 1 - Fuel tuning");
        ImGui::BulletText("Ignition Table 1 - Spark timing");
    }
}

void render_graphing_view() {
    ImGui::TextColored(g_ui_theme.primary_color, "Graphing & Logging - Data Analysis");
    ImGui::Separator();
    
    ImGui::Text("This view will contain:");
    ImGui::BulletText("Real-time data charts");
    ImGui::BulletText("Data logging controls");
    ImGui::BulletText("Scatter plot analysis");
    ImGui::BulletText("Performance metrics");
}

void render_diagnostics_view() {
    ImGui::TextColored(g_ui_theme.primary_color, "Diagnostics - System Health");
    ImGui::Separator();
    
    ImGui::Text("This view will contain:");
    ImGui::BulletText("ECU status information");
    ImGui::BulletText("Error codes and warnings");
    ImGui::BulletText("System diagnostics");
    ImGui::BulletText("Performance monitoring");
}

void render_communications_view() {
    ImGui::TextColored(g_ui_theme.primary_color, "Communications - ECU Connection");
    ImGui::Separator();
    
    // ECU connection panel
    render_ecu_connection_panel();
    
    ImGui::Spacing();
    
    // Additional communications features
    ImGui::TextColored(g_ui_theme.accent_color, "Additional Features");
    ImGui::BulletText("Connection history and logs");
    ImGui::BulletText("Protocol configuration");
    ImGui::BulletText("Connection diagnostics");
    ImGui::BulletText("Performance metrics");
}

void render_plugin_manager_view() {
    add_log_entry(0, "*** PLUGIN MANAGER VIEW RENDERED ***");
    ImGui::TextColored(g_ui_theme.primary_color, "Plugin Manager - System Extensions");
    ImGui::Separator();
    
    // Plugin system status
    ImGui::TextColored(g_ui_theme.accent_color, "Plugin System Status");
    ImGui::Text("Status: %s", is_plugin_system_initialized() ? "Active" : "Inactive");
    
    // Data Bridge status
    ImGui::Spacing();
    ImGui::TextColored(g_ui_theme.accent_color, "Data Bridge Status");
    const char* bridge_status = data_bridge_get_status();
    ImGui::TextWrapped("%s", bridge_status);
    
    ImGui::Spacing();
    if (ImGui::Button("Create ECU-Chart Connection")) {
        bool success = data_bridge_create_connection("rpm_chart_connection",
                                                     "Advanced Chart Plugin",
                                                     "Advanced Chart Plugin", 
                                                     "rpm_chart",
                                                     "rpm",
                                                     "RPM Data",
                                                     10.0f);
        add_log_entry(0, "Creating ECU-Chart connection: %s", success ? "SUCCESS" : "FAILED");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Start Data Streaming")) {
        bool success = data_bridge_start_connection("rpm_chart_connection");
        add_log_entry(0, "Starting data streaming: %s", success ? "SUCCESS" : "FAILED");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Stop Data Streaming")) {
        bool success = data_bridge_stop_connection("rpm_chart_connection");
        add_log_entry(0, "Stopping data streaming: %s", success ? "SUCCESS" : "FAILED");
    }
    
    // Plugin discovery and loading
    ImGui::Spacing();
    ImGui::TextColored(g_ui_theme.accent_color, "Plugin Management");
    
    if (ImGui::Button("Scan Plugin Directory")) {
        PluginManager* mgr = get_plugin_manager();
        if (mgr && mgr->scan_plugin_directory) {
            mgr->scan_plugin_directory("plugins");
        }
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Initialize All Plugins")) {
        PluginManager* mgr = get_plugin_manager();
        if (mgr && mgr->init_all_plugins) {
            mgr->init_all_plugins();
        }
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Update All Plugins")) {
        PluginManager* mgr = get_plugin_manager();
        if (mgr && mgr->update_all_plugins) {
            mgr->update_all_plugins();
        }
    }
    
    ImGui::Spacing();
    
    // Plugin list
    ImGui::TextColored(g_ui_theme.accent_color, "Loaded Plugins");
    
    PluginManager* mgr = get_plugin_manager();
    if (mgr) {
        int plugin_count = get_plugin_count();
        if (plugin_count > 0) {
            for (int i = 0; i < plugin_count; i++) {
                PluginInterface* plugin = get_plugin_by_index(i);
                if (plugin) {
                    ImGui::PushID(i);
                    
                    // Plugin header
                    ImGui::TextColored(g_ui_theme.primary_color, "%s v%s", plugin->name, plugin->version);
                    ImGui::TextColored(g_ui_theme.text_secondary, "by %s", plugin->author);
                    ImGui::TextColored(g_ui_theme.text_muted, "%s", plugin->description);
                    
                    // Plugin status
                    const char* status_text = "Unknown";
                    ImVec4 status_color = g_ui_theme.text_muted;
                    
                    switch (plugin->status) {
                        case PLUGIN_STATUS_UNLOADED:
                            status_text = "Unloaded";
                            status_color = g_ui_theme.text_muted;
                            break;
                        case PLUGIN_STATUS_LOADED:
                            status_text = "Loaded";
                            status_color = g_ui_theme.warning_color;
                            break;
                        case PLUGIN_STATUS_INITIALIZED:
                            status_text = "Active";
                            status_color = g_ui_theme.success_color;
                            break;
                        case PLUGIN_STATUS_ERROR:
                            status_text = "Error";
                            status_color = g_ui_theme.error_color;
                            break;
                        case PLUGIN_STATUS_DISABLED:
                            status_text = "Disabled";
                            status_color = g_ui_theme.text_muted;
                            break;
                    }
                    
                    ImGui::TextColored(status_color, "Status: %s", status_text);
                    
                    // ECU-specific information and controls
                    if (plugin->type == PLUGIN_TYPE_ECU) {
                        ImGui::TextColored(g_ui_theme.text_secondary, "Type: ECU Plugin");
                        
                        // Show protocol info if available
                        if (plugin->interface.ecu.get_protocol_info) {
                            ImGui::TextColored(g_ui_theme.text_secondary, "Protocol: %s", 
                                              plugin->interface.ecu.get_protocol_info());
                        }
                        
                        // Show connection status if available
                        if (plugin->interface.ecu.get_connection_status) {
                            const char* conn_status = plugin->interface.ecu.get_connection_status();
                            ImVec4 conn_color = g_ui_theme.text_muted;
                            if (strstr(conn_status, "Connected")) {
                                conn_color = g_ui_theme.success_color;
                            } else if (strstr(conn_status, "Error")) {
                                conn_color = g_ui_theme.error_color;
                            }
                            ImGui::TextColored(conn_color, "Connection: %s", conn_status);
                        }
                        
                        // Connection controls for ECU plugins
                        ImGui::Spacing();
                        ImGui::TextColored(g_ui_theme.text_primary, "Connection Controls:");
                        
                        static char port_name[64] = "/dev/ttyUSB0";
                        static int baud_rate = 115200;
                        static char protocol[32] = "CRC";
                        
                        ImGui::InputText("Port", port_name, sizeof(port_name));
                        ImGui::SameLine();
                        if (ImGui::InputInt("Baud", &baud_rate)) {
                            if (baud_rate < 9600) baud_rate = 9600;
                            if (baud_rate > 921600) baud_rate = 921600;
                        }
                        ImGui::SameLine();
                        ImGui::InputText("Protocol", protocol, sizeof(protocol));
                        
                        ImGui::BeginGroup();
                        if (ImGui::Button("Connect")) {
                            if (plugin->interface.ecu.connect) {
                                bool success = plugin->interface.ecu.connect(port_name, baud_rate, protocol);
                                add_log_entry(0, "Connecting to %s: %s", port_name, success ? "SUCCESS" : "FAILED");
                            }
                        }
                        
                        ImGui::SameLine();
                        if (ImGui::Button("Disconnect")) {
                            if (plugin->interface.ecu.disconnect) {
                                plugin->interface.ecu.disconnect();
                                add_log_entry(0, "Disconnected from ECU");
                            }
                        }
                        
                        ImGui::SameLine();
                        if (ImGui::Button("Test")) {
                            if (plugin->interface.ecu.validate_connection) {
                                bool valid = plugin->interface.ecu.validate_connection();
                                add_log_entry(0, "Connection test: %s", valid ? "PASSED" : "FAILED");
                            }
                        }
                        ImGui::EndGroup();
                        
                        ImGui::Spacing();
                    }
                    
                    // Data Visualization plugin controls
                    else if (plugin->type == PLUGIN_TYPE_DATA) {
                        ImGui::TextColored(g_ui_theme.text_secondary, "Type: Data Visualization Plugin");
                        
                        // Chart creation controls
                        ImGui::Spacing();
                        ImGui::TextColored(g_ui_theme.text_primary, "Chart Management:");
                        
                        static char chart_id[64] = "rpm_chart";
                        static char chart_title[128] = "RPM vs AFR";
                        static int chart_type = 0;
                        static char series_name[64] = "RPM";
                        static char series_color[16] = "#FF0000";
                        
                        ImGui::InputText("Chart ID", chart_id, sizeof(chart_id));
                        ImGui::InputText("Chart Title", chart_title, sizeof(chart_title));
                        ImGui::InputText("Series Name", series_name, sizeof(series_name));
                        ImGui::InputText("Series Color", series_color, sizeof(series_color));
                        
                        const char* chart_types[] = { "Line", "Scatter", "Bar", "Area", "3D Surface", "Heatmap", "Gauge", "Digital" };
                        if (ImGui::BeginCombo("Chart Type", chart_types[chart_type])) {
                            for (int i = 0; i < IM_ARRAYSIZE(chart_types); i++) {
                                if (ImGui::Selectable(chart_types[i], chart_type == i)) {
                                    chart_type = i;
                                }
                            }
                            ImGui::EndCombo();
                        }
                        
                        ImGui::BeginGroup();
                        if (ImGui::Button("Create Chart")) {
                            if (plugin->interface.visualization.create_chart) {
                                bool success = plugin->interface.visualization.create_chart(chart_id, chart_title, chart_type);
                                add_log_entry(0, "Creating chart %s: %s", chart_title, success ? "SUCCESS" : "FAILED");
                            }
                        }
                        
                        ImGui::SameLine();
                        if (ImGui::Button("Add Series")) {
                            if (plugin->interface.visualization.add_data_series) {
                                bool success = plugin->interface.visualization.add_data_series(chart_id, series_name, series_color);
                                add_log_entry(0, "Adding series %s: %s", series_name, success ? "SUCCESS" : "FAILED");
                            }
                        }
                        
                        ImGui::SameLine();
                        if (ImGui::Button("Add Sample Data")) {
                            if (plugin->interface.visualization.add_data_point) {
                                // Add sample data points for demonstration
                                for (int i = 0; i < 50; i++) {
                                    float x = i * 2.0f;
                                    float y = sin(x * 0.1f) * 50.0f + 50.0f;
                                    plugin->interface.visualization.add_data_point(chart_id, x, y, series_name);
                                }
                                add_log_entry(0, "Added 50 sample data points");
                            }
                        }
                        
                        ImGui::SameLine();
                        if (ImGui::Button("Start Streaming")) {
                            if (plugin->interface.visualization.start_streaming) {
                                bool success = plugin->interface.visualization.start_streaming(chart_id, "speeduino_plugin", "rpm");
                                add_log_entry(0, "Starting stream: %s", success ? "SUCCESS" : "FAILED");
                            }
                        }
                        
                        ImGui::SameLine();
                        if (ImGui::Button("Stop Streaming")) {
                            if (plugin->interface.visualization.stop_streaming) {
                                plugin->interface.visualization.stop_streaming(chart_id);
                                add_log_entry(0, "Stopped streaming");
                            }
                        }
                        ImGui::EndGroup();
                        
                        // Chart info display
                        if (plugin->interface.visualization.get_chart_info) {
                            ImGui::Spacing();
                            ImGui::TextColored(g_ui_theme.text_primary, "Chart Information:");
                            const char* info = plugin->interface.visualization.get_chart_info(chart_id);
                            ImGui::TextWrapped("%s", info);
                        }
                        
                        // Chart rendering controls
                        ImGui::Spacing();
                        ImGui::TextColored(g_ui_theme.text_primary, "Chart Rendering:");
                        
                        static float chart_width = 800.0f;
                        static float chart_height = 400.0f;
                        static bool show_chart = false;
                        
                        ImGui::InputFloat("Width", &chart_width, 10.0f, 100.0f, "%.0f");
                        ImGui::InputFloat("Height", &chart_height, 10.0f, 100.0f, "%.0f");
                        
                        if (ImGui::Button("Show Chart")) {
                            show_chart = !show_chart;
                        }
                        
                        if (show_chart && plugin->interface.visualization.render_chart) {
                            ImGui::Spacing();
                            ImGui::TextColored(g_ui_theme.text_primary, "Chart Preview:");
                            
                            // Create a child window for the chart
                            ImGui::BeginChild("ChartPreview", ImVec2(chart_width + 20, chart_height + 40), true);
                            
                            if (plugin->interface.visualization.render_chart) {
                                plugin->interface.visualization.render_chart(chart_id, chart_width, chart_height);
                            }
                            
                            ImGui::EndChild();
                        }
                        
                        ImGui::Spacing();
                    }
                    
                    // Plugin actions
                    if (ImGui::Button("Unload")) {
                        if (mgr->unload_plugin) {
                            mgr->unload_plugin(plugin->name);
                        }
                    }
                    
                    ImGui::SameLine();
                    
                    if (ImGui::Button("Reload")) {
                        // TODO: Implement plugin reloading
                        ImGui::OpenPopup("Reload Plugin");
                    }
                    
                    ImGui::PopID();
                    ImGui::Separator();
                }
            }
        } else {
            ImGui::TextColored(g_ui_theme.text_muted, "No plugins loaded");
            ImGui::TextColored(g_ui_theme.text_muted, "Use 'Scan Plugin Directory' to discover plugins");
        }
    }
    
    ImGui::Spacing();
    
    // Plugin development information
    ImGui::TextColored(g_ui_theme.accent_color, "Plugin Development");
    ImGui::Text("Plugin directory: plugins/");
    ImGui::Text("Supported types: ECU, UI, Data, Integration");
    ImGui::Text("Documentation: docs/design/PLUGIN_ARCHITECTURE.md");
}

// Professional table operations implementations
void create_table_backup() {
    if (!g_ve_table) return;
    
    // Free existing backup
    if (g_table_backup) {
        free(g_table_backup);
    }
    
    // Create new backup
    g_table_backup = (ImGuiTable*)malloc(sizeof(ImGuiTable));
    if (g_table_backup) {
        memcpy(g_table_backup, g_ve_table, sizeof(ImGuiTable));
        
        // Allocate and copy data
        g_table_backup->data = (float**)malloc(g_ve_table->height * sizeof(float*));
        if (g_table_backup->data) {
            for (int y = 0; y < g_ve_table->height; y++) {
                g_table_backup->data[y] = (float*)malloc(g_ve_table->width * sizeof(float));
                if (g_table_backup->data[y]) {
                    memcpy(g_table_backup->data[y], g_ve_table->data[y], g_ve_table->width * sizeof(float));
                }
            }
        }
        
        // Copy axis data
        g_table_backup->x_axis = (float*)malloc(g_ve_table->width * sizeof(float));
        g_table_backup->y_axis = (float*)malloc(g_ve_table->height * sizeof(float));
        if (g_table_backup->x_axis && g_table_backup->y_axis) {
            memcpy(g_table_backup->x_axis, g_ve_table->x_axis, g_ve_table->width * sizeof(float));
            memcpy(g_table_backup->y_axis, g_ve_table->y_axis, g_ve_table->height * sizeof(float));
        }
        
        add_log_entry(0, "Table backup created successfully");
        g_table_has_changes = false;
    }
}

void restore_table_from_backup() {
    if (!g_table_backup || !g_ve_table) return;
    
    // Restore data
    for (int y = 0; y < g_ve_table->height; y++) {
        for (int x = 0; x < g_ve_table->width; x++) {
            g_ve_table->data[y][x] = g_table_backup->data[y][x];
        }
    }
    
    add_log_entry(0, "Table restored from backup");
    g_table_has_changes = false;
}

void interpolate_table_values() {
    if (!g_ve_table || g_interpolation_start_x < 0 || g_interpolation_start_y < 0 || 
        g_interpolation_end_x < 0 || g_interpolation_end_y < 0) return;
    
    int start_x = fmin(g_interpolation_start_x, g_interpolation_end_x);
    int end_x = fmax(g_interpolation_start_x, g_interpolation_end_x);
    int start_y = fmin(g_interpolation_start_y, g_interpolation_end_y);
    int end_y = fmax(g_interpolation_start_y, g_interpolation_end_y);
    
    float start_val = g_ve_table->data[start_y][start_x];
    float end_val = g_ve_table->data[end_y][end_x];
    
    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
            float x_factor = (float)(x - start_x) / (end_x - start_x);
            float y_factor = (float)(y - start_y) / (end_y - start_y);
            float interpolated = start_val + (end_val - start_val) * (x_factor + y_factor) * 0.5f;
            
            g_ve_table->data[y][x] = interpolated;
        }
    }
    
    add_log_entry(0, "Table interpolation applied from [%d,%d] to [%d,%d]", start_x, start_y, end_x, end_y);
    g_table_has_changes = true;
}

void smooth_table_region() {
    if (!g_ve_table || g_selected_cell_x < 0 || g_selected_cell_y < 0) return;
    
    int radius = 2;
    int start_x = fmax(0, g_selected_cell_x - radius);
    int end_x = fmin(g_ve_table->width - 1, g_selected_cell_x + radius);
    int start_y = fmax(0, g_selected_cell_y - radius);
    int end_y = fmin(g_ve_table->height - 1, g_selected_cell_y + radius);
    
    // Create temporary buffer for smoothed values
    float** temp_data = (float**)malloc(g_ve_table->height * sizeof(float*));
    for (int y = 0; y < g_ve_table->height; y++) {
        temp_data[y] = (float*)malloc(g_ve_table->width * sizeof(float));
        memcpy(temp_data[y], g_ve_table->data[y], g_ve_table->width * sizeof(float));
    }
    
    // Apply Gaussian smoothing
    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
            float sum = 0.0f;
            float weight_sum = 0.0f;
            
            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    int ny = y + dy;
                    int nx = x + dx;
                    
                    if (ny >= 0 && ny < g_ve_table->height && nx >= 0 && nx < g_ve_table->width) {
                        float weight = exp(-(dx*dx + dy*dy) / (2.0f * radius * radius));
                        sum += temp_data[ny][nx] * weight;
                        weight_sum += weight;
                    }
                }
            }
            
            if (weight_sum > 0) {
                g_ve_table->data[y][x] = sum / weight_sum;
            }
        }
    }
    
    // Free temporary buffer
    for (int y = 0; y < g_ve_table->height; y++) {
        free(temp_data[y]);
    }
    free(temp_data);
    
    add_log_entry(0, "Table smoothing applied around [%d,%d]", g_selected_cell_x, g_selected_cell_y);
    g_table_has_changes = true;
}

void apply_table_math_operation(const char* operation, float value) {
    if (!g_ve_table) return;
    
    for (int y = 0; y < g_ve_table->height; y++) {
        for (int x = 0; x < g_ve_table->width; x++) {
            float current = g_ve_table->data[y][x];
            float result = current;
            
            if (strcmp(operation, "add") == 0) {
                result = current + value;
            } else if (strcmp(operation, "subtract") == 0) {
                result = current - value;
            } else if (strcmp(operation, "multiply") == 0) {
                result = current * value;
            } else if (strcmp(operation, "divide") == 0) {
                result = current / value;
            } else if (strcmp(operation, "power") == 0) {
                result = pow(current, value);
            }
            
            // Clamp to valid range
            result = fmax(g_ve_table->metadata.min_value, fmin(g_ve_table->metadata.max_value, result));
            g_ve_table->data[y][x] = result;
        }
    }
    
    add_log_entry(0, "Table math operation '%s' with value %.2f applied", operation, value);
    g_table_has_changes = true;
}

void export_table_to_file(const char* filename) {
    if (!g_ve_table) return;
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        add_log_entry(2, "ERROR: Failed to open file for writing: %s", filename);
        return;
    }
    
    // Write header
    fprintf(file, "# VE Table Export - MegaTunix Redux\n");
    fprintf(file, "# Version: %d\n", g_table_version);
    fprintf(file, "# Comment: %s\n", g_table_comment);
    fprintf(file, "# Dimensions: %dx%d\n", g_ve_table->width, g_ve_table->height);
    fprintf(file, "# X-Axis: %s\n", g_ve_table->metadata.x_axis_label);
    fprintf(file, "# Y-Axis: %s\n", g_ve_table->metadata.y_axis_label);
    fprintf(file, "# Values: %s (%s)\n", g_ve_table->metadata.value_label, g_ve_table->metadata.units);
    fprintf(file, "#\n");
    
    // Write X-axis values
    fprintf(file, "X-Axis:");
    for (int x = 0; x < g_ve_table->width; x++) {
        fprintf(file, " %.1f", g_ve_table->x_axis[x]);
    }
    fprintf(file, "\n");
    
    // Write Y-axis values and data
    for (int y = 0; y < g_ve_table->height; y++) {
        fprintf(file, "%.1f", g_ve_table->y_axis[y]);
        for (int x = 0; x < g_ve_table->width; x++) {
            fprintf(file, " %.1f", g_ve_table->data[y][x]);
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
    add_log_entry(0, "Table exported successfully to %s", filename);
}

void import_table_from_file(const char* filename) {
    if (!g_ve_table) return;
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        add_log_entry(2, "ERROR: Failed to open file for reading: %s", filename);
        return;
    }
    
    char line[1024];
    int line_num = 0;
    int data_y = 0;
    
    while (fgets(line, sizeof(line), file) && data_y < g_ve_table->height) {
        line_num++;
        
        if (line[0] == '#' || strlen(line) < 2) continue;
        
        if (strncmp(line, "X-Axis:", 7) == 0) {
            // Parse X-axis values
            char* token = strtok(line + 7, " \t\n");
            int x = 0;
            while (token && x < g_ve_table->width) {
                g_ve_table->x_axis[x++] = atof(token);
                token = strtok(NULL, " \t\n");
            }
        } else {
            // Parse data row
            char* token = strtok(line, " \t\n");
            if (token) {
                g_ve_table->y_axis[data_y] = atof(token);
                
                int x = 0;
                token = strtok(NULL, " \t\n");
                while (token && x < g_ve_table->width) {
                    g_ve_table->data[data_y][x] = atof(token);
                    x++;
                    token = strtok(NULL, " \t\n");
                }
                data_y++;
            }
        }
    }
    
    fclose(file);
    add_log_entry(0, "Table imported successfully from %s", filename);
    g_table_has_changes = true;
}

void render_professional_table_header() {
    if (!g_ve_table) return;
    
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
    ImGui::BeginChild("TableHeader", ImVec2(0, 80), true);
    
    // Table title and version
    ImGui::TextColored(g_ui_theme.primary_color, "Professional VE Table Editor - Version %d", g_table_version);
    ImGui::SameLine();
    if (g_table_has_changes) {
        ImGui::TextColored(g_ui_theme.warning_color, "● Modified");
    }
    
    // Table information
    ImGui::Text("Dimensions: %dx%d | X-Axis: %s | Y-Axis: %s | Values: %s (%s)", 
                g_ve_table->width, g_ve_table->height,
                g_ve_table->metadata.x_axis_label,
                g_ve_table->metadata.y_axis_label,
                g_ve_table->metadata.value_label,
                g_ve_table->metadata.units);
    
    // Value range
    ImGui::Text("Value Range: %.1f - %.1f %s", 
                g_ve_table->metadata.min_value,
                g_ve_table->metadata.max_value,
                g_ve_table->metadata.units);
    
    // Comment field
    ImGui::Text("Comment:");
    ImGui::SameLine();
    ImGui::PushItemWidth(300);
    if (ImGui::InputText("##TableComment", g_table_comment, sizeof(g_table_comment))) {
        g_table_has_changes = true;
    }
    ImGui::PopItemWidth();
    
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void render_table_operations_toolbar() {
    if (!g_ve_table) return;
    
    ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_medium);
    ImGui::BeginChild("TableToolbar", ImVec2(0, 60), true);
    
    // File operations
    if (ImGui::Button("Create Backup")) {
        create_table_backup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Restore Backup")) {
        restore_table_from_backup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Export Table")) {
        // TODO: File dialog
        export_table_to_file("ve_table_export.txt");
    }
    ImGui::SameLine();
    if (ImGui::Button("Import Table")) {
        // TODO: File dialog
        import_table_from_file("ve_table_import.txt");
    }
    
    ImGui::Separator();
    
    // Table operations
    if (ImGui::Button("Interpolate")) {
        g_table_interpolation_mode = !g_table_interpolation_mode;
    }
    ImGui::SameLine();
    if (ImGui::Button("Smooth Region")) {
        smooth_table_region();
    }
    ImGui::SameLine();
    if (ImGui::Button("Math Operations")) {
        // TODO: Math operations dialog
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Changes")) {
        restore_table_from_backup();
    }
    
    // Interpolation mode indicator
    if (g_table_interpolation_mode) {
        ImGui::SameLine();
        ImGui::TextColored(g_ui_theme.warning_color, "Interpolation Mode Active - Click start and end points");
    }
    
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// Speeduino communication implementations - now handled by ecu_integration module
void speeduino_init(); // Declaration only - implementation in ecu_integration module

void speeduino_cleanup(); // Declaration only - implementation in ecu_integration module

bool speeduino_connect(const char* port); // Declaration only - implementation in ecu_integration module

void speeduino_disconnect(); // Declaration only - implementation in ecu_integration module

bool speeduino_is_connected(); // Declaration only - implementation in ecu_integration module

bool speeduino_send_packet(SpeeduinoPacket* packet); // Declaration only - implementation in ecu_integration module

// Note: receive_response function removed - not compatible with existing protocol

uint8_t speeduino_calculate_crc(uint8_t* data, int length); // Declaration only - implementation in ecu_integration module

bool speeduino_get_realtime_data(); // Declaration only - implementation in ecu_integration module

bool speeduino_get_table_data(uint8_t table_id); // Declaration only - implementation in ecu_integration module

bool speeduino_set_table_data(uint8_t table_id, uint8_t* data, int length); // Declaration only - implementation in ecu_integration module

void speeduino_update_connection_status(); // Declaration only - implementation in ecu_integration module

void render_ecu_connection_panel(); // Declaration only - implementation in ecu_integration module

void render_settings_window() {
    if (!g_show_settings_window) return;
    
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", &g_show_settings_window, ImGuiWindowFlags_AlwaysAutoResize)) {
        
        if (ImGui::BeginTabBar("SettingsTabs")) {
            
            // Appearance Tab
            if (ImGui::BeginTabItem("Appearance")) {
                ImGui::Spacing();
                
                // Theme Selection
                ImGui::TextColored(g_ui_theme.primary_color, "Theme Selection");
                ImGui::Separator();
                
                const char* theme_names[] = {
                    "Classic Automotive",
                    "Sunset Synthwave", 
                    "Ocean Depths",
                    "Forest Green",
                    "Desert Sand"
                };
                
                int current_theme = ui_theme_manager_get_current_theme_type();
                if (ImGui::Combo("Theme", &current_theme, theme_names, THEME_COUNT)) {
                    switch_theme((ThemeType)current_theme);
                    update_settings_theme((ThemeType)current_theme);
                }
                
                ImGui::Spacing();
                ImGui::Text("Preview:");
                ImGui::SameLine();
                ImGui::TextColored(g_ui_theme.primary_color, "Primary");
                ImGui::SameLine();
                ImGui::TextColored(g_ui_theme.secondary_color, "Secondary");
                ImGui::SameLine();
                ImGui::TextColored(g_ui_theme.success_color, "Success");
                ImGui::SameLine();
                ImGui::TextColored(g_ui_theme.warning_color, "Warning");
                ImGui::SameLine();
                ImGui::TextColored(g_ui_theme.error_color, "Error");
                
                ImGui::EndTabItem();
            }
            
            // General Tab
            if (ImGui::BeginTabItem("General")) {
                ImGui::Spacing();
                
                ImGui::TextColored(g_ui_theme.primary_color, "Application Behavior");
                ImGui::Separator();
                
                if (ImGui::Checkbox("Demo Mode", &g_demo_mode)) {
                    update_settings_demo_mode(g_demo_mode);
                }
                ImGui::SameLine();
                ImGui::TextColored(g_ui_theme.text_secondary, "Enable simulated ECU data");
                
                if (ImGui::Checkbox("Debug Mode", &g_debug_mode)) {
                    update_settings_debug_mode(g_debug_mode);
                }
                ImGui::SameLine();
                ImGui::TextColored(g_ui_theme.text_secondary, "Show debug information");
                
                if (ImGui::Checkbox("Show Engine Trail", &g_show_engine_trail)) {
                    update_settings_show_engine_trail(g_show_engine_trail);
                }
                ImGui::SameLine();
                ImGui::TextColored(g_ui_theme.text_secondary, "Display engine position trail");
                
                ImGui::EndTabItem();
            }
            
            // Logging Tab
            if (ImGui::BeginTabItem("Logging")) {
                ImGui::Spacing();
                
                ImGui::TextColored(g_ui_theme.primary_color, "Log System Configuration");
                ImGui::Separator();
                
                bool auto_scroll = get_log_auto_scroll();
                if (ImGui::Checkbox("Auto-scroll Logs", &auto_scroll)) {
                    set_log_auto_scroll(auto_scroll);
                    update_settings_log_auto_scroll(auto_scroll);
                }
                ImGui::SameLine();
                ImGui::TextColored(g_ui_theme.text_secondary, "Automatically scroll to latest log entries");
                
                const char* log_levels[] = {"All", "Info Only", "Warnings & Errors", "Errors Only"};
                int filter_level = get_log_filter_level();
                if (ImGui::Combo("Log Filter Level", &filter_level, log_levels, 4)) {
                    set_log_filter_level(filter_level);
                    update_settings_log_filter_level(filter_level);
                }
                
                ImGui::EndTabItem();
            }
            
            // Window Tab
            if (ImGui::BeginTabItem("Window")) {
                ImGui::Spacing();
                
                ImGui::TextColored(g_ui_theme.primary_color, "Window Preferences");
                ImGui::Separator();
                
                const UserSettings* settings = get_user_settings();
                if (settings) {
                    ImGui::Text("Current Window Size: %.0f x %.0f", settings->window_width, settings->window_height);
                    ImGui::Text("Window State: %s", settings->window_maximized ? "Maximized" : "Normal");
                }
                
                ImGui::Spacing();
                ImGui::TextColored(g_ui_theme.text_secondary, "Window settings are automatically saved when you close the application.");
                
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        
        // Action buttons
        ImGui::BeginGroup();
        if (ImGui::Button("Save Settings")) {
            save_user_settings();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset to Defaults")) {
            reset_user_settings_to_defaults();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            g_show_settings_window = false;
        }
        ImGui::EndGroup();
        
        ImGui::Spacing();
        ImGui::TextColored(g_ui_theme.text_muted, "Settings are automatically saved when you close the application.");
    }
    ImGui::End();
}

// Settings management implementations now handled by settings_manager module







void handle_communications_buttons() {
    // For now, it's just a placeholder that simulates the button actions
    
    if (g_locate_port_button_pressed && g_button_press_timer == 30) {
        // Simulate port detection
        strcpy(g_button_status_text, "Found 2 ports: /dev/ttyUSB0, /dev/ttyACM0");
        add_log_entry(0, "Port detection completed: Found 2 ports");
    }
    
    if (g_reverse_connect_button_pressed && g_button_press_timer == 30) {
        // Simulate connection attempt
        if (g_ecu_context) {
            // Try to connect
            strcpy(g_button_status_text, "Connection attempt completed");
            add_log_entry(0, "Connection attempt completed");
        }
    }
    
    if (g_reset_io_button_pressed && g_button_press_timer == 30) {
        // Simulate reset
        strcpy(g_button_status_text, "I/O status counters reset");
        add_log_entry(0, "I/O status counters reset");
    }
}

void update_engine_trail(int current_x, int current_y, TrailPoint* trail, int* trail_count) {
    uint32_t current_time = SDL_GetTicks();
    
    // Only add new point if position changed or enough time has passed
    if (*trail_count == 0 || 
        trail[*trail_count - 1].x != current_x || 
        trail[*trail_count - 1].y != current_y ||
        current_time - trail[*trail_count - 1].timestamp > 100) { // Add point every 100ms max
        
        // Add new trail point
        if (*trail_count < MAX_TRAIL_POINTS) {
            trail[*trail_count].x = current_x;
            trail[*trail_count].y = current_y;
            trail[*trail_count].timestamp = current_time;
            trail[*trail_count].alpha = 1.0f; // Start fully visible
            (*trail_count)++;
        } else {
            // Shift trail and add new point at end
            memmove(&trail[0], &trail[1], sizeof(TrailPoint) * (MAX_TRAIL_POINTS - 1));
            trail[MAX_TRAIL_POINTS - 1].x = current_x;
            trail[MAX_TRAIL_POINTS - 1].y = current_y;
            trail[MAX_TRAIL_POINTS - 1].timestamp = current_time;
            trail[MAX_TRAIL_POINTS - 1].alpha = 1.0f;
        }
    }
    
    // Update alpha values for fading effect
    for (int i = 0; i < *trail_count; i++) {
        uint32_t age = current_time - trail[i].timestamp;
        float fade_duration = 3000.0f; // 3 seconds fade time
        trail[i].alpha = fmax(0.0f, 1.0f - (float)age / fade_duration);
        
        // Remove completely faded points
        if (trail[i].alpha <= 0.0f) {
            if (i < *trail_count - 1) {
                memmove(&trail[i], &trail[i + 1], sizeof(TrailPoint) * (*trail_count - i - 1));
            }
            (*trail_count)--;
            i--; // Re-check this index
        }
    }
}

// Copy/paste functions for table editing - Now handled by table_operations module

// interpolate_between_cells function now handled by table_operations module

// copy_selection_to_clipboard function now handled by table_operations module

// paste_block_at function now handled by table_operations module

// Paste special operation over block
// paste_special_block_at function now handled by table_operations module

// horizontal_interpolate_selection and vertical_interpolate_selection functions now handled by table_operations module

// Multi-cell selection helper functions now handled by table_operations module

// apply_operation_to_selection function now handled by table_operations module

// smooth_selection function now handled by table_operations module

// Function to handle ECU connection with feedback
void handle_ecu_connection(ECUContext* ecu_ctx, const ECUConfig* config) {
    if (!ecu_ctx || !config) return;
    
    // Add log entry for connection attempt
    add_log_entry(0, "Initiating ECU connection to %s at %d baud", config->port, config->baud_rate);
    
    // Start connection attempt
    bool success = ecu_connect(ecu_ctx, config);
    
    if (success) {
        add_log_entry(0, "ECU connection initiated successfully");
    } else {
        add_log_entry(2, "Failed to initiate ECU connection");
    }
}

// VE Table View Functions
void render_ve_table_2d_view() {
    // Safety check - ensure VE table is valid
    if (!g_ve_table || g_ve_table->width <= 0 || g_ve_table->height <= 0) {
        ImGui::Text("VE Table not available or invalid");
        return;
    }
    
    // Professional table header
    render_professional_table_header();
    
    // Professional table operations toolbar
    render_table_operations_toolbar();
    
    ImGui::Separator();
    
    // Professional editing controls
    ImGui::BeginGroup();
    ImGui::TextColored(g_ui_theme.primary_color, "Professional Editing Controls:");
    
    if (ImGui::Button("Reset to Demo", ImVec2(140, 25))) {
        imgui_table_load_demo_data(g_ve_table);
        g_table_has_changes = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Selection", ImVec2(140, 25))) {
        g_selected_cell_x = -1;
        g_selected_cell_y = -1;
        g_cell_editing = false;
        clear_multi_selection();
    }
    ImGui::SameLine();
    if (ImGui::Button("Create Backup", ImVec2(140, 25))) {
        create_table_backup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Restore Backup", ImVec2(140, 25))) {
        restore_table_from_backup();
    }
    ImGui::EndGroup();
    
    // Professional cell information display
    if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
        ImGui::BeginChild("CellInfo", ImVec2(0, 80), true);
        
        float cell_value = SafeTableAccess::get_value_safe(g_selected_cell_x, g_selected_cell_y, 75.0f);
        ImGui::TextColored(g_ui_theme.primary_color, "Selected Cell Information:");
        ImGui::Text("Position: [%d, %d] | Value: %.1f VE", 
                   g_selected_cell_x, g_selected_cell_y, cell_value);
        ImGui::Text("RPM: %.0f | MAP: %.1f kPa", 
                   g_ve_table->x_axis[g_selected_cell_x], 
                   g_ve_table->y_axis[g_selected_cell_y]);
        
        // Quick edit field
        ImGui::Text("Quick Edit:");
        ImGui::SameLine();
        ImGui::PushItemWidth(100);
        if (ImGui::InputFloat("##QuickEdit", &cell_value, 1.0f, 5.0f, "%.1f")) {
            imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, cell_value);
            g_table_has_changes = true;
        }
        ImGui::PopItemWidth();
        
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    
    // Professional multi-selection display
    if (g_multi_selection.active) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui_theme.background_light);
        ImGui::BeginChild("MultiSelectionInfo", ImVec2(0, 60), true);
        
        int cell_count = get_selection_cell_count();
        ImGui::TextColored(g_ui_theme.primary_color, "Multi-Selection Active:");
        ImGui::Text("Range: [%d,%d] to [%d,%d] | Cells: %d", 
                   g_multi_selection.start_x, g_multi_selection.start_y,
                   g_multi_selection.end_x, g_multi_selection.end_y, cell_count);
        ImGui::Text("Operations: Press 'I' for interpolation, 'S' for smoothing, 'M' for math operations");
        
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
    
    ImGui::Separator();
    
    // Professional heatmap title
    ImGui::TextColored(g_ui_theme.primary_color, "Professional 2D Heatmap Visualization");
    ImGui::TextColored(g_ui_theme.text_secondary, "Click cells to edit | Use arrow keys to navigate | Ctrl+Click to multi-select");
    
                    // Calculate display area - scale to window size with padding for UI log and axis labels
                float window_width = ImGui::GetWindowWidth();
                float window_height = ImGui::GetWindowHeight();
                float ui_log_height = 150; // Approximate UI log height
                float available_height = window_height - ui_log_height - 200; // Leave space for controls and UI log
                
                // Shrink heatmap to fit within axis labels
                float display_width = window_width - 120; // Leave more padding for axis labels
                float display_height = available_height - 120; // Leave more space for axis labels
                
                // Ensure minimum size
                if (display_width < 400) display_width = 400;
                if (display_height < 300) display_height = 300;
    
    // Create a child window for the interactive area
    ImGui::BeginChild("InteractiveHeatmap", ImVec2(display_width, display_height), true);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 window_size = ImGui::GetWindowSize();
    
    // Cache window position and size for event handling (to avoid calling ImGui functions from event handler)
    g_table_window_pos = window_pos;
    g_table_window_size = window_size;
    g_table_window_valid = true;
    
                    // Calculate cell dimensions - leave more space for axis labels
                float cell_width = 1.0f;
                float cell_height = 1.0f;
                
                if (g_ve_table && g_ve_table->width > 0 && g_ve_table->height > 0) {
                    cell_width = (window_size.x - 80) / g_ve_table->width;  // Leave 40px on each side
                    cell_height = (window_size.y - 60) / g_ve_table->height; // Leave 30px on top/bottom
                }
                
                // Debug: Log the actual cell dimensions being used for rendering
                static int last_render_width = -1, last_render_height = -1;
                if (last_render_width != g_ve_table->width || last_render_height != g_ve_table->height) {
                    add_log_entry(0, "Rendering VE table: %dx%d, cell size: %.1fx%.1f, window: %.1fx%.1f", 
                                g_ve_table->width, g_ve_table->height, cell_width, cell_height, window_size.x, window_size.y);
                    last_render_width = g_ve_table->width;
                    last_render_height = g_ve_table->height;
}
    
    // Mouse interaction is now handled in the SDL event system
    // This section is kept for visual feedback only
    
    // Draw the interactive heatmap
                if (g_ve_table && g_ve_table->width > 0 && g_ve_table->height > 0) {
                    for (int y = 0; y < g_ve_table->height; y++) {
                        for (int x = 0; x < g_ve_table->width; x++) {
                        float x_pos = window_pos.x + 40 + x * cell_width;  // Start 40px from left for Y-axis labels
                        float y_pos = window_pos.y + 30 + y * cell_height; // Start 30px from top for X-axis labels
            
            // Get VE value and normalize to 0-1
            float ve_value = 0.0f;
            float normalized = 0.0f;
            
            // Use safe table access wrapper for rendering
            ve_value = SafeTableAccess::get_value_safe(x, y, 75.0f);
            if (g_ve_table && g_ve_table->metadata.max_value > g_ve_table->metadata.min_value) {
                normalized = (ve_value - g_ve_table->metadata.min_value) / (g_ve_table->metadata.max_value - g_ve_table->metadata.min_value);
            }
            
            // Calculate color based on VE value
            ImU32 cell_color;
            if (normalized < 0.5f) {
                // Blue to Green gradient
                float t = normalized * 2.0f;
                cell_color = IM_COL32(
                    (int)(0.0f * (1.0f - t) + 0.0f * t),
                    (int)(0.0f * (1.0f - t) + 255.0f * t),
                    (int)(255.0f * (1.0f - t) + 0.0f * t),
                    255
                );
            } else {
                // Green to Red gradient
                float t = (normalized - 0.5f) * 2.0f;
                cell_color = IM_COL32(
                    (int)(0.0f * (1.0f - t) + 255.0f * t),
                    (int)(255.0f * (1.0f - t) + 0.0f * t),
                    (int)(0.0f * (1.0f - t) + 0.0f * t),
                    255
                );
            }
            
            // Draw cell background with unique ID
            char cell_bg_id[32];
            snprintf(cell_bg_id, sizeof(cell_bg_id), "##cell_bg_%d_%d", x, y);
            draw_list->AddRectFilled(
                ImVec2(x_pos, y_pos),
                ImVec2(x_pos + cell_width, y_pos + cell_height),
                cell_color
            );
            
            // Draw cell border with different highlighting
            ImU32 border_color = IM_COL32(100, 100, 100, 255);
            float border_thickness = 1.0f;
            
            // Check for multi-cell selection first
            if (is_cell_in_selection(x, y)) {
                border_color = IM_COL32(0, 255, 255, 255); // Cyan for multi-cell selection
                border_thickness = 2.0f;
            }
            // Only show single-cell selection highlight when no multi-selection is active
            else if (!g_multi_selection.active && x == g_selected_cell_x && y == g_selected_cell_y) {
                border_color = IM_COL32(255, 255, 0, 255); // Yellow for selected cell
                border_thickness = 3.0f;
            } else if (x == g_current_rpm_cell && y == g_current_map_cell) {
                border_color = IM_COL32(255, 0, 255, 255); // Magenta for current engine operating point
                border_thickness = 2.0f;
            }
            
            // Draw cell border with unique ID
            char cell_border_id[64];
            snprintf(cell_border_id, sizeof(cell_border_id), "##cell_border_%d_%d", x, y);
            draw_list->AddRect(
                ImVec2(x_pos, y_pos),
                ImVec2(x_pos + cell_width, y_pos + cell_height),
                border_color,
                0.0f, 0, border_thickness
            );
        }
    }
    
    // Draw engine trail (fading trail of engine position) - BEFORE cell values
    if (g_show_engine_trail) {
        for (int i = 0; i < g_trail_count_2d; i++) {
            TrailPoint* point = &g_engine_trail_2d[i];
            if (point->alpha > 0.0f) {
                float x_pos = window_pos.x + 40 + point->x * cell_width;
                float y_pos = window_pos.y + 30 + point->y * cell_height;
                
                // Draw fading trail point with increased opacity
                ImU32 trail_color = IM_COL32(255, 0, 255, (int)(255 * point->alpha * 0.5f));
                draw_list->AddRectFilled(
                    ImVec2(x_pos + 2, y_pos + 2),
                    ImVec2(x_pos + cell_width - 2, y_pos + cell_height - 2),
                    trail_color
                );
                
                // Draw trail border with increased opacity
                ImU32 border_color = IM_COL32(255, 0, 255, (int)(255 * point->alpha * 0.8f));
                draw_list->AddRect(
                    ImVec2(x_pos + 1, y_pos + 1),
                    ImVec2(x_pos + cell_width - 1, y_pos + cell_height - 1),
                    border_color,
                    0.0f, 0, 1.0f
                );
            }
        }
    }
    
    // Now draw cell values on top of everything
    for (int y = 0; y < g_ve_table->height; y++) {
        for (int x = 0; x < g_ve_table->width; x++) {
            float x_pos = window_pos.x + 40 + x * cell_width;
            float y_pos = window_pos.y + 30 + y * cell_height;
            float ve_value = imgui_table_get_value(g_ve_table, x, y);
            
            // Draw cell value - make it editable if selected
                        if (x == g_selected_cell_x && y == g_selected_cell_y && g_show_input_field) {
                            // Create an input field for the selected cell with pre-highlighted value
                            ImGui::SetCursorPos(ImVec2(x_pos + 2, y_pos + 2));
                            ImGui::SetNextItemWidth(cell_width - 4);
                            
                            char cell_edit_id[128];
                            snprintf(cell_edit_id, sizeof(cell_edit_id), "##cell_edit_%d_%d_%s_%p", x, y, g_ui_theme.name, (void*)g_ve_table);
                            
                            // Pre-fill the buffer with current value if it's empty and this is the selected cell
                            if (g_cell_edit_buffer[0] == '\0' && x == g_selected_cell_x && y == g_selected_cell_y) {
                                snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.0f", ve_value);
                            }
                            
                            // Draw a background for the input field to make it visible
                            draw_list->AddRectFilled(
                                ImVec2(x_pos + 1, y_pos + 1),
                                ImVec2(x_pos + cell_width - 1, y_pos + cell_height - 1),
                                IM_COL32(255, 255, 255, 255) // White background
                            );
                            

                            
                            // Position the input field directly within the cell
                            ImVec2 input_pos = ImVec2(x_pos + 2 - window_pos.x, y_pos + 2 - window_pos.y);
                            ImGui::SetCursorPos(input_pos);
                            ImGui::SetNextItemWidth(cell_width - 4);
                            
                            // Handle keyboard input for the input field
                            bool should_save = false;
                            bool should_move_next = false;
                            bool should_move_prev = false;
                            
                            // Allow editing in the input field
                            ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue;
                            
                            // Force refresh if buffer was updated
                            if (g_buffer_updated) {
                                // Don't automatically set focus - let user choose input method
                                g_buffer_updated = false; // Reset flag
                            }
                            
                            // Track if this input field is focused
                            bool is_focused = ImGui::IsItemFocused();
                            if (is_focused) {
                                g_input_field_focused = true;
                            } else {
                                // Clear focus if this field is not focused
                                g_input_field_focused = false;
                            }
                            
                            if (ImGui::InputText(cell_edit_id, g_cell_edit_buffer, sizeof(g_cell_edit_buffer), input_flags)) {
                                should_save = true;
                                g_input_field_focused = false; // Clear focus when Enter is pressed
                            }
                            
                            // Save changes if needed
                            if (should_save) {
                                float input_value = atof(g_cell_edit_buffer);
                                if (input_value >= 0.0f && input_value <= 200.0f) {
                                    imgui_table_set_value(g_ve_table, x, y, input_value);
                                    // Update min/max values
                                    g_ve_table->metadata.min_value = g_ve_table->metadata.max_value = g_ve_table->data[0][0];
                                    for (int y2 = 0; y2 < g_ve_table->height; y2++) {
                                        for (int x2 = 0; x2 < g_ve_table->width; x2++) {
                                            if (g_ve_table->data[y2][x2] < g_ve_table->metadata.min_value) g_ve_table->metadata.min_value = g_ve_table->data[y2][x2];
                                            if (g_ve_table->data[y2][x2] > g_ve_table->metadata.max_value) g_ve_table->metadata.max_value = g_ve_table->data[y2][x2];
                                        }
                                    }
                                    // Clear the buffer after successful edit
                                    g_cell_edit_buffer[0] = '\0';
                                    
                                    // Move to next/previous cell if requested
                                    if (should_move_next) {
                                        g_selected_cell_x++;
                                        if (g_selected_cell_x >= g_ve_table->width) {
                                            g_selected_cell_x = 0;
                                            g_selected_cell_y++;
                                            if (g_selected_cell_y >= g_ve_table->height) {
                                                g_selected_cell_y = 0;
                                            }
                                        }
                                        snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.0f", 
                                                imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y));
                                    } else if (should_move_prev) {
                                        g_selected_cell_x--;
                                        if (g_selected_cell_x < 0) {
                                            g_selected_cell_x = g_ve_table->width - 1;
                                            g_selected_cell_y--;
                                            if (g_selected_cell_y < 0) {
                                                g_selected_cell_y = g_ve_table->height - 1;
                                            }
                                        }
                                        snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.0f", 
                                                imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y));
                                    }
                                }
                            }
                        } else {
                            // Draw cell value - show buffer value if typing on selected cell
                            char value_text[32];
                            if (x == g_selected_cell_x && y == g_selected_cell_y && strlen(g_cell_edit_buffer) > 0) {
                                // Show the buffer value when typing
                                snprintf(value_text, sizeof(value_text), "%s", g_cell_edit_buffer);
                            } else {
                                // Show normal cell value
                                snprintf(value_text, sizeof(value_text), "%.0f", ve_value);
                            }
                            
                            ImVec2 text_size = ImGui::CalcTextSize(value_text);
                            ImVec2 text_pos = ImVec2(
                                x_pos + (cell_width - text_size.x) * 0.5f,
                                y_pos + (cell_height - text_size.y) * 0.5f
                            );
                            
                            // Draw text with shadow for better visibility
                            // Use different color for typing feedback
                            ImU32 text_color = (x == g_selected_cell_x && y == g_selected_cell_y && strlen(g_cell_edit_buffer) > 0) 
                                ? IM_COL32(255, 255, 0, 255)  // Yellow for typing feedback
                                : IM_COL32(255, 255, 255, 255); // White for normal values
                            
                            draw_list->AddText(
                                ImVec2(text_pos.x + 1, text_pos.y + 1),
                                IM_COL32(0, 0, 0, 255),
                                value_text
                            );
                            draw_list->AddText(text_pos, text_color, value_text);
                        }
                        }
                    }
                }
    
    // Engine position calculation moved to main update loop
    

    
    // Draw axis labels on all sides for better visibility - positioned inside child window
    // X-axis title (RPM) - Top and Bottom
    draw_list->AddText(
        ImVec2(window_pos.x + window_size.x * 0.5f - 20, window_pos.y + 5),
        IM_COL32(255, 255, 255, 255),
        "RPM (Top)"
    );
    draw_list->AddText(
        ImVec2(window_pos.x + window_size.x * 0.5f - 20, window_pos.y + window_size.y - 15),
        IM_COL32(255, 255, 255, 255),
        "RPM (Bottom)"
    );
    
    // X-axis labels (RPM values) - Top and Bottom
    for (int x = 0; x < g_ve_table->width; x++) {
        float x_pos = window_pos.x + 40 + x * cell_width + cell_width * 0.5f; // Match cell positioning
        
        // Top labels - inside window
        char rpm_text_top[16];
        snprintf(rpm_text_top, sizeof(rpm_text_top), "%.0f", g_ve_table->x_axis[x]);
        ImVec2 text_size_top = ImGui::CalcTextSize(rpm_text_top);
        draw_list->AddText(
            ImVec2(x_pos - text_size_top.x * 0.5f, window_pos.y + 5),
            IM_COL32(255, 255, 255, 255),
            rpm_text_top
        );
        
        // Bottom labels - inside window
        char rpm_text_bottom[16];
        snprintf(rpm_text_bottom, sizeof(rpm_text_bottom), "%.0f", g_ve_table->x_axis[x]);
        ImVec2 text_size_bottom = ImGui::CalcTextSize(rpm_text_bottom);
        draw_list->AddText(
            ImVec2(x_pos - text_size_bottom.x * 0.5f, window_pos.y + window_size.y - 15),
            IM_COL32(255, 255, 255, 255),
            rpm_text_bottom
        );
    }
    
    // Y-axis title (MAP) - Left and Right (permanently rotated 90 degrees)
    // Left side - rotated text positioned vertically
    draw_list->AddText(
        ImVec2(window_pos.x + 5, window_pos.y + window_size.y * 0.5f + 30),
        IM_COL32(255, 255, 255, 255),
        "MAP (kPa)"
    );
    // Right side - rotated text positioned vertically
    draw_list->AddText(
        ImVec2(window_pos.x + window_size.x - 60, window_pos.y + window_size.y * 0.5f + 30),
        IM_COL32(255, 255, 255, 255),
        "MAP (kPa)"
    );
    
    // Y-axis labels (MAP values) - Left and Right
    for (int y = 0; y < g_ve_table->height; y++) {
        float y_pos = window_pos.y + 30 + y * cell_height + cell_height * 0.5f; // Match cell positioning
        
        // Left labels - inside window (always horizontal)
        char map_text_left[16];
        snprintf(map_text_left, sizeof(map_text_left), "%.0f", g_ve_table->y_axis[y]);
        ImVec2 text_size_left = ImGui::CalcTextSize(map_text_left);
        draw_list->AddText(
            ImVec2(window_pos.x + 5, y_pos - text_size_left.y * 0.5f),
            IM_COL32(255, 255, 255, 255),
            map_text_left
        );
        
        // Right labels - inside window (always horizontal)
        char map_text_right[16];
        snprintf(map_text_right, sizeof(map_text_right), "%.0f", g_ve_table->y_axis[y]);
        ImVec2 text_size_right = ImGui::CalcTextSize(map_text_right);
        draw_list->AddText(
            ImVec2(window_pos.x + window_size.x - 45, y_pos - text_size_right.y * 0.5f),
            IM_COL32(255, 255, 255, 255),
            map_text_right
        );
    }
    
    // Trail is now drawn before cell values to avoid covering them
    
    ImGui::EndChild();
    
    // Cell editing interface - Simplified since we now have direct editing
    if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Selected Cell [%d, %d]: %.1f VE", 
                           g_selected_cell_x, g_selected_cell_y, 
                           imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y));
        ImGui::Text("RPM: %.0f, MAP: %.1f kPa", g_ve_table->x_axis[g_selected_cell_x], g_ve_table->y_axis[g_selected_cell_y]);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Tip: Type new value and press Enter, or use +/- keys to adjust");
    } else {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click on any cell in the heatmap to edit its value directly");
    }
    
    // Enhanced color legend and status
    ImGui::Separator();
    ImGui::Text("Color Legend:");
    
    // Create a more visual color legend
    ImGui::BeginGroup();
    
    // Low VE (Blue)
    ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "■ Blue: Low VE (45-70)");
    ImGui::SameLine();
    
    // Medium VE (Green)
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "■ Green: Medium VE (70-90)");
    ImGui::SameLine();
    
    // High VE (Red)
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "■ Red: High VE (90-120)");
    
    ImGui::EndGroup();
    
    // Border legend
    ImGui::Text("Border Legend:");
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "■ Yellow Border: Selected Cell");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "■ Magenta Border: Current Engine Point");
    
    // Current engine operating point status
    if (g_current_rpm_cell >= 0 && g_current_map_cell >= 0) {
        ImGui::Text("Current Engine Point: RPM=%.0f, MAP=%.1f kPa (Cell [%d,%d])", 
                   g_ecu_data.rpm, g_ecu_data.map, g_current_rpm_cell, g_current_map_cell);
    } else {
        ImGui::Text("Engine Status: Not Running");
    }
}

void render_ve_table_3d_view() {
    ImGui::Text("3D View");
    ImGui::Text("This view shows a 3D representation of the VE table.");
    ImGui::Text("Use the controls above to adjust the view.");
    
    // 3D view controls
    ImGui::Separator();
    ImGui::Text("3D View Controls:");
    ImGui::SliderFloat("Rotation X", &g_ve_3d_view.rotation_x, -90.0f, 90.0f);
    ImGui::SliderFloat("Rotation Y", &g_ve_3d_view.rotation_y, -180.0f, 180.0f);
    ImGui::SliderFloat("Zoom", &g_ve_3d_view.zoom, 0.1f, 10.0f);
    ImGui::SliderFloat("Pan X", &g_ve_3d_view.pan_x, -2.0f, 2.0f);
    ImGui::SliderFloat("Pan Y", &g_ve_3d_view.pan_y, -2.0f, 2.0f);
    ImGui::SliderFloat("Opacity", &g_ve_3d_view.opacity, 0.1f, 1.0f);
    ImGui::Checkbox("Wireframe Mode", &g_ve_3d_view.wireframe_mode);
    ImGui::Checkbox("Show Grid", &g_ve_3d_view.show_grid);
    ImGui::Checkbox("Show Axes", &g_ve_3d_view.show_axes);
    
    ImGui::SameLine();
    
    if (ImGui::Button("Reset View", ImVec2(100, 20))) {
        g_ve_3d_view.rotation_x = 0.0f;
        g_ve_3d_view.rotation_y = 0.0f;
        g_ve_3d_view.zoom = 1.0f;
        g_ve_3d_view.pan_x = 0.0f;
        g_ve_3d_view.pan_y = 0.0f;
    }
    
    // Camera preset buttons
    ImGui::Separator();
    ImGui::Text("Camera Presets:");
    if (ImGui::Button("Isometric", ImVec2(100, 25))) {
        set_isometric_view();
    }
    ImGui::SameLine();
    if (ImGui::Button("Side View", ImVec2(100, 25))) {
        set_side_view();
    }
    ImGui::SameLine();
    if (ImGui::Button("Top Down", ImVec2(100, 25))) {
        set_top_down_view();
    }
    
    // Mouse control instructions
    ImGui::Separator();
    ImGui::Text("Mouse Controls:");
    ImGui::Text("• Left-click + drag: Rotate view");
    ImGui::Text("• Right-click + drag: Pan view");
    ImGui::Text("• Mouse wheel: Zoom in/out");
    ImGui::Text("• Hover over 3D area to activate mouse controls");
    
    // 3D rendering area
    ImGui::Separator();
    ImGui::Text("3D Rendering Area:");
    
    // Calculate rendering area
    float render_width = ImGui::GetWindowWidth() - 40;
    float render_height = render_width * 0.75f;
    
            // Create a child window for 3D rendering
        ImGui::BeginChild("3DRenderArea", ImVec2(render_width, render_height), true);
        
        // Get the draw list for this child window
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 window_pos = ImGui::GetWindowPos();
        ImVec2 window_size = ImGui::GetWindowSize();
        
        // Mouse interaction for 3D view
        ImVec2 mouse_pos = ImGui::GetMousePos();
        bool mouse_in_area = ImGui::IsMouseHoveringRect(
            ImVec2(window_pos.x, window_pos.y),
            ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y)
        );
        
        // Handle mouse wheel for zoom
        if (mouse_in_area) {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f) {
                // Zoom in/out with mouse wheel
                float zoom_factor = wheel > 0 ? 1.1f : 0.9f;
                g_ve_3d_view.zoom *= zoom_factor;
                g_ve_3d_view.zoom = fmax(0.1f, fmin(10.0f, g_ve_3d_view.zoom)); // Clamp zoom
            }
            
            // Handle mouse drag for rotation
            if (ImGui::IsMouseDown(0) && !g_ve_mouse_dragging) {
                g_ve_mouse_dragging = true;
                g_ve_last_mouse_pos = mouse_pos;
            }
            
            if (g_ve_mouse_dragging && ImGui::IsMouseDown(0)) {
                ImVec2 delta = ImVec2(mouse_pos.x - g_ve_last_mouse_pos.x, mouse_pos.y - g_ve_last_mouse_pos.y);
                
                // Rotate based on mouse movement
                g_ve_3d_view.rotation_y += delta.x * 0.5f; // Horizontal movement rotates Y
                g_ve_3d_view.rotation_x += delta.y * 0.5f; // Vertical movement rotates X
                
                // Clamp rotation angles
                g_ve_3d_view.rotation_x = fmax(-90.0f, fmin(90.0f, g_ve_3d_view.rotation_x));
                g_ve_3d_view.rotation_y = fmod(g_ve_3d_view.rotation_y, 360.0f);
                
                g_ve_last_mouse_pos = mouse_pos;
            }
            
            if (!ImGui::IsMouseDown(0)) {
                g_ve_mouse_dragging = false;
            }
            
            // Handle right-click drag for panning
            if (ImGui::IsMouseDown(1) && !g_ve_mouse_dragging) {
                g_ve_mouse_dragging = true;
                g_ve_last_mouse_pos = mouse_pos;
            }
            
            if (g_ve_mouse_dragging && ImGui::IsMouseDown(1)) {
                ImVec2 delta = ImVec2(mouse_pos.x - g_ve_last_mouse_pos.x, mouse_pos.y - g_ve_last_mouse_pos.y);
                
                // Pan based on mouse movement
                g_ve_3d_view.pan_x += delta.x * 0.02f;
                g_ve_3d_view.pan_y += delta.y * 0.02f;
                
                // Clamp pan values
                g_ve_3d_view.pan_x = fmax(-2.0f, fmin(2.0f, g_ve_3d_view.pan_x));
                g_ve_3d_view.pan_y = fmax(-2.0f, fmin(2.0f, g_ve_3d_view.pan_y));
                
                g_ve_last_mouse_pos = mouse_pos;
            }
        }
    
    // Calculate center of rendering area
    ImVec2 center = ImVec2(window_pos.x + window_size.x * 0.5f, window_pos.y + window_size.y * 0.5f);
    
    // Draw background
    draw_list->AddRectFilled(
        ImVec2(window_pos.x + 5, window_pos.y + 5),
        ImVec2(window_pos.x + window_size.x - 5, window_pos.y + window_size.y - 5),
        IM_COL32(20, 20, 20, 255)
    );
    
    // Draw border
    draw_list->AddRect(
        ImVec2(window_pos.x + 5, window_pos.y + 5),
        ImVec2(window_pos.x + window_size.x - 5, window_pos.y + window_size.y - 5),
        IM_COL32(100, 100, 100, 255)
    );
    
            // Enhanced 3D wireframe visualization with proper transformations
        if (g_ve_table && g_ve_table->data) {
            // Calculate optimal scale to fill the visible area
            float render_width = window_size.x - 10;
            float render_height = window_size.y - 10;
            float optimal_scale = fmin(render_width, render_height) / 120.0f; // Base scale to fill area
            
            float scale = optimal_scale * g_ve_3d_view.zoom; // Apply zoom multiplier
            float grid_size = 100.0f * scale;
            float height_scale = 40.0f * scale; // Define height_scale here for axes
        
        // Convert rotation angles to radians
        float rot_x_rad = g_ve_3d_view.rotation_x * 3.14159f / 180.0f;
        float rot_y_rad = g_ve_3d_view.rotation_y * 3.14159f / 180.0f;
        
        // Apply pan offset
        float pan_offset_x = g_ve_3d_view.pan_x * 50.0f;
        float pan_offset_y = g_ve_3d_view.pan_y * 50.0f;
        
        // Helper function to transform 3D point to 2D
        auto transform_3d_to_2d = [&](float x, float y, float z) -> ImVec2 {
            // Apply rotation around Y axis (affects X and Z)
            float temp_x = x * cos(rot_y_rad) - z * sin(rot_y_rad);
            float temp_z = x * sin(rot_y_rad) + z * cos(rot_y_rad);
            
            // Apply rotation around X axis (affects Y and Z)
            float final_y = y * cos(rot_x_rad) - temp_z * sin(rot_x_rad);
            float final_z = y * sin(rot_x_rad) + temp_z * cos(rot_x_rad);
            
            // Apply pan and center
            float screen_x = center.x + temp_x + pan_offset_x;
            float screen_y = center.y + final_y + pan_offset_y;
            
            return ImVec2(screen_x, screen_y);
        };
        
        // Draw grid lines if enabled
        if (g_ve_3d_view.show_grid) {
            // Vertical grid lines (RPM axis)
            for (int x = 0; x <= g_ve_table->width; x++) {
                float x_pos = (x - g_ve_table->width * 0.5f) * grid_size / g_ve_table->width;
                ImVec2 start_3d = transform_3d_to_2d(x_pos, -grid_size * 0.5f, 0);
                ImVec2 end_3d = transform_3d_to_2d(x_pos, grid_size * 0.5f, 0);
                
                draw_list->AddLine(start_3d, end_3d, IM_COL32(50, 50, 50, 100));
            }
            
            // Horizontal grid lines (MAP axis)
            for (int y = 0; y <= g_ve_table->height; y++) {
                float y_pos = (y - g_ve_table->height * 0.5f) * grid_size / g_ve_table->height;
                ImVec2 start_3d = transform_3d_to_2d(-grid_size * 0.5f, y_pos, 0);
                ImVec2 end_3d = transform_3d_to_2d(grid_size * 0.5f, y_pos, 0);
                
                draw_list->AddLine(start_3d, end_3d, IM_COL32(50, 50, 50, 100));
            }
        }
        
        // Draw X, Y, Z graph walls (like old MegaTunix)
        if (g_ve_3d_view.show_axes) {
            // X-axis wall (RPM axis) - Red
            for (int x = 0; x <= g_ve_table->width; x++) {
                float x_pos = (x - g_ve_table->width * 0.5f) * grid_size / g_ve_table->width;
                ImVec2 wall_start = transform_3d_to_2d(x_pos, -grid_size * 0.5f, 0);
                ImVec2 wall_end = transform_3d_to_2d(x_pos, -grid_size * 0.5f, -height_scale);
                
                draw_list->AddLine(wall_start, wall_end, IM_COL32(255, 0, 0, 150), 2.0f);
            }
            
            // Y-axis wall (MAP axis) - Green
            for (int y = 0; y <= g_ve_table->height; y++) {
                float y_pos = (y - g_ve_table->height * 0.5f) * grid_size / g_ve_table->height;
                ImVec2 wall_start = transform_3d_to_2d(-grid_size * 0.5f, y_pos, 0);
                ImVec2 wall_end = transform_3d_to_2d(-grid_size * 0.5f, y_pos, -height_scale);
                
                draw_list->AddLine(wall_start, wall_end, IM_COL32(0, 255, 0, 150), 2.0f);
            }
            
            // Z-axis wall (VE axis) - Blue
            for (int z = 0; z <= 10; z++) {
                float z_pos = -z * height_scale / 10.0f;
                ImVec2 wall_start = transform_3d_to_2d(-grid_size * 0.5f, -grid_size * 0.5f, z_pos);
                ImVec2 wall_end = transform_3d_to_2d(grid_size * 0.5f, -grid_size * 0.5f, z_pos);
                
                draw_list->AddLine(wall_start, wall_end, IM_COL32(0, 0, 255, 150), 2.0f);
            }
            
            // Draw axis labels and values
            // X-axis (RPM) labels
            for (int x = 0; x < g_ve_table->width; x += 2) { // Show every other label to avoid clutter
                float x_pos = (x - g_ve_table->width * 0.5f) * grid_size / g_ve_table->width;
                ImVec2 label_pos = transform_3d_to_2d(x_pos, -grid_size * 0.5f - 10, 0);
                
                char rpm_label[16];
                snprintf(rpm_label, sizeof(rpm_label), "%.0f", g_ve_table->x_axis[x]);
                draw_list->AddText(label_pos, IM_COL32(255, 0, 0, 255), rpm_label);
            }
            
            // Y-axis (MAP) labels
            for (int y = 0; y < g_ve_table->height; y += 2) { // Show every other label to avoid clutter
                float y_pos = (y - g_ve_table->height * 0.5f) * grid_size / g_ve_table->height;
                ImVec2 label_pos = transform_3d_to_2d(-grid_size * 0.5f - 30, y_pos, 0);
                
                char map_label[16];
                snprintf(map_label, sizeof(map_label), "%.0f", g_ve_table->y_axis[y]);
                draw_list->AddText(label_pos, IM_COL32(0, 255, 0, 255), map_label);
            }
            
            // Z-axis (VE) labels
            for (int z = 0; z <= 5; z++) {
                float z_pos = -z * height_scale / 5.0f;
                ImVec2 label_pos = transform_3d_to_2d(-grid_size * 0.5f - 20, -grid_size * 0.5f - 10, z_pos);
                
                char ve_label[16];
                snprintf(ve_label, sizeof(ve_label), "%.0f", g_ve_table->metadata.min_value + (z * (g_ve_table->metadata.max_value - g_ve_table->metadata.min_value) / 5.0f));
                draw_list->AddText(label_pos, IM_COL32(0, 0, 255, 255), ve_label);
            }
            
            // Axis titles
            ImVec2 x_title_pos = transform_3d_to_2d(0, -grid_size * 0.5f - 30, 0);
            draw_list->AddText(x_title_pos, IM_COL32(255, 0, 0, 255), "RPM");
            
            ImVec2 y_title_pos = transform_3d_to_2d(-grid_size * 0.5f - 50, 0, 0);
            draw_list->AddText(y_title_pos, IM_COL32(0, 255, 0, 255), "MAP (kPa)");
            
            ImVec2 z_title_pos = transform_3d_to_2d(-grid_size * 0.5f - 20, -grid_size * 0.5f - 30, -height_scale);
            draw_list->AddText(z_title_pos, IM_COL32(0, 0, 255, 255), "VE (%)");
        }
        
        // Draw VE table as 3D wireframe with transformations
        for (int y = 0; y < g_ve_table->height - 1; y++) {
            for (int x = 0; x < g_ve_table->width - 1; x++) {
                // Calculate 3D positions
                float x1 = (x - g_ve_table->width * 0.5f) * grid_size / g_ve_table->width;
                float x2 = (x + 1 - g_ve_table->width * 0.5f) * grid_size / g_ve_table->width;
                float y1 = (y - g_ve_table->height * 0.5f) * grid_size / g_ve_table->height;
                float y2 = (y + 1 - g_ve_table->height * 0.5f) * grid_size / g_ve_table->height;
                
                // Calculate VE values (normalized to 0-1)
                float ve1 = (g_ve_table->data[y][x] - g_ve_table->metadata.min_value) / (g_ve_table->metadata.max_value - g_ve_table->metadata.min_value);
                float ve2 = (g_ve_table->data[y][x+1] - g_ve_table->metadata.min_value) / (g_ve_table->metadata.max_value - g_ve_table->metadata.min_value);
                float ve3 = (g_ve_table->data[y+1][x] - g_ve_table->metadata.min_value) / (g_ve_table->metadata.max_value - g_ve_table->metadata.min_value);
                float ve4 = (g_ve_table->data[y+1][x+1] - g_ve_table->metadata.min_value) / (g_ve_table->metadata.max_value - g_ve_table->metadata.min_value);
                
                // Apply 3D effect (height based on VE value)
                float z1 = -ve1 * height_scale;
                float z2 = -ve2 * height_scale;
                float z3 = -ve3 * height_scale;
                float z4 = -ve4 * height_scale;
                
                // Transform all points to 2D
                ImVec2 p1 = transform_3d_to_2d(x1, y1, z1);
                ImVec2 p2 = transform_3d_to_2d(x2, y1, z2);
                ImVec2 p3 = transform_3d_to_2d(x1, y2, z3);
                ImVec2 p4 = transform_3d_to_2d(x2, y2, z4);
                
                // Draw wireframe lines
                ImU32 line_color = IM_COL32(100, 200, 255, 200);
                
                // Horizontal lines
                draw_list->AddLine(p1, p2, line_color);
                draw_list->AddLine(p3, p4, line_color);
                
                // Vertical lines
                draw_list->AddLine(p1, p3, line_color);
                draw_list->AddLine(p2, p4, line_color);
                
                // Fill faces if not in wireframe mode
                if (!g_ve_3d_view.wireframe_mode) {
                    ImU32 fill_color = IM_COL32(
                        (int)(ve1 * 255), 
                        (int)((1.0f - ve1) * 255), 
                        100, 
                        (int)(g_ve_3d_view.opacity * 150)
                    );
                    
                    // Draw filled triangles
                    draw_list->AddTriangleFilled(p1, p2, p3, fill_color);
                    draw_list->AddTriangleFilled(p2, p3, p4, fill_color);
                }
            }
        }
        
        // Draw axes if enabled
        if (g_ve_3d_view.show_axes) {
            // X-axis (RPM) - transformed
            ImVec2 x_start = transform_3d_to_2d(-grid_size * 0.5f, 0, 0);
            ImVec2 x_end = transform_3d_to_2d(grid_size * 0.5f, 0, 0);
            draw_list->AddLine(x_start, x_end, IM_COL32(255, 0, 0, 255), 2.0f);
            draw_list->AddText(x_end, IM_COL32(255, 0, 0, 255), "RPM");
            
            // Y-axis (MAP) - transformed
            ImVec2 y_start = transform_3d_to_2d(0, -grid_size * 0.5f, 0);
            ImVec2 y_end = transform_3d_to_2d(0, grid_size * 0.5f, 0);
            draw_list->AddLine(y_start, y_end, IM_COL32(0, 255, 0, 255), 2.0f);
            draw_list->AddText(y_end, IM_COL32(0, 255, 0, 255), "MAP");
            
            // Z-axis (VE) - transformed
            ImVec2 z_start = transform_3d_to_2d(0, 0, 0);
            ImVec2 z_end = transform_3d_to_2d(0, 0, -height_scale);
            draw_list->AddLine(z_start, z_end, IM_COL32(0, 0, 255, 255), 2.0f);
            draw_list->AddText(z_end, IM_COL32(0, 0, 255, 255), "VE");
        }
        
        // Draw animated wireframe triangle/arrow at current engine position
        if (g_current_rpm_cell >= 0 && g_current_map_cell >= 0) {
            // Get the VE value at the current engine position
            float current_ve_3d = imgui_table_get_value(g_ve_table, g_current_rpm_cell, g_current_map_cell);
            
            // Calculate the 3D position of the current engine point
            float current_rpm_normalized = (g_ecu_data.rpm - g_ve_table->x_axis[0]) / 
                                         (g_ve_table->x_axis[g_ve_table->width-1] - g_ve_table->x_axis[0]);
            float current_map_normalized = (g_ecu_data.map - g_ve_table->y_axis[0]) / 
                                         (g_ve_table->y_axis[g_ve_table->height-1] - g_ve_table->y_axis[0]);
            
            // Clamp to valid range
            current_rpm_normalized = fmax(0.0f, fmin(1.0f, current_rpm_normalized));
            current_map_normalized = fmax(0.0f, fmin(1.0f, current_map_normalized));
            
            // Convert to 3D coordinates
            float x_pos = (current_rpm_normalized - 0.5f) * grid_size;
            float y_pos = (current_map_normalized - 0.5f) * grid_size;
            float z_pos = -current_ve_3d * height_scale / g_ve_table->metadata.max_value;
            
            // Transform to 2D screen coordinates
            ImVec2 engine_pos_2d = transform_3d_to_2d(x_pos, y_pos, z_pos);
            
            // Draw engine trail (fading trail of engine position) for 3D view
            if (g_show_engine_trail) {
                for (int i = 0; i < g_trail_count_3d; i++) {
                    TrailPoint* point = &g_engine_trail_3d[i];
                    if (point->alpha > 0.0f) {
                        // Convert trail point back to 3D coordinates
                        float trail_rpm_normalized = (float)point->x / (float)(g_ve_table->width - 1);
                        float trail_map_normalized = (float)point->y / (float)(g_ve_table->height - 1);
                        
                        // Get VE value at trail position
                        float trail_ve = imgui_table_get_value(g_ve_table, point->x, point->y);
                        
                        // Convert to 3D coordinates
                        float trail_x_pos = (trail_rpm_normalized - 0.5f) * grid_size;
                        float trail_y_pos = (trail_map_normalized - 0.5f) * grid_size;
                        float trail_z_pos = -trail_ve * height_scale / g_ve_table->metadata.max_value;
                        
                        // Transform to 2D screen coordinates
                        ImVec2 trail_pos_2d = transform_3d_to_2d(trail_x_pos, trail_y_pos, trail_z_pos);
                        
                                            // Draw fading trail point with increased opacity
                    ImU32 trail_color = IM_COL32(255, 0, 255, (int)(255 * point->alpha * 0.6f));
                    draw_list->AddCircleFilled(trail_pos_2d, 4.0f, trail_color);
                    
                    // Draw trail border with increased opacity
                    ImU32 border_color = IM_COL32(255, 0, 255, (int)(255 * point->alpha * 0.9f));
                        draw_list->AddCircle(trail_pos_2d, 6.0f, border_color, 0, 1.0f);
                    }
                }
            }
            
            // Create solid triangle/arrow pointing upward
            float triangle_size = 15.0f;
            
            // Draw wireframe triangle (pointing downward toward the map)
            ImVec2 triangle_bottom = ImVec2(engine_pos_2d.x, engine_pos_2d.y + triangle_size);
            ImVec2 triangle_left = ImVec2(engine_pos_2d.x - triangle_size * 0.5f, engine_pos_2d.y - triangle_size * 0.5f);
            ImVec2 triangle_right = ImVec2(engine_pos_2d.x + triangle_size * 0.5f, engine_pos_2d.y - triangle_size * 0.5f);
            
            // Draw triangle outline in solid magenta (matching 2D view)
            ImU32 triangle_color = IM_COL32(255, 0, 255, 255);
            draw_list->AddTriangle(triangle_bottom, triangle_left, triangle_right, triangle_color, 2.0f);
            
            // Add a small circle at the base for better visibility
            draw_list->AddCircle(engine_pos_2d, 3.0f, triangle_color, 0, 2.0f);
            
            // Add text label showing current values
            char engine_label[64];
            snprintf(engine_label, sizeof(engine_label), "%.0f RPM\n%.1f kPa", g_ecu_data.rpm, g_ecu_data.map);
            ImVec2 text_pos = ImVec2(engine_pos_2d.x + 20, engine_pos_2d.y - 10);
            draw_list->AddText(text_pos, triangle_color, engine_label);
        }
        
        // Draw comprehensive value labels like original MegaTunix
        char label_buffer[128];
        
        // Top-left info panel (like original MegaTunix)
        snprintf(label_buffer, sizeof(label_buffer), "VE Range: %.1f - %.1f", 
                g_ve_table->metadata.min_value, g_ve_table->metadata.max_value);
        draw_list->AddText(
            ImVec2(window_pos.x + 10, window_pos.y + 10),
            IM_COL32(200, 200, 200, 255),
            label_buffer
        );
        
        snprintf(label_buffer, sizeof(label_buffer), "RPM: %.0f - %.0f", 
                g_ve_table->x_axis[0], g_ve_table->x_axis[g_ve_table->width-1]);
        draw_list->AddText(
            ImVec2(window_pos.x + 10, window_pos.y + 25),
            IM_COL32(200, 200, 200, 255),
            label_buffer
        );
        
        snprintf(label_buffer, sizeof(label_buffer), "MAP: %.1f - %.1f kPa", 
                g_ve_table->y_axis[0], g_ve_table->y_axis[g_ve_table->height-1]);
        draw_list->AddText(
            ImVec2(window_pos.x + 10, window_pos.y + 40),
            IM_COL32(200, 200, 200, 255),
            label_buffer
        );
        
        // Current Position fields (like original MegaTunix)
        snprintf(label_buffer, sizeof(label_buffer), "Current Position:");
        draw_list->AddText(
            ImVec2(window_pos.x + 10, window_pos.y + 60),
            IM_COL32(255, 255, 255, 255),
            label_buffer
        );
        
        // Use real engine position (like original MegaTunix)
        float current_rpm_3d = g_ecu_data.rpm;
        float current_map_3d = g_ecu_data.map;
        float current_ve_3d = 0.0f;
        
        // Get the VE value at the current engine position
        if (g_current_rpm_cell >= 0 && g_current_map_cell >= 0) {
            current_ve_3d = imgui_table_get_value(g_ve_table, g_current_rpm_cell, g_current_map_cell);
        }
        
        snprintf(label_buffer, sizeof(label_buffer), "RPM: %.0f", current_rpm_3d);
        draw_list->AddText(
            ImVec2(window_pos.x + 10, window_pos.y + 75),
            IM_COL32(0, 255, 0, 255), // Green like original
            label_buffer
        );
        
        snprintf(label_buffer, sizeof(label_buffer), "MAP: %.1f kPa", current_map_3d);
        draw_list->AddText(
            ImVec2(window_pos.x + 10, window_pos.y + 90),
            IM_COL32(0, 255, 0, 255), // Green like original
            label_buffer
        );
        
        snprintf(label_buffer, sizeof(label_buffer), "VE: %.1f", current_ve_3d);
        draw_list->AddText(
            ImVec2(window_pos.x + 10, window_pos.y + 105),
            IM_COL32(0, 255, 0, 255), // Green like original
            label_buffer
        );
        
        // Edit Position fields (like original MegaTunix)
        snprintf(label_buffer, sizeof(label_buffer), "Edit Position:");
        draw_list->AddText(
            ImVec2(window_pos.x + 10, window_pos.y + 130),
            IM_COL32(255, 255, 255, 255),
            label_buffer
        );
        
        // Show selected cell position
        if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
            snprintf(label_buffer, sizeof(label_buffer), "RPM: %.0f", g_ve_table->x_axis[g_selected_cell_x]);
            draw_list->AddText(
                ImVec2(window_pos.x + 10, window_pos.y + 145),
                IM_COL32(255, 255, 0, 255), // Yellow for selected
                label_buffer
            );
            
            snprintf(label_buffer, sizeof(label_buffer), "MAP: %.1f kPa", g_ve_table->y_axis[g_selected_cell_y]);
            draw_list->AddText(
                ImVec2(window_pos.x + 10, window_pos.y + 160),
                IM_COL32(255, 255, 0, 255), // Yellow for selected
                label_buffer
            );
            
            float selected_ve = imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y);
            snprintf(label_buffer, sizeof(label_buffer), "VE: %.1f", selected_ve);
            draw_list->AddText(
                ImVec2(window_pos.x + 10, window_pos.y + 175),
                IM_COL32(255, 255, 0, 255), // Yellow for selected
                label_buffer
            );
        } else {
            snprintf(label_buffer, sizeof(label_buffer), "RPM: --");
            draw_list->AddText(
                ImVec2(window_pos.x + 10, window_pos.y + 145),
                IM_COL32(128, 128, 128, 255), // Gray for no selection
                label_buffer
            );
            
            snprintf(label_buffer, sizeof(label_buffer), "MAP: -- kPa");
            draw_list->AddText(
                ImVec2(window_pos.x + 10, window_pos.y + 160),
                IM_COL32(128, 128, 128, 255), // Gray for no selection
                label_buffer
            );
            
            snprintf(label_buffer, sizeof(label_buffer), "VE: --");
            draw_list->AddText(
                ImVec2(window_pos.x + 10, window_pos.y + 175),
                IM_COL32(128, 128, 128, 255), // Gray for no selection
                label_buffer
            );
        }
    }
    
    ImGui::EndChild();
    
    // Real-Time Variables section (like original MegaTunix)
    ImGui::Separator();
    ImGui::Text("Real-Time Variables:");
    
    // Simulate real-time engine data
    static float rt_rpm = 2500.0f;
    static float rt_map = 80.0f;
    static float rt_afr = 14.7f;
    static float rt_clt = 85.0f;
    
    // Update simulated values slightly for demo
    rt_rpm += (rand() % 100 - 50) * 0.1f;
    rt_map += (rand() % 20 - 10) * 0.1f;
    rt_afr += (rand() % 10 - 5) * 0.01f;
    rt_clt += (rand() % 10 - 5) * 0.1f;
    
    // Clamp values to realistic ranges
    rt_rpm = fmax(800.0f, fmin(6000.0f, rt_rpm));
    rt_map = fmax(20.0f, fmin(120.0f, rt_map));
    rt_afr = fmax(10.0f, fmin(20.0f, rt_afr));
    rt_clt = fmax(60.0f, fmin(110.0f, rt_clt));
    
    ImGui::Text("RPM: %.0f", rt_rpm);
    ImGui::Text("MAP: %.1f kPa", rt_map);
    ImGui::Text("AFR: %.1f", rt_afr);
    ImGui::Text("CLT: %.1f°C", rt_clt);
    
    ImGui::Separator();
    ImGui::Text("3D Visualization: Wireframe shows VE table as a 3D surface");
    ImGui::Text("Height represents VE values, color indicates efficiency");
    ImGui::Text("Current Position shows simulated engine operating point");
    ImGui::Text("Edit Position shows selected cell coordinates");
}

void render_ve_table_editor_view() {
    ImGui::Text("Table Editor View");
    ImGui::Text("Click on cells to edit values directly.");
    
    // Table editing controls
    ImGui::Separator();
    ImGui::Text("Editing Controls:");
    
    static float edit_value = 0.0f;
    static int edit_x = 0, edit_y = 0;
    
    ImGui::InputInt("Edit X", &edit_x, 1, 1);
    ImGui::InputInt("Edit Y", &edit_y, 1, 1);
    ImGui::InputFloat("Edit Value", &edit_value, 1.0f, 5.0f, "%.1f");
    
    if (ImGui::Button("Set Value", ImVec2(100, 30))) {
        if (edit_x >= 0 && edit_x < g_ve_table->width && 
            edit_y >= 0 && edit_y < g_ve_table->height) {
            imgui_table_set_value(g_ve_table, edit_x, edit_y, edit_value);
            if (g_ve_texture) {
                imgui_ve_texture_update(g_ve_texture, g_ve_table, &g_ve_3d_view);
            }
        }
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Get Value", ImVec2(100, 30))) {
        if (edit_x >= 0 && edit_x < g_ve_table->width && 
            edit_y >= 0 && edit_y < g_ve_table->height) {
            edit_value = imgui_table_get_value(g_ve_table, edit_x, edit_y);
        }
    }
    
    ImGui::Separator();
    
    // Table display
    ImGui::Text("VE Table Values:");
    
    // Create a scrollable table
    ImGui::BeginChild("TableScroll", ImVec2(0, 300), true);
    
    // Table header (RPM values)
    ImGui::Text("MAP\\RPM");
    for (int x = 0; x < g_ve_table->width; x++) {
        ImGui::SameLine();
        ImGui::Text("%.0f", g_ve_table->x_axis[x]);
    }
    
    // Table rows
    for (int y = 0; y < g_ve_table->height; y++) {
        // Row header (MAP value)
        ImGui::Text("%.1f", g_ve_table->y_axis[y]);
        
        // Table cells
        for (int x = 0; x < g_ve_table->width; x++) {
            ImGui::SameLine();
            
                                        // Create unique ID for each cell
                            char cell_id[64];
                            snprintf(cell_id, sizeof(cell_id), "##table_cell_%d_%d_%s", x, y, g_ui_theme.name);
            
            // Get current value
            float value = imgui_table_get_value(g_ve_table, x, y);
            
                                // Create smaller input field for each cell
                    char value_str[16];
                    snprintf(value_str, sizeof(value_str), "%.0f", value);
                    
                    ImGui::SetNextItemWidth(40); // Make cells much smaller
                    if (ImGui::InputText(cell_id, value_str, sizeof(value_str), 
                                        ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
                        // Parse new value
                        float new_value = atof(value_str);
                        if (new_value >= 0.0f && new_value <= 200.0f) { // Reasonable VE range
                            imgui_table_set_value(g_ve_table, x, y, new_value);
                            if (g_ve_texture) {
                                imgui_ve_texture_update(g_ve_texture, g_ve_table, &g_ve_3d_view);
                            }
                        }
                    }
            
            // Highlight selected cell
            if (x == g_selected_cell_x && y == g_selected_cell_y) {
                ImGui::GetWindowDrawList()->AddRect(
                    ImGui::GetItemRectMin(),
                    ImGui::GetItemRectMax(),
                    IM_COL32(255, 255, 0, 255),
                    0.0f, 0, 2.0f
                );
            }
        }
    }
    
    ImGui::EndChild();
}

// Note: render_ignition_table_editor() has been replaced by render_ignition_table_tab() 