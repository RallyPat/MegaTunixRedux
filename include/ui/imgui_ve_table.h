#ifndef IMGUI_VE_TABLE_H
#define IMGUI_VE_TABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stddef.h>

// Table types supported by the system
typedef enum {
    TABLE_TYPE_VE = 0,
    TABLE_TYPE_IGNITION,
    TABLE_TYPE_MAF_VE,
    TABLE_TYPE_IDLE_VE,
    TABLE_TYPE_BOOST,
    TABLE_TYPE_VVT,
    TABLE_TYPE_CORRECTION,
    TABLE_TYPE_GPPWM,
    TABLE_TYPE_COUNT
} TableType;

// Table metadata structure
typedef struct {
    char name[64];
    char display_name[64];
    char units[16];
    float scale;
    float min_value;
    float max_value;
    int digits;
    TableType type;
    bool enabled;
    bool editable;
} TableMetadata;

// 3D view state
typedef struct {
    float rotation_x;
    float rotation_y;
    float zoom;
    float pan_x;
    float pan_y;
    bool wireframe_mode;
    bool show_grid;
    bool show_axes;
    float opacity;
} ImGuiVE3DView;

// OpenGL texture for ImGui integration
typedef struct {
    unsigned int texture_id;
    int width;
    int height;
    bool initialized;
} ImGuiVETexture;

// Extended table structure
typedef struct {
    // Basic table data
    float** data;
    float* x_axis;
    float* y_axis;
    int width;
    int height;
    
    // Table metadata
    TableMetadata metadata;
    
    // Table switching support
    int active_table_index;
    int table_count;
    char table_names[4][64];  // Support up to 4 switchable tables
    
    // 3D view data
    ImGuiVE3DView* view_3d;
    ImGuiVETexture* texture;
    
    // Table-specific data
    union {
        struct {
            // VE table specific
            float idle_rpm;
            float max_rpm;
        } ve;
        struct {
            // Ignition table specific
            float base_timing;
            float max_advance;
        } ignition;
        struct {
            // Boost table specific
            float target_boost;
            float wastegate_duty;
        } boost;
    } specific;
} ImGuiTable;

// Function declarations
ImGuiTable* imgui_table_create(TableType type, int width, int height);
void imgui_table_destroy(ImGuiTable* table);
void imgui_table_set_value(ImGuiTable* table, int x, int y, float value);
float imgui_table_get_value(ImGuiTable* table, int x, int y);
void imgui_table_load_demo_data(ImGuiTable* table);

// Table management functions
bool imgui_table_resize(ImGuiTable* table, int new_width, int new_height);
bool imgui_table_configure_from_ini(ImGuiTable* table, const char* ini_file_path);
void imgui_table_set_axis_ranges(ImGuiTable* table, float x_min, float x_max, float y_min, float y_max);
void imgui_table_set_axis_names(ImGuiTable* table, const char* x_name, const char* y_name, const char* x_units, const char* y_units);

// Table switching functions
bool imgui_table_add_switchable_table(ImGuiTable* table, const char* table_name);
bool imgui_table_switch_to(ImGuiTable* table, int table_index);
const char* imgui_table_get_active_name(ImGuiTable* table);

// Table type specific functions
ImGuiTable* imgui_ve_table_create(int width, int height);
ImGuiTable* imgui_ignition_table_create(int width, int height);
ImGuiTable* imgui_boost_table_create(int width, int height);
ImGuiTable* imgui_vvt_table_create(int width, int height);

// Rendering functions
void imgui_table_render_2d(ImGuiTable* table, float width, float height);
void imgui_table_render_3d(ImGuiTable* table, float width, float height);
void imgui_table_render_editor(ImGuiTable* table, float width, float height);

// Texture-based rendering for ImGui
ImGuiVETexture* imgui_ve_texture_create(int width, int height);
void imgui_ve_texture_destroy(ImGuiVETexture* texture);
void imgui_ve_texture_update(ImGuiVETexture* texture, ImGuiTable* table, ImGuiVE3DView* view);
unsigned int imgui_ve_texture_get_id(ImGuiVETexture* texture);

// 3D view management
ImGuiVE3DView* imgui_ve_3d_view_create(void);
void imgui_ve_3d_view_destroy(ImGuiVE3DView* view);
void imgui_ve_3d_view_reset(ImGuiVE3DView* view);

// Legacy function declarations (for backward compatibility)
void imgui_ve_table_render_2d(ImGuiTable* table, float width, float height);
void imgui_ve_table_render_3d(ImGuiTable* table, ImGuiVE3DView* view, float width, float height);
void imgui_ve_table_handle_input(ImGuiTable* table, ImGuiVE3DView* view, float width, float height);

#ifdef __cplusplus
}
#endif

#endif // IMGUI_VE_TABLE_H 