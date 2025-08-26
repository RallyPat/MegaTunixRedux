/*
 * MegaTunix Redux - Table Operations Module
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

#ifndef TABLE_OPERATIONS_H
#define TABLE_OPERATIONS_H

#include <stdbool.h>
#include <stdint.h>
#include "imgui_ve_table.h"

// Forward declarations
// Note: ImGuiTable is defined in imgui_ve_table.h

// Clipboard data structure
typedef struct {
    float data[32][32];  // Maximum clipboard size
    int width;
    int height;
    bool has_data;
} TableClipboard;

// Multi-selection structure
typedef struct {
    bool active;
    int start_x, start_y;
    int end_x, end_y;
} MultiSelection;

// Interpolation modes
typedef enum {
    TABLE_INTERPOLATION_LINEAR = 0,
    TABLE_INTERPOLATION_CUBIC = 1,
    TABLE_INTERPOLATION_GAUSSIAN = 2
} InterpolationMode;

// Smoothing modes
typedef enum {
    SMOOTH_GAUSSIAN = 0,
    SMOOTH_MOVING_AVERAGE = 1,
    SMOOTH_BILATERAL = 2
} SmoothingMode;

// Module initialization and cleanup
bool init_table_operations(void);
void cleanup_table_operations(void);

// Clipboard operations
void copy_selection_to_clipboard(ImGuiTable* table, const MultiSelection* selection);
void paste_from_clipboard(ImGuiTable* table, int target_x, int target_y);
void paste_block_at(ImGuiTable* table, int start_x, int start_y);
void paste_special_block_at(ImGuiTable* table, int start_x, int start_y, int mode);
void clear_clipboard(void);
bool has_clipboard_data(void);
const TableClipboard* get_clipboard(void);

// Interpolation operations
void interpolate_between_cells(ImGuiTable* table, int x1, int y1, int x2, int y2);
void horizontal_interpolate_selection(ImGuiTable* table, const MultiSelection* selection);
void vertical_interpolate_selection(ImGuiTable* table, const MultiSelection* selection);
void interpolate_table_values(ImGuiTable* table, int start_x, int start_y, int end_x, int end_y);

// Smoothing operations
void smooth_selection(ImGuiTable* table, const MultiSelection* selection);
void gaussian_smooth_selection(ImGuiTable* table, const MultiSelection* selection);
void moving_average_smooth_selection(ImGuiTable* table, const MultiSelection* selection);
void bilateral_smooth_selection(ImGuiTable* table, const MultiSelection* selection);
void smooth_table_region(ImGuiTable* table, int center_x, int center_y, int radius);

// Professional table operations
void apply_table_math_operation(ImGuiTable* table, const char* operation, float value);
void export_table_to_file(ImGuiTable* table, const char* filename);
void import_table_from_file(ImGuiTable* table, const char* filename);

// Multi-selection management
void start_multi_selection(MultiSelection* selection, int x, int y);
void update_multi_selection(MultiSelection* selection, int x, int y);
void end_multi_selection(MultiSelection* selection);
void clear_multi_selection(MultiSelection* selection);
bool is_cell_in_selection(const MultiSelection* selection, int x, int y);
void get_selection_bounds(const MultiSelection* selection, int* min_x, int* min_y, int* max_x, int* max_y);
int get_selection_cell_count(const MultiSelection* selection);

// Utility functions
bool validate_table_operation(ImGuiTable* table, int x, int y);
bool validate_selection_bounds(const MultiSelection* selection, int max_width, int max_height);
float calculate_interpolation_weight(float t, InterpolationMode mode);

// Legacy function names for compatibility
void copy_selection_to_clipboard_legacy(void);
void paste_from_clipboard_legacy(void);
void interpolate_between_cells_legacy(void);
void smooth_selection_legacy(void);
void gaussian_smooth_selection_legacy(void);
void moving_average_smooth_selection_legacy(void);
void bilateral_smooth_selection_legacy(void);
void horizontal_interpolate_selection_legacy(void);
void vertical_interpolate_selection_legacy(void);

// Additional legacy functions needed by main.cpp
void start_multi_selection(int x, int y);
void update_multi_selection(int x, int y);
void end_multi_selection(void);
void clear_multi_selection(void);
bool is_cell_in_selection(int x, int y);
void get_selection_bounds(int* min_x, int* min_y, int* max_x, int* max_y);
int get_selection_cell_count(void);

// Global clipboard access for compatibility
extern TableClipboard* g_table_clipboard_global;

#endif // TABLE_OPERATIONS_H
