/*
 * ImGui File Dialog - Simple file browser for ImGui
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * Provides file browsing functionality for INI file selection.
 */

#include "../../include/ui/imgui_file_dialog.h"
#include "../../external/imgui/imgui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <string>

// Global default path
static char g_default_path[512] = "/home/pat/Documents/TunerStudioFiles";

// File entry structure
struct FileEntry {
    std::string name;
    std::string path;
    bool is_directory;
    bool is_parent;
};

// File dialog implementation
ImGuiFileDialog* imgui_file_dialog_create(void) {
    ImGuiFileDialog* dialog = (ImGuiFileDialog*)malloc(sizeof(ImGuiFileDialog));
    if (!dialog) return NULL;
    
    memset(dialog, 0, sizeof(ImGuiFileDialog));
    strcpy(dialog->current_path, g_default_path);
    strcpy(dialog->filter, "INI files (*.ini)");
    
    return dialog;
}

void imgui_file_dialog_destroy(ImGuiFileDialog* dialog) {
    if (dialog) {
        free(dialog);
    }
}

void imgui_file_dialog_open(ImGuiFileDialog* dialog, const char* title, const char* filter) {
    if (!dialog) return;
    
    dialog->is_open = true;
    dialog->file_selected = false;
    dialog->cancelled = false;
    memset(dialog->selected_file, 0, sizeof(dialog->selected_file));
    
    if (filter) {
        strncpy(dialog->filter, filter, sizeof(dialog->filter) - 1);
    }
}

void imgui_file_dialog_close(ImGuiFileDialog* dialog) {
    if (!dialog) return;
    
    dialog->is_open = false;
}

bool imgui_file_dialog_render(ImGuiFileDialog* dialog) {
    if (!dialog || !dialog->is_open) return false;
    
    bool result = false;
    
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Select INI File", &dialog->is_open, ImGuiWindowFlags_NoCollapse)) {
        
        // Current path display
        ImGui::Text("Current Path: %s", dialog->current_path);
        ImGui::Separator();
        
        // File list
        if (ImGui::BeginChild("##FileList", ImVec2(0, -60), true)) {
            
            std::vector<FileEntry> files;
            
            // Add parent directory entry
            FileEntry parent;
            parent.name = "..";
            parent.path = dialog->current_path;
            parent.is_directory = true;
            parent.is_parent = true;
            files.push_back(parent);
            
            // Read directory
            DIR* dir = opendir(dialog->current_path);
            if (dir) {
                struct dirent* entry;
                while ((entry = readdir(dir)) != NULL) {
                    // Skip . and .. entries (we handle .. manually)
                    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                        continue;
                    }
                    
                    FileEntry file;
                    file.name = entry->d_name;
                    file.path = std::string(dialog->current_path) + "/" + entry->d_name;
                    file.is_parent = false;
                    
                    // Check if it's a directory
                    struct stat st;
                    if (stat(file.path.c_str(), &st) == 0) {
                        file.is_directory = S_ISDIR(st.st_mode);
                    } else {
                        file.is_directory = false;
                    }
                    
                    // Filter for INI files only (unless it's a directory)
                    if (file.is_directory || strstr(entry->d_name, ".ini") != NULL) {
                        files.push_back(file);
                    }
                }
                closedir(dir);
            }
            
            // Display files
            for (const auto& file : files) {
                ImGui::PushID(file.name.c_str());
                
                // Icon and name
                const char* icon = file.is_directory ? "📁" : "📄";
                const char* name = file.name.c_str();
                
                if (ImGui::Selectable(name, false, ImGuiSelectableFlags_AllowDoubleClick)) {
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        if (file.is_directory) {
                            if (file.is_parent) {
                                // Go to parent directory
                                char* last_slash = strrchr(dialog->current_path, '/');
                                if (last_slash && last_slash != dialog->current_path) {
                                    *last_slash = '\0';
                                }
                            } else {
                                // Enter directory
                                strncpy(dialog->current_path, file.path.c_str(), sizeof(dialog->current_path) - 1);
                            }
                        } else {
                            // Select file
                            strncpy(dialog->selected_file, file.path.c_str(), sizeof(dialog->selected_file) - 1);
                            dialog->file_selected = true;
                            dialog->is_open = false;
                            result = true;
                        }
                    }
                }
                
                // Show icon and name
                ImGui::SameLine();
                ImGui::Text("%s %s", icon, name);
                
                ImGui::PopID();
            }
            
            ImGui::EndChild();
        }
        
        // Buttons
        ImGui::Separator();
        
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            dialog->cancelled = true;
            dialog->is_open = false;
        }
        
        ImGui::SameLine();
        ImGui::Text("Double-click to select file or enter directory");
        
        ImGui::End();
    }
    
    return result;
}

const char* imgui_file_dialog_get_selected_file(ImGuiFileDialog* dialog) {
    if (!dialog) return NULL;
    return dialog->selected_file;
}

bool imgui_file_dialog_was_cancelled(ImGuiFileDialog* dialog) {
    if (!dialog) return true;
    return dialog->cancelled;
}

// Utility functions
bool imgui_file_dialog_browse_ini_file(char* file_path, size_t max_len) {
    ImGuiFileDialog* dialog = imgui_file_dialog_create();
    if (!dialog) return false;
    
    imgui_file_dialog_open(dialog, "Select INI File", "INI files (*.ini)");
    
    // This would need to be called from the main render loop
    // For now, we'll implement a simpler approach
    imgui_file_dialog_destroy(dialog);
    return false;
}

void imgui_file_dialog_set_default_path(const char* path) {
    if (path) {
        strncpy(g_default_path, path, sizeof(g_default_path) - 1);
    }
} 