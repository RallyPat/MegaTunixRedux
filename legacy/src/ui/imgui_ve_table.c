#include "../../include/ui/imgui_ve_table.h"
#include "../../include/ecu/ecu_ini_parser.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <GL/glu.h>

// Create a new table with specified type and dimensions
ImGuiTable* imgui_table_create(TableType type, int width, int height) {
    ImGuiTable* table = (ImGuiTable*)malloc(sizeof(ImGuiTable));
    if (!table) return NULL;
    
    // Initialize basic structure
    memset(table, 0, sizeof(ImGuiTable));
    table->width = width;
    table->height = height;
    table->metadata.type = type;
    table->is_modified = false;
    
    // Set up metadata based on table type
    switch (type) {
        case TABLE_TYPE_VE:
            strcpy(table->metadata.name, "VE Table");
            strcpy(table->metadata.description, "Volumetric Efficiency Table");
            strcpy(table->metadata.x_axis_label, "RPM");
            strcpy(table->metadata.y_axis_label, "MAP (kPa)");
            strcpy(table->metadata.value_label, "VE");
            strcpy(table->metadata.units, "%");
            table->metadata.min_value = 0.0f;
            table->metadata.max_value = 650.0f;
            table->metadata.default_value = 75.0f;
            break;
            
        case TABLE_TYPE_IGNITION:
            strcpy(table->metadata.name, "Ignition Table");
            strcpy(table->metadata.description, "Base Ignition Timing Table");
            strcpy(table->metadata.x_axis_label, "RPM");
            strcpy(table->metadata.y_axis_label, "MAP (kPa)");
            strcpy(table->metadata.value_label, "Timing");
            strcpy(table->metadata.units, "°BTDC");
            table->metadata.min_value = -50.0f;
            table->metadata.max_value = 50.0f;
            table->metadata.default_value = 15.0f;
            break;
            
        case TABLE_TYPE_AFR:
            strcpy(table->metadata.name, "AFR Table");
            strcpy(table->metadata.description, "Air-Fuel Ratio Table");
            strcpy(table->metadata.x_axis_label, "RPM");
            strcpy(table->metadata.y_axis_label, "MAP (kPa)");
            strcpy(table->metadata.value_label, "AFR");
            strcpy(table->metadata.units, "λ");
            table->metadata.min_value = 10.0f;
            table->metadata.max_value = 20.0f;
            table->metadata.default_value = 14.7f;
            break;
            
        case TABLE_TYPE_BOOST:
            strcpy(table->metadata.name, "Boost Table");
            strcpy(table->metadata.description, "Boost Control Table");
            strcpy(table->metadata.x_axis_label, "RPM");
            strcpy(table->metadata.y_axis_label, "MAP (kPa)");
            strcpy(table->metadata.value_label, "Boost");
            strcpy(table->metadata.units, "kPa");
            table->metadata.min_value = 0.0f;
            table->metadata.max_value = 300.0f;
            table->metadata.default_value = 100.0f;
            break;
            
        default:
            strcpy(table->metadata.name, "Unknown Table");
            strcpy(table->metadata.description, "Unknown Table Type");
            strcpy(table->metadata.x_axis_label, "X");
            strcpy(table->metadata.y_axis_label, "Y");
            strcpy(table->metadata.value_label, "Value");
            strcpy(table->metadata.units, "");
            table->metadata.min_value = 0.0f;
            table->metadata.max_value = 100.0f;
            table->metadata.default_value = 50.0f;
            break;
    }
    
    table->metadata.editable = true;
    table->metadata.visible = true;
    
    // Allocate data arrays
    table->data = (float**)malloc(height * sizeof(float*));
    if (!table->data) {
        free(table);
        return NULL;
    }
    
    for (int i = 0; i < height; i++) {
        table->data[i] = (float*)calloc(width, sizeof(float));
        if (!table->data[i]) {
            // Cleanup on failure
            for (int j = 0; j < i; j++) {
                free(table->data[j]);
            }
            free(table->data);
            free(table);
            return NULL;
        }
    }
    
    // Allocate axis arrays
    table->x_axis = (float*)malloc(width * sizeof(float));
    table->y_axis = (float*)malloc(height * sizeof(float));
    
    if (!table->x_axis || !table->y_axis) {
        imgui_table_destroy(table);
        return NULL;
    }
    
    // Initialize with default ranges that scale properly
    for (int i = 0; i < width; i++) {
        // RPM range: 500 to 8000, distributed across all cells
        table->x_axis[i] = 500.0f + (i * (7500.0f / (width - 1)));
    }
    
    for (int i = 0; i < height; i++) {
        // MAP range: 20 to 400, distributed across all cells
        table->y_axis[i] = 20.0f + (i * (380.0f / (height - 1)));
    }
    
    // Initialize 3D view
    // Note: view_3d and texture are no longer part of the table structure
    
    // Load demo data and calculate min/max values
    printf("[DEBUG] Loading demo data for newly created table\n");
    imgui_table_load_demo_data(table);
    
    return table;
}

// Create specific table types
ImGuiTable* imgui_ve_table_create(int width, int height) {
    printf("[DEBUG] imgui_ve_table_create called: %dx%d\n", width, height);
    printf("*** VE TABLE CREATE CALLED: %dx%d ***\n", width, height);
    ImGuiTable* result = imgui_table_create(TABLE_TYPE_VE, width, height);
    printf("[DEBUG] imgui_ve_table_create returning: %p\n", result);
    printf("*** VE TABLE CREATE RETURNING: %p ***\n", result);
    return result;
}

// Note: imgui_ignition_table_create is now implemented below with the new signature

ImGuiTable* imgui_boost_table_create(int width, int height) {
    return imgui_table_create(TABLE_TYPE_BOOST, width, height);
}

ImGuiTable* imgui_vvt_table_create(int width, int height) {
    return imgui_table_create(TABLE_TYPE_VE, width, height); // Fallback to VE table type
}

// Destroy table
void imgui_table_destroy(ImGuiTable* table) {
    if (!table) return;
    
    if (table->data) {
        for (int i = 0; i < table->height; i++) {
            if (table->data[i]) {
                free(table->data[i]);
            }
        }
        free(table->data);
    }
    
    if (table->x_axis) free(table->x_axis);
    if (table->y_axis) free(table->y_axis);
    
    // Note: view_3d and texture are no longer part of the table structure
    
    free(table);
}

// Set value in table
void imgui_table_set_value(ImGuiTable* table, int x, int y, float value) {
    if (!table || !table->data || x < 0 || x >= table->width || y < 0 || y >= table->height) {
        return;
    }
    
    // Clamp value to table's valid range
    if (value < table->metadata.min_value) value = table->metadata.min_value;
    if (value > table->metadata.max_value) value = table->metadata.max_value;
    
    table->data[y][x] = value;
}

// Get value from table
float imgui_table_get_value(ImGuiTable* table, int x, int y) {
    if (!table || !table->data || x < 0 || x >= table->width || y < 0 || y >= table->height) {
        return 0.0f;
    }
    
    return table->data[y][x];
}

