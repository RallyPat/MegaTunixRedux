/*
 * MegaTunix Redux - UI Theme Manager Module
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

#ifndef UI_THEME_MANAGER_H
#define UI_THEME_MANAGER_H

#include <stdbool.h>

// ImGui includes
#include "../../external/imgui/imgui.h"

// UI Theme and Styling System
typedef enum {
    THEME_CLASSIC_AUTOMOTIVE = 0,
    THEME_MODERN_TECH,
    THEME_RACING_GREEN,
    THEME_SUNSET_ORANGE,
    THEME_OCEAN_BLUE,
    THEME_COUNT
} ThemeType;

typedef struct {
    ImVec4 primary_color;           // Primary brand color
    ImVec4 secondary_color;         // Secondary accent
    ImVec4 accent_color;            // Accent highlight
    ImVec4 success_color;           // Success/OK (green)
    ImVec4 warning_color;           // Warning (yellow/orange)
    ImVec4 error_color;             // Error/danger (red)
    ImVec4 background_dark;         // Dark background
    ImVec4 background_medium;       // Medium background
    ImVec4 background_light;        // Light background
    ImVec4 text_primary;            // Primary text
    ImVec4 text_secondary;          // Secondary text
    ImVec4 text_muted;              // Muted text
    ImVec4 border_color;            // Border color
    ImVec4 highlight_color;         // Selection highlight
    float corner_radius;             // Corner radius for rounded elements
    float border_thickness;          // Border thickness
    float padding_multiplier;        // Padding multiplier for spacing
    const char* name;                // Theme name for display
} UITheme;

// Theme Management Functions
void ui_theme_manager_init_themes(void);
void ui_theme_manager_init_current_theme(void);
void ui_theme_manager_apply_theme(void);
void ui_theme_manager_switch_theme(ThemeType theme);

// Legacy function names for compatibility
void init_ui_theme(void);
void apply_ui_theme(void);
void switch_theme(ThemeType theme);

// Theme Access Functions
const UITheme* ui_theme_manager_get_current_theme(void);
ThemeType ui_theme_manager_get_current_theme_type(void);
const UITheme* ui_theme_manager_get_theme(ThemeType theme_type);
const char* ui_theme_manager_get_theme_name(ThemeType theme_type);

// Professional Rendering Functions
void ui_theme_manager_render_professional_button(const char* label, const ImVec2& size, bool* clicked, 
                                                const ImVec4& color = ImVec4(0,0,0,0), 
                                                const char* tooltip = nullptr);
void ui_theme_manager_render_section_header(const char* title, const char* subtitle, ImVec4 color);
void ui_theme_manager_render_status_indicator(const char* label, bool is_active, const char* status_text);
void ui_theme_manager_render_metric_card(const char* label, const char* value, const char* unit, 
                                        const ImVec4& value_color, const ImVec4& background_color);

// Legacy function names for compatibility
void render_professional_button(const char* label, const ImVec2& size, bool* clicked, 
                               const ImVec4& color = ImVec4(0,0,0,0), 
                               const char* tooltip = nullptr);
void render_section_header(const char* title, const char* subtitle, const ImVec4& accent_color);
void render_status_indicator(const char* label, bool is_active, const char* status_text);
void render_metric_card(const char* label, const char* value, const char* unit, 
                       const ImVec4& value_color, const char* description);

// Theme Customization Functions
bool ui_theme_manager_customize_theme(ThemeType theme_type, const UITheme* custom_theme);
bool ui_theme_manager_save_custom_theme(const char* name, const UITheme* theme);
bool ui_theme_manager_load_custom_theme(const char* name, UITheme* theme);

// Module Initialization and Cleanup
bool init_ui_theme_manager(void);
void cleanup_ui_theme_manager(void);

// Global theme variables for compatibility
extern UITheme g_ui_theme;
extern ThemeType g_current_theme_global;

#endif // UI_THEME_MANAGER_H
