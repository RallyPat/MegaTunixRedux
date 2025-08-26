/*
 * MegaTunix Redux - Logging System Module Implementation
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

#include "../../include/ui/logging_system.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>

// Local module state
static bool g_module_initialized = false;
static LoggingSystemState g_logging_state = {0};

// Global state access for compatibility
LoggingSystemState* g_logging_system_state = &g_logging_state;

// ============================================================================
// Core Logging Functions
// ============================================================================

void add_log_entry(int level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    add_log_entry_va(level, format, args);
    va_end(args);
}

void add_log_entry_va(int level, const char* format, va_list args) {
    if (!g_module_initialized) {
        return;
    }
    
    // Create timestamp
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
    
    // Format the message
    char temp_message[MAX_LOG_LINE_LENGTH];
    vsnprintf(temp_message, sizeof(temp_message), format, args);
    
    // Create full log entry
    char full_message[MAX_LOG_LINE_LENGTH];
    const char* level_str[] = {"INFO", "WARN", "ERROR", "DEBUG"};
    snprintf(full_message, sizeof(full_message), "[%s] %s: %s", time_str, level_str[level], temp_message);
    
    // Add to circular buffer
    g_logging_state.entries[g_logging_state.index].timestamp = now;
    g_logging_state.entries[g_logging_state.index].level = level;
    strncpy(g_logging_state.entries[g_logging_state.index].message, full_message, MAX_LOG_LINE_LENGTH - 1);
    g_logging_state.entries[g_logging_state.index].message[MAX_LOG_LINE_LENGTH - 1] = '\0';
    
    g_logging_state.index = (g_logging_state.index + 1) % MAX_LOG_ENTRIES;
    if (g_logging_state.count < MAX_LOG_ENTRIES) {
        g_logging_state.count++;
    }
    
    // Update last log time
    g_logging_state.last_log_time = SDL_GetTicks();
}

// ============================================================================
// Log System Management
// ============================================================================

bool init_logging_system() {
    if (g_module_initialized) {
        return true;
    }
    
    // Initialize logging state
    memset(&g_logging_state, 0, sizeof(LoggingSystemState));
    g_logging_state.window_open = true;
    g_logging_state.auto_scroll = true;
    g_logging_state.filter_level = 0;
    g_logging_state.last_log_time = 0;
    
    g_module_initialized = true;
    return true;
}

void cleanup_logging_system() {
    if (!g_module_initialized) {
        return;
    }
    
    // Clear state
    memset(&g_logging_state, 0, sizeof(LoggingSystemState));
    g_module_initialized = false;
}

void reset_logging_system() {
    if (!g_module_initialized) {
        return;
    }
    
    g_logging_state.count = 0;
    g_logging_state.index = 0;
    g_logging_state.last_log_time = 0;
}

// ============================================================================
// Log Display and UI
// ============================================================================

void render_log_window() {
    if (!g_module_initialized) {
        return;
    }
    
    // Position the log window at the bottom of the screen
    ImVec2 display_size = ImGui::GetIO().DisplaySize;
    float log_height = 180.0f;
    float log_y = display_size.y - log_height - 10.0f; // 10px margin from bottom
    
    ImGui::SetNextWindowPos(ImVec2(10, log_y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(display_size.x - 20, log_height), ImGuiCond_Always);
    
    ImGuiWindowFlags window_flags = 
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove;
    
    if (!ImGui::Begin("System Log", &g_logging_state.window_open, window_flags)) {
        ImGui::End();
        return;
    }
    
    // Log controls section
    ImGui::BeginGroup();
    
    // Auto-scroll toggle
    ImGui::Checkbox("Auto-scroll", &g_logging_state.auto_scroll);
    ImGui::SameLine();
    
    // Filter dropdown
    ImGui::Text("Filter:");
    ImGui::SameLine();
    
    const char* filter_items[] = {"All", "Warning+", "Error Only", "High Priority Only"};
    ImGui::SetNextItemWidth(150);
    if (ImGui::BeginCombo("##filter", filter_items[g_logging_state.filter_level])) {
        for (int i = 0; i < IM_ARRAYSIZE(filter_items); i++) {
            const bool is_selected = (g_logging_state.filter_level == i);
            if (ImGui::Selectable(filter_items[i], is_selected)) {
                g_logging_state.filter_level = i;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    ImGui::EndGroup();
    
    ImGui::Separator();
    
    // Log display area
    float controls_height = ImGui::GetFrameHeight() + 8.0f;
    ImVec2 child_size = ImVec2(0, ImGui::GetWindowHeight() - controls_height - ImGui::GetStyle().WindowPadding.y * 2);
    if (child_size.y < 60.0f) child_size.y = 60.0f;
    
    ImGui::BeginChild("LogContent", child_size, false, ImGuiWindowFlags_HorizontalScrollbar);
    
    // Display log entries
    for (int i = 0; i < g_logging_state.count; i++) {
        int idx = (g_logging_state.index - g_logging_state.count + i + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
        LogEntry* entry = &g_logging_state.entries[idx];
        
        // Apply filter
        bool show_entry = true;
        if (g_logging_state.filter_level == 1 && entry->level < 1) { // Warning+
            show_entry = false;
        } else if (g_logging_state.filter_level == 2 && entry->level < 2) { // Error Only
            show_entry = false;
        } else if (g_logging_state.filter_level == 3) { // High Priority Only
            show_entry = (strstr(entry->message, "***") != NULL);
        }
        
        if (!show_entry) {
            continue;
        }
        
        // Color based on log level
        ImVec4 color;
        if (strstr(entry->message, "***") != NULL) {
            // High priority messages
            color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f); // Gold
        } else {
            switch (entry->level) {
                case 0: // INFO
                    color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
                    break;
                case 1: // WARNING
                    color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
                    break;
                case 2: // ERROR
                    color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
                    break;
                case 3: // DEBUG
                    color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); // Gray
                    break;
                default:
                    color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
            }
        }
        
        ImGui::TextColored(color, "%s", entry->message);
    }
    
    // Auto-scroll to bottom
    if (g_logging_state.auto_scroll) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
    ImGui::End();
}

void render_log_tab_content() {
    if (!g_module_initialized) {
        return;
    }
    
    // Log management section
    ImGui::Text("Log Management");
    ImGui::Separator();
    
    ImGui::BeginGroup();
    
    // Log control buttons
    bool clear_logs_clicked = false;
    if (ImGui::Button("Clear Logs", ImVec2(120, 30))) {
        clear_logs();
    }
    
    ImGui::SameLine();
    
    bool export_logs_clicked = false;
    if (ImGui::Button("Export Logs", ImVec2(120, 30))) {
        export_logs_to_file("megatunix_logs.txt");
    }
    
    ImGui::SameLine();
    
    bool rotate_logs_clicked = false;
    if (ImGui::Button("Rotate Logs", ImVec2(120, 30))) {
        rotate_logs();
    }
    
    ImGui::EndGroup();
    
    ImGui::Spacing();
    
    // Log statistics
    ImGui::Text("Log Statistics:");
    ImGui::Text("Total entries: %d", g_logging_state.count);
    ImGui::Text("Current index: %d", g_logging_state.index);
    ImGui::Text("Filter level: %d", g_logging_state.filter_level);
    ImGui::Text("Auto-scroll: %s", g_logging_state.auto_scroll ? "Enabled" : "Disabled");
}

// ============================================================================
// Log Filtering and Control
// ============================================================================

void set_log_filter_level(int level) {
    if (g_module_initialized && level >= 0 && level <= 3) {
        g_logging_state.filter_level = level;
    }
}

int get_log_filter_level() {
    return g_module_initialized ? g_logging_state.filter_level : 0;
}

void set_log_auto_scroll(bool enabled) {
    if (g_module_initialized) {
        g_logging_state.auto_scroll = enabled;
    }
}

bool get_log_auto_scroll() {
    return g_module_initialized ? g_logging_state.auto_scroll : true;
}

void toggle_log_window() {
    if (g_module_initialized) {
        g_logging_state.window_open = !g_logging_state.window_open;
    }
}

bool is_log_window_open() {
    return g_module_initialized ? g_logging_state.window_open : false;
}

// ============================================================================
// Log Data Access
// ============================================================================

const LogEntry* get_log_entry(int index) {
    if (!g_module_initialized || index < 0 || index >= g_logging_state.count) {
        return NULL;
    }
    
    int actual_index = (g_logging_state.index - g_logging_state.count + index + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
    return &g_logging_state.entries[actual_index];
}

int get_log_count() {
    return g_module_initialized ? g_logging_state.count : 0;
}

int get_log_index() {
    return g_module_initialized ? g_logging_state.index : 0;
}

const LogEntry* get_log_entries() {
    return g_module_initialized ? g_logging_state.entries : NULL;
}

// ============================================================================
// Log Utility Functions
// ============================================================================

void clear_logs() {
    if (g_module_initialized) {
        reset_logging_system();
    }
}

void export_logs_to_file(const char* filename) {
    if (!g_module_initialized || !filename) {
        return;
    }
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        return;
    }
    
    fprintf(file, "MegaTunix Redux - Log Export\n");
    fprintf(file, "Generated: %s\n", ctime(&g_logging_state.entries[0].timestamp));
    fprintf(file, "Total entries: %d\n\n", g_logging_state.count);
    
    for (int i = 0; i < g_logging_state.count; i++) {
        const LogEntry* entry = get_log_entry(i);
        if (entry) {
            fprintf(file, "%s\n", entry->message);
        }
    }
    
    fclose(file);
}

void rotate_logs() {
    if (!g_module_initialized) {
        return;
    }
    
    // For now, just clear the logs
    // TODO: Implement proper log rotation with file archiving
    clear_logs();
}

// ============================================================================
// Legacy Function Names for Compatibility
// ============================================================================

void render_log_window_legacy() {
    render_log_window();
}