// Load demo data for testing
void imgui_table_load_demo_data(ImGuiTable* table) {
    if (!table) return;
    
    // Generate demo data based on table type
    switch (table->metadata.type) {
        case TABLE_TYPE_VE:
            // VE table: realistic values based on RPM and MAP
            for (int y = 0; y < table->height; y++) {
                for (int x = 0; x < table->width; x++) {
                    // Calculate normalized position (0.0 to 1.0)
                    float rpm_norm = (float)x / (table->width - 1);
                    float map_norm = (float)y / (table->height - 1);
                    
                    // Create realistic VE curve
                    // Base VE increases with RPM, peaks around 70-80% of RPM range
                    float base_ve = 60.0f + (rpm_norm * 40.0f);
                    
                    // MAP correction: VE decreases at very high MAP (boost)
                    float map_correction = 1.0f - (map_norm * 0.2f);
                    
                    // RPM correction: slight increase at high RPM
                    float rpm_correction = 1.0f + (rpm_norm * 0.1f);
                    
                    // Calculate final VE value
                    float ve_value = base_ve * map_correction * rpm_correction;
                    
                    // Add some realistic variation
                    ve_value += sinf(rpm_norm * 3.14159f * 2.0f) * 3.0f;
                    ve_value += cosf(map_norm * 3.14159f * 2.0f) * 2.0f;
                    
                    // Clamp to reasonable range
                    if (ve_value < 30.0f) ve_value = 30.0f;
                    if (ve_value > 120.0f) ve_value = 120.0f;
                    
                    table->data[y][x] = ve_value;
                }
            }
            break;
            
        case TABLE_TYPE_IGNITION:
            // Ignition table: timing advance based on RPM and MAP
            for (int y = 0; y < table->height; y++) {
                for (int x = 0; x < table->width; x++) {
                    float rpm_norm = (float)x / (table->width - 1);
                    float map_norm = (float)y / (table->height - 1);
                    
                    // Base timing: increases with RPM, decreases with MAP
                    float base_timing = 15.0f + (rpm_norm * 25.0f) - (map_norm * 10.0f);
                    
                    // Add some variation
                    base_timing += sinf(rpm_norm * 3.14159f) * 2.0f;
                    base_timing += cosf(map_norm * 3.14159f) * 1.5f;
                    
                    // Clamp to reasonable range
                    if (base_timing < -10.0f) base_timing = -10.0f;
                    if (base_timing > 35.0f) base_timing = 35.0f;
                    
                    table->data[y][x] = base_timing;
                }
            }
            break;
            
        case TABLE_TYPE_BOOST:
            // Boost table: target boost increases with RPM
            for (int y = 0; y < table->height; y++) {
                for (int x = 0; x < table->width; x++) {
                    float rpm_norm = (float)x / (table->width - 1);
                    float map_norm = (float)y / (table->height - 1);
                    
                    // Base boost: increases with RPM
                    float base_boost = 100.0f + (rpm_norm * 150.0f);
                    
                    // MAP correction: boost decreases at very high MAP
                    float map_correction = 1.0f - (map_norm * 0.3f);
                    
                    float boost_value = base_boost * map_correction;
                    
                    // Add some variation
                    boost_value += sinf(rpm_norm * 3.14159f) * 10.0f;
                    
                    // Clamp to reasonable range
                    if (boost_value < 100.0f) boost_value = 100.0f;
                    if (boost_value > 300.0f) boost_value = 300.0f;
                    
                    table->data[y][x] = boost_value;
                }
            }
            break;
            
        default:
            // Generic demo data for other table types
            for (int y = 0; y < table->height; y++) {
                for (int x = 0; x < table->width; x++) {
                    float x_norm = (float)x / (table->width - 1);
                    float y_norm = (float)y / (table->height - 1);
                    
                    // Create a simple pattern
                    float value = 50.0f + (x_norm * 30.0f) + (y_norm * 20.0f);
                    value += sinf(x_norm * 3.14159f * 2.0f) * 5.0f;
                    value += cosf(y_norm * 3.14159f * 2.0f) * 3.0f;
                    
                    table->data[y][x] = value;
                }
            }
            break;
    }
    
    // Calculate min/max values from the loaded data
    printf("[DEBUG] Calculating min/max values from demo data\n");
    if (table->data && table->width > 0 && table->height > 0) {
        table->metadata.min_value = table->data[0][0];
        table->metadata.max_value = table->data[0][0];
        
        for (int y = 0; y < table->height; y++) {
            for (int x = 0; x < table->width; x++) {
                if (table->data[y][x] < table->metadata.min_value) {
                    table->metadata.min_value = table->data[y][x];
                }
                if (table->data[y][x] > table->metadata.max_value) {
                    table->metadata.max_value = table->data[y][x];
                }
            }
        }
        printf("[DEBUG] Demo data range calculated: %.1f - %.1f\n", table->metadata.min_value, table->metadata.max_value);
    }
}

// Legacy function implementations (for backward compatibility)
void imgui_ve_table_render_2d(ImGuiTable* table, float width, float height) {
    imgui_table_render_2d(table, width, height);
}

void imgui_ve_table_render_3d(ImGuiTable* table, ImGuiVE3DView* view, float width, float height) {
    imgui_table_render_3d(table, width, height);
}

void imgui_ve_table_handle_input(ImGuiTable* table, ImGuiVE3DView* view, float width, float height) {
    // Handle input for 3D view
    if (!table || !view) return;
    
    // Placeholder for input handling
    printf("[DEBUG] Input handling for %s table\n", table->metadata.name);
}

