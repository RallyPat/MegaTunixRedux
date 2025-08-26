/*
 * ImGui File Dialog - Simple file browser for ImGui
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * Provides file browsing functionality for INI file selection.
 */

#ifndef IMGUI_FILE_DIALOG_H
#define IMGUI_FILE_DIALOG_H

#include <stdbool.h>
#include <SDL2/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

// File dialog state
typedef struct {
    bool is_open;
    char current_path[512];
    char selected_file[512];
    char filter[64];
    bool file_selected;
    bool cancelled;
} ImGuiFileDialog;

// File dialog functions
ImGuiFileDialog* imgui_file_dialog_create(void);
void imgui_file_dialog_destroy(ImGuiFileDialog* dialog);
void imgui_file_dialog_open(ImGuiFileDialog* dialog, const char* title, const char* filter);
void imgui_file_dialog_close(ImGuiFileDialog* dialog);
bool imgui_file_dialog_render(ImGuiFileDialog* dialog);
const char* imgui_file_dialog_get_selected_file(ImGuiFileDialog* dialog);
bool imgui_file_dialog_was_cancelled(ImGuiFileDialog* dialog);

// Utility functions
bool imgui_file_dialog_browse_ini_file(char* file_path, size_t max_len);
void imgui_file_dialog_set_default_path(const char* path);

#ifdef __cplusplus
}
#endif

#endif // IMGUI_FILE_DIALOG_H 