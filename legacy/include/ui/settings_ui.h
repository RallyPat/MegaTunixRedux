#ifndef MEGATUNIX_REDUX_SETTINGS_UI_H
#define MEGATUNIX_REDUX_SETTINGS_UI_H

#include <stdbool.h>
#include <SDL2/SDL.h>
#include "../utils/config.h"
#include "../ecu/ecu_communication.h"
// Clay removed - will be replaced with ImGui

// Settings UI state
typedef struct {
    bool is_open;
    bool is_dirty;
    int active_tab;
    AppConfig temp_config;
    char error_message[256];
    char success_message[256];
} SettingsUIState;

// Settings categories
typedef enum {
    SETTINGS_TAB_GENERAL = 0,
    SETTINGS_TAB_ECU,
    SETTINGS_TAB_DASHBOARD,
    SETTINGS_TAB_LOGGING,
    SETTINGS_TAB_UI,
    SETTINGS_TAB_COUNT
} SettingsTab;

// Function declarations
bool settings_ui_init(SettingsUIState* state);
void settings_ui_cleanup(SettingsUIState* state);
void settings_ui_open(SettingsUIState* state, const AppConfig* current_config);
void settings_ui_close(SettingsUIState* state);
bool settings_ui_is_open(const SettingsUIState* state);

// Main rendering function
void settings_ui_render(SettingsUIState* state);

// Tab-specific rendering functions
void settings_ui_render_general_tab(SettingsUIState* state);
void settings_ui_render_ecu_tab(SettingsUIState* state);
void settings_ui_render_dashboard_tab(SettingsUIState* state);
void settings_ui_render_logging_tab(SettingsUIState* state);
void settings_ui_render_ui_tab(SettingsUIState* state);

// Action functions
bool settings_ui_apply_changes(SettingsUIState* state, AppConfig* target_config);
bool settings_ui_reset_to_defaults(SettingsUIState* state);
bool settings_ui_import_config(SettingsUIState* state, const char* filename);
bool settings_ui_export_config(SettingsUIState* state, const char* filename);

// Validation functions
bool settings_ui_validate_config(const AppConfig* config);
void settings_ui_fix_config(AppConfig* config);

// Utility functions
const char* settings_ui_get_tab_name(SettingsTab tab);
void settings_ui_set_error(SettingsUIState* state, const char* error);
void settings_ui_set_success(SettingsUIState* state, const char* message);
void settings_ui_clear_messages(SettingsUIState* state);

#endif // MEGATUNIX_REDUX_SETTINGS_UI_H 