// Create OpenGL texture for ImGui integration
ImGuiVETexture* imgui_ve_texture_create(int width, int height) {
    ImGuiVETexture* texture = (ImGuiVETexture*)malloc(sizeof(ImGuiVETexture));
    if (!texture) return NULL;
    
    texture->width = width;
    texture->height = height;
    texture->initialized = false;
    
    // Generate OpenGL texture
    glGenTextures(1, &texture->texture_id);
    glBindTexture(GL_TEXTURE_2D, texture->texture_id);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Allocate texture memory
    unsigned char* data = (unsigned char*)malloc(width * height * 4);
    if (data) {
        memset(data, 0, width * height * 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        free(data);
        texture->initialized = true;
    }
    
    return texture;
}

// Destroy texture
void imgui_ve_texture_destroy(ImGuiVETexture* texture) {
    if (texture) {
        if (texture->initialized) {
            glDeleteTextures(1, &texture->texture_id);
        }
        free(texture);
    }
}

// Texture update function
void imgui_ve_texture_update(ImGuiVETexture* texture, ImGuiTable* table, ImGuiVE3DView* view) {
    if (!texture || !table || !view) return;
    
    // Update texture with table data
    // This is a placeholder - actual texture update would be implemented here
    printf("[DEBUG] Texture update for %s table\n", table->metadata.name);
}

// Get texture ID for ImGui
unsigned int imgui_ve_texture_get_id(ImGuiVETexture* texture) {
    return texture ? texture->texture_id : 0;
} 

// Table management functions
bool imgui_table_resize(ImGuiTable* table, int new_width, int new_height) {
    printf("[DEBUG] imgui_table_resize called: %dx%d -> %dx%d\n", 
           table ? table->width : -1, table ? table->height : -1, new_width, new_height);
    
    if (!table) {
        printf("[DEBUG] imgui_table_resize failed: table is NULL\n");
        return false;
    }
    
    // Free existing data
    if (table->data) {
        printf("[DEBUG] Freeing existing data arrays\n");
        for (int i = 0; i < table->height; i++) {
            if (table->data[i]) {
                free(table->data[i]);
            }
        }
        free(table->data);
    }
    
    if (table->x_axis) free(table->x_axis);
    if (table->y_axis) free(table->y_axis);
    
    // Update dimensions
    table->width = new_width;
    table->height = new_height;
    
    printf("[DEBUG] Allocating new data arrays: %dx%d\n", new_width, new_height);
    
    // Allocate new data arrays
    table->data = (float**)malloc(new_height * sizeof(float*));
    if (!table->data) {
        printf("[DEBUG] Failed to allocate data array\n");
        return false;
    }
    
    for (int i = 0; i < new_height; i++) {
        table->data[i] = (float*)calloc(new_width, sizeof(float));
        if (!table->data[i]) {
            printf("[DEBUG] Failed to allocate row %d\n", i);
            // Cleanup on failure
            for (int j = 0; j < i; j++) {
                free(table->data[j]);
            }
            free(table->data);
            return false;
        }
    }
    
    // Allocate new axis arrays
    table->x_axis = (float*)malloc(new_width * sizeof(float));
    table->y_axis = (float*)malloc(new_height * sizeof(float));
    
    if (!table->x_axis || !table->y_axis) {
        printf("[DEBUG] Failed to allocate axis arrays\n");
        imgui_table_destroy(table);
        return false;
    }
    
    // Initialize with default ranges that scale properly
    for (int i = 0; i < new_width; i++) {
        // RPM range: 500 to 8000, distributed across all cells
        table->x_axis[i] = 500.0f + (i * (7500.0f / (new_width - 1)));
    }
    
    for (int i = 0; i < new_height; i++) {
        // MAP range: 20 to 400, distributed across all cells
        table->y_axis[i] = 20.0f + (i * (380.0f / (new_height - 1)));
    }
    
    printf("[DEBUG] imgui_table_resize completed successfully: %dx%d\n", table->width, table->height);
    
    // Load demo data for the new table size
    printf("[DEBUG] Loading demo data for resized table\n");
    imgui_table_load_demo_data(table);
    
    // Calculate min/max values from the loaded data
    printf("[DEBUG] Calculating min/max values from loaded data\n");
    if (table->data && table->width > 0 && table->height > 0) {
        table->metadata.min_value = table->data[0][0];
        table->metadata.max_value = table->data[0][0];
        
        for (int y = 0; y < table->height; y++) {
            for (int x = 0; x < table->width; x++) {
                if (table->data[y][x] < table->metadata.min_value) {
                    table->metadata.min_value = table->data[y][x];
                }
                if (table->data[y][x] > table->metadata.max_value) {
                    table->metadata.max_value = table->data[y][x];
                }
            }
        }
        printf("[DEBUG] Calculated range: %.1f - %.1f\n", table->metadata.min_value, table->metadata.max_value);
    }
    
    return true;
}

bool imgui_table_configure_from_ini(ImGuiTable* table, const char* ini_file_path) {
    if (!table || !ini_file_path) return false;
    
    // Load INI configuration
    INIConfig* config = ecu_load_ini_file(ini_file_path);
    if (!config) return false;
    
    // Find table in the configuration based on table type
    const char* table_name = NULL;
    switch (table->metadata.type) {
        case TABLE_TYPE_VE:
            table_name = "veTable";
            break;
        case TABLE_TYPE_IGNITION:
            table_name = "ignitionTable";
            break;
        case TABLE_TYPE_AFR:
            table_name = "afrTable";
            break;
        case TABLE_TYPE_BOOST:
            table_name = "boostTable";
            break;
        default:
            table_name = table->metadata.name;
            break;
    }
    
    INITableInfo* ini_table = ecu_find_table_by_name(config, table_name);
    if (ini_table) {
        // Resize table to match INI dimensions
        if (!imgui_table_resize(table, ini_table->width, ini_table->height)) {
            ecu_free_ini_config(config);
            return false;
        }
        
        // Set axis ranges
        imgui_table_set_axis_ranges(table, ini_table->x_min, ini_table->x_max, 
                                   ini_table->y_min, ini_table->y_max);
        
        // Set axis names and units
        imgui_table_set_axis_names(table, ini_table->x_axis_name, ini_table->y_axis_name,
                                  ini_table->x_units, ini_table->y_units);
        
        // Update metadata
        table->metadata.min_value = ini_table->min_value;
        table->metadata.max_value = ini_table->max_value;
        
        printf("[DEBUG] Configured %s: %dx%d, X: %.0f-%.0f %s, Y: %.0f-%.0f %s\n",
               table->metadata.name, ini_table->width, ini_table->height,
               ini_table->x_min, ini_table->x_max, ini_table->x_units,
               ini_table->y_min, ini_table->y_max, ini_table->y_units);
    }
    
    ecu_free_ini_config(config);
    return true;
}

void imgui_table_set_axis_ranges(ImGuiTable* table, float x_min, float x_max, float y_min, float y_max) {
    if (!table) return;
    
    // Update X-axis
    for (int i = 0; i < table->width; i++) {
        float ratio = (float)i / (table->width - 1);
        table->x_axis[i] = x_min + (ratio * (x_max - x_min));
    }
    
    // Update Y-axis
    for (int i = 0; i < table->height; i++) {
        float ratio = (float)i / (table->height - 1);
        table->y_axis[i] = y_min + (ratio * (y_max - y_min));
    }
}

void imgui_table_set_axis_names(ImGuiTable* table, const char* x_name, const char* y_name, 
                                const char* x_units, const char* y_units) {
    // This would store axis names for display purposes
    printf("[DEBUG] Setting axis names for %s: X=%s (%s), Y=%s (%s)\n", 
           table->metadata.name,
           x_name ? x_name : "RPM", x_units ? x_units : "RPM",
           y_name ? y_name : "MAP", y_units ? y_units : "kPa");
} 

// Table switching functions
bool imgui_table_add_switchable_table(ImGuiTable* table, const char* table_name) {
    // Note: switchable tables are no longer supported in the new structure
    printf("[DEBUG] Switchable tables not supported in new structure\n");
    return false;
}

bool imgui_table_switch_to(ImGuiTable* table, int table_index) {
    // Note: table switching is no longer supported in the new structure
    printf("[DEBUG] Table switching not supported in new structure\n");
    return false;
}

const char* imgui_table_get_active_name(ImGuiTable* table) {
    // Note: table switching is no longer supported in the new structure
    if (!table) return "Unknown";
    return table->metadata.name;
} 

// 3D view management functions
ImGuiVE3DView* imgui_ve_3d_view_create(void) {
    ImGuiVE3DView* view = (ImGuiVE3DView*)malloc(sizeof(ImGuiVE3DView));
    if (view) {
        view->rotation_x = 30.0f;
        view->rotation_y = 45.0f;
        view->zoom = 1.0f;
        view->pan_x = 0.0f;
        view->pan_y = 0.0f;
        view->wireframe_mode = false;
        view->show_grid = true;
        view->show_axes = true;
        view->opacity = 0.8f;
    }
    return view;
}

void imgui_ve_3d_view_destroy(ImGuiVE3DView* view) {
    if (view) {
        free(view);
    }
}

void imgui_ve_3d_view_reset(ImGuiVE3DView* view) {
    if (view) {
        view->rotation_x = 30.0f;
        view->rotation_y = 45.0f;
        view->zoom = 1.0f;
        view->pan_x = 0.0f;
        view->pan_y = 0.0f;
    }
}

// Rendering functions
void imgui_table_render_2d(ImGuiTable* table, float width, float height) {
    if (!table) return;
    
    // Placeholder for 2D table rendering
    printf("[DEBUG] 2D View: %s\n", table->metadata.name);
    printf("[DEBUG] Size: %dx%d\n", table->width, table->height);
    printf("[DEBUG] Range: %.1f - %.1f %s\n", table->metadata.min_value, table->metadata.max_value, table->metadata.units);
}

void imgui_table_render_3d(ImGuiTable* table, float width, float height) {
    if (!table) return;
    
    // Placeholder for 3D table rendering
    printf("[DEBUG] 3D View: %s\n", table->metadata.name);
    printf("[DEBUG] Size: %dx%d\n", table->width, table->height);
}

void imgui_table_render_editor(ImGuiTable* table, float width, float height) {
    if (!table) return;
    
    // Placeholder for table editor
    printf("[DEBUG] Editor: %s\n", table->metadata.name);
    printf("[DEBUG] Size: %dx%d\n", table->width, table->height);
} 

// Forward declarations for helper functions
static float cubic_interpolate(float v0, float v1, float t);
static float spline_interpolate(float v0, float v1, float t);
static float bilinear_interpolate(float v00, float v10, float v01, float v11, float x_t, float y_t);
static float bicubic_interpolate(ImGuiTable* table, int x, int y, int start_x, int start_y, int end_x, int end_y);
static float bicubic_kernel_interpolate(float values[4][4], float x, float y);
static float catmull_rom_kernel(float t);
static float bspline_interpolate(ImGuiTable* table, int x, int y, int start_x, int start_y, int end_x, int end_y);
static float bspline_kernel_interpolate(float values[3][3], float x, float y);
static float bspline_kernel(float t);

// Professional interpolation functions for table editing
// These implement the core functionality that professional tuners need

/**
 * Interpolate between two selected cells with professional-grade algorithms
 * Supports linear, cubic, and spline interpolation methods
 */
bool imgui_table_interpolate_between_cells(ImGuiTable* table, int x1, int y1, int x2, int y2, 
                                         InterpolationMethod method, bool preview_mode) {
    if (!table || !table->data) return false;
    
    // Validate cell coordinates
    if (x1 < 0 || x1 >= table->width || y1 < 0 || y1 >= table->height ||
        x2 < 0 || x2 >= table->width || y2 < 0 || y2 >= table->height) {
        return false;
    }
    
    // Get values at the two points
    float val1 = table->data[y1][x1];
    float val2 = table->data[y2][x2];
    
    // Calculate distance and direction
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = (int)sqrt(dx*dx + dy*dy);
    
    if (steps == 0) return false; // Same cell
    
    // Create temporary buffer for preview mode
    float** temp_data = NULL;
    if (preview_mode) {
        temp_data = (float**)malloc(table->height * sizeof(float*));
        if (!temp_data) return false;
        
        for (int i = 0; i < table->height; i++) {
            temp_data[i] = (float*)malloc(table->width * sizeof(float));
            if (!temp_data[i]) {
                // Cleanup on failure
                for (int j = 0; j < i; j++) free(temp_data[j]);
                free(temp_data);
                return false;
            }
            // Copy current data
            memcpy(temp_data[i], table->data[i], table->width * sizeof(float));
        }
    }
    
    // Interpolate along the line between the two points
    for (int step = 0; step <= steps; step++) {
        float t = (float)step / (float)steps;
        
        // Calculate interpolated position
        int x = x1 + (int)(dx * t + 0.5f);
        int y = y1 + (int)(dy * t + 0.5f);
        
        // Clamp to table bounds
        x = fmax(0, fmin(table->width - 1, x));
        y = fmax(0, fmin(table->height - 1, y));
        
        // Calculate interpolated value based on method
        float interpolated_value;
        switch (method) {
            case INTERPOLATION_LINEAR:
                interpolated_value = val1 + (val2 - val1) * t;
                break;
                
            case INTERPOLATION_CUBIC:
                // Cubic interpolation for smoother curves
                interpolated_value = cubic_interpolate(val1, val2, t);
                break;
                
            case INTERPOLATION_SPLINE:
                // Spline interpolation for natural curves
                interpolated_value = spline_interpolate(val1, val2, t);
                break;
                
            default:
                interpolated_value = val1 + (val2 - val1) * t;
                break;
        }
        
        // Clamp to table value range
        interpolated_value = fmax(table->metadata.min_value, 
                                fmin(table->metadata.max_value, interpolated_value));
        
        // Apply the interpolated value
        if (preview_mode && temp_data) {
            temp_data[y][x] = interpolated_value;
        } else {
            table->data[y][x] = interpolated_value;
        }
    }
    
    // Cleanup preview buffer
    if (preview_mode && temp_data) {
        for (int i = 0; i < table->height; i++) {
            free(temp_data[i]);
        }
        free(temp_data);
    }
    
    return true;
}

/**
 * Horizontal interpolation across selected cells
 * Useful for smoothing RPM-based changes
 */
bool imgui_table_interpolate_horizontal(ImGuiTable* table, int start_x, int end_x, int y, 
                                      InterpolationMethod method) {
    if (!table || !table->data) return false;
    
    // Validate coordinates
    if (start_x < 0 || start_x >= table->width || end_x < 0 || end_x >= table->width ||
        y < 0 || y >= table->height) {
        return false;
    }
    
    // Ensure start_x < end_x
    if (start_x > end_x) {
        int temp = start_x;
        start_x = end_x;
        end_x = temp;
    }
    
    // Get values at start and end
    float start_val = table->data[y][start_x];
    float end_val = table->data[y][end_x];
    
    // Interpolate across the row
    for (int x = start_x; x <= end_x; x++) {
        float t = (float)(x - start_x) / (float)(end_x - start_x);
        
        float interpolated_value;
        switch (method) {
            case INTERPOLATION_LINEAR:
                interpolated_value = start_val + (end_val - start_val) * t;
                break;
            case INTERPOLATION_CUBIC:
                interpolated_value = cubic_interpolate(start_val, end_val, t);
                break;
            case INTERPOLATION_SPLINE:
                interpolated_value = spline_interpolate(start_val, end_val, t);
                break;
            default:
                interpolated_value = start_val + (end_val - start_val) * t;
                break;
        }
        
        // Clamp to valid range
        interpolated_value = fmax(table->metadata.min_value, 
                                fmin(table->metadata.max_value, interpolated_value));
        
        table->data[y][x] = interpolated_value;
    }
    
    return true;
}

/**
 * Vertical interpolation across selected cells
 * Useful for smoothing MAP-based changes
 */
bool imgui_table_interpolate_vertical(ImGuiTable* table, int x, int start_y, int end_y, 
                                    InterpolationMethod method) {
    if (!table || !table->data) return false;
    
    // Validate coordinates
    if (x < 0 || x >= table->width || start_y < 0 || start_y >= table->height ||
        end_y < 0 || end_y >= table->height) {
        return false;
    }
    
    // Ensure start_y < end_y
    if (start_y > end_y) {
        int temp = start_y;
        start_y = end_y;
        end_y = temp;
    }
    
    // Get values at start and end
    float start_val = table->data[start_y][x];
    float end_val = table->data[end_y][x];
    
    // Interpolate down the column
    for (int y = start_y; y <= end_y; y++) {
        float t = (float)(y - start_y) / (float)(end_y - start_y);
        
        float interpolated_value;
        switch (method) {
            case INTERPOLATION_LINEAR:
                interpolated_value = start_val + (end_val - start_val) * t;
                break;
            case INTERPOLATION_CUBIC:
                interpolated_value = cubic_interpolate(start_val, end_val, t);
                break;
            case INTERPOLATION_SPLINE:
                interpolated_value = spline_interpolate(start_val, end_val, t);
                break;
            default:
                interpolated_value = start_val + (end_val - start_val) * t;
                break;
        }
        
        // Clamp to valid range
        interpolated_value = fmax(table->metadata.min_value, 
                                fmin(table->metadata.max_value, interpolated_value));
        
        table->data[y][x] = interpolated_value;
    }
    
    return true;
}

/**
 * 2D interpolation for rectangular selections
 * Creates smooth surfaces across multiple cells
 */
bool imgui_table_interpolate_2d(ImGuiTable* table, int start_x, int start_y, int end_x, int end_y, 
                               InterpolationMethod method) {
    if (!table || !table->data) return false;
    
    // Validate coordinates
    if (start_x < 0 || start_x >= table->width || end_x < 0 || end_x >= table->width ||
        start_y < 0 || start_y >= table->height || end_y < 0 || end_y >= table->height) {
        return false;
    }
    
    // Ensure proper ordering
    if (start_x > end_x) { int temp = start_x; start_x = end_x; end_x = temp; }
    if (start_y > end_y) { int temp = start_y; start_y = end_y; end_y = temp; }
    
    // Get corner values
    float top_left = table->data[start_y][start_x];
    float top_right = table->data[start_y][end_x];
    float bottom_left = table->data[end_y][start_x];
    float bottom_right = table->data[end_y][end_x];
    
    // Interpolate across the entire rectangle
    for (int y = start_y; y <= end_y; y++) {
        float y_t = (float)(y - start_y) / (float)(end_y - start_y);
        
        for (int x = start_x; x <= end_x; x++) {
            float x_t = (float)(x - start_x) / (float)(end_x - start_x);
            
            float interpolated_value;
            switch (method) {
                case INTERPOLATION_LINEAR:
                    // Bilinear interpolation
                    interpolated_value = bilinear_interpolate(top_left, top_right, bottom_left, bottom_right, x_t, y_t);
                    break;
                    
                case INTERPOLATION_CUBIC:
                    // Bicubic interpolation
                    interpolated_value = bicubic_interpolate(table, x, y, start_x, start_y, end_x, end_y);
                    break;
                    
                case INTERPOLATION_SPLINE:
                    // B-spline interpolation
                    interpolated_value = bspline_interpolate(table, x, y, start_x, start_y, end_x, end_y);
                    break;
                    
                default:
                    interpolated_value = bilinear_interpolate(top_left, top_right, bottom_left, bottom_right, x_t, y_t);
                    break;
            }
            
            // Clamp to valid range
            interpolated_value = fmax(table->metadata.min_value, 
                                    fmin(table->metadata.max_value, interpolated_value));
            
            table->data[y][x] = interpolated_value;
        }
    }
    
    return true;
}

// Helper functions for advanced interpolation methods

/**
 * Cubic interpolation for smooth curves
 */
static float cubic_interpolate(float v0, float v1, float t) {
    // Catmull-Rom spline interpolation
    float t2 = t * t;
    float t3 = t2 * t;
    
    // Smooth curve with natural tension
    return (2 * t3 - 3 * t2 + 1) * v0 + 
           (-2 * t3 + 3 * t2) * v1;
}

/**
 * Spline interpolation for natural curves
 */
static float spline_interpolate(float v0, float v1, float t) {
    // Natural cubic spline with zero second derivatives at endpoints
    float t2 = t * t;
    float t3 = t2 * t;
    
    return (1 - 3*t2 + 2*t3) * v0 + 
           (3*t2 - 2*t3) * v1;
}

/**
 * Bilinear interpolation for 2D surfaces
 */
static float bilinear_interpolate(float v00, float v10, float v01, float v11, float x_t, float y_t) {
    float v0 = v00 + (v10 - v00) * x_t;
    float v1 = v01 + (v11 - v01) * x_t;
    return v0 + (v1 - v0) * y_t;
}

/**
 * Bicubic interpolation for smooth 2D surfaces
 */
static float bicubic_interpolate(ImGuiTable* table, int x, int y, int start_x, int start_y, int end_x, int end_y) {
    // 4x4 grid around the target point for bicubic interpolation
    float values[4][4];
    
    // Fill the 4x4 grid with surrounding values
    for (int dy = -1; dy <= 2; dy++) {
        for (int dx = -1; dx <= 2; dx++) {
            int sample_x = fmax(start_x, fmin(end_x, x + dx));
            int sample_y = fmax(start_y, fmin(end_y, y + dy));
            
            values[dy+1][dx+1] = table->data[sample_y][sample_x];
        }
    }
    
    // Calculate local coordinates within the 4x4 grid
    float local_x = (float)(x - start_x) / (float)(end_x - start_x);
    float local_y = (float)(y - start_y) / (float)(end_y - start_y);
    
    // Apply bicubic interpolation kernel
    return bicubic_kernel_interpolate(values, local_x, local_y);
}

/**
 * Bicubic kernel interpolation
 */
static float bicubic_kernel_interpolate(float values[4][4], float x, float y) {
    // Bicubic interpolation using Catmull-Rom splines
    float result = 0.0f;
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float weight = catmull_rom_kernel(x - i + 1) * catmull_rom_kernel(y - j + 1);
            result += values[i][j] * weight;
        }
    }
    
    return result;
}

