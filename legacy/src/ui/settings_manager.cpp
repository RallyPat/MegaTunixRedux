/*
 * MegaTunix Redux - Settings Management Module Implementation
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

#include "../../include/ui/settings_manager.h"
#include "../../include/ui/ui_theme_manager.h"
#include "../../include/ui/logging_system.h"
#include <stdio.h>
#include <string.h>

// Local module state
static bool g_module_initialized = false;
static UserSettings g_user_settings = {0};

// Global settings access for compatibility
UserSettings* g_user_settings_global = &g_user_settings;

// External dependencies (will be resolved at link time)
extern void add_log_entry(int level, const char* format, ...);
extern void switch_theme(ThemeType theme);

// ============================================================================
// Settings Management Functions
// ============================================================================

bool init_settings_manager() {
    if (g_module_initialized) {
        return true;
    }
    
    // Initialize with default settings
    g_user_settings = get_default_user_settings();
    
    g_module_initialized = true;
    return true;
}

void cleanup_settings_manager() {
    if (!g_module_initialized) {
        return;
    }
    
    // Save settings before cleanup
    save_user_settings();
    
    g_module_initialized = false;
}

// ============================================================================
// Core Settings Operations
// ============================================================================

void load_user_settings() {
    if (!g_module_initialized) {
        return;
    }
    
    FILE* file = fopen(SETTINGS_FILE, "rb");
    if (!file) {
        add_log_entry(0, "No settings file found, using defaults");
        return;
    }
    
    UserSettings loaded_settings;
    size_t read_size = fread(&loaded_settings, sizeof(UserSettings), 1, file);
    fclose(file);
    
    if (read_size == 1 && loaded_settings.version == SETTINGS_VERSION) {
        if (validate_user_settings(&loaded_settings)) {
            g_user_settings = loaded_settings;
            add_log_entry(0, "Settings loaded successfully from %s", SETTINGS_FILE);
            
            // Apply loaded settings
            apply_user_settings();
        } else {
            add_log_entry(1, "Settings file validation failed, using defaults");
            g_user_settings = get_default_user_settings();
        }
    } else {
        add_log_entry(1, "Settings file corrupted or outdated, using defaults");
        g_user_settings = get_default_user_settings();
    }
}

void save_user_settings() {
    if (!g_module_initialized) {
        return;
    }
    
    // Validate settings before saving
    if (!validate_user_settings(&g_user_settings)) {
        add_log_entry(2, "ERROR: Invalid settings detected, not saving");
        return;
    }
    
    FILE* file = fopen(SETTINGS_FILE, "wb");
    if (!file) {
        add_log_entry(2, "ERROR: Failed to open settings file for writing: %s", SETTINGS_FILE);
        return;
    }
    
    size_t write_size = fwrite(&g_user_settings, sizeof(UserSettings), 1, file);
    fclose(file);
    
    if (write_size == 1) {
        add_log_entry(0, "Settings saved successfully to %s", SETTINGS_FILE);
    } else {
        add_log_entry(2, "ERROR: Failed to write settings to file");
    }
}

void apply_user_settings() {
    if (!g_module_initialized) {
        return;
    }
    
    // Apply theme
    if (g_user_settings.theme != ui_theme_manager_get_current_theme_type()) {
        switch_theme(g_user_settings.theme);
    }
    
    // Apply other settings (these will be handled by the calling code)
    // The actual application of demo_mode, debug_mode, etc. is done in main.cpp
    // to maintain the existing architecture
    
    add_log_entry(0, "User settings applied successfully");
}

void reset_user_settings_to_defaults() {
    if (!g_module_initialized) {
        return;
    }
    
    g_user_settings = get_default_user_settings();
    
    // Apply the default settings
    apply_user_settings();
    
    // Save the default settings
    save_user_settings();
    
    add_log_entry(0, "Settings reset to defaults");
}

// ============================================================================
// Settings Access Functions
// ============================================================================

const UserSettings* get_user_settings() {
    return g_module_initialized ? &g_user_settings : NULL;
}

UserSettings* get_user_settings_mutable() {
    return g_module_initialized ? &g_user_settings : NULL;
}

// ============================================================================
// Settings Update Functions
// ============================================================================

void update_settings_from_ui() {
    if (!g_module_initialized) {
        return;
    }
    
    // Update theme from UI theme manager
    g_user_settings.theme = ui_theme_manager_get_current_theme_type();
    
    // Update logging settings from logging system
    g_user_settings.log_auto_scroll = get_log_auto_scroll();
    g_user_settings.log_filter_level = get_log_filter_level();
    
    // Note: demo_mode, debug_mode, and show_engine_trail are updated
    // by the calling code to maintain existing architecture
}

void update_settings_from_window(SDL_Window* window) {
    if (!g_module_initialized || !window) {
        return;
    }
    
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    g_user_settings.window_width = (float)width;
    g_user_settings.window_height = (float)height;
    g_user_settings.window_maximized = (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED) != 0;
}

void update_settings_theme(ThemeType theme) {
    if (g_module_initialized) {
        g_user_settings.theme = theme;
    }
}

void update_settings_demo_mode(bool demo_mode) {
    if (g_module_initialized) {
        g_user_settings.demo_mode = demo_mode;
    }
}

void update_settings_debug_mode(bool debug_mode) {
    if (g_module_initialized) {
        g_user_settings.debug_mode = debug_mode;
    }
}

void update_settings_log_auto_scroll(bool auto_scroll) {
    if (g_module_initialized) {
        g_user_settings.log_auto_scroll = auto_scroll;
    }
}

void update_settings_log_filter_level(int filter_level) {
    if (g_module_initialized) {
        g_user_settings.log_filter_level = filter_level;
    }
}

void update_settings_show_engine_trail(bool show_trail) {
    if (g_module_initialized) {
        g_user_settings.show_engine_trail = show_trail;
    }
}

// ============================================================================
// Settings Validation and Defaults
// ============================================================================

bool validate_user_settings(const UserSettings* settings) {
    if (!settings) {
        return false;
    }
    
    // Check version
    if (settings->version != SETTINGS_VERSION) {
        return false;
    }
    
    // Check theme range
    if (settings->theme < 0 || settings->theme >= 5) { // Assuming 5 themes
        return false;
    }
    
    // Check log filter level range
    if (settings->log_filter_level < 0 || settings->log_filter_level > 3) {
        return false;
    }
    
    // Check window dimensions (reasonable bounds)
    if (settings->window_width < 800.0f || settings->window_width > 3840.0f) {
        return false;
    }
    if (settings->window_height < 600.0f || settings->window_height > 2160.0f) {
        return false;
    }
    
    return true;
}

UserSettings get_default_user_settings() {
    return (UserSettings){
        .version = SETTINGS_VERSION,
        .theme = THEME_CLASSIC_AUTOMOTIVE,
        .demo_mode = false,
        .debug_mode = false,
        .log_auto_scroll = true,
        .log_filter_level = 0,
        .show_engine_trail = true,
        .window_width = 1280.0f,
        .window_height = 720.0f,
        .window_maximized = false
    };
}

// ============================================================================
// Window State Management
// ============================================================================

void apply_window_settings(SDL_Window* window) {
    if (!g_module_initialized || !window) {
        return;
    }
    
    SDL_SetWindowSize(window, (int)g_user_settings.window_width, (int)g_user_settings.window_height);
    if (g_user_settings.window_maximized) {
        SDL_MaximizeWindow(window);
    }
}

void save_window_settings(SDL_Window* window) {
    if (!g_module_initialized || !window) {
        return;
    }
    
    update_settings_from_window(window);
    save_user_settings();
}

// ============================================================================
// Legacy Function Names for Compatibility
// ============================================================================

void load_user_settings_legacy() {
    load_user_settings();
}

void save_user_settings_legacy() {
    save_user_settings();
}

void apply_user_settings_legacy() {
    apply_user_settings();
}

void reset_user_settings_to_defaults_legacy() {
    reset_user_settings_to_defaults();
}
