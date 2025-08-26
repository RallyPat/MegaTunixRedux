/*
 * MegaTunix Redux - UI Theme Manager Module Implementation
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

#include "../../include/ui/ui_theme_manager.h"
#include <string.h>

// Local module state
static bool g_module_initialized = false;
static UITheme g_current_theme;
static ThemeType g_current_theme_type = THEME_CLASSIC_AUTOMOTIVE;
static UITheme g_themes[THEME_COUNT];

// Global theme variables for compatibility
UITheme g_ui_theme;
ThemeType g_current_theme_global;

// ============================================================================
// Theme Definitions
// ============================================================================

static void init_theme_definitions() {
    // Classic Automotive Theme - Deep reds, chrome silvers, professional blacks
    g_themes[THEME_CLASSIC_AUTOMOTIVE] = {
        .primary_color = ImVec4(0.8f, 0.1f, 0.1f, 1.0f),      // Deep automotive red
        .secondary_color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f),    // Chrome silver
        .accent_color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f),       // Gold accent
        .success_color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f),      // Green
        .warning_color = ImVec4(1.0f, 0.6f, 0.0f, 1.0f),      // Orange
        .error_color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f),        // Red
        .background_dark = ImVec4(0.05f, 0.05f, 0.05f, 1.0f), // Deep black
        .background_medium = ImVec4(0.1f, 0.1f, 0.1f, 1.0f),  // Medium black
        .background_light = ImVec4(0.15f, 0.15f, 0.15f, 1.0f), // Light black
        .text_primary = ImVec4(0.95f, 0.95f, 0.95f, 1.0f),    // Bright white
        .text_secondary = ImVec4(0.8f, 0.8f, 0.8f, 1.0f),     // Light gray
        .text_muted = ImVec4(0.6f, 0.6f, 0.6f, 1.0f),         // Medium gray
        .border_color = ImVec4(0.3f, 0.3f, 0.3f, 1.0f),       // Subtle border
        .highlight_color = ImVec4(0.8f, 0.1f, 0.1f, 0.3f),    // Selection highlight
        .corner_radius = 6.0f,                                  // Rounded corners
        .border_thickness = 1.0f,                               // Border thickness
        .padding_multiplier = 1.2f,                             // Increased spacing
        .name = "Classic Automotive"
    };

    // Modern Tech Theme - Electric blue, light gray, cyan
    g_themes[THEME_MODERN_TECH] = {
        .primary_color = ImVec4(0.0f, 0.6f, 1.0f, 1.0f),      // Electric blue
        .secondary_color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f),    // Light gray
        .accent_color = ImVec4(0.0f, 0.9f, 0.9f, 1.0f),       // Cyan
        .success_color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f),      // Green
        .warning_color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f),      // Yellow
        .error_color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f),        // Red
        .background_dark = ImVec4(0.08f, 0.08f, 0.1f, 1.0f),  // Dark blue-gray
        .background_medium = ImVec4(0.12f, 0.12f, 0.15f, 1.0f), // Medium blue-gray
        .background_light = ImVec4(0.18f, 0.18f, 0.22f, 1.0f), // Light blue-gray
        .text_primary = ImVec4(0.95f, 0.95f, 0.95f, 1.0f),    // Bright white
        .text_secondary = ImVec4(0.8f, 0.8f, 0.8f, 1.0f),     // Light gray
        .text_muted = ImVec4(0.6f, 0.6f, 0.6f, 1.0f),         // Medium gray
        .border_color = ImVec4(0.25f, 0.25f, 0.3f, 1.0f),     // Subtle border
        .highlight_color = ImVec4(0.0f, 0.6f, 1.0f, 0.3f),    // Selection highlight
        .corner_radius = 8.0f,                                  // Rounded corners
        .border_thickness = 1.0f,                               // Border thickness
        .padding_multiplier = 1.3f,                             // Increased spacing
        .name = "Modern Tech"
    };

    // Racing Green Theme - British racing green, gold
    g_themes[THEME_RACING_GREEN] = {
        .primary_color = ImVec4(0.0f, 0.4f, 0.2f, 1.0f),      // British racing green
        .secondary_color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f),    // Gold
        .accent_color = ImVec4(0.0f, 0.8f, 0.4f, 1.0f),       // Bright green
        .success_color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f),      // Green
        .warning_color = ImVec4(1.0f, 0.6f, 0.0f, 1.0f),      // Orange
        .error_color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f),        // Red
        .background_dark = ImVec4(0.05f, 0.08f, 0.05f, 1.0f), // Dark green-black
        .background_medium = ImVec4(0.08f, 0.12f, 0.08f, 1.0f), // Medium green-black
        .background_light = ImVec4(0.12f, 0.16f, 0.12f, 1.0f), // Light green-black
        .text_primary = ImVec4(0.95f, 0.95f, 0.95f, 1.0f),    // Bright white
        .text_secondary = ImVec4(0.8f, 0.8f, 0.8f, 1.0f),     // Light gray
        .text_muted = ImVec4(0.6f, 0.6f, 0.6f, 1.0f),         // Medium gray
        .border_color = ImVec4(0.2f, 0.3f, 0.2f, 1.0f),       // Subtle border
        .highlight_color = ImVec4(0.0f, 0.4f, 0.2f, 0.3f),    // Selection highlight
        .corner_radius = 6.0f,                                  // Rounded corners
        .border_thickness = 1.0f,                               // Border thickness
        .padding_multiplier = 1.2f,                             // Increased spacing
        .name = "Racing Green"
    };

    // Sunset Orange Theme - Warm orange, deep purple
    g_themes[THEME_SUNSET_ORANGE] = {
        .primary_color = ImVec4(1.0f, 0.4f, 0.0f, 1.0f),      // Warm orange
        .secondary_color = ImVec4(0.6f, 0.2f, 0.8f, 1.0f),    // Deep purple
        .accent_color = ImVec4(0.8f, 0.5f, 0.3f, 1.0f),       // Darker orange-brown for better button contrast
        .success_color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f),      // Green
        .warning_color = ImVec4(0.8f, 0.6f, 0.0f, 1.0f),      // Darker yellow for better button contrast
        .error_color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f),        // Red
        .background_dark = ImVec4(0.08f, 0.05f, 0.08f, 1.0f), // Dark purple-black
        .background_medium = ImVec4(0.12f, 0.08f, 0.12f, 1.0f), // Medium purple-black
        .background_light = ImVec4(0.15f, 0.10f, 0.15f, 1.0f), // Much darker for better button contrast
        .text_primary = ImVec4(0.95f, 0.95f, 0.95f, 1.0f),    // Bright white
        .text_secondary = ImVec4(0.8f, 0.8f, 0.8f, 1.0f),     // Light gray
        .text_muted = ImVec4(0.6f, 0.6f, 0.6f, 1.0f),         // Medium gray
        .border_color = ImVec4(0.5f, 0.3f, 0.5f, 1.0f),      // Much darker border for better contrast
        .highlight_color = ImVec4(1.0f, 0.4f, 0.0f, 0.3f),    // Selection highlight
        .corner_radius = 7.0f,                                  // Rounded corners
        .border_thickness = 1.0f,                               // Border thickness
        .padding_multiplier = 1.25f,                            // Increased spacing
        .name = "Sunset Synthwave"
    };

    // Ocean Blue Theme - Deep ocean blue, teal
    g_themes[THEME_OCEAN_BLUE] = {
        .primary_color = ImVec4(0.0f, 0.4f, 0.8f, 1.0f),      // Deep ocean blue
        .secondary_color = ImVec4(0.0f, 0.7f, 0.7f, 1.0f),    // Teal
        .accent_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f),       // White
        .success_color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f),      // Green
        .warning_color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f),      // Yellow
        .error_color = ImVec4(0.8f, 0.2f, 0.2f, 1.0f),        // Red
        .background_dark = ImVec4(0.05f, 0.08f, 0.12f, 1.0f), // Dark blue-black
        .background_medium = ImVec4(0.08f, 0.12f, 0.18f, 1.0f), // Medium blue-black
        .background_light = ImVec4(0.12f, 0.16f, 0.24f, 1.0f), // Light blue-black
        .text_primary = ImVec4(0.95f, 0.95f, 0.95f, 1.0f),    // Bright white
        .text_secondary = ImVec4(0.8f, 0.8f, 0.8f, 1.0f),     // Light gray
        .text_muted = ImVec4(0.6f, 0.6f, 0.6f, 1.0f),         // Medium gray
        .border_color = ImVec4(0.2f, 0.3f, 0.4f, 1.0f),       // Subtle border
        .highlight_color = ImVec4(0.0f, 0.4f, 0.8f, 0.3f),    // Selection highlight
        .corner_radius = 6.0f,                                  // Rounded corners
        .border_thickness = 1.0f,                               // Border thickness
        .padding_multiplier = 1.2f,                             // Increased spacing
        .name = "Ocean Blue"
    };
}

// ============================================================================
// Theme Management Functions
// ============================================================================

void ui_theme_manager_init_themes() {
    init_theme_definitions();
}

void ui_theme_manager_init_current_theme() {
    // Copy the selected theme to the current theme
    memcpy(&g_current_theme, &g_themes[g_current_theme_type], sizeof(UITheme));
}

void ui_theme_manager_apply_theme() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Colors
    style.Colors[ImGuiCol_Text] = g_current_theme.text_primary;
    style.Colors[ImGuiCol_TextDisabled] = g_current_theme.text_muted;
    style.Colors[ImGuiCol_WindowBg] = g_current_theme.background_dark;
    style.Colors[ImGuiCol_ChildBg] = g_current_theme.background_medium;
    style.Colors[ImGuiCol_PopupBg] = g_current_theme.background_medium;
    style.Colors[ImGuiCol_Border] = g_current_theme.border_color;
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    
    // Button colors
    style.Colors[ImGuiCol_Button] = g_current_theme.background_light;
    style.Colors[ImGuiCol_ButtonHovered] = g_current_theme.primary_color;
    style.Colors[ImGuiCol_ButtonActive] = g_current_theme.secondary_color;
    
    // Header colors
    style.Colors[ImGuiCol_Header] = g_current_theme.background_light;
    style.Colors[ImGuiCol_HeaderHovered] = g_current_theme.primary_color;
    style.Colors[ImGuiCol_HeaderActive] = g_current_theme.secondary_color;
    
    // Tab colors
    style.Colors[ImGuiCol_Tab] = g_current_theme.background_light;
    style.Colors[ImGuiCol_TabHovered] = g_current_theme.primary_color;
    style.Colors[ImGuiCol_TabActive] = g_current_theme.secondary_color;
    
    // Selection colors
    style.Colors[ImGuiCol_NavHighlight] = g_current_theme.accent_color;
    
    // Slider colors
    style.Colors[ImGuiCol_SliderGrab] = g_current_theme.primary_color;
    style.Colors[ImGuiCol_SliderGrabActive] = g_current_theme.secondary_color;
    
    // Progress bar colors
    style.Colors[ImGuiCol_PlotHistogram] = g_current_theme.primary_color;
    style.Colors[ImGuiCol_PlotHistogramHovered] = g_current_theme.secondary_color;
    
    // Frame colors
    style.Colors[ImGuiCol_FrameBg] = g_current_theme.background_light;
    style.Colors[ImGuiCol_FrameBgHovered] = g_current_theme.background_medium;
    style.Colors[ImGuiCol_FrameBgActive] = g_current_theme.primary_color;
    
    // Input field colors
    style.Colors[ImGuiCol_TextSelectedBg] = g_current_theme.highlight_color;
    
    // Spacing and sizing
    style.WindowPadding = ImVec2(15 * g_current_theme.padding_multiplier, 15 * g_current_theme.padding_multiplier);
    style.FramePadding = ImVec2(8 * g_current_theme.padding_multiplier, 4 * g_current_theme.padding_multiplier);
    style.ItemSpacing = ImVec2(10 * g_current_theme.padding_multiplier, 8 * g_current_theme.padding_multiplier);
    style.ItemInnerSpacing = ImVec2(6 * g_current_theme.padding_multiplier, 4 * g_current_theme.padding_multiplier);
    style.ScrollbarSize = 16.0f;
    style.GrabMinSize = 8.0f;
    
    // Borders and corners
    style.WindowRounding = g_current_theme.corner_radius;
    style.ChildRounding = g_current_theme.corner_radius;
    style.FrameRounding = g_current_theme.corner_radius;
    style.PopupRounding = g_current_theme.corner_radius;
    style.ScrollbarRounding = g_current_theme.corner_radius;
    style.GrabRounding = g_current_theme.corner_radius;
    style.TabRounding = g_current_theme.corner_radius;
    
    // Borders
    style.WindowBorderSize = g_current_theme.border_thickness;
    style.ChildBorderSize = g_current_theme.border_thickness;
    style.PopupBorderSize = g_current_theme.border_thickness;
    style.FrameBorderSize = g_current_theme.border_thickness;
    style.TabBorderSize = g_current_theme.border_thickness;
}

void ui_theme_manager_switch_theme(ThemeType theme) {
    if (theme >= 0 && theme < THEME_COUNT) {
        g_current_theme_type = theme;
        ui_theme_manager_init_current_theme();
        ui_theme_manager_apply_theme();
        
        // Update global compatibility variables
        g_ui_theme = g_current_theme;
        g_current_theme_global = g_current_theme_type;
    }
}

// ============================================================================
// Theme Access Functions
// ============================================================================

const UITheme* ui_theme_manager_get_current_theme() {
    return &g_current_theme;
}

ThemeType ui_theme_manager_get_current_theme_type() {
    return g_current_theme_type;
}

const UITheme* ui_theme_manager_get_theme(ThemeType theme_type) {
    if (theme_type >= 0 && theme_type < THEME_COUNT) {
        return &g_themes[theme_type];
    }
    return NULL;
}

const char* ui_theme_manager_get_theme_name(ThemeType theme_type) {
    if (theme_type >= 0 && theme_type < THEME_COUNT) {
        return g_themes[theme_type].name;
    }
    return "Unknown";
}

// ============================================================================
// Professional Rendering Functions
// ============================================================================

void ui_theme_manager_render_professional_button(const char* label, const ImVec2& size, bool* clicked, 
                                                const ImVec4& color, const char* tooltip) {
    ImGui::PushStyleColor(ImGuiCol_Button, color.x == 0 ? g_current_theme.background_light : color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color.x == 0 ? g_current_theme.primary_color : 
                         ImVec4(color.x * 1.2f, color.y * 1.2f, color.z * 1.2f, color.w));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, color.x == 0 ? g_current_theme.secondary_color : 
                         ImVec4(color.x * 0.8f, color.y * 0.8f, color.z * 0.8f, color.w));
    
    if (ImGui::Button(label, size)) {
        if (clicked) *clicked = true;
    }
    
    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }
    
    ImGui::PopStyleColor(3);
}

void ui_theme_manager_render_section_header(const char* title, const char* subtitle, ImVec4 color) {
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::Text("%s", title);
    if (subtitle && strlen(subtitle) > 0) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, g_current_theme.text_secondary);
        ImGui::Text(" - %s", subtitle);
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleColor();
    ImGui::Separator();
}



void ui_theme_manager_render_metric_card(const char* label, const char* value, const char* unit, 
                                        const ImVec4& value_color, const ImVec4& background_color) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, background_color);
    ImGui::BeginChild(label, ImVec2(0, 60), true);
    
    ImGui::TextColored(g_current_theme.text_secondary, "%s", label);
    ImGui::TextColored(value_color, "%s", value);
    if (unit && strlen(unit) > 0) {
        ImGui::SameLine();
        ImGui::TextColored(g_current_theme.text_muted, " %s", unit);
    }
    
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void ui_theme_manager_render_status_indicator(const char* label, bool is_active, const char* status_text) {
    ImGui::Text("%s: ", label);
    ImGui::SameLine();
    
    ImVec4 status_color = is_active ? g_current_theme.success_color : g_current_theme.error_color;
    ImGui::PushStyleColor(ImGuiCol_Text, status_color);
    ImGui::Text("%s", status_text);
    ImGui::PopStyleColor();
}

// ============================================================================
// Theme Customization Functions
// ============================================================================

bool ui_theme_manager_customize_theme(ThemeType theme_type, const UITheme* custom_theme) {
    if (theme_type >= 0 && theme_type < THEME_COUNT && custom_theme) {
        memcpy(&g_themes[theme_type], custom_theme, sizeof(UITheme));
        return true;
    }
    return false;
}

bool ui_theme_manager_save_custom_theme(const char* name, const UITheme* theme) {
    // TODO: Implement theme saving to file
    (void)name;
    (void)theme;
    return false;
}

bool ui_theme_manager_load_custom_theme(const char* name, UITheme* theme) {
    // TODO: Implement theme loading from file
    (void)name;
    (void)theme;
    return false;
}

// ============================================================================
// Module Initialization and Cleanup
// ============================================================================

bool init_ui_theme_manager() {
    if (g_module_initialized) {
        return true;
    }
    
    // Initialize theme definitions
    ui_theme_manager_init_themes();
    
    // Initialize current theme
    ui_theme_manager_init_current_theme();
    
    // Apply the theme
    ui_theme_manager_apply_theme();
    
    // Initialize global compatibility variables
    g_ui_theme = g_current_theme;
    g_current_theme_global = g_current_theme_type;
    
    g_module_initialized = true;
    return true;
}

void cleanup_ui_theme_manager() {
    if (!g_module_initialized) {
        return;
    }
    
    // Clear module state
    g_module_initialized = false;
}

// ============================================================================
// Legacy Function Names for Compatibility
// ============================================================================

void init_ui_theme() {
    ui_theme_manager_init_themes();
}

void apply_ui_theme() {
    ui_theme_manager_apply_theme();
}

void switch_theme(ThemeType theme) {
    ui_theme_manager_switch_theme(theme);
}

void render_section_header(const char* title, const char* subtitle, const ImVec4& accent_color) {
    ui_theme_manager_render_section_header(title, subtitle, accent_color);
}

void render_status_indicator(const char* label, bool is_active, const char* status_text) {
    ui_theme_manager_render_status_indicator(label, is_active, status_text);
}

void render_metric_card(const char* label, const char* value, const char* unit, 
                       const ImVec4& value_color, const char* description) {
    ui_theme_manager_render_metric_card(label, value, unit, value_color, 
                                       value_color.x == 0 ? g_current_theme.background_medium : value_color);
}

void render_professional_button(const char* label, const ImVec2& size, bool* clicked, 
                               const ImVec4& color, const char* tooltip) {
    ui_theme_manager_render_professional_button(label, size, clicked, color, tooltip);
}

void render_professional_button(const char* label, const ImVec2& size, bool* clicked, 
                               const ImVec4& color) {
    ui_theme_manager_render_professional_button(label, size, clicked, color, nullptr);
}

void render_professional_button(const char* label, const ImVec2& size, bool* clicked) {
    ui_theme_manager_render_professional_button(label, size, clicked, ImVec4(0,0,0,0), nullptr);
}