/**
 * Catmull-Rom spline kernel function
 */
static float catmull_rom_kernel(float t) {
    float abs_t = fabs(t);
    float abs_t2 = abs_t * abs_t;
    float abs_t3 = abs_t2 * abs_t;
    
    if (abs_t <= 1.0f) {
        return 1.5f * abs_t3 - 2.5f * abs_t2 + 1.0f;
    } else if (abs_t <= 2.0f) {
        return -0.5f * abs_t3 + 2.5f * abs_t2 - 4.0f * abs_t + 2.0f;
    } else {
        return 0.0f;
    }
}

/**
 * B-spline interpolation for very smooth surfaces
 */
static float bspline_interpolate(ImGuiTable* table, int x, int y, int start_x, int start_y, int end_x, int end_y) {
    // B-spline interpolation using 3x3 grid
    float values[3][3];
    
    // Fill the 3x3 grid
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int sample_x = fmax(start_x, fmin(end_x, x + dx));
            int sample_y = fmax(start_y, fmin(end_y, y + dy));
            
            values[dy+1][dx+1] = table->data[sample_y][sample_x];
        }
    }
    
    // Calculate local coordinates
    float local_x = (float)(x - start_x) / (float)(end_x - start_x);
    float local_y = (float)(y - start_y) / (float)(end_y - start_y);
    
    // Apply B-spline kernel
    return bspline_kernel_interpolate(values, local_x, local_y);
}

