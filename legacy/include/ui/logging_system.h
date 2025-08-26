/*
 * MegaTunix Redux - Logging System Module
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

#ifndef LOGGING_SYSTEM_H
#define LOGGING_SYSTEM_H

#include <stdbool.h>
#include <time.h>
#include <cstdint>

// ImGui includes
#include "../../external/imgui/imgui.h"

// Log system constants
#define MAX_LOG_ENTRIES 1000
#define MAX_LOG_LINE_LENGTH 256

// Log entry structure
typedef struct {
    char message[MAX_LOG_LINE_LENGTH];
    time_t timestamp;
    int level; // 0=INFO, 1=WARNING, 2=ERROR, 3=DEBUG
} LogEntry;

// Log system state
typedef struct {
    LogEntry entries[MAX_LOG_ENTRIES];
    int count;
    int index;
    bool window_open;
    bool auto_scroll;
    int filter_level; // 0=all, 1=warning+, 2=error only, 3=high priority only
    uint32_t last_log_time;
} LoggingSystemState;

// Core logging functions
void add_log_entry(int level, const char* format, ...);
void add_log_entry_va(int level, const char* format, va_list args);

// Log system management
bool init_logging_system(void);
void cleanup_logging_system(void);
void reset_logging_system(void);

// Log display and UI
void render_log_window(void);
void render_log_tab_content(void);

// Log filtering and control
void set_log_filter_level(int level);
int get_log_filter_level(void);
void set_log_auto_scroll(bool enabled);
bool get_log_auto_scroll(void);
void toggle_log_window(void);
bool is_log_window_open(void);

// Log data access
const LogEntry* get_log_entry(int index);
int get_log_count(void);
int get_log_index(void);
const LogEntry* get_log_entries(void);

// Log utility functions
void clear_logs(void);
void export_logs_to_file(const char* filename);
void rotate_logs(void);

// Legacy function names for compatibility
void render_log_window_legacy(void);

// Global state access for compatibility
extern LoggingSystemState* g_logging_system_state;

#endif // LOGGING_SYSTEM_H
