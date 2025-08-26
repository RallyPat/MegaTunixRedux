/*
 * MegaTunix Redux - Settings Management Module
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

#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <stdbool.h>
#include <SDL2/SDL.h>
#include "ui_theme_manager.h"
#include "imgui_ve_table.h"

// Forward declarations
// Note: ThemeType and ImGuiTable are defined in ui_theme_manager.h and imgui_ve_table.h respectively

// Settings and persistence constants
#define SETTINGS_FILE "megatunix-redux.conf"
#define SETTINGS_VERSION 1

// User settings structure
typedef struct {
    int version;
    ThemeType theme;
    bool demo_mode;
    bool debug_mode;
    bool log_auto_scroll;
    int log_filter_level;
    bool show_engine_trail;
    float window_width;
    float window_height;
    bool window_maximized;
} UserSettings;

// Settings management functions
bool init_settings_manager(void);
void cleanup_settings_manager(void);

// Core settings operations
void load_user_settings(void);
void save_user_settings(void);
void apply_user_settings(void);
void reset_user_settings_to_defaults(void);

// Settings access functions
const UserSettings* get_user_settings(void);
UserSettings* get_user_settings_mutable(void);

// Settings update functions
void update_settings_from_ui(void);
void update_settings_from_window(SDL_Window* window);
void update_settings_theme(ThemeType theme);
void update_settings_demo_mode(bool demo_mode);
void update_settings_debug_mode(bool debug_mode);
void update_settings_log_auto_scroll(bool auto_scroll);
void update_settings_log_filter_level(int filter_level);
void update_settings_show_engine_trail(bool show_trail);

// Settings validation and defaults
bool validate_user_settings(const UserSettings* settings);
UserSettings get_default_user_settings(void);

// Window state management
void apply_window_settings(SDL_Window* window);
void save_window_settings(SDL_Window* window);

// Legacy function names for compatibility
void load_user_settings_legacy(void);
void save_user_settings_legacy(void);
void apply_user_settings_legacy(void);
void reset_user_settings_to_defaults_legacy(void);

// Global settings access for compatibility
extern UserSettings* g_user_settings_global;

#endif // SETTINGS_MANAGER_H