/**
 * B-spline kernel interpolation
 */
static float bspline_kernel_interpolate(float values[3][3], float x, float y) {
    // B-spline interpolation using quadratic B-splines
    float result = 0.0f;
    
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            float weight = bspline_kernel(x - i + 1) * bspline_kernel(y - j + 1);
            result += values[i][j] * weight;
        }
    }
    
    return result;
}

/**
 * Quadratic B-spline kernel function
 */
static float bspline_kernel(float t) {
    float abs_t = fabs(t);
    
    if (abs_t <= 0.5f) {
        return 0.75f - abs_t * abs_t;
    } else if (abs_t <= 1.5f) {
        float temp = 1.5f - abs_t;
        return 0.5f * temp * temp;
    } else {
        return 0.0f;
    }
} 

// Professional smoothing tools for table optimization
// These implement industry-standard smoothing algorithms used in commercial ECU tuning software

/**
 * Gaussian smoothing with configurable strength
 * Industry-standard smoothing algorithm for fuel maps
 */
bool imgui_table_gaussian_smooth(ImGuiTable* table, int start_x, int start_y, int end_x, int end_y, 
                                float sigma, bool preserve_edges) {
    if (!table || !table->data) return false;
    
    // Validate coordinates
    if (start_x < 0 || start_x >= table->width || end_x < 0 || end_x >= table->width ||
        start_y < 0 || start_y >= table->height || end_y < 0 || end_y >= table->height) {
        return false;
    }
    
    // Ensure proper ordering
    if (start_x > end_x) { int temp = start_x; start_x = end_x; end_x = temp; }
    if (start_y > end_y) { int temp = start_y; start_y = end_y; end_y = temp; }
    
    // Calculate kernel size based on sigma (3*sigma rule)
    int kernel_size = (int)(3.0f * sigma) * 2 + 1;
    if (kernel_size < 3) kernel_size = 3;
    if (kernel_size > 9) kernel_size = 9; // Limit kernel size for performance
    
    int kernel_radius = kernel_size / 2;
    
    // Create temporary buffer for smoothed values
    float** temp_buffer = (float**)malloc((end_y - start_y + 1) * sizeof(float*));
    if (!temp_buffer) return false;
    
    for (int y = 0; y <= end_y - start_y; y++) {
        temp_buffer[y] = (float*)malloc((end_x - start_x + 1) * sizeof(float));
        if (!temp_buffer[y]) {
            // Cleanup on failure
            for (int j = 0; j < y; j++) free(temp_buffer[j]);
            free(temp_buffer);
            return false;
        }
    }
    
    // Copy current values to temp buffer
    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
            temp_buffer[y - start_y][x - start_x] = table->data[y][x];
        }
    }
    
    // Apply Gaussian smoothing
    int cells_processed = 0;
    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
            float sum = 0.0f;
            float weight_sum = 0.0f;
            
            // Apply Gaussian kernel
            for (int ky = -kernel_radius; ky <= kernel_radius; ky++) {
                for (int kx = -kernel_radius; kx <= kernel_radius; kx++) {
                    int nx = x + kx;
                    int ny = y + ky;
                    
                    // Check bounds
                    if (nx >= start_x && nx <= end_x && ny >= start_y && ny <= end_y) {
                        // Calculate Gaussian weight
                        float distance = sqrt(kx*kx + ky*ky);
                        float weight = exp(-(distance * distance) / (2.0f * sigma * sigma));
                        
                        // Edge preservation logic
                        if (preserve_edges) {
                            float center_val = table->data[y][x];
                            float neighbor_val = table->data[ny][nx];
                            float difference = fabs(center_val - neighbor_val);
                            
                            // Reduce weight for large differences (edges)
                            if (difference > table->metadata.max_value * 0.1f) {
                                weight *= 0.3f; // Reduce edge smoothing
                            }
                        }
                        
                        sum += table->data[ny][nx] * weight;
                        weight_sum += weight;
                    }
                }
            }
            
            if (weight_sum > 0.0f) {
                float smoothed_val = sum / weight_sum;
                
                // Clamp to table bounds
                smoothed_val = fmax(table->metadata.min_value, 
                                  fmin(table->metadata.max_value, smoothed_val));
                
                temp_buffer[y - start_y][x - start_x] = smoothed_val;
                cells_processed++;
            }
        }
    }
    
    // Apply smoothed values back to table
    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
            table->data[y][x] = temp_buffer[y - start_y][x - start_x];
        }
    }
    
    // Cleanup temp buffer
    for (int y = 0; y <= end_y - start_y; y++) {
        free(temp_buffer[y]);
    }
    free(temp_buffer);
    
    return true;
}

