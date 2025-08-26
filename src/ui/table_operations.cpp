/*
 * MegaTunix Redux - Table Operations Module Implementation
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

#include "../../include/ui/table_operations.h"
#include "../../include/ui/logging_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Module state
static bool g_module_initialized = false;
static TableClipboard g_clipboard = {0};
static MultiSelection g_multi_selection = {0};

// Global clipboard access for compatibility
TableClipboard* g_table_clipboard_global = &g_clipboard;

// Module initialization and cleanup
bool init_table_operations(void) {
    if (g_module_initialized) {
        return true;
    }
    
    // Initialize clipboard
    memset(&g_clipboard, 0, sizeof(TableClipboard));
    g_clipboard.has_data = false;
    g_clipboard.width = 0;
    g_clipboard.height = 0;
    
    // Initialize multi-selection
    memset(&g_multi_selection, 0, sizeof(MultiSelection));
    g_multi_selection.active = false;
    
    g_module_initialized = true;
    add_log_entry(0, "Table Operations module initialized successfully");
    return true;
}

void cleanup_table_operations(void) {
    if (!g_module_initialized) {
        return;
    }
    
    // Clear clipboard
    clear_clipboard();
    
    // Clear multi-selection
    clear_multi_selection(&g_multi_selection);
    
    g_module_initialized = false;
    add_log_entry(0, "Table Operations module cleaned up");
}

// Clipboard operations
void copy_selection_to_clipboard(ImGuiTable* table, const MultiSelection* selection) {
    if (!table || !selection || !selection->active) {
        add_log_entry(2, "Copy failed: Invalid table or selection");
        return;
    }
    
    // Get selection bounds
    int min_x, min_y, max_x, max_y;
    get_selection_bounds(selection, &min_x, &min_y, &max_x, &max_y);
    
    // Calculate dimensions
    int width = max_x - min_x + 1;
    int height = max_y - min_y + 1;
    
    // Validate dimensions
    if (width <= 0 || height <= 0 || width > 32 || height > 32) {
        add_log_entry(2, "Copy failed: Invalid selection dimensions %dx%d", width, height);
        return;
    }
    
    // Clear existing clipboard
    clear_clipboard();
    
    // Copy data
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int table_x = min_x + x;
            int table_y = min_y + y;
            
            if (table_x >= 0 && table_x < table->width && 
                table_y >= 0 && table_y < table->height) {
                g_clipboard.data[y][x] = imgui_table_get_value(table, table_x, table_y);
            } else {
                g_clipboard.data[y][x] = 0.0f;
            }
        }
    }
    
    g_clipboard.width = width;
    g_clipboard.height = height;
    g_clipboard.has_data = true;
    
    add_log_entry(0, "Copied selection [%d,%d] to [%d,%d] (%dx%d) to clipboard", 
                 min_x, min_y, max_x, max_y, width, height);
}

void paste_from_clipboard(ImGuiTable* table, int target_x, int target_y) {
    if (!g_clipboard.has_data || !table) {
        add_log_entry(2, "Paste failed: No clipboard data or invalid table");
        return;
    }
    
    paste_block_at(table, target_x, target_y);
}

void paste_block_at(ImGuiTable* table, int start_x, int start_y) {
    if (!g_clipboard.has_data || !table) {
        add_log_entry(2, "Paste failed: No clipboard data or invalid table");
        return;
    }
    
    // Validate target position
    if (start_x < 0 || start_y < 0 || 
        start_x + g_clipboard.width > table->width ||
        start_y + g_clipboard.height > table->height) {
        add_log_entry(2, "Paste failed: Target position [%d,%d] would exceed table bounds", start_x, start_y);
        return;
    }
    
    // Paste data
    for (int y = 0; y < g_clipboard.height; y++) {
        for (int x = 0; x < g_clipboard.width; x++) {
            int table_x = start_x + x;
            int table_y = start_y + y;
            
            if (table_x >= 0 && table_x < table->width && 
                table_y >= 0 && table_y < table->height) {
                imgui_table_set_value(table, table_x, table_y, g_clipboard.data[y][x]);
            }
        }
    }
    
    add_log_entry(0, "Pasted clipboard data (%dx%d) at position [%d,%d]", 
                 g_clipboard.width, g_clipboard.height, start_x, start_y);
}

void paste_special_block_at(ImGuiTable* table, int start_x, int start_y, int mode) {
    if (!g_clipboard.has_data || !table) {
        add_log_entry(2, "Special paste failed: No clipboard data or invalid table");
        return;
    }
    
    // Validate target position
    if (start_x < 0 || start_y < 0 || 
        start_x + g_clipboard.width > table->width ||
        start_y + g_clipboard.height > table->height) {
        add_log_entry(2, "Special paste failed: Target position [%d,%d] would exceed table bounds", start_x, start_y);
        return;
    }
    
    // Apply special paste mode
    for (int y = 0; y < g_clipboard.height; y++) {
        for (int x = 0; x < g_clipboard.width; x++) {
            int table_x = start_x + x;
            int table_y = start_y + y;
            
            if (table_x >= 0 && table_x < table->width && 
                table_y >= 0 && table_y < table->height) {
                
                float current_value = imgui_table_get_value(table, table_x, table_y);
                float clipboard_value = g_clipboard.data[y][x];
                float new_value = current_value;
                
                switch (mode) {
                    case 0: // Multiply percentage
                        new_value = current_value * (clipboard_value / 100.0f);
                        break;
                    case 1: // Multiply raw
                        new_value = current_value * clipboard_value;
                        break;
                    case 2: // Add
                        new_value = current_value + clipboard_value;
                        break;
                    case 3: // Subtract
                        new_value = current_value - clipboard_value;
                        break;
                    default:
                        new_value = clipboard_value;
                        break;
                }
                
                imgui_table_set_value(table, table_x, table_y, new_value);
            }
        }
    }
    
    const char* mode_names[] = {"Multiply %", "Multiply Raw", "Add", "Subtract", "Replace"};
    add_log_entry(0, "Special paste mode '%s' applied at position [%d,%d]", 
                 mode_names[mode < 4 ? mode : 4], start_x, start_y);
}

void clear_clipboard(void) {
    memset(&g_clipboard, 0, sizeof(TableClipboard));
    g_clipboard.has_data = false;
    g_clipboard.width = 0;
    g_clipboard.height = 0;
    add_log_entry(0, "Clipboard cleared");
}

bool has_clipboard_data(void) {
    return g_clipboard.has_data;
}

const TableClipboard* get_clipboard(void) {
    return g_clipboard.has_data ? &g_clipboard : NULL;
}

// Interpolation operations
void interpolate_between_cells(ImGuiTable* table, int x1, int y1, int x2, int y2) {
    if (!table) {
        add_log_entry(2, "Interpolation failed: No table available");
        return;
    }
    
    if (x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0 ||
        x1 >= table->width || y1 >= table->height ||
        x2 >= table->width || y2 >= table->height) {
        add_log_entry(2, "Interpolation failed: Invalid coordinates [%d,%d] to [%d,%d]", x1, y1, x2, y2);
        return;
    }
    
    // Get values at endpoints
    float value1 = imgui_table_get_value(table, x1, y1);
    float value2 = imgui_table_get_value(table, x2, y2);
    
    // Calculate distance
    int dx = x2 - x1;
    int dy = y2 - y1;
    int distance = (int)sqrt(dx * dx + dy * dy);
    
    if (distance == 0) {
        add_log_entry(0, "Interpolation skipped: Same start and end points");
        return;
    }
    
    // Interpolate along the line
    for (int i = 1; i < distance; i++) {
        float t = (float)i / distance;
        int x = x1 + (int)(dx * t);
        int y = y1 + (int)(dy * t);
        
        if (x >= 0 && x < table->width && y >= 0 && y < table->height) {
            float interpolated_value = value1 + (value2 - value1) * t;
            imgui_table_set_value(table, x, y, interpolated_value);
        }
    }
    
    add_log_entry(0, "Interpolated between cells [%d,%d] and [%d,%d] over %d points", 
                 x1, y1, x2, y2, distance);
}

void horizontal_interpolate_selection(ImGuiTable* table, const MultiSelection* selection) {
    if (!table || !selection || !selection->active) {
        add_log_entry(2, "Horizontal interpolation failed: Invalid table or selection");
        return;
    }
    
    int min_x, min_y, max_x, max_y;
    get_selection_bounds(selection, &min_x, &min_y, &max_x, &max_y);
    
            // Interpolate each row in the selection
        for (int y = min_y; y <= max_y; y++) {
            if (y >= 0 && y < table->height) {
                bool success = imgui_table_interpolate_horizontal(table, min_x, max_x, y, INTERPOLATION_CUBIC);
                if (!success) {
                    add_log_entry(2, "Horizontal interpolation failed for row %d", y);
                }
            }
        }
    
    add_log_entry(0, "Horizontal interpolation completed for selection [%d,%d] to [%d,%d]", 
                 min_x, min_y, max_x, max_y);
}

void vertical_interpolate_selection(ImGuiTable* table, const MultiSelection* selection) {
    if (!table || !selection || !selection->active) {
        add_log_entry(2, "Vertical interpolation failed: Invalid table or selection");
        return;
    }
    
    int min_x, min_y, max_x, max_y;
    get_selection_bounds(selection, &min_x, &min_y, &max_x, &max_y);
    
            // Interpolate each column in the selection
        for (int x = min_x; x <= max_x; x++) {
            if (x >= 0 && x < table->width) {
                bool success = imgui_table_interpolate_vertical(table, x, min_y, max_y, INTERPOLATION_CUBIC);
                if (!success) {
                    add_log_entry(2, "Vertical interpolation failed for column %d", x);
                }
            }
        }
    
    add_log_entry(0, "Vertical interpolation completed for selection [%d,%d] to [%d,%d]", 
                 min_x, min_y, max_x, max_y);
}

void interpolate_table_values(ImGuiTable* table, int start_x, int start_y, int end_x, int end_y) {
    if (!table) {
        add_log_entry(2, "Table interpolation failed: No table available");
        return;
    }
    
    if (start_x < 0 || start_y < 0 || end_x < 0 || end_y < 0 ||
        start_x >= table->width || start_y >= table->height ||
        end_x >= table->width || end_y >= table->height) {
        add_log_entry(2, "Table interpolation failed: Invalid coordinates [%d,%d] to [%d,%d]", 
                     start_x, start_y, end_x, end_y);
        return;
    }
    
    // Ensure start is before end
    if (start_x > end_x) { int temp = start_x; start_x = end_x; end_x = temp; }
    if (start_y > end_y) { int temp = start_y; start_y = end_y; end_y = temp; }
    
    // Get corner values
    float top_left = imgui_table_get_value(table, start_x, start_y);
    float top_right = imgui_table_get_value(table, end_x, start_y);
    float bottom_left = imgui_table_get_value(table, start_x, end_y);
    float bottom_right = imgui_table_get_value(table, end_x, end_y);
    
    // Interpolate the interior
    for (int y = start_y + 1; y < end_y; y++) {
        for (int x = start_x + 1; x < end_x; x++) {
            float t_x = (float)(x - start_x) / (end_x - start_x);
            float t_y = (float)(y - start_y) / (end_y - start_y);
            
            // Bilinear interpolation
            float top = top_left + (top_right - top_left) * t_x;
            float bottom = bottom_left + (bottom_right - bottom_left) * t_x;
            float interpolated = top + (bottom - top) * t_y;
            
            imgui_table_set_value(table, x, y, interpolated);
        }
    }
    
    add_log_entry(0, "Table interpolation completed for region [%d,%d] to [%d,%d]", 
                 start_x, start_y, end_x, end_y);
}

// Smoothing operations
void smooth_selection(ImGuiTable* table, const MultiSelection* selection) {
    if (!table || !selection || !selection->active) {
        add_log_entry(2, "Smoothing failed: Invalid table or selection");
        return;
    }
    
    // Default to Gaussian smoothing
    gaussian_smooth_selection(table, selection);
}

void gaussian_smooth_selection(ImGuiTable* table, const MultiSelection* selection) {
    if (!table || !selection || !selection->active) {
        add_log_entry(2, "Gaussian smoothing failed: Invalid table or selection");
        return;
    }
    
    int min_x, min_y, max_x, max_y;
    get_selection_bounds(selection, &min_x, &min_y, &max_x, &max_y);
    
    // Use the existing imgui_table_gaussian_smooth function
    bool success = imgui_table_gaussian_smooth(table, min_x, min_y, max_x, max_y, 1.5f, true);
    
    if (success) {
        add_log_entry(0, "Gaussian smoothing completed for selection [%d,%d] to [%d,%d]", 
                     min_x, min_y, max_x, max_y);
    } else {
        add_log_entry(2, "Gaussian smoothing failed for selection [%d,%d] to [%d,%d]", 
                     min_x, min_y, max_x, max_y);
    }
}

void moving_average_smooth_selection(ImGuiTable* table, const MultiSelection* selection) {
    if (!table || !selection || !selection->active) {
        add_log_entry(2, "Moving average smoothing failed: Invalid table or selection");
        return;
    }
    
    int min_x, min_y, max_x, max_y;
    get_selection_bounds(selection, &min_x, &min_y, &max_x, &max_y);
    
    // Simple 3x3 moving average smoothing
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            if (x > 0 && x < table->width - 1 && y > 0 && y < table->height - 1) {
                float sum = 0.0f;
                int count = 0;
                
                // Sum 3x3 neighborhood
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        sum += imgui_table_get_value(table, x + dx, y + dy);
                        count++;
                    }
                }
                
                float average = sum / count;
                imgui_table_set_value(table, x, y, average);
            }
        }
    }
    
    add_log_entry(0, "Moving average smoothing completed for selection [%d,%d] to [%d,%d]", 
                 min_x, min_y, max_x, max_y);
}

void bilateral_smooth_selection(ImGuiTable* table, const MultiSelection* selection) {
    if (!table || !selection || !selection->active) {
        add_log_entry(2, "Bilateral smoothing failed: Invalid table or selection");
        return;
    }
    
    int min_x, min_y, max_x, max_y;
    get_selection_bounds(selection, &min_x, &min_y, &max_x, &max_y);
    
    // Bilateral filtering with edge preservation
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            if (x > 0 && x < table->width - 1 && y > 0 && y < table->height - 1) {
                float center_value = imgui_table_get_value(table, x, y);
                float sum = 0.0f;
                float weight_sum = 0.0f;
                
                // 5x5 bilateral filter
                for (int dy = -2; dy <= 2; dy++) {
                    for (int dx = -2; dx <= 2; dx++) {
                        float neighbor_value = imgui_table_get_value(table, x + dx, y + dy);
                        float spatial_weight = exp(-(dx * dx + dy * dy) / 8.0f);
                        float intensity_weight = exp(-(neighbor_value - center_value) * (neighbor_value - center_value) / 100.0f);
                        float weight = spatial_weight * intensity_weight;
                        
                        sum += neighbor_value * weight;
                        weight_sum += weight;
                    }
                }
                
                if (weight_sum > 0) {
                    float smoothed_value = sum / weight_sum;
                    imgui_table_set_value(table, x, y, smoothed_value);
                }
            }
        }
    }
    
    add_log_entry(0, "Bilateral smoothing completed for selection [%d,%d] to [%d,%d]", 
                 min_x, min_y, max_x, max_y);
}

void smooth_table_region(ImGuiTable* table, int center_x, int center_y, int radius) {
    if (!table) {
        add_log_entry(2, "Region smoothing failed: No table available");
        return;
    }
    
    if (center_x < 0 || center_y < 0 || center_x >= table->width || center_y >= table->height) {
        add_log_entry(2, "Region smoothing failed: Invalid center coordinates [%d,%d]", center_x, center_y);
        return;
    }
    
    if (radius <= 0) {
        add_log_entry(2, "Region smoothing failed: Invalid radius %d", radius);
        return;
    }
    
    // Create a temporary selection for the region
    MultiSelection region_selection = {0};
    region_selection.active = true;
    region_selection.start_x = center_x - radius;
    region_selection.start_y = center_y - radius;
    region_selection.end_x = center_x + radius;
    region_selection.end_y = center_y + radius;
    
    // Clamp to table bounds
    region_selection.start_x = (region_selection.start_x < 0) ? 0 : region_selection.start_x;
    region_selection.start_y = (region_selection.start_y < 0) ? 0 : region_selection.start_y;
    region_selection.end_x = (region_selection.end_x >= table->width) ? table->width - 1 : region_selection.end_x;
    region_selection.end_y = (region_selection.end_y >= table->height) ? table->height - 1 : region_selection.end_y;
    
    // Apply Gaussian smoothing to the region
    gaussian_smooth_selection(table, &region_selection);
}

// Professional table operations
void apply_table_math_operation(ImGuiTable* table, const char* operation, float value) {
    if (!table || !operation) {
        add_log_entry(2, "Math operation failed: Invalid table or operation");
        return;
    }
    
    // Apply operation to all cells
    for (int y = 0; y < table->height; y++) {
        for (int x = 0; x < table->width; x++) {
            float current_value = imgui_table_get_value(table, x, y);
            float new_value = current_value;
            
            if (strcmp(operation, "add") == 0) {
                new_value = current_value + value;
            } else if (strcmp(operation, "subtract") == 0) {
                new_value = current_value - value;
            } else if (strcmp(operation, "multiply") == 0) {
                new_value = current_value * value;
            } else if (strcmp(operation, "divide") == 0) {
                if (value != 0.0f) {
                    new_value = current_value / value;
                }
            } else if (strcmp(operation, "power") == 0) {
                new_value = pow(current_value, value);
            } else if (strcmp(operation, "sqrt") == 0) {
                if (current_value >= 0) {
                    new_value = sqrt(current_value);
                }
            }
            
            imgui_table_set_value(table, x, y, new_value);
        }
    }
    
    add_log_entry(0, "Math operation '%s' with value %.2f applied to entire table", operation, value);
}

void export_table_to_file(ImGuiTable* table, const char* filename) {
    if (!table || !filename) {
        add_log_entry(2, "Export failed: Invalid table or filename");
        return;
    }
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        add_log_entry(2, "Export failed: Cannot open file %s", filename);
        return;
    }
    
    // Write header
    fprintf(file, "Table Export: %dx%d\n", table->width, table->height);
    fprintf(file, "X Axis: %s (%s)\n", table->metadata.name, table->metadata.units);
    fprintf(file, "Y Axis: %s (%s)\n", table->metadata.name, table->metadata.units);
    fprintf(file, "Value Range: %.1f - %.1f (%s)\n", 
            table->metadata.min_value, table->metadata.max_value, table->metadata.units);
    fprintf(file, "\n");
    
    // Write data
    for (int y = 0; y < table->height; y++) {
        for (int x = 0; x < table->width; x++) {
            fprintf(file, "%.2f", imgui_table_get_value(table, x, y));
            if (x < table->width - 1) fprintf(file, "\t");
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
    add_log_entry(0, "Table exported to %s", filename);
}

void import_table_from_file(ImGuiTable* table, const char* filename) {
    if (!table || !filename) {
        add_log_entry(2, "Import failed: Invalid table or filename");
        return;
    }
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        add_log_entry(2, "Import failed: Cannot open file %s", filename);
        return;
    }
    
    // Skip header lines (simple heuristic)
    char line[256];
    int line_count = 0;
    while (fgets(line, sizeof(line), file) && line_count < 10) {
        if (strstr(line, "\t") || strstr(line, " ")) {
            // This looks like data, rewind and start reading
            fseek(file, 0, SEEK_SET);
            break;
        }
        line_count++;
    }
    
    // Read data
    int y = 0;
    while (fgets(line, sizeof(line), file) && y < table->height) {
        char* token = strtok(line, "\t \n");
        int x = 0;
        
        while (token && x < table->width) {
            float value = atof(token);
            imgui_table_set_value(table, x, y, value);
            token = strtok(NULL, "\t \n");
            x++;
        }
        y++;
    }
    
    fclose(file);
    add_log_entry(0, "Table imported from %s", filename);
}

// Multi-selection management
void start_multi_selection(MultiSelection* selection, int x, int y) {
    if (!selection) return;
    
    selection->active = true;
    selection->start_x = x;
    selection->start_y = y;
    selection->end_x = x;
    selection->end_y = y;
}

void update_multi_selection(MultiSelection* selection, int x, int y) {
    if (!selection || !selection->active) return;
    
    selection->end_x = x;
    selection->end_y = y;
}

void end_multi_selection(MultiSelection* selection) {
    if (!selection) return;
    
    // Ensure start is before end
    if (selection->start_x > selection->end_x) {
        int temp = selection->start_x;
        selection->start_x = selection->end_x;
        selection->end_x = temp;
    }
    if (selection->start_y > selection->end_y) {
        int temp = selection->start_y;
        selection->start_y = selection->end_y;
        selection->end_y = temp;
    }
}

void clear_multi_selection(MultiSelection* selection) {
    if (!selection) return;
    
    selection->active = false;
    selection->start_x = 0;
    selection->start_y = 0;
    selection->end_x = 0;
    selection->end_y = 0;
}

bool is_cell_in_selection(const MultiSelection* selection, int x, int y) {
    if (!selection || !selection->active) return false;
    
    return (x >= selection->start_x && x <= selection->end_x &&
            y >= selection->start_y && y <= selection->end_y);
}

void get_selection_bounds(const MultiSelection* selection, int* min_x, int* min_y, int* max_x, int* max_y) {
    if (!selection || !min_x || !min_y || !max_x || !max_y) return;
    
    *min_x = (selection->start_x < selection->end_x) ? selection->start_x : selection->end_x;
    *min_y = (selection->start_y < selection->end_y) ? selection->start_y : selection->end_y;
    *max_x = (selection->start_x > selection->end_x) ? selection->start_x : selection->end_x;
    *max_y = (selection->start_y > selection->end_y) ? selection->start_y : selection->end_y;
}

int get_selection_cell_count(const MultiSelection* selection) {
    if (!selection || !selection->active) return 0;
    
    int width = abs(selection->end_x - selection->start_x) + 1;
    int height = abs(selection->end_y - selection->start_y) + 1;
    return width * height;
}

// Utility functions
bool validate_table_operation(ImGuiTable* table, int x, int y) {
    return table && x >= 0 && y >= 0 && x < table->width && y < table->height;
}

bool validate_selection_bounds(const MultiSelection* selection, int max_width, int max_height) {
    if (!selection || !selection->active) return false;
    
    int min_x, min_y, max_x, max_y;
    get_selection_bounds(selection, &min_x, &min_y, &max_x, &max_y);
    
    return min_x >= 0 && min_y >= 0 && max_x < max_width && max_y < max_height;
}

float calculate_interpolation_weight(float t, InterpolationMode mode) {
    switch (mode) {
        case TABLE_INTERPOLATION_LINEAR:
            return t;
        case TABLE_INTERPOLATION_CUBIC:
            return 3.0f * t * t - 2.0f * t * t * t;
        case TABLE_INTERPOLATION_GAUSSIAN:
            return exp(-(t - 0.5f) * (t - 0.5f) / 0.125f);
        default:
            return t;
    }
}

// Legacy function names for compatibility
// These functions are called from main.cpp but can't access global variables directly
// For now, they just log that they were called
void copy_selection_to_clipboard_legacy(void) {
    add_log_entry(0, "Legacy copy_selection_to_clipboard called - functionality moved to table_operations module");
}

void paste_from_clipboard_legacy(void) {
    add_log_entry(0, "Legacy paste_from_clipboard called - functionality moved to table_operations module");
}

void interpolate_between_cells_legacy(void) {
    add_log_entry(0, "Legacy interpolate_between_cells called - functionality moved to table_operations module");
}

void smooth_selection_legacy(void) {
    add_log_entry(0, "Legacy smooth_selection called - functionality moved to table_operations module");
}

void gaussian_smooth_selection_legacy(void) {
    add_log_entry(0, "Legacy gaussian_smooth_selection called - functionality moved to table_operations module");
}

void moving_average_smooth_selection_legacy(void) {
    add_log_entry(0, "Legacy moving_average_smooth_selection called - functionality moved to table_operations module");
}

void bilateral_smooth_selection_legacy(void) {
    add_log_entry(0, "Legacy bilateral_smooth_selection called - functionality moved to table_operations module");
}

// Additional legacy functions for horizontal/vertical interpolation
void horizontal_interpolate_selection_legacy(void) {
    add_log_entry(0, "Legacy horizontal_interpolate_selection called - functionality moved to table_operations module");
}

void vertical_interpolate_selection_legacy(void) {
    add_log_entry(0, "Legacy vertical_interpolate_selection called - functionality moved to table_operations module");
}

// Additional legacy functions needed by main.cpp
// These functions are called from main.cpp but can't access global variables directly
// For now, they just log that they were called
void start_multi_selection(int x, int y) {
    add_log_entry(0, "Legacy start_multi_selection called with [%d,%d] - functionality moved to table_operations module", x, y);
}

void update_multi_selection(int x, int y) {
    add_log_entry(0, "Legacy update_multi_selection called with [%d,%d] - functionality moved to table_operations module", x, y);
}

void end_multi_selection(void) {
    add_log_entry(0, "Legacy end_multi_selection called - functionality moved to table_operations module");
}

void clear_multi_selection(void) {
    add_log_entry(0, "Legacy clear_multi_selection called - functionality moved to table_operations module");
}

bool is_cell_in_selection(int x, int y) {
    add_log_entry(0, "Legacy is_cell_in_selection called with [%d,%d] - functionality moved to table_operations module", x, y);
    return false; // Return false for now
}

void get_selection_bounds(int* min_x, int* min_y, int* max_x, int* max_y) {
    add_log_entry(0, "Legacy get_selection_bounds called - functionality moved to table_operations module");
    if (min_x) *min_x = -1;
    if (min_y) *min_y = -1;
    if (max_x) *max_x = -1;
    if (max_y) *max_y = -1;
}

int get_selection_cell_count(void) {
    add_log_entry(0, "Legacy get_selection_cell_count called - functionality moved to table_operations module");
    return 0; // Return 0 for now
}
