#ifndef IMGUI_VE_TABLE_H
#define IMGUI_VE_TABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stddef.h>

// Table types for different ECU parameters
typedef enum {
    TABLE_TYPE_VE = 0,           // Volumetric Efficiency (fuel)
    TABLE_TYPE_IGNITION,         // Ignition timing
    TABLE_TYPE_AFR,              // Air-Fuel Ratio
    TABLE_TYPE_BOOST,            // Boost control
    TABLE_TYPE_IDLE,             // Idle control
    TABLE_TYPE_LAUNCH,           // Launch control
    TABLE_TYPE_COUNT
} TableType;

// Table metadata with type-specific information
typedef struct {
    TableType type;
    char name[64];
    char description[256];
    char x_axis_label[32];
    char y_axis_label[32];
    char value_label[32];
    char units[16];
    float min_value;
    float max_value;
    float default_value;
    bool editable;
    bool visible;
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

// Enhanced table structure with type support
typedef struct {
    int width;
    int height;
    float** data;
    float* x_axis;
    float* y_axis;
    TableMetadata metadata;
    bool is_modified;
    char filename[256];
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
// Note: ignition table functions are defined below

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

// Interpolation methods for professional table editing
typedef enum {
    INTERPOLATION_LINEAR = 0,    // Linear interpolation (fast, simple)
    INTERPOLATION_CUBIC,         // Cubic interpolation (smooth curves)
    INTERPOLATION_SPLINE,        // Spline interpolation (natural curves)
    INTERPOLATION_COUNT
} InterpolationMethod;

// Professional interpolation functions
bool imgui_table_interpolate_between_cells(ImGuiTable* table, int x1, int y1, int x2, int y2, 
                                         InterpolationMethod method, bool preview_mode);
bool imgui_table_interpolate_horizontal(ImGuiTable* table, int start_x, int end_x, int y, 
                                      InterpolationMethod method);
bool imgui_table_interpolate_vertical(ImGuiTable* table, int x, int start_y, int end_y, 
                                    InterpolationMethod method);
bool imgui_table_interpolate_2d(ImGuiTable* table, int start_x, int start_y, int end_x, int end_y, 
                               InterpolationMethod method);

// Professional smoothing functions
bool imgui_table_gaussian_smooth(ImGuiTable* table, int start_x, int start_y, int end_x, int end_y, 
                                float sigma, bool preserve_edges);
bool imgui_table_moving_average_smooth(ImGuiTable* table, int start_x, int start_y, int end_x, int end_y, 
                                     int window_size, bool horizontal_only);
bool imgui_table_bilateral_smooth(ImGuiTable* table, int start_x, int start_y, int end_x, int end_y, 
                                 float spatial_sigma, float intensity_sigma);
bool imgui_table_smart_smooth(ImGuiTable* table, int start_x, int start_y, int end_x, int end_y);

// Ignition table specific functions
bool imgui_ignition_table_create(ImGuiTable* table, int width, int height);
void imgui_ignition_table_load_demo_data(ImGuiTable* table);
void imgui_ignition_table_render_editor(ImGuiTable* table, float width, float height);
void imgui_ignition_table_handle_input(ImGuiTable* table, ImGuiVE3DView* view, float width, float height);

#ifdef __cplusplus
}
#endif

#endif // IMGUI_VE_TABLE_H 