/**
 * Moving average smoothing for trend-based smoothing
 * Useful for smoothing RPM-based trends while preserving MAP transitions
 */
bool imgui_table_moving_average_smooth(ImGuiTable* table, int start_x, int start_y, int end_x, int end_y, 
                                     int window_size, bool horizontal_only) {
    if (!table || !table->data) return false;
    
    // Validate coordinates
    if (start_x < 0 || start_x >= table->width || end_x < 0 || end_x >= table->width ||
        start_y < 0 || start_y >= table->height || end_y < 0 || end_y >= table->height) {
        return false;
    }
    
    // Ensure proper ordering
    if (start_x > end_x) { int temp = start_x; start_x = end_x; end_x = temp; }
    if (start_y > end_y) { int temp = start_y; start_y = end_y; end_y = temp; }
    
    // Limit window size
    if (window_size < 3) window_size = 3;
    if (window_size > 9) window_size = 9;
    
    int window_radius = window_size / 2;
    
    // Create temporary buffer
    float** temp_buffer = (float**)malloc((end_y - start_y + 1) * sizeof(float*));
    if (!temp_buffer) return false;
    
    for (int y = 0; y <= end_y - start_y; y++) {
        temp_buffer[y] = (float*)malloc((end_x - start_x + 1) * sizeof(float));
        if (!temp_buffer[y]) {
            for (int j = 0; j < y; j++) free(temp_buffer[j]);
            free(temp_buffer);
            return false;
        }
    }
    
    // Copy current values
    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
            temp_buffer[y - start_y][x - start_x] = table->data[y][x];
        }
    }
    
    // Apply moving average smoothing
    int cells_processed = 0;
    
    if (horizontal_only) {
        // Smooth only horizontally (RPM-based smoothing)
        for (int y = start_y; y <= end_y; y++) {
            for (int x = start_x; x <= end_x; x++) {
                float sum = 0.0f;
                int count = 0;
                
                // Apply horizontal window
                for (int kx = -window_radius; kx <= window_radius; kx++) {
                    int nx = x + kx;
                    if (nx >= start_x && nx <= end_x) {
                        sum += table->data[y][nx];
                        count++;
                    }
                }
                
                if (count > 0) {
                    float smoothed_val = sum / count;
                    temp_buffer[y - start_y][x - start_x] = smoothed_val;
                    cells_processed++;
                }
            }
        }
    } else {
        // Smooth in both directions
        for (int y = start_y; y <= end_y; y++) {
            for (int x = start_x; x <= end_x; x++) {
                float sum = 0.0f;
                int count = 0;
                
                // Apply 2D window
                for (int ky = -window_radius; ky <= window_radius; ky++) {
                    for (int kx = -window_radius; kx <= window_radius; kx++) {
                        int nx = x + kx;
                        int ny = y + ky;
                        
                        if (nx >= start_x && nx <= end_x && ny >= start_y && ny <= end_y) {
                            sum += table->data[ny][nx];
                            count++;
                        }
                    }
                }
                
                if (count > 0) {
                    float smoothed_val = sum / count;
                    temp_buffer[y - start_y][x - start_x] = smoothed_val;
                    cells_processed++;
                }
            }
        }
    }
    
    // Apply smoothed values back to table
    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
            table->data[y][x] = temp_buffer[y - start_y][x - start_x];
        }
    }
    
    // Cleanup
    for (int y = 0; y <= end_y - start_y; y++) {
        free(temp_buffer[y]);
    }
    free(temp_buffer);
    
    return true;
}

