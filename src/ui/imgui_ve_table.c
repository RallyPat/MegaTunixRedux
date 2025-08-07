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
    table->active_table_index = 0;
    table->table_count = 1;
    
    // Set up metadata based on table type
    switch (type) {
        case TABLE_TYPE_VE:
            strcpy(table->metadata.name, "veTable");
            strcpy(table->metadata.display_name, "VE Table");
            strcpy(table->metadata.units, "%");
            table->metadata.scale = 0.01f;
            table->metadata.min_value = 0.0f;
            table->metadata.max_value = 650.0f;
            table->metadata.digits = 2;
            table->specific.ve.idle_rpm = 800.0f;
            table->specific.ve.max_rpm = 8000.0f;
            break;
            
        case TABLE_TYPE_IGNITION:
            strcpy(table->metadata.name, "ignitionTable");
            strcpy(table->metadata.display_name, "Ignition Table");
            strcpy(table->metadata.units, "deg");
            table->metadata.scale = 0.1f;
            table->metadata.min_value = -50.0f;
            table->metadata.max_value = 50.0f;
            table->metadata.digits = 1;
            table->specific.ignition.base_timing = 10.0f;
            table->specific.ignition.max_advance = 35.0f;
            break;
            
        case TABLE_TYPE_MAF_VE:
            strcpy(table->metadata.name, "veTableMaf");
            strcpy(table->metadata.display_name, "MAF VE Table");
            strcpy(table->metadata.units, "%");
            table->metadata.scale = 0.01f;
            table->metadata.min_value = 0.0f;
            table->metadata.max_value = 650.0f;
            table->metadata.digits = 2;
            break;
            
        case TABLE_TYPE_BOOST:
            strcpy(table->metadata.name, "boostTable");
            strcpy(table->metadata.display_name, "Boost Table");
            strcpy(table->metadata.units, "kPa");
            table->metadata.scale = 1.0f;
            table->metadata.min_value = 0.0f;
            table->metadata.max_value = 300.0f;
            table->metadata.digits = 0;
            table->specific.boost.target_boost = 200.0f;
            table->specific.boost.wastegate_duty = 50.0f;
            break;
            
        default:
            strcpy(table->metadata.name, "unknownTable");
            strcpy(table->metadata.display_name, "Unknown Table");
            strcpy(table->metadata.units, "");
            table->metadata.scale = 1.0f;
            table->metadata.min_value = 0.0f;
            table->metadata.max_value = 100.0f;
            table->metadata.digits = 0;
            break;
    }
    
    table->metadata.enabled = true;
    table->metadata.editable = true;
    
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
    table->view_3d = imgui_ve_3d_view_create();
    table->texture = imgui_ve_texture_create(256, 256);
    
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

ImGuiTable* imgui_ignition_table_create(int width, int height) {
    return imgui_table_create(TABLE_TYPE_IGNITION, width, height);
}

ImGuiTable* imgui_boost_table_create(int width, int height) {
    return imgui_table_create(TABLE_TYPE_BOOST, width, height);
}

ImGuiTable* imgui_vvt_table_create(int width, int height) {
    return imgui_table_create(TABLE_TYPE_VVT, width, height);
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
    
    if (table->view_3d) imgui_ve_3d_view_destroy(table->view_3d);
    if (table->texture) imgui_ve_texture_destroy(table->texture);
    
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
                    float base_timing = 10.0f + (rpm_norm * 25.0f) - (map_norm * 10.0f);
                    
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
    printf("[DEBUG] Input handling for %s table\n", table->metadata.display_name);
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
    printf("[DEBUG] Texture update for %s table\n", table->metadata.display_name);
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
        case TABLE_TYPE_MAF_VE:
            table_name = "veTableMaf";
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
        table->metadata.scale = ini_table->scale;
        table->metadata.min_value = ini_table->min_value;
        table->metadata.max_value = ini_table->max_value;
        
        printf("[DEBUG] Configured %s: %dx%d, X: %.0f-%.0f %s, Y: %.0f-%.0f %s\n",
               table->metadata.display_name, ini_table->width, ini_table->height,
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
           table->metadata.display_name,
           x_name ? x_name : "RPM", x_units ? x_units : "RPM",
           y_name ? y_name : "MAP", y_units ? y_units : "kPa");
} 

// Table switching functions
bool imgui_table_add_switchable_table(ImGuiTable* table, const char* table_name) {
    if (!table || !table_name || table->table_count >= 4) return false;
    
    strncpy(table->table_names[table->table_count], table_name, sizeof(table->table_names[0]) - 1);
    table->table_names[table->table_count][sizeof(table->table_names[0]) - 1] = '\0';
    table->table_count++;
    
    printf("[DEBUG] Added switchable table: %s (total: %d)\n", table_name, table->table_count);
    return true;
}

bool imgui_table_switch_to(ImGuiTable* table, int table_index) {
    if (!table || table_index < 0 || table_index >= table->table_count) return false;
    
    table->active_table_index = table_index;
    printf("[DEBUG] Switched to table: %s\n", table->table_names[table_index]);
    return true;
}

const char* imgui_table_get_active_name(ImGuiTable* table) {
    if (!table || table->active_table_index < 0 || table->active_table_index >= table->table_count) {
        return table->metadata.name;
    }
    
    return table->table_names[table->active_table_index];
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
    printf("[DEBUG] 2D View: %s\n", table->metadata.display_name);
    printf("[DEBUG] Size: %dx%d\n", table->width, table->height);
    printf("[DEBUG] Range: %.1f - %.1f %s\n", table->metadata.min_value, table->metadata.max_value, table->metadata.units);
}

void imgui_table_render_3d(ImGuiTable* table, float width, float height) {
    if (!table) return;
    
    // Placeholder for 3D table rendering
    printf("[DEBUG] 3D View: %s\n", table->metadata.display_name);
    printf("[DEBUG] Size: %dx%d\n", table->width, table->height);
}

void imgui_table_render_editor(ImGuiTable* table, float width, float height) {
    if (!table) return;
    
    // Placeholder for table editor
    printf("[DEBUG] Editor: %s\n", table->metadata.display_name);
    printf("[DEBUG] Size: %dx%d\n", table->width, table->height);
} 