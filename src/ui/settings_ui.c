/*
 * Settings UI Implementation
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * This is a minimal implementation that will be updated for ImGui.
 */

#include "../include/ui/settings_ui.h"
// Clay removed - will be replaced with ImGui
#include <stdio.h>
#include <string.h>

bool settings_ui_init(SettingsUIState* state) {
    if (!state) return false;
    
    memset(state, 0, sizeof(SettingsUIState));
    state->is_open = false;
    state->active_tab = SETTINGS_TAB_GENERAL;
    
    return true;
}

void settings_ui_cleanup(SettingsUIState* state) {
    if (!state) return;
    // Nothing to clean up for now
}

void settings_ui_open(SettingsUIState* state, const AppConfig* current_config) {
    if (!state || !current_config) return;
    
    state->is_open = true;
    state->temp_config = *current_config;
    state->is_dirty = false;
    settings_ui_clear_messages(state);
}

void settings_ui_close(SettingsUIState* state) {
    if (!state) return;
    state->is_open = false;
}

bool settings_ui_is_open(const SettingsUIState* state) {
    return state && state->is_open;
}

void settings_ui_render(SettingsUIState* state) {
    if (!state || !state->is_open) return;
    
    // Placeholder for ImGui rendering
    // This will be implemented with ImGui
}

void settings_ui_render_general_tab(SettingsUIState* state) {
    // Placeholder for ImGui rendering
    // This will be implemented with ImGui
}

void settings_ui_render_ecu_tab(SettingsUIState* state) {
    // Placeholder for ImGui rendering
    // This will be implemented with ImGui
}

void settings_ui_render_dashboard_tab(SettingsUIState* state) {
    // Placeholder for ImGui rendering
    // This will be implemented with ImGui
}

void settings_ui_render_logging_tab(SettingsUIState* state) {
    // Placeholder for ImGui rendering
    // This will be implemented with ImGui
}

void settings_ui_render_ui_tab(SettingsUIState* state) {
    // Placeholder for ImGui rendering
    // This will be implemented with ImGui
}

bool settings_ui_apply_changes(SettingsUIState* state, AppConfig* target_config) {
    if (!state || !target_config) return false;
    
    if (state->is_dirty) {
        *target_config = state->temp_config;
        state->is_dirty = false;
        settings_ui_set_success(state, "Settings applied successfully");
        return true;
    }
    
    return false;
}

bool settings_ui_reset_to_defaults(SettingsUIState* state) {
    if (!state) return false;
    
    // Reset to default configuration
    state->temp_config = config_get_defaults();
    state->is_dirty = true;
    settings_ui_set_success(state, "Settings reset to defaults");
    
    return true;
}

bool settings_ui_import_config(SettingsUIState* state, const char* filename) {
    // Placeholder for ImGui implementation
    // This will be implemented with ImGui
    return false;
}

bool settings_ui_export_config(SettingsUIState* state, const char* filename) {
    // Placeholder for ImGui implementation
    // This will be implemented with ImGui
    return false;
}

bool settings_ui_validate_config(const AppConfig* config) {
    if (!config) return false;
    
    // Basic validation
    if (config->window_width < 800 || config->window_width > 1920) return false;
    if (config->window_height < 600 || config->window_height > 1080) return false;
    
    return true;
}

void settings_ui_fix_config(AppConfig* config) {
    if (!config) return;
    
    // Fix invalid values
    if (config->window_width < 800) config->window_width = 800;
    if (config->window_width > 1920) config->window_width = 1920;
    if (config->window_height < 600) config->window_height = 600;
    if (config->window_height > 1080) config->window_height = 1080;
}

const char* settings_ui_get_tab_name(SettingsTab tab) {
    switch (tab) {
        case SETTINGS_TAB_GENERAL: return "General";
        case SETTINGS_TAB_ECU: return "ECU";
        case SETTINGS_TAB_DASHBOARD: return "Dashboard";
        case SETTINGS_TAB_LOGGING: return "Logging";
        case SETTINGS_TAB_UI: return "UI";
        default: return "Unknown";
    }
}

void settings_ui_set_error(SettingsUIState* state, const char* error) {
    if (!state || !error) return;
    strncpy(state->error_message, error, sizeof(state->error_message) - 1);
    state->error_message[sizeof(state->error_message) - 1] = '\0';
}

void settings_ui_set_success(SettingsUIState* state, const char* message) {
    if (!state || !message) return;
    strncpy(state->success_message, message, sizeof(state->success_message) - 1);
    state->success_message[sizeof(state->success_message) - 1] = '\0';
}

void settings_ui_clear_messages(SettingsUIState* state) {
    if (!state) return;
    state->error_message[0] = '\0';
    state->success_message[0] = '\0';
} 