/**
 * Edge-preserving smoothing using bilateral filtering
 * Maintains sharp transitions while smoothing noise
 */
bool imgui_table_bilateral_smooth(ImGuiTable* table, int start_x, int start_y, int end_x, int end_y, 
                                 float spatial_sigma, float intensity_sigma) {
    if (!table || !table->data) return false;
    
    // Validate coordinates
    if (start_x < 0 || start_x >= table->width || end_x < 0 || end_x >= table->width ||
        start_y < 0 || start_y >= table->height || end_y < 0 || end_y >= table->height) {
        return false;
    }
    
    // Ensure proper ordering
    if (start_x > end_x) { int temp = start_x; start_x = end_x; end_x = temp; }
    if (start_y > end_y) { int temp = start_y; start_y = end_y; end_y = temp; }
    
    // Calculate kernel size
    int kernel_size = (int)(3.0f * spatial_sigma) * 2 + 1;
    if (kernel_size < 3) kernel_size = 3;
    if (kernel_size > 9) kernel_size = 9;
    
    int kernel_radius = kernel_size / 2;
    
    // Create temporary buffer
    float** temp_buffer = (float**)malloc((end_y - start_y + 1) * sizeof(float*));
    if (!temp_buffer) return false;
    
    for (int y = 0; y <= end_y - start_y; y++) {
        temp_buffer[y] = (float*)malloc((end_x - start_x + 1) * sizeof(float));
        if (!temp_buffer[y]) {
            for (int j = 0; j < y; j++) free(temp_buffer[j]);
            free(temp_buffer);
            return false;
        }
    }
    
    // Copy current values
    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
            temp_buffer[y - start_y][x - start_x] = table->data[y][x];
        }
    }
    
    // Apply bilateral filtering
    int cells_processed = 0;
    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
            float sum = 0.0f;
            float weight_sum = 0.0f;
            float center_val = table->data[y][x];
            
            // Apply bilateral kernel
            for (int ky = -kernel_radius; ky <= kernel_radius; ky++) {
                for (int kx = -kernel_radius; kx <= kernel_radius; kx++) {
                    int nx = x + kx;
                    int ny = y + ky;
                    
                    if (nx >= start_x && nx <= end_x && ny >= start_y && ny <= end_y) {
                        float neighbor_val = table->data[ny][nx];
                        
                        // Spatial weight (distance-based)
                        float spatial_dist = sqrt(kx*kx + ky*ky);
                        float spatial_weight = exp(-(spatial_dist * spatial_dist) / (2.0f * spatial_sigma * spatial_sigma));
                        
                        // Intensity weight (value difference-based)
                        float intensity_diff = fabs(center_val - neighbor_val);
                        float intensity_weight = exp(-(intensity_diff * intensity_diff) / (2.0f * intensity_sigma * intensity_sigma));
                        
                        // Combined weight
                        float total_weight = spatial_weight * intensity_weight;
                        
                        sum += neighbor_val * total_weight;
                        weight_sum += total_weight;
                    }
                }
            }
            
            if (weight_sum > 0.0f) {
                float smoothed_val = sum / weight_sum;
                
                // Clamp to table bounds
                smoothed_val = fmax(table->metadata.min_value, 
                                  fmin(table->metadata.max_value, smoothed_val));
                
                temp_buffer[y - start_y][x - start_x] = smoothed_val;
                cells_processed++;
            }
        }
    }
    
    // Apply smoothed values back to table
    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
            table->data[y][x] = temp_buffer[y - start_y][x - start_x];
        }
    }
    
    // Cleanup
    for (int y = 0; y <= end_y - start_y; y++) {
        free(temp_buffer[y]);
    }
    free(temp_buffer);
    
    return true;
}

/**
 * Smart smoothing that automatically chooses the best method
 * Analyzes table characteristics and applies appropriate smoothing
 */
bool imgui_table_smart_smooth(ImGuiTable* table, int start_x, int start_y, int end_x, int end_y) {
    if (!table || !table->data) return false;
    
    // Analyze table characteristics
    float min_val = table->metadata.max_value;
    float max_val = table->metadata.min_value;
    float total_variance = 0.0f;
    int cell_count = 0;
    
    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
            float val = table->data[y][x];
            if (val < min_val) min_val = val;
            if (val > max_val) max_val = val;
            total_variance += val;
            cell_count++;
        }
    }
    
    if (cell_count == 0) return false;
    
    float mean_val = total_variance / cell_count;
    float range = max_val - min_val;
    float relative_variance = range / mean_val;
    
    // Choose smoothing method based on characteristics
    if (relative_variance > 0.5f) {
        // High variance - use edge-preserving smoothing
        return imgui_table_bilateral_smooth(table, start_x, start_y, end_x, end_y, 1.5f, range * 0.1f);
    } else if (relative_variance > 0.2f) {
        // Medium variance - use Gaussian smoothing
        return imgui_table_gaussian_smooth(table, start_x, start_y, end_x, end_y, 1.0f, true);
    } else {
        // Low variance - use moving average for gentle smoothing
        return imgui_table_moving_average_smooth(table, start_x, start_y, end_x, end_y, 3, false);
    }
} 

// Ignition table specific functions
// These implement the core functionality for ignition timing tables

/**
 * Create a new ignition table with proper metadata
 */
bool imgui_ignition_table_create(ImGuiTable* table, int width, int height) {
    if (!table || width <= 0 || height <= 0) return false;
    
    // Initialize table structure
    table->width = width;
    table->height = height;
    table->is_modified = false;
    
    // Set ignition table metadata - FIXED to match TunerStudio orientation
    table->metadata.type = TABLE_TYPE_IGNITION;
    strncpy(table->metadata.name, "Ignition Table", sizeof(table->metadata.name) - 1);
    strncpy(table->metadata.description, "Base ignition timing table for engine tuning", sizeof(table->metadata.description) - 1);
    // FIXED: X-axis is MAP (kPa), Y-axis is RPM - matching TunerStudio
    strncpy(table->metadata.x_axis_label, "MAP (kPa)", sizeof(table->metadata.x_axis_label) - 1);
    strncpy(table->metadata.y_axis_label, "RPM", sizeof(table->metadata.y_axis_label) - 1);
    strncpy(table->metadata.value_label, "Timing", sizeof(table->metadata.value_label) - 1);
    strncpy(table->metadata.units, "°BTDC", sizeof(table->metadata.units) - 1);
    
    // Ignition timing values (degrees Before Top Dead Center)
    table->metadata.min_value = -10.0f;    // 10° ATDC (after TDC)
    table->metadata.max_value = 45.0f;     // 45° BTDC (before TDC)
    table->metadata.default_value = 15.0f; // 15° BTDC default
    table->metadata.editable = true;
    table->metadata.visible = true;
    
    // Allocate data arrays
    table->data = (float**)malloc(height * sizeof(float*));
    if (!table->data) return false;
    
    for (int y = 0; y < height; y++) {
        table->data[y] = (float*)malloc(width * sizeof(float));
        if (!table->data[y]) {
            // Cleanup on failure
            for (int j = 0; j < y; j++) free(table->data[j]);
            free(table->data);
            return false;
        }
    }
    
    // Allocate axis arrays
    table->x_axis = (float*)malloc(width * sizeof(float));
    table->y_axis = (float*)malloc(height * sizeof(float));
    if (!table->x_axis || !table->y_axis) {
        // Cleanup on failure
        for (int y = 0; y < height; y++) free(table->data[y]);
        free(table->data);
        free(table->x_axis);
        free(table->y_axis);
        return false;
    }
    
    // Initialize axis values for ignition table - FIXED orientation
    // X-axis: MAP range (20 - 200 kPa) - columns
    for (int x = 0; x < width; x++) {
        table->x_axis[x] = 20.0f + (180.0f * x) / (width - 1);
    }
    
    // Y-axis: RPM range (500 - 8000 RPM) - rows
    for (int y = 0; y < height; y++) {
        table->y_axis[y] = 500.0f + (7500.0f * y) / (height - 1);
    }
    
    // Initialize all cells to default value
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            table->data[y][x] = table->metadata.default_value;
        }
    }
    
    printf("[DEBUG] Ignition table created: %dx%d (X=MAP, Y=RPM)\n", width, height);
    return true;
}

/**
 * Load realistic demo data for ignition table
 * Creates a typical ignition advance curve
 */
void imgui_ignition_table_load_demo_data(ImGuiTable* table) {
    if (!table || !table->data) return;
    
    printf("[DEBUG] Loading demo data for ignition table\n");
    
    // Create realistic ignition advance curve - FIXED to use correct axis orientation
    for (int y = 0; y < table->height; y++) {
        for (int x = 0; x < table->width; x++) {
            // FIXED: y=RPM, x=MAP to match TunerStudio orientation
            float rpm = table->y_axis[y];      // Y-axis is RPM (rows)
            float map = table->x_axis[x];      // X-axis is MAP (columns)
            
            // Base timing calculation
            float base_timing = 15.0f; // Base 15° BTDC
            
            // RPM advance (more advance at higher RPM)
            float rpm_advance = 0.0f;
            if (rpm > 1000) {
                rpm_advance = (rpm - 1000) * 0.02f; // 2° per 1000 RPM
                rpm_advance = fmin(rpm_advance, 20.0f); // Max 20° advance
            }
            
            // MAP advance (less advance at high load)
            float map_advance = 0.0f;
            if (map < 100) {
                map_advance = (100 - map) * 0.1f; // 1° per 10 kPa below 100
                map_advance = fmin(map_advance, 8.0f); // Max 8° advance
            }
            
            // Calculate final timing
            float timing = base_timing + rpm_advance + map_advance;
            
            // Clamp to valid range
            timing = fmax(table->metadata.min_value, 
                         fmin(table->metadata.max_value, timing));
            
            table->data[y][x] = timing;
        }
    }
    
    // Recalculate min/max values
    float min_val = table->metadata.max_value;
    float max_val = table->metadata.min_value;
    
    for (int y = 0; y < table->height; y++) {
        for (int x = 0; x < table->width; x++) {
            float val = table->data[y][x];
            if (val < min_val) min_val = val;
            if (val > max_val) max_val = val;
        }
    }
    
    table->metadata.min_value = min_val;
    table->metadata.max_value = max_val;
    
    printf("[DEBUG] Ignition demo data range: %.1f - %.1f %s\n", 
           min_val, max_val, table->metadata.units);
}

/**
 * Render the ignition table editor
 */
void imgui_ignition_table_render_editor(ImGuiTable* table, float width, float height) {
    if (!table || !table->data) return;
    
    // Note: This is a C file, so ImGui calls are not available
    // The actual rendering will be handled in the main.cpp C++ file
    printf("[DEBUG] Ignition table editor render requested for %s\n", table->metadata.name);
    printf("[DEBUG] Table size: %dx%d\n", table->width, table->height);
    printf("[DEBUG] X-Axis: %s (%.0f - %.0f)\n", 
           table->metadata.x_axis_label, 
           table->x_axis[0], 
           table->x_axis[table->width - 1]);
    printf("[DEBUG] Y-Axis: %s (%.1f - %.1f)\n", 
           table->metadata.y_axis_label, 
           table->y_axis[0], 
           table->y_axis[table->height - 1]);
    printf("[DEBUG] Values: %s (%.1f - %.1f %s)\n", 
           table->metadata.value_label,
           table->metadata.min_value,
           table->metadata.max_value,
           table->metadata.units);
}

/**
 * Handle input for ignition table
 */
void imgui_ignition_table_handle_input(ImGuiTable* table, ImGuiVE3DView* view, float width, float height) {
    if (!table || !table->data) return;
    
    // Handle keyboard input for ignition table
    // This can be extended with ignition-specific shortcuts
    
    // Note: This is a C file, so ImGui calls are not available
    // The actual input handling will be done in the main.cpp C++ file
    printf("[DEBUG] Ignition table input handling requested for %s\n", table->metadata.name);
} 