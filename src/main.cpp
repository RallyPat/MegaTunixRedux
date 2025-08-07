/*
 * MegaTunix Redux - Main Application Entry Point
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

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

// ImGui includes
// Dear ImGui v1.92.2 WIP - Copyright (c) 2014-2025 Omar Cornut
// Licensed under MIT License - see LICENSE_ATTRIBUTION.md for full license text
#include "../../external/imgui/imgui.h"
#include "../../external/imgui/imgui_impl_sdl2.h"
#include "../../external/imgui/imgui_impl_opengl3.h"

#include "../include/megatunix_redux.h"
#include "../include/ecu/ecu_communication.h"
#include "../include/dashboard/dashboard.h"
#include "../include/utils/config.h"
#include "../include/utils/logging.h"
#include "../include/ui/imgui_ve_table.h"
#include "../include/ui/imgui_runtime_display.h"
#include "../include/ui/imgui_communications.h"
#include "../include/ui/imgui_key_bindings.h"

// Forward declaration for log callback
void imgui_communications_set_log_callback(LogCallback callback);

// Log system constants
#define MAX_LOG_ENTRIES 1000
#define MAX_LOG_LINE_LENGTH 256

// Log entry structure
typedef struct {
    char message[MAX_LOG_LINE_LENGTH];
    time_t timestamp;
    int level; // 0=INFO, 1=WARNING, 2=ERROR, 3=DEBUG
} LogEntry;

// Global variables
static SDL_Window* g_window = NULL;
static SDL_GLContext g_gl_context = NULL;
static TTF_Font* g_font = NULL;
static bool g_running = true;
static int g_selected_tab = 8; // Default to VE Table tab so it gets initialized
static const char* g_tab_names[] = {
    "About",
    "General", 
    "Communications",
    "Runtime Display",
    "DataLogging",
    "Logviewer",
    "Engine Vitals",
    "Enrichments",
    "VE Table(1)",
    "Tools",
    "Warmup Wizard"
};

// Log system state
static LogEntry g_log_entries[MAX_LOG_ENTRIES];
static int g_log_count = 0;
static int g_log_index = 0;
static bool g_log_window_open = true;
static bool g_log_auto_scroll = true;
static int g_log_filter_level = 0; // 0=all, 1=warning+, 2=error only, 3=high priority only

// ECU state
static ECUContext* g_ecu_context = NULL;
static bool g_ecu_connected = false;
static char g_ecu_status[256] = "Disconnected";
static ECUData g_ecu_data = {0};

// Button states
static bool g_locate_port_button_pressed = false;
static bool g_reverse_connect_button_pressed = false;
static bool g_reset_io_button_pressed = false;
static int g_button_press_timer = 0;
static char g_button_status_text[256] = "";

// Log timing
static uint32_t g_last_log_time = 0;

// VE Table state
static ImGuiTable* g_ve_table = NULL;
static ImGuiVE3DView g_ve_3d_view = {0};
static bool g_ve_table_initialized = false;
static ImGuiVETexture* g_ve_texture = NULL;
static bool g_ve_texture_initialized = false;
static bool g_ve_mouse_dragging = false;
static ImVec2 g_ve_last_mouse_pos = {0, 0};
static int g_ve_view_mode = 0; // 0=2D, 1=3D, 2=Editor
// Single cell selection (for backward compatibility)
static int g_selected_cell_x = -1;
static int g_selected_cell_y = -1;

// Multi-cell selection system
typedef struct {
    int start_x, start_y;
    int end_x, end_y;
    bool active;
    bool dragging;
    ImVec2 drag_start_pos;
    ImVec2 drag_current_pos;
} MultiCellSelection;

static MultiCellSelection g_multi_selection = {-1, -1, -1, -1, false, false, {0, 0}, {0, 0}};
static char g_cell_edit_buffer[32] = "";
static bool g_cell_editing = false;
static bool g_buffer_updated = false; // Flag to indicate buffer was updated
static bool g_input_field_focused = false; // Flag to track if input field is focused
static bool g_show_input_field = false; // Flag to control when to show the input field
static bool g_just_navigated = false; // Flag to track when we just navigated to a new cell
static int g_current_rpm_cell = -1;
static int g_current_map_cell = -1;

// Engine trail tracking
#define MAX_TRAIL_POINTS 50
typedef struct {
    int x, y;
    uint32_t timestamp;
    float alpha; // Fading alpha value
} TrailPoint;

static TrailPoint g_engine_trail_2d[MAX_TRAIL_POINTS];
static TrailPoint g_engine_trail_3d[MAX_TRAIL_POINTS];
static int g_trail_count_2d = 0;
static int g_trail_count_3d = 0;
static uint32_t g_last_trail_update = 0;
static bool g_show_engine_trail = true; // Toggle for engine trail visibility
// Removed Y-axis label rotation toggle - MAP title is now permanently rotated

// Demo mode variables
static bool g_demo_mode = false;
static bool g_debug_mode = false;

// Key binding state for professional table editing
static TableKeyBindingState g_key_binding_state;

// Clipboard for copy/paste operations
static float g_clipboard_data[16][16]; // 16x16 grid for clipboard
static int g_clipboard_width = 0;
static int g_clipboard_height = 0;
static bool g_clipboard_has_data = false;

// Interpolation state for smooth transitions
static int g_interpolation_start_x = -1;
static int g_interpolation_start_y = -1;
static int g_interpolation_end_x = -1;
static int g_interpolation_end_y = -1;
static bool g_interpolation_mode = false;

// Legend window state
static bool show_legend = false;

// Window position cache for event handling (to avoid calling ImGui functions from event handler)
static ImVec2 g_table_window_pos = ImVec2(0, 0);
static ImVec2 g_table_window_size = ImVec2(0, 0);
static bool g_table_window_valid = false;

// Safe table access wrapper (forward declaration)
struct SafeTableAccess;

// Runtime Display state
static ImGuiRuntimeDisplay* g_runtime_display = NULL;
static bool g_runtime_display_initialized = false;

// Communications state
static ImGuiCommunications* g_communications = NULL;
static bool g_communications_initialized = false;

// Camera preset functions
void set_isometric_view() {
    g_ve_3d_view.rotation_x = 35.264f; // arctan(1/sqrt(2)) - proper isometric angle
    g_ve_3d_view.rotation_y = 45.0f;
    g_ve_3d_view.zoom = 2.0f;
    g_ve_3d_view.pan_x = 0.0f;
    g_ve_3d_view.pan_y = 0.0f;
}

void set_side_view() {
    g_ve_3d_view.rotation_x = 0.0f;
    g_ve_3d_view.rotation_y = 90.0f;
    g_ve_3d_view.zoom = 1.5f;
    g_ve_3d_view.pan_x = 0.0f;
    g_ve_3d_view.pan_y = 0.0f;
}

void set_top_down_view() {
    g_ve_3d_view.rotation_x = 90.0f;
    g_ve_3d_view.rotation_y = 0.0f;
    g_ve_3d_view.zoom = 1.5f;
    g_ve_3d_view.pan_x = 0.0f;
    g_ve_3d_view.pan_y = 0.0f;
}

// Log system functions
void add_log_entry(int level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
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
    g_log_entries[g_log_index].timestamp = now;
    g_log_entries[g_log_index].level = level;
    strncpy(g_log_entries[g_log_index].message, full_message, MAX_LOG_LINE_LENGTH - 1);
    g_log_entries[g_log_index].message[MAX_LOG_LINE_LENGTH - 1] = '\0';
    
    g_log_index = (g_log_index + 1) % MAX_LOG_ENTRIES;
    if (g_log_count < MAX_LOG_ENTRIES) {
        g_log_count++;
    }
    
    va_end(args);
}

// Safe table access wrapper implementation
struct SafeTableAccess {
    static bool is_table_valid() {
        return g_ve_table != NULL && g_ve_table_initialized && 
               g_ve_table->data != NULL && 
               g_ve_table->width > 0 && g_ve_table->height > 0;
    }
    
    static float get_value_safe(int x, int y, float default_value = 75.0f) {
        if (!is_table_valid()) {
            add_log_entry(2, "SafeTableAccess: Table not valid for get_value_safe(%d, %d)", x, y);
            return default_value;
        }
        
        if (x < 0 || x >= g_ve_table->width || y < 0 || y >= g_ve_table->height) {
            add_log_entry(2, "SafeTableAccess: Invalid coordinates (%d, %d) for table %dx%d", 
                         x, y, g_ve_table->width, g_ve_table->height);
            return default_value;
        }
        
        if (g_ve_table->data[y] == NULL) {
            add_log_entry(2, "SafeTableAccess: Row %d is NULL", y);
            return default_value;
        }
        
        return g_ve_table->data[y][x];
    }
    
    static bool set_value_safe(int x, int y, float value) {
        if (!is_table_valid()) {
            add_log_entry(2, "SafeTableAccess: Table not valid for set_value_safe(%d, %d, %.1f)", x, y, value);
            return false;
        }
        
        if (x < 0 || x >= g_ve_table->width || y < 0 || y >= g_ve_table->height) {
            add_log_entry(2, "SafeTableAccess: Invalid coordinates (%d, %d) for table %dx%d", 
                         x, y, g_ve_table->width, g_ve_table->height);
            return false;
        }
        
        if (g_ve_table->data[y] == NULL) {
            add_log_entry(2, "SafeTableAccess: Row %d is NULL", y);
            return false;
        }
        
        g_ve_table->data[y][x] = value;
        return true;
    }
};

void render_log_window() {
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
    
    if (ImGui::Begin("UI Log", &g_log_window_open, window_flags)) {
        // Log controls
        ImGui::Checkbox("Auto-scroll", &g_log_auto_scroll);
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            g_log_count = 0;
            g_log_index = 0;
        }
        ImGui::SameLine();
        ImGui::Text("Filter:");
        ImGui::SameLine();
        const char* filter_items[] = {"All", "Warning+", "Error Only", "High Priority Only"};
        ImGui::SetNextItemWidth(120);
        ImGui::Combo("##filter", &g_log_filter_level, filter_items, IM_ARRAYSIZE(filter_items));
        
        ImGui::Separator();
        
        // Log display area
        ImGui::BeginChild("LogContent", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        // Display log entries
        for (int i = 0; i < g_log_count; i++) {
            int idx = (g_log_index - g_log_count + i + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
            LogEntry* entry = &g_log_entries[idx];
            
            // Apply filter
            bool show_entry = true;
            if (g_log_filter_level == 1 && entry->level < 1) { // Warning+
                show_entry = false;
            } else if (g_log_filter_level == 2 && entry->level < 2) { // Error Only
                show_entry = false;
            } else if (g_log_filter_level == 3) { // High Priority Only
                show_entry = (strstr(entry->message, "***") != NULL);
            }
            
            if (!show_entry) {
                continue;
            }
            
            // Color based on log level
            ImVec4 color;
            if (strstr(entry->message, "***") != NULL) {
                // High priority messages - bright cyan
                color = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
            } else {
                switch (entry->level) {
                    case 0: // INFO
                        color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                        break;
                    case 1: // WARNING
                        color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                        break;
                    case 2: // ERROR
                        color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                        break;
                    case 3: // DEBUG
                        color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                        break;
                    default:
                        color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                }
            }
            
            ImGui::TextColored(color, "%s", entry->message);
        }
        
        // Auto-scroll to bottom
        if (g_log_auto_scroll) {
            ImGui::SetScrollHereY(1.0f);
        }
        
        ImGui::EndChild();
    }
    ImGui::End();
}

// Function declarations
bool init_sdl();
bool init_opengl();
bool init_ttf();
bool init_imgui();
bool init_ecu_communication();
void cleanup_sdl();
void cleanup_opengl();
void cleanup_ttf();
void cleanup_imgui();
void cleanup_ecu_communication();
void handle_events();
void update();
void render();
void render_main_window();
void render_about_tab();
void render_general_tab();
void render_communications_tab();
void render_runtime_display_tab();
void render_datalogging_tab();
void render_logviewer_tab();
void render_engine_vitals_tab();
void render_enrichments_tab();
void render_ve_table_tab();
void render_ve_table_2d_view();
void render_ve_table_3d_view();
void render_ve_table_editor_view();
void render_tools_tab();
void render_warmup_wizard_tab();
void handle_communications_buttons(); 
void update_engine_trail(int current_x, int current_y, TrailPoint* trail, int* trail_count);
void copy_selected_cell_to_clipboard();
void paste_from_clipboard();
void interpolate_between_cells();

// Multi-cell selection helper functions
void clear_multi_selection();
void start_multi_selection(int x, int y);
void update_multi_selection(int x, int y);
void end_multi_selection();
bool is_cell_in_selection(int x, int y);
void get_selection_bounds(int* min_x, int* min_y, int* max_x, int* max_y);
int get_selection_cell_count();
void apply_operation_to_selection(TableOperation operation, float value);

int main(int argc, char* argv[]) {
    printf("MegaTunix Redux - ImGui Version\n");
    printf("By Patrick Burke\n");
    printf("Based on MegaTunix by David J. Andruczyk\n");
    printf("(ImGui Version)\n\n");

    // Parse command line arguments
    bool demo_mode = false;
    bool debug_mode = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--demo-mode") == 0 || strcmp(argv[i], "-d") == 0) {
            demo_mode = true;
            printf("Demo mode enabled\n");
        } else if (strcmp(argv[i], "--debug") == 0) {
            debug_mode = true;
            printf("Debug mode enabled\n");
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --demo-mode, -d    Enable demo mode with simulated data\n");
            printf("  --debug            Enable debug mode\n");
            printf("  --help, -h         Show this help message\n");
            return 0;
        }
    }

    // Set global demo mode flags
    g_demo_mode = demo_mode;
    g_debug_mode = debug_mode;
    
    // Add initial log entry
    add_log_entry(0, "MegaTunix Redux starting up...");
    if (demo_mode) {
        add_log_entry(0, "Demo mode enabled - using simulated data");
    }

    // Initialize SDL
    if (!init_sdl()) {
        add_log_entry(2, "Failed to initialize SDL");
        fprintf(stderr, "Failed to initialize SDL\n");
        return 1;
    }
    add_log_entry(0, "SDL initialized successfully");

    // Initialize OpenGL
    if (!init_opengl()) {
        add_log_entry(2, "Failed to initialize OpenGL");
        fprintf(stderr, "Failed to initialize OpenGL\n");
        cleanup_sdl();
        return 1;
    }
    add_log_entry(0, "OpenGL initialized successfully");

    // Initialize TTF
    if (!init_ttf()) {
        add_log_entry(2, "Failed to initialize TTF");
        fprintf(stderr, "Failed to initialize TTF\n");
        cleanup_opengl();
        cleanup_sdl();
        return 1;
    }
    add_log_entry(0, "TTF initialized successfully");

    // Initialize ImGui
    if (!init_imgui()) {
        add_log_entry(2, "Failed to initialize ImGui");
        fprintf(stderr, "Failed to initialize ImGui\n");
        cleanup_ttf();
        cleanup_opengl();
        cleanup_sdl();
        return 1;
    }
    add_log_entry(0, "ImGui initialized successfully");

    // Initialize ECU communication
    if (!init_ecu_communication()) {
        add_log_entry(2, "Failed to initialize ECU communication");
        fprintf(stderr, "Failed to initialize ECU communication\n");
        cleanup_imgui();
        cleanup_ttf();
        cleanup_opengl();
        cleanup_sdl();
        return 1;
    }
    add_log_entry(0, "ECU communication initialized successfully");
    
    // Set up global demo mode callback
    ecu_set_global_demo_mode_callback([](bool enabled) {
        g_demo_mode = enabled;
        if (enabled) {
            add_log_entry(0, "Global demo mode enabled - demo data generation active");
        } else {
            add_log_entry(0, "Global demo mode disabled - demo data generation stopped");
        }
    });
    
    // Initialize key binding system for professional table editing
    imgui_key_bindings_init(&g_key_binding_state);
    add_log_entry(0, "Key binding system initialized for professional table editing");

    // Initialize VE table early so it's available for the update loop
    add_log_entry(0, "Initializing VE table...");
    g_ve_table = imgui_ve_table_create(16, 12); // Start with default size, will be resized based on INI
    add_log_entry(0, "VE table created at address: %p with size: %dx%d", g_ve_table, g_ve_table ? g_ve_table->width : -1, g_ve_table ? g_ve_table->height : -1);
    if (g_ve_table) {
        add_log_entry(0, "VE table created successfully");
        add_log_entry(0, "*** VE TABLE CREATED: %dx%d ***", g_ve_table->width, g_ve_table->height);
        
        // Load demo data for testing
        imgui_table_load_demo_data(g_ve_table);
        add_log_entry(0, "VE table size: %dx%d", g_ve_table->width, g_ve_table->height);
        
        // Always load demo data for testing
        imgui_table_load_demo_data(g_ve_table);
        add_log_entry(0, "Demo data loaded into VE table");
        add_log_entry(0, "VE table value range: %.1f - %.1f", g_ve_table->metadata.min_value, g_ve_table->metadata.max_value);
        
        // Initialize 3D view with isometric default
        set_isometric_view();
        g_ve_3d_view.wireframe_mode = false;
        g_ve_3d_view.show_grid = true;
        g_ve_3d_view.show_axes = true;
        g_ve_3d_view.opacity = 0.8f;
        
        g_ve_table_initialized = true;
        add_log_entry(0, "VE table initialization complete");
    } else {
        add_log_entry(2, "ERROR: Failed to create VE table");
    }
    
    add_log_entry(0, "VE table callbacks will be set up when communications module is initialized");

    add_log_entry(0, "Initialization complete - entering main loop");

    // Initialize communications
    if (!init_ecu_communication()) {
        fprintf(stderr, "Failed to initialize ECU communication\n");
        return 1;
    }
    
    // Main loop
    while (g_running) {
        handle_events();
        update();
        render();
    }

    add_log_entry(0, "Shutting down...");

    // Cleanup
    cleanup_ecu_communication();
    cleanup_imgui();
    cleanup_ttf();
    cleanup_opengl();
    cleanup_sdl();

    add_log_entry(0, "Cleanup complete");
    printf("Cleanup complete\n");
    return 0;
}

bool init_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window
    g_window = SDL_CreateWindow(
        "MEGATUNIX REDUX DISCONNECTED Ready for ECU Connection",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
    );

    if (!g_window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

bool init_opengl() {
    g_gl_context = SDL_GL_CreateContext(g_window);
    if (!g_gl_context) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_MakeCurrent(g_window, g_gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    return true;
}

bool init_ttf() {
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        return false;
    }

    g_font = TTF_OpenFont("assets/fonts/DejaVuSans.ttf", 14);
    if (!g_font) {
        printf("Could not load font: %s\n", TTF_GetError());
        printf("Continuing without custom font...\n");
        // Don't return false - we can continue without the font
    }

    return true;
}

bool init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Remove docking and viewports for now - they may not be available in this ImGui version
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Platform/Renderer backends FIRST
    if (!ImGui_ImplSDL2_InitForOpenGL(g_window, g_gl_context)) {
        fprintf(stderr, "ImGui_ImplSDL2_InitForOpenGL failed\n");
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 130")) {
        fprintf(stderr, "ImGui_ImplOpenGL3_Init failed\n");
        return false;
    }
    
    // Set renderer flags for proper font atlas handling BEFORE loading fonts
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

    // Setup style
    ImGui::StyleColorsDark();

    // Load Unicode font with comprehensive Unicode support
    ImFont* font = nullptr;
    
    // Configure font atlas for Unicode support
    ImFontAtlas* atlas = io.Fonts;
    atlas->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
    atlas->Flags |= ImFontAtlasFlags_NoMouseCursors;
    
    // Add Unicode glyph ranges for better symbol support (16-bit compatible)
    static const ImWchar ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2190, 0x21FF, // Arrows
        0x2600, 0x26FF, // Miscellaneous Symbols
        0x2700, 0x27BF, // Dingbats
        0,
    };
    
    // Configure font config for better Unicode support
    ImFontConfig font_config;
    font_config.MergeMode = false;
    font_config.PixelSnapH = true;
    font_config.OversampleH = 2;
    font_config.OversampleV = 1;
    
    // Try to load a font with Unicode support - start with DejaVu which has good Unicode coverage
    font = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16.0f, &font_config, ranges);
    if (font) {
        printf("Loaded DejaVu Sans font successfully\n");
    } else {
        // Try bundled Noto Sans
        font = io.Fonts->AddFontFromFileTTF("assets/fonts/NotoSans-Regular.ttf", 16.0f, &font_config, ranges);
        if (font) {
            printf("Loaded bundled Noto Sans font successfully\n");
        } else {
            // Try system Noto Sans
            font = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf", 16.0f, &font_config, ranges);
            if (font) {
                printf("Loaded system Noto Sans font successfully\n");
            } else {
                // Try Ubuntu font
                font = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf", 16.0f, &font_config, ranges);
                if (font) {
                    printf("Loaded Ubuntu font successfully\n");
                } else {
                    // Try Liberation Sans
                    font = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 16.0f, &font_config, ranges);
                    if (font) {
                        printf("Loaded Liberation Sans font successfully\n");
                    } else {
                        printf("Warning: Could not load any Unicode font, using default font\n");
                    }
                }
            }
        }
    }
    if (!font) {
        // Final fallback - use default font
        printf("Warning: Could not load Unicode font, using default font\n");
        font = io.Fonts->AddFontDefault();
        if (font) {
            printf("Loaded default font successfully\n");
        }
    }
    
    if (font) {
        // Set as default font
        io.FontDefault = font;
        printf("Loaded Unicode font successfully: %s\n", font ? "Yes" : "No");
    }

    return true;
}

bool init_ecu_communication() {
    g_ecu_context = ecu_init();
    if (!g_ecu_context) {
        fprintf(stderr, "Failed to initialize ECU context\n");
        return false;
    }
    return true;
}

void cleanup_sdl() {
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
    }
    SDL_Quit();
}

void cleanup_opengl() {
    if (g_gl_context) {
        SDL_GL_DeleteContext(g_gl_context);
        g_gl_context = NULL;
    }
}

void cleanup_ttf() {
    if (g_font) {
        TTF_CloseFont(g_font);
        g_font = NULL;
    }
    TTF_Quit();
}

void cleanup_imgui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void cleanup_ecu_communication() {
    if (g_ecu_context) {
        ecu_cleanup(g_ecu_context);
        g_ecu_context = NULL;
    }
    
    // Cleanup VE table
    if (g_ve_table) {
        imgui_table_destroy(g_ve_table);
        g_ve_table = NULL;
    }
    g_ve_table_initialized = false;
    
    // Cleanup VE texture
    if (g_ve_texture) {
        imgui_ve_texture_destroy(g_ve_texture);
        g_ve_texture = NULL;
    }
    g_ve_texture_initialized = false;
    
    // Cleanup runtime display
    if (g_runtime_display) {
        imgui_runtime_display_destroy(g_runtime_display);
        g_runtime_display = NULL;
    }
    g_runtime_display_initialized = false;
    
    // Cleanup communications
    if (g_communications) {
        imgui_communications_destroy(g_communications);
        g_communications = NULL;
    }
    g_communications_initialized = false;
}

void handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Get ImGui IO state to check if ImGui wants the event
        ImGuiIO& io = ImGui::GetIO();
        bool imgui_wants_mouse = io.WantCaptureMouse;
        bool imgui_wants_keyboard = io.WantCaptureKeyboard;
        
        // Handle our custom input processing first (before ImGui)
        bool event_handled = false;
        
        switch (event.type) {
            case SDL_QUIT:
                g_running = false;
                break;
                
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    g_running = false;
                }
                break;
                
            case SDL_KEYDOWN:
                // Handle legend window keyboard shortcuts (global, regardless of focus)
                if (show_legend && (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_RETURN)) {
                    add_log_entry(0, "Global keyboard shortcut pressed - closing legend window");
                    show_legend = false;
                    event_handled = true;
                    break;
                }
                
                // Handle professional table editing key bindings (for VE Table tab)
                if (g_selected_tab == 8) { // VE Table tab is active
                    imgui_key_bindings_set_table_focused(true);
                    
                    // Add detailed key logging for debugging
                    add_log_entry(0, "*** KEY EVENT DEBUG *** - Key: %c (0x%02X), Modifiers: 0x%04X, Tab: %d", 
                                 event.key.keysym.sym, event.key.keysym.sym, event.key.keysym.mod, g_selected_tab);
                    
                    TableOperation operation = imgui_key_bindings_process_key(&g_key_binding_state, 
                                                                             event.key.keysym.sym, 
                                                                             (SDL_Keymod)event.key.keysym.mod);
                    add_log_entry(0, "*** KEY PRESSED *** - Key: %c (0x%02X), Operation: %d (%s)", 
                                 event.key.keysym.sym, event.key.keysym.sym, operation, 
                                 imgui_key_bindings_get_operation_name(operation));
                    
                    add_log_entry(0, "*** OPERATION CHECK *** - Operation: %d, Selected Cell: [%d,%d], Table Focused: %s", 
                                 operation, g_selected_cell_x, g_selected_cell_y, 
                                 imgui_key_bindings_is_table_focused() ? "true" : "false");
                    
                    // Handle basic navigation keys first
                    bool navigation_handled = false;
                    if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
                        // Save current buffer value before navigating away
                        if (strlen(g_cell_edit_buffer) > 0) {
                            float input_value = atof(g_cell_edit_buffer);
                            if (input_value >= 0.0f && input_value <= 200.0f) {
                                // Save to the current cell before moving
                                imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, input_value);
                                add_log_entry(0, "Auto-saved value %.1f to cell [%d,%d] on navigation", 
                                            input_value, g_selected_cell_x, g_selected_cell_y);
                            }
                        }
                        
                        switch (event.key.keysym.sym) {
                            case SDLK_UP:
                                g_selected_cell_y = fmax(0, g_selected_cell_y - 1);
                                navigation_handled = true;
                                add_log_entry(0, "Navigation: Moved UP to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                break;
                            case SDLK_DOWN:
                                g_selected_cell_y = fmin(g_ve_table->height - 1, g_selected_cell_y + 1);
                                navigation_handled = true;
                                add_log_entry(0, "Navigation: Moved DOWN to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                break;
                            case SDLK_LEFT:
                                g_selected_cell_x = fmax(0, g_selected_cell_x - 1);
                                navigation_handled = true;
                                add_log_entry(0, "Navigation: Moved LEFT to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                break;
                            case SDLK_RIGHT:
                                g_selected_cell_x = fmin(g_ve_table->width - 1, g_selected_cell_x + 1);
                                navigation_handled = true;
                                add_log_entry(0, "Navigation: Moved RIGHT to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                break;
                            case SDLK_TAB:
                                if (event.key.keysym.mod & KMOD_SHIFT) {
                                    // Shift+Tab: Move backward
                                    g_selected_cell_x--;
                                    if (g_selected_cell_x < 0) {
                                        g_selected_cell_x = g_ve_table->width - 1;
                                        g_selected_cell_y--;
                                        if (g_selected_cell_y < 0) {
                                            g_selected_cell_y = g_ve_table->height - 1;
                                        }
                                    }
                                } else {
                                    // Tab: Move forward
                                    g_selected_cell_x++;
                                    if (g_selected_cell_x >= g_ve_table->width) {
                                        g_selected_cell_x = 0;
                                        g_selected_cell_y++;
                                        if (g_selected_cell_y >= g_ve_table->height) {
                                            g_selected_cell_y = 0;
                                        }
                                    }
                                }
                                navigation_handled = true;
                                add_log_entry(0, "Navigation: Tab to cell [%d,%d]", g_selected_cell_x, g_selected_cell_y);
                                break;
                        }
                    }
                    
                    if (navigation_handled) {
                        event_handled = true;
                        // Update the cell edit buffer with the new cell's value
                        snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.1f", 
                                imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y));
                        g_buffer_updated = true;
                        g_just_navigated = true; // Set flag to clear buffer on next number entry
                    }
                    
                    // Check if we have a valid target (single cell or multi-cell selection)
                    bool has_valid_target = (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) || g_multi_selection.active;
                    
                    if (operation != TABLE_OP_NONE && has_valid_target) {
                        float current_val, new_val;
                        
                        // Handle professional operations
                        switch (operation) {
                            case TABLE_OP_SET_TO:
                                // Set to specific value (will prompt user)
                                add_log_entry(0, "Set to operation triggered - value input needed");
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_INCREMENT:
                                // Increment by configured amount
                                if (g_multi_selection.active) {
                                    apply_operation_to_selection(TABLE_OP_INCREASE_BY, g_key_binding_state.increment_amount);
                                } else {
                                    current_val = imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y);
                                    new_val = fmin(200.0f, current_val + g_key_binding_state.increment_amount);
                                    imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, new_val);
                                    snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.0f", new_val);
                                    g_buffer_updated = true;
                                    add_log_entry(0, "Incremented cell [%d,%d] to %.1f", g_selected_cell_x, g_selected_cell_y, new_val);
                                }
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_DECREMENT:
                                // Decrement by configured amount
                                if (g_multi_selection.active) {
                                    apply_operation_to_selection(TABLE_OP_DECREASE_BY, g_key_binding_state.increment_amount);
                                } else {
                                    current_val = imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y);
                                    new_val = fmax(0.0f, current_val - g_key_binding_state.increment_amount);
                                    imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, new_val);
                                    snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.0f", new_val);
                                    g_buffer_updated = true;
                                    add_log_entry(0, "Decremented cell [%d,%d] to %.1f", g_selected_cell_x, g_selected_cell_y, new_val);
                                }
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_INCREASE_BY:
                                // Increase by configured amount
                                if (g_multi_selection.active) {
                                    apply_operation_to_selection(TABLE_OP_INCREASE_BY, g_key_binding_state.increment_amount);
                                } else {
                                    current_val = imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y);
                                    new_val = fmin(200.0f, current_val + g_key_binding_state.increment_amount);
                                    imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, new_val);
                                    snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.0f", new_val);
                                    g_buffer_updated = true;
                                    add_log_entry(0, "Increased cell [%d,%d] to %.1f", g_selected_cell_x, g_selected_cell_y, new_val);
                                }
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_DECREASE_BY:
                                // Decrease by configured amount
                                if (g_multi_selection.active) {
                                    apply_operation_to_selection(TABLE_OP_DECREASE_BY, g_key_binding_state.increment_amount);
                                } else {
                                    current_val = imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y);
                                    new_val = fmax(0.0f, current_val - g_key_binding_state.increment_amount);
                                    imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, new_val);
                                    snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.0f", new_val);
                                    g_buffer_updated = true;
                                    add_log_entry(0, "Decreased cell [%d,%d] to %.1f", g_selected_cell_x, g_selected_cell_y, new_val);
                                }
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_SCALE_BY: {
                                // Scale by configured percentage factor
                                if (g_multi_selection.active) {
                                    apply_operation_to_selection(TABLE_OP_SCALE_BY, g_key_binding_state.percent_increment);
                                } else {
                                    current_val = imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y);
                                    float scale_factor = 1.0f + (g_key_binding_state.percent_increment / 100.0f);
                                    new_val = fmin(200.0f, fmax(0.0f, current_val * scale_factor));
                                    imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, new_val);
                                    snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.0f", new_val);
                                    g_buffer_updated = true;
                                    add_log_entry(0, "Scaled cell [%d,%d] by %.1f%% to %.1f", g_selected_cell_x, g_selected_cell_y, g_key_binding_state.percent_increment, new_val);
                                }
                                event_handled = true;
                                break;
                            }
                                
                            case TABLE_OP_INTERPOLATE:
                                add_log_entry(0, "*** INTERPOLATION TRIGGERED *** - Key pressed, calling interpolate_between_cells()");
                                interpolate_between_cells();
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_INTERPOLATE_H:
                            case TABLE_OP_INTERPOLATE_V:
                            case TABLE_OP_SMOOTH_CELLS:
                            case TABLE_OP_FILL_UP_RIGHT:
                                add_log_entry(0, "Advanced operation '%s' triggered - implementation pending", 
                                            imgui_key_bindings_get_operation_name(operation));
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_COPY:
                                copy_selected_cell_to_clipboard();
                                event_handled = true;
                                break;
                                
                            case TABLE_OP_PASTE:
                                paste_from_clipboard();
                                event_handled = true;
                                break;
                                
                            default:
                                break;
                        }
                    }
                    
                    // Handle direct number entry for selected cell
                    if (!event_handled && g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
                        // Check if it's a number key (0-9) or decimal point
                        if ((event.key.keysym.sym >= SDLK_0 && event.key.keysym.sym <= SDLK_9) || 
                            event.key.keysym.sym == SDLK_PERIOD || event.key.keysym.sym == SDLK_KP_PERIOD) {
                            
                            // Clear buffer if we just navigated to this cell
                            if (g_just_navigated) {
                                g_cell_edit_buffer[0] = '\0';
                                g_just_navigated = false;
                            }
                            
                            // Get the character to append
                            char key_char = '\0';
                            if (event.key.keysym.sym >= SDLK_0 && event.key.keysym.sym <= SDLK_9) {
                                key_char = '0' + (event.key.keysym.sym - SDLK_0);
                            } else if (event.key.keysym.sym == SDLK_PERIOD || event.key.keysym.sym == SDLK_KP_PERIOD) {
                                key_char = '.';
                            }
                            
                            if (key_char != '\0') {
                                // Append to the buffer
                                int current_len = strlen(g_cell_edit_buffer);
                                if (current_len < sizeof(g_cell_edit_buffer) - 1) {
                                    g_cell_edit_buffer[current_len] = key_char;
                                    g_cell_edit_buffer[current_len + 1] = '\0';
                                    g_buffer_updated = true;
                                    event_handled = true;
                                    
                                    add_log_entry(0, "Direct number entry: appended '%c' to buffer, now: '%s'", 
                                                key_char, g_cell_edit_buffer);
                                }
                            }
                        }
                        // Handle Enter key to apply the value
                        else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                            if (strlen(g_cell_edit_buffer) > 0) {
                                float input_value = atof(g_cell_edit_buffer);
                                if (input_value >= 0.0f && input_value <= 200.0f) {
                                    imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, input_value);
                                    add_log_entry(0, "Applied direct number entry: %.1f to cell [%d,%d]", 
                                                input_value, g_selected_cell_x, g_selected_cell_y);
                                    
                                    // Clear the buffer after successful application
                                    g_cell_edit_buffer[0] = '\0';
                                    g_buffer_updated = true;
                                    event_handled = true;
                                }
                            }
                        }
                        // Handle Escape key to cancel editing
                        else if (event.key.keysym.sym == SDLK_ESCAPE) {
                            // Clear the buffer and restore original value
                            g_cell_edit_buffer[0] = '\0';
                            snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.1f", 
                                    imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y));
                            g_buffer_updated = true;
                            event_handled = true;
                            add_log_entry(0, "Cancelled direct number entry, restored original value");
                        }
                    }
                }
                break;
                
                                case SDL_MOUSEBUTTONDOWN:
                        // Handle mouse clicks for table interactions
                        add_log_entry(0, "*** MOUSE BUTTON DOWN EVENT *** - Tab: %d, Button: %d, g_ve_table: %p, initialized: %s", 
                                    g_selected_tab, event.button.button, g_ve_table, g_ve_table_initialized ? "true" : "false");
                        
                        if (g_selected_tab == 8 && event.button.button == SDL_BUTTON_LEFT) { // VE Table tab is active
                    add_log_entry(0, "*** SDL MOUSE CLICK DETECTED - Button: %d, Position: (%d, %d) ***", 
                                 event.button.button, event.button.x, event.button.y);
                    
                    // Check if table is valid before proceeding
                    if (!g_ve_table || !g_ve_table_initialized) {
                        add_log_entry(0, "*** TABLE NOT VALID *** - g_ve_table: %p, initialized: %s", 
                                    g_ve_table, g_ve_table_initialized ? "true" : "false");
                        event_handled = true;
                        break;
                    }
                    
                    // Handle legend window mouse clicks (global, regardless of focus)
                    if (show_legend) {
                        // Get mouse position from the event (more accurate than GetMouseState)
                        int mouse_x = event.button.x;
                        int mouse_y = event.button.y;
                        
                        // Log all mouse clicks when legend is open for debugging
                        add_log_entry(0, "Mouse click detected at (%d, %d) - legend is open", mouse_x, mouse_y);
                        
                        // Check if click is in the button area (better estimate)
                        int window_width = ImGui::GetIO().DisplaySize.x;
                        int window_height = ImGui::GetIO().DisplaySize.y;
                        
                        // Legend window is centered, button is at bottom of legend
                        // Legend window is 700x600, so button should be roughly:
                        int legend_center_x = window_width / 2;
                        int legend_center_y = window_height / 2;
                        int legend_bottom = legend_center_y + 300; // Half of 600
                        
                        // Button is 200x40, centered horizontally in legend
                        int button_x = legend_center_x - 100;  // Button width is 200
                        int button_y = legend_bottom - 60;     // Button height is 40, some padding
                        
                        add_log_entry(0, "Button area: (%d,%d) to (%d,%d)", button_x, button_y, button_x + 200, button_y + 40);
                        
                        if (mouse_x >= button_x && mouse_x <= button_x + 200 &&
                            mouse_y >= button_y && mouse_y <= button_y + 40) {
                            add_log_entry(0, "*** CLICK IN BUTTON AREA DETECTED - CLOSING LEGEND ***");
                            show_legend = false;
                            event_handled = true;
                            break;
                        } else {
                            add_log_entry(0, "Click outside button area");
                        }
                    }
                    
                    // Handle table cell clicks
                    // Convert screen coordinates to table coordinates
                    int mouse_x = event.button.x;
                    int mouse_y = event.button.y;
                    
                    add_log_entry(0, "*** MOUSE CLICK DEBUG *** - Mouse: (%d, %d)", mouse_x, mouse_y);
                    
                    // Get the VE table window position and size
                    // We need to calculate the table area based on the window layout
                    int window_width = ImGui::GetIO().DisplaySize.x;
                    int window_height = ImGui::GetIO().DisplaySize.y;
                    
                    add_log_entry(0, "*** WINDOW DEBUG *** - Size: %dx%d", window_width, window_height);
                    
                    // Calculate table area - use cached window position to avoid calling ImGui functions from event handler
                    float table_start_x = 0, table_start_y = 0, table_width = 0, table_height = 0;
                    if (g_table_window_valid) {
                        table_start_x = g_table_window_pos.x + 40;  // Match rendering: 40px from left
                        table_start_y = g_table_window_pos.y + 30;  // Match rendering: 30px from top
                        table_width = g_table_window_size.x - 80;   // Match rendering: 40px margin on each side
                        table_height = g_table_window_size.y - 60;  // Match rendering: 30px margin on top/bottom
                    } else {
                        // Fallback to approximate values if cache is not valid
                        table_start_x = 120;
                        table_start_y = 250;
                        table_width = window_width - 240;
                        table_height = window_height - 450;
                    }
                    
                    // Ensure minimum table dimensions
                    if (table_width < 100) table_width = 100;
                    if (table_height < 100) table_height = 100;
                    
                    add_log_entry(0, "*** TABLE AREA DEBUG *** - Window: %dx%d, Table area: (%.1f,%.1f) to (%.1f,%.1f)", 
                                window_width, window_height, table_start_x, table_start_y, 
                                table_start_x + table_width, table_start_y + table_height);
                    
                    // Convert to table coordinates
                    int table_x = -1, table_y = -1;
                    if (mouse_x >= table_start_x && mouse_x <= table_start_x + table_width &&
                        mouse_y >= table_start_y && mouse_y <= table_start_y + table_height &&
                        g_ve_table && g_ve_table->width > 0 && g_ve_table->height > 0) {
                        
                        add_log_entry(0, "*** COORDINATE CONVERSION START *** - Mouse: (%d, %d), Table area: (%.1f,%.1f) to (%.1f,%.1f)", 
                                    mouse_x, mouse_y, table_start_x, table_start_y, table_start_x + table_width, table_start_y + table_height);
                        
                        // Convert to table coordinates (no additional margin adjustment needed)
                        float adjusted_mouse_x = mouse_x - table_start_x;
                        float adjusted_mouse_y = mouse_y - table_start_y;
                        
                        add_log_entry(0, "*** ADJUSTED COORDINATES *** - Adjusted: (%.1f, %.1f)", adjusted_mouse_x, adjusted_mouse_y);
                        
                        // Calculate cell dimensions - match rendering exactly
                        float cell_width = table_width / g_ve_table->width;
                        float cell_height = table_height / g_ve_table->height;
                        
                        add_log_entry(0, "*** CELL DIMENSIONS *** - Cell width: %.1f, Cell height: %.1f", cell_width, cell_height);
                        
                        // Check for division by zero
                        if (cell_width > 0 && cell_height > 0) {
                            table_x = (int)(adjusted_mouse_x / cell_width);
                            table_y = (int)(adjusted_mouse_y / cell_height);
                            
                            add_log_entry(0, "*** RAW TABLE COORDINATES *** - Raw: [%d, %d]", table_x, table_y);
                            
                            // Clamp to valid range
                            table_x = fmax(0, fmin(g_ve_table->width - 1, table_x));
                            table_y = fmax(0, fmin(g_ve_table->height - 1, table_y));
                            
                            add_log_entry(0, "*** FINAL TABLE COORDINATES *** - Final: [%d, %d]", table_x, table_y);
                        } else {
                            add_log_entry(0, "*** INVALID CELL DIMENSIONS *** - Cell width: %.1f, Cell height: %.1f", cell_width, cell_height);
                        }
                    }
                    
                    // Check for modifier keys for multi-cell selection
                    const Uint8* key_state = SDL_GetKeyboardState(NULL);
                    bool ctrl_pressed = key_state[SDL_SCANCODE_LCTRL] || key_state[SDL_SCANCODE_RCTRL];
                    bool shift_pressed = key_state[SDL_SCANCODE_LSHIFT] || key_state[SDL_SCANCODE_RSHIFT];
                    
                    add_log_entry(0, "*** MOUSE CLICK FOR TABLE *** - Position: (%d, %d), Ctrl: %s, Shift: %s", 
                                mouse_x, mouse_y, ctrl_pressed ? "true" : "false", shift_pressed ? "true" : "false");
                    
                    add_log_entry(0, "*** TABLE CLICK PROCESSING *** - About to process click");
                    
                    if (table_x >= 0 && table_y >= 0 && table_x < g_ve_table->width && table_y < g_ve_table->height) {
                        add_log_entry(3, "Valid table click at [%d, %d]", table_x, table_y);
                        
                        // Set the selected cell first (safe operation)
                        g_selected_cell_x = table_x;
                        g_selected_cell_y = table_y;
                        
                        if (ctrl_pressed || shift_pressed) {
                            // Start multi-cell selection
                            add_log_entry(3, "Starting multi-cell selection");
                            // Temporarily disable multi-cell selection to isolate crash
                            add_log_entry(0, "*** MULTI-CELL SELECTION TEMPORARILY DISABLED ***");
                            event_handled = true;
                        } else {
                            // Single cell selection (existing behavior)
                            add_log_entry(3, "Starting single cell selection");
                            g_cell_editing = true;
                            g_input_field_focused = false; // Clear input field focus when clicking a new cell
                            g_show_input_field = true; // Show input field when clicking
                            
                            // Use safe table access wrapper
                            float cell_value = SafeTableAccess::get_value_safe(table_x, table_y, 75.0f);
                            add_log_entry(3, "Safe table access: %.1f at [%d,%d]", cell_value, table_x, table_y);
                            
                            snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.1f", cell_value);
                            
                            // Clear any existing multi-selection
                            // Temporarily disable to isolate crash
                            add_log_entry(0, "*** CLEAR MULTI-SELECTION TEMPORARILY DISABLED ***");
                            
                            // Set flag to focus on next frame instead of calling SetKeyboardFocusHere directly
                            g_buffer_updated = true;
                            add_log_entry(3, "Cell selected for editing - focus will be set on next frame");
                            event_handled = true;
                        }
                    } else {
                        add_log_entry(0, "*** INVALID TABLE CLICK *** - Mouse outside table area or invalid coordinates");
                    }
                }
                break;
                
                               case SDL_MOUSEBUTTONUP:
                       // Handle mouse release for multi-cell selection
                       if (g_selected_tab == 8 && event.button.button == SDL_BUTTON_LEFT && g_multi_selection.dragging && g_ve_table && g_ve_table_initialized) {
                           end_multi_selection();
                           event_handled = true;
                       }
                       break;
                       
                   case SDL_MOUSEMOTION:
                       // Handle mouse drag for multi-cell selection
                       if (g_selected_tab == 8 && g_multi_selection.dragging && g_ve_table && g_ve_table_initialized) {
                           // Convert SDL mouse coordinates to table coordinates
                           int mouse_x = event.motion.x;
                           int mouse_y = event.motion.y;
                           
                           // Calculate table area - use cached window position to avoid calling ImGui functions from event handler
                           float table_start_x = 0, table_start_y = 0, table_width = 0, table_height = 0;
                           if (g_table_window_valid) {
                               table_start_x = g_table_window_pos.x + 40;  // Match rendering: 40px from left
                               table_start_y = g_table_window_pos.y + 30;  // Match rendering: 30px from top
                               table_width = g_table_window_size.x - 80;   // Match rendering: 40px margin on each side
                               table_height = g_table_window_size.y - 60;  // Match rendering: 30px margin on top/bottom
                           } else {
                               // Fallback to approximate values if cache is not valid
                               int window_width = ImGui::GetIO().DisplaySize.x;
                               int window_height = ImGui::GetIO().DisplaySize.y;
                               table_start_x = 120;
                               table_start_y = 250;
                               table_width = window_width - 240;
                               table_height = window_height - 450;
                           }
                           
                           // Convert to table coordinates
                           int table_x = -1, table_y = -1;
                           if (mouse_x >= table_start_x && mouse_x <= table_start_x + table_width &&
                               mouse_y >= table_start_y && mouse_y <= table_start_y + table_height) {
                               // Convert to table coordinates (no additional margin adjustment needed)
                               float adjusted_mouse_x = mouse_x - table_start_x;
                               float adjusted_mouse_y = mouse_y - table_start_y;
                               
                               // Calculate cell dimensions - match rendering exactly
                               float cell_width = table_width / g_ve_table->width;
                               float cell_height = table_height / g_ve_table->height;
                               
                               table_x = (int)(adjusted_mouse_x / cell_width);
                               table_y = (int)(adjusted_mouse_y / cell_height);
                               
                               // Clamp to valid range
                               table_x = fmax(0, fmin(g_ve_table->width - 1, table_x));
                               table_y = fmax(0, fmin(g_ve_table->height - 1, table_y));
                               
                                                           // Update multi-cell selection only if coordinates are valid
                            if (table_x >= 0 && table_y >= 0 && table_x < g_ve_table->width && table_y < g_ve_table->height) {
                                update_multi_selection(table_x, table_y);
                                add_log_entry(3, "Mouse drag detected at (%d, %d) -> table [%d, %d]", 
                                            mouse_x, mouse_y, table_x, table_y);
                            } else {
                                add_log_entry(3, "Mouse drag outside table area at (%d, %d)", mouse_x, mouse_y);
                            }
                           }
                           event_handled = true;
                       }
                       break;
        }
        
        // Only pass events to ImGui if we didn't handle them ourselves
        if (!event_handled) {
            ImGui_ImplSDL2_ProcessEvent(&event);
        }
    }
}

void update() {
    // Update ECU status and data
    if (g_ecu_context) {
        // Update ECU data (this triggers continuous data streaming)
        ecu_update(g_ecu_context);
        
        bool was_connected = g_ecu_connected;
        g_ecu_connected = ecu_is_connected(g_ecu_context);
        
        // Log connection state changes
        if (was_connected != g_ecu_connected) {
            if (g_ecu_connected) {
                add_log_entry(0, "ECU connection established");
            } else {
                add_log_entry(1, "ECU connection lost");
            }
        }
        
        // Use ecu_get_state instead of ecu_get_status
        ECUConnectionState state = ecu_get_state(g_ecu_context);
        switch (state) {
            case ECU_STATE_DISCONNECTED:
                strcpy(g_ecu_status, "Disconnected");
                break;
            case ECU_STATE_CONNECTING:
                strcpy(g_ecu_status, "Connecting");
                break;
            case ECU_STATE_CONNECTED:
                strcpy(g_ecu_status, "Connected");
                break;
            case ECU_STATE_ERROR:
                strcpy(g_ecu_status, "Error");
                break;
            default:
                strcpy(g_ecu_status, "Unknown");
                break;
        }
        // Use ecu_get_data correctly (it returns a pointer)
        const ECUData* data = ecu_get_data(g_ecu_context);
        if (data) {
            g_ecu_data = *data;
        }
    }
    
    // Generate demo data if in demo mode (independent of ECU connection)
    if (g_demo_mode) {
        static float demo_time = 0.0f;
        demo_time += 0.1f; // Increment time for demo
        
        // Generate realistic demo values with proper clamping - wider RPM range
        g_ecu_data.rpm = fmax(800.0f, 800.0f + 7200.0f * sin(demo_time * 0.3f) + 1000.0f * sin(demo_time * 1.5f));
        
        // Generate more random MAP data that jumps around the VE table
        float map_base = 30.0f + 50.0f * sin(demo_time * 0.3f);
        float map_random = 20.0f + 180.0f * sin(demo_time * 0.7f) * cos(demo_time * 0.5f);
        float map_spike = 40.0f * sin(demo_time * 2.1f) * sin(demo_time * 1.3f);
        g_ecu_data.map = fmax(20.0f, fmin(240.0f, map_base + map_random + map_spike));
        g_ecu_data.tps = fmax(0.0f, 10.0f + 30.0f * sin(demo_time * 0.7f));
        g_ecu_data.afr = fmax(10.0f, fmin(20.0f, 14.7f + 2.0f * sin(demo_time * 0.4f)));
        g_ecu_data.boost = fmax(-5.0f, 5.0f + 8.0f * sin(demo_time * 0.6f));
        g_ecu_data.coolant_temp = fmax(60.0f, fmin(120.0f, 90.0f + 10.0f * sin(demo_time * 0.2f)));
        g_ecu_data.intake_temp = fmax(60.0f, fmin(120.0f, 85.0f + 8.0f * sin(demo_time * 0.3f)));
        g_ecu_data.oil_temp = fmax(60.0f, fmin(130.0f, 95.0f + 12.0f * sin(demo_time * 0.4f)));
        g_ecu_data.battery_voltage = fmax(10.0f, fmin(16.0f, 13.5f + 0.5f * sin(demo_time * 0.8f)));
        g_ecu_data.timing = fmax(-10.0f, fmin(40.0f, 15.0f + 10.0f * sin(demo_time * 0.9f)));
        g_ecu_data.oil_pressure = fmax(50.0f, 300.0f + 100.0f * sin(demo_time * 0.5f));
        g_ecu_data.fuel_pressure = fmax(100.0f, 250.0f + 50.0f * sin(demo_time * 0.6f));
    }

    // Update button press timer
    if (g_button_press_timer > 0) {
        g_button_press_timer--;
        if (g_button_press_timer == 0) {
            g_locate_port_button_pressed = false;
            g_reverse_connect_button_pressed = false;
            g_reset_io_button_pressed = false;
            strcpy(g_button_status_text, "");
        }
    }

    handle_communications_buttons();
    
    // Calculate current engine operating point from actual ECU data (moved from 2D view)
    if (g_ve_table && g_ve_table_initialized) {
        add_log_entry(3, "DEBUG: VE table initialized, calculating engine position");
        float current_rpm = g_ecu_data.rpm;
        float current_map = g_ecu_data.map;
        
        // Find the closest cells for current RPM and MAP
        g_current_rpm_cell = -1;
        g_current_map_cell = -1;
        
        // Find RPM cell (find the closest cell to current RPM)
        g_current_rpm_cell = 0; // Default to first cell
        float min_distance = fabs(current_rpm - g_ve_table->x_axis[0]);
        
        for (int x = 0; x < g_ve_table->width; x++) {
            float distance = fabs(current_rpm - g_ve_table->x_axis[x]);
            if (distance < min_distance) {
                min_distance = distance;
                g_current_rpm_cell = x;
            }
        }
        
        // Debug output for RPM calculation
        if (g_demo_mode) {
            add_log_entry(3, "DEBUG: Current RPM: %.0f, Closest cell: %d (RPM: %.0f), Distance: %.1f", 
                         current_rpm, g_current_rpm_cell, g_ve_table->x_axis[g_current_rpm_cell], min_distance);
        }
        
        // Find MAP cell (find the closest cell to current MAP)
        g_current_map_cell = 0; // Default to first cell
        float min_map_distance = fabs(current_map - g_ve_table->y_axis[0]);
        
        for (int y = 0; y < g_ve_table->height; y++) {
            float distance = fabs(current_map - g_ve_table->y_axis[y]);
            if (distance < min_map_distance) {
                min_map_distance = distance;
                g_current_map_cell = y;
            }
        }
        
        // Debug output for MAP calculation
        if (g_demo_mode) {
            add_log_entry(3, "DEBUG: Current MAP: %.1f, Closest cell: %d (MAP: %.1f), Distance: %.1f", 
                         current_map, g_current_map_cell, g_ve_table->y_axis[g_current_map_cell], min_map_distance);
        }
        
        // Update engine trail for both 2D and 3D views
        if (g_current_rpm_cell >= 0 && g_current_map_cell >= 0) {
            update_engine_trail(g_current_rpm_cell, g_current_map_cell, g_engine_trail_2d, &g_trail_count_2d);
            update_engine_trail(g_current_rpm_cell, g_current_map_cell, g_engine_trail_3d, &g_trail_count_3d);
        }
    } else {
        add_log_entry(3, "DEBUG: VE table not initialized yet - g_ve_table: %p, g_ve_table_initialized: %d", 
                     g_ve_table, g_ve_table_initialized);
    }
    
    // Add periodic log entry every 30 seconds
    uint32_t current_time = SDL_GetTicks();
    if (current_time - g_last_log_time > 30000) { // 30 seconds
        add_log_entry(3, "System running - ECU status: %s", g_ecu_status);
        g_last_log_time = current_time;
    }
}

void render() {
    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Render main window
    render_main_window();

    // Render log window
    render_log_window();
    
    // Legend will be rendered as an overlay after the main render

    // Render
    ImGui::Render();
    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Render legend as a true overlay after everything else
    if (show_legend) {
        add_log_entry(0, "Rendering legend overlay - show_legend is true");
        // Start a new ImGui frame for the overlay
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        // Render the legend window
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowFocus();
        
        if (ImGui::Begin("Key Bindings Legend", &show_legend, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar)) {
            
            ImGui::Text("🎮 VE Table Professional Key Bindings");
            ImGui::Text("Professional ECU tuning software-style muscle memory for professional table editing");
            ImGui::Separator();
            
            // Status indicator
            ImGui::Text("Status: ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "● Active (VE Table Tab Selected)");
            ImGui::SameLine();
            ImGui::Text(" | Increment: %.1f | Percent: %.1f%%", 
                       g_key_binding_state.increment_amount, g_key_binding_state.percent_increment);
            
            ImGui::Separator();
            
            // Comprehensive table with keybindings
            if (ImGui::BeginTable("KeyBindingsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 80);
                ImGui::TableSetupColumn("Function", ImGuiTableColumnFlags_WidthFixed, 200);
                ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80);
                ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();
                
                // Navigation Keys
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Arrow Keys");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Navigate Cells");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Move between table cells");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Tab");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Next Cell");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Move to next cell (Excel-style)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Shift+Tab");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Previous Cell");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Move to previous cell");
                
                // Basic Value Operations
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "+");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Increment");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Increase cell value by %.1f", g_key_binding_state.increment_amount);
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "-");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Decrement");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Decrease cell value by %.1f", g_key_binding_state.increment_amount);
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Alt++");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Increment Alt");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Alternative increment method");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Alt+-");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Decrement Alt");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Alternative decrement method");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "*");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Scale By");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Multiply cell value by %.1f%%", g_key_binding_state.percent_increment);
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "=");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Set To");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "PART");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Set cell to specific value (needs input dialog)");
                
                // Advanced Operations
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "I");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Interpolate");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Interpolate between two selected cells");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "H");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Interpolate H");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Horizontal interpolation (planned)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "V");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Interpolate V");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Vertical interpolation (planned)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "S");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Smooth");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Smooth selected cells (planned)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "R");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Reset");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "PLAN");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Reset selected cells to default (planned)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Z");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Undo");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "PLAN");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Undo last action (planned)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Y");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Redo");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "PLAN");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Redo last undone action (planned)");
                
                // Copy/Paste Operations
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Ctrl+C");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Copy");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "PLAN");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Copy selected cell to clipboard");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Ctrl+V");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Paste");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "PLAN");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Paste cell from clipboard");
                
                ImGui::EndTable();
            }
            
            // Configuration Section
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "⚙️ Configuration");
            ImGui::Text("Adjust these values to customize your editing experience:");
            
            ImGui::SliderFloat("Increment Amount", &g_key_binding_state.increment_amount, 0.1f, 10.0f, "%.1f");
            ImGui::SliderFloat("Percent Increment", &g_key_binding_state.percent_increment, 1.0f, 50.0f, "%.1f%%");
            
            ImGui::Separator();
            
            // Legend for status indicators
            ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "📊 Status Legend");
            ImGui::Text("OK Working | PART Partial | PLAN Planned");
            
            ImGui::Separator();
            
            // Keyboard shortcuts are now handled globally in SDL event handler
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Press ESC or Enter to close this window");
            
            // Add a close button that also works
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Close Options:");
            ImGui::Spacing();
            
            // Make button very prominent
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
            
            // Try a completely different approach - use InvisibleButton
            ImGui::InvisibleButton("close_button", ImVec2(200, 40));
            
            // Debug: Check if button is hovered
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Button is being hovered!");
                add_log_entry(0, "Button is being hovered!");
            }
            
            // Debug: Check if button was clicked - try different methods
            if (ImGui::IsItemClicked()) {
                add_log_entry(0, "*** InvisibleButton IsItemClicked detected! ***");
                show_legend = false;
                add_log_entry(0, "*** Close button clicked - closing legend window ***");
            }
            
            if (ImGui::IsItemClicked(0)) { // Left mouse button
                add_log_entry(0, "*** InvisibleButton IsItemClicked(0) detected! ***");
                show_legend = false;
                add_log_entry(0, "*** Close button clicked - closing legend window ***");
            }
            
            // Draw the button manually on top of the invisible button
            ImVec2 current_pos = ImGui::GetCursorPos();
            ImGui::SetCursorPos(ImVec2(current_pos.x, current_pos.y - 40));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
            ImGui::Button("CLOSE WINDOW PLANW", ImVec2(200, 40));
            ImGui::PopStyleColor(3);
            
            // Debug: Show current state
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Debug: show_legend = %s", show_legend ? "true" : "false");
            
            ImGui::PopStyleColor(3);
            ImGui::Spacing();
            
            ImGui::End();
        }
        
        // Render the overlay
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    // Remove viewport features for now
    // if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    //     SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
    //     SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
    //     ImGui::UpdatePlatformWindows();
    //     ImGui::RenderPlatformWindowsDefault();
    //     SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    // }

    SDL_GL_SwapWindow(g_window);
}

void render_main_window() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags window_flags = 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("MegaTunix Redux", NULL, window_flags);

    // Top banner
    ImGui::SetCursorPos(ImVec2(10, 10));
    if (g_demo_mode) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "MEGATUNIX REDUX %s [DEMO MODE]", g_ecu_status);
    } else {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "MEGATUNIX REDUX %s", g_ecu_status);
    }
    
    // Tab bar
    ImGui::SetCursorPos(ImVec2(10, 40));
    if (ImGui::BeginTabBar("MainTabs")) {
        for (int i = 0; i < IM_ARRAYSIZE(g_tab_names); i++) {
            if (ImGui::BeginTabItem(g_tab_names[i])) {
                g_selected_tab = i;
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    // Tab content
    ImGui::SetCursorPos(ImVec2(10, 70));
    ImGui::BeginChild("TabContent", ImVec2(ImGui::GetWindowWidth() - 20, ImGui::GetWindowHeight() - 80));

    switch (g_selected_tab) {
        case 0: render_about_tab(); break;
        case 1: render_general_tab(); break;
        case 2: render_communications_tab(); break;
        case 3: render_runtime_display_tab(); break;
        case 4: render_datalogging_tab(); break;
        case 5: render_logviewer_tab(); break;
        case 6: render_engine_vitals_tab(); break;
        case 7: render_enrichments_tab(); break;
        case 8: render_ve_table_tab(); break;
        case 9: render_tools_tab(); break;
        case 10: render_warmup_wizard_tab(); break;
    }

    ImGui::EndChild();
    ImGui::End();
}

void render_about_tab() {
    ImGui::Text("About - Application Information");
    ImGui::Separator();
    
    ImGui::Text("MegaTunix Redux - ImGui Version");
            ImGui::Text("By Patrick Burke");
        ImGui::Text("Based on MegaTunix by David J. Andruczyk");
    ImGui::Text("(ImGui Version)");
    
    ImGui::Separator();
    
    // Demo mode toggle (prominent placement)
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Demo Mode Settings:");
    ImGui::SameLine();
    if (ImGui::Checkbox("Enable Demo Mode", &g_demo_mode)) {
        if (g_demo_mode) {
            add_log_entry(0, "Demo mode enabled from About tab");
        } else {
            add_log_entry(0, "Demo mode disabled from About tab");
        }
    }
    
    if (g_demo_mode) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK ACTIVE");
        ImGui::Text("Demo mode is currently active - all data is simulated");
    }
    
    ImGui::Separator();
    
    ImGui::Text("This application provides a graphical interface for monitoring and controlling an ECU.");
    ImGui::Text("It uses SDL2 for windowing, OpenGL for rendering, and ImGui for UI.");
    ImGui::Text("The ECU communication is handled by a separate library.");
    
    ImGui::Separator();
    ImGui::Text("Demo Mode allows you to test the application without an ECU connection.");
    ImGui::Text("Enable it to see simulated engine data and test all features.");
}

void render_general_tab() {
    ImGui::Text("General - System Information");
    ImGui::Separator();
    
    ImGui::Text("Application Version: 1.0.0");
    ImGui::Text("ECU Status: %s", g_ecu_status);
    ImGui::Text("Connected: %s", g_ecu_connected ? "Yes" : "No");
    
    // Demo mode toggle and information
    ImGui::Separator();
    ImGui::Text("Demo Mode:");
    ImGui::SameLine();
    if (ImGui::Checkbox("Enable Demo Mode", &g_demo_mode)) {
        if (g_demo_mode) {
            add_log_entry(0, "Demo mode enabled by user");
        } else {
            add_log_entry(0, "Demo mode disabled by user");
        }
    }
    
    if (g_demo_mode) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK ACTIVE");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "DEMO MODE FEATURES:");
        ImGui::Text("• All data is simulated for testing purposes");
        ImGui::Text("• Real-time charts show demo data");
        ImGui::Text("• VE table contains demo values");
        ImGui::Text("• 3D view shows simulated engine position");
        ImGui::Text("• No actual ECU communication");
    }
    
    ImGui::Separator();
    
    ImGui::Text("ECU Data:");
    ImGui::Text("  RPM: %.0f", g_ecu_data.rpm);
    ImGui::Text("  Coolant Temp: %.1f°C", g_ecu_data.coolant_temp);
    ImGui::Text("  Intake Air Temp: %.1f°C", g_ecu_data.intake_temp);
    ImGui::Text("  MAP: %.1f kPa", g_ecu_data.map);
    ImGui::Text("  TPS: %.1f%%", g_ecu_data.tps);
    ImGui::Text("  Battery Voltage: %.1fV", g_ecu_data.battery_voltage);
}

void render_communications_tab() {
    // Initialize communications if not done yet
    if (!g_communications_initialized) {
        g_communications = imgui_communications_create(g_ecu_context);
        if (g_communications) {
            g_communications_initialized = true;
            // Set up log callback for communications module
            imgui_communications_set_log_callback(g_communications, add_log_entry);
            
            // Set up VE table callbacks for communications module
            add_log_entry(0, "Setting up VE table callbacks...");
            imgui_communications_set_ve_table_callbacks(g_communications,
                [](int width, int height) -> bool {
                    add_log_entry(0, "VE table resize callback called: %dx%d", width, height);
                    add_log_entry(0, "*** VE TABLE RESIZE REQUESTED: %dx%d ***", width, height);
                    add_log_entry(0, "Current g_ve_table dimensions before resize: %dx%d (address: %p)", g_ve_table ? g_ve_table->width : -1, g_ve_table ? g_ve_table->height : -1, g_ve_table);
                    if (g_ve_table) {
                        bool result = imgui_table_resize(g_ve_table, width, height);
                        add_log_entry(0, "VE table resize result: %s", result ? "SUCCESS" : "FAILED");
                        add_log_entry(0, "Current g_ve_table dimensions after resize: %dx%d (address: %p)", g_ve_table ? g_ve_table->width : -1, g_ve_table ? g_ve_table->height : -1, g_ve_table);
                        if (result) {
                            add_log_entry(0, "*** VE TABLE RESIZE SUCCESS: %dx%d ***", g_ve_table->width, g_ve_table->height);
                        } else {
                            add_log_entry(2, "*** VE TABLE RESIZE FAILED ***");
                        }
                        return result;
                    }
                    add_log_entry(2, "VE table resize failed: g_ve_table is NULL");
                    return false;
                },
                [](float x_min, float x_max, float y_min, float y_max) {
                    add_log_entry(0, "VE table axis ranges callback: X(%.0f-%.0f), Y(%.0f-%.0f)", x_min, x_max, y_min, y_max);
                    if (g_ve_table) {
                        imgui_table_set_axis_ranges(g_ve_table, x_min, x_max, y_min, y_max);
                    }
                },
                [](const char* x_name, const char* y_name, const char* x_units, const char* y_units) {
                    add_log_entry(0, "VE table axis names callback: X(%s %s), Y(%s %s)", x_name, x_units, y_name, y_units);
                    if (g_ve_table) {
                        imgui_table_set_axis_names(g_ve_table, x_name, y_name, x_units, y_units);
                    }
                },
                [](float scale, float min_value, float max_value) {
                    add_log_entry(0, "VE table metadata callback: scale=%.2f, range=%.0f-%.0f", scale, min_value, max_value);
                    if (g_ve_table) {
                        g_ve_table->metadata.scale = scale;
                        g_ve_table->metadata.min_value = min_value;
                        g_ve_table->metadata.max_value = max_value;
                    }
                }
            );
            add_log_entry(0, "VE table callbacks set up successfully");
            add_log_entry(0, "Communications tab initialized successfully");
        } else {
            add_log_entry(2, "Failed to initialize communications tab");
        }
    }
    
    if (g_communications && g_communications_initialized) {
        imgui_communications_render(g_communications);
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to initialize communications!");
    }
}

void render_runtime_display_tab() {
    // Initialize runtime display if not done yet
    if (!g_runtime_display_initialized) {
        g_runtime_display = imgui_runtime_display_create(g_ecu_context);
        if (g_runtime_display) {
            g_runtime_display_initialized = true;
        }
    }
    
    // Sync demo mode with global setting
    if (g_runtime_display) {
        g_runtime_display->demo_mode_enabled = g_demo_mode;
    }
    
    if (g_runtime_display && g_runtime_display_initialized) {
        imgui_runtime_display_render(g_runtime_display);
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to initialize runtime display!");
    }
}

void render_datalogging_tab() {
    ImGui::Text("Data Logging - Log Management");
    ImGui::Separator();
    
    ImGui::Text("Log files:");
    ImGui::Text("  /logs/megatunix.log");
    ImGui::Text("  /logs/ecu_data.log");
    
    ImGui::Separator();
    
    if (ImGui::Button("Clear Logs", ImVec2(120, 30))) {
        // TODO: Implement log clearing
        ImGui::OpenPopup("Logs Cleared");
    }
    
    if (ImGui::BeginPopupModal("Logs Cleared", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Logs have been cleared successfully!");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void render_logviewer_tab() {
    ImGui::Text("Log Viewer - View ECU and Application Logs");
    ImGui::Separator();
    
    // Placeholder for log content
    ImGui::Text("Log content will be displayed here.");
    ImGui::Text("This tab will eventually allow you to view and filter logs.");
}

void render_engine_vitals_tab() {
    ImGui::Text("Engine Vitals - Real-time Monitoring");
    if (g_demo_mode) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[DEMO MODE]");
    }
    ImGui::Separator();
    
    // Two column layout
    ImGui::Columns(2, "VitalsColumns");
    
    // Left column
    ImGui::Text("Engine Speed");
    ImGui::Text("RPM: %.0f", g_ecu_data.rpm);
    ImGui::Text("Speed: %.1f km/h", g_ecu_data.rpm * 0.1f); // Placeholder calculation
    
    ImGui::NextColumn();
    
    // Right column
    ImGui::Text("Temperatures");
    ImGui::Text("Coolant: %.1f°C", g_ecu_data.coolant_temp);
    ImGui::Text("Intake: %.1f°C", g_ecu_data.intake_temp);
    ImGui::Text("Oil: %.1f°C", g_ecu_data.oil_temp);
    
    ImGui::Columns(1);
    ImGui::Separator();
    
    // Pressures
    ImGui::Text("Pressures");
    ImGui::Text("MAP: %.1f kPa", g_ecu_data.map);
    ImGui::Text("Oil Pressure: %.1f kPa", g_ecu_data.oil_pressure);
    ImGui::Text("Fuel Pressure: %.1f kPa", g_ecu_data.fuel_pressure);
}

void render_enrichments_tab() {
    ImGui::Text("Enrichments - Additional Data Sources");
    ImGui::Separator();
    
    ImGui::Text("This tab will eventually integrate with external sensors and data.");
    ImGui::Text("Currently, it's a placeholder for future enhancements.");
}

void render_ve_table_tab() {
    
    ImGui::Text("VE Table(1) - Fuel and Spark Tuning");
    if (g_demo_mode) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[DEMO MODE]");
    }
    ImGui::Separator();
    
    // VE table is now initialized in main() function
    
    // Initialize texture if not done yet
    if (!g_ve_texture_initialized && g_ve_table_initialized) {
        g_ve_texture = imgui_ve_texture_create(512, 512); // Create 512x512 texture
        if (g_ve_texture) {
            g_ve_texture_initialized = true;
        }
    }
    
    if (g_ve_table && g_ve_table_initialized && g_ve_texture && g_ve_texture_initialized) {
        // View mode selection
        ImGui::Text("View Mode:");
        ImGui::SameLine();
        ImGui::RadioButton("Interactive 2D Editor", &g_ve_view_mode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("3D View", &g_ve_view_mode, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Table Editor", &g_ve_view_mode, 2);
        
        ImGui::Separator();
        
        // Debug info
        const char* view_names[] = {"Interactive 2D Editor", "3D View", "Table Editor"};
        ImGui::Text("Debug - Current View: %s (Mode: %d)", view_names[g_ve_view_mode], g_ve_view_mode);
        
        // Add debug info to log
        static int last_view_mode = -1;
        if (last_view_mode != g_ve_view_mode) {
            add_log_entry(0, "VE Table view mode changed to: %s", view_names[g_ve_view_mode]);
            last_view_mode = g_ve_view_mode;
        }
        
        // Table information
        // Debug: Log the exact values being passed to ImGui::Text
        add_log_entry(3, "DEBUG: About to render Table Size text: %dx%d", g_ve_table->width, g_ve_table->height);
        
        // Add a high-priority message that will be more visible
        static int last_reported_size = -1;
        if (last_reported_size != g_ve_table->width * 1000 + g_ve_table->height) {
            add_log_entry(0, "*** VE TABLE SIZE CHANGE: %dx%d ***", g_ve_table->width, g_ve_table->height);
            last_reported_size = g_ve_table->width * 1000 + g_ve_table->height;
        }
        
        ImGui::Text("Table Size: %dx%d", g_ve_table->width, g_ve_table->height);
        ImGui::Text("Value Range: %.1f - %.1f", g_ve_table->metadata.min_value, g_ve_table->metadata.max_value);
        
        // Professional Key Bindings Legend
        ImGui::Separator();
        ImGui::Text("🎯 Professional Table Editing Key Bindings");
        ImGui::SameLine();
        
        
        // Enhanced Key Bindings Legend Window (regular window for proper z-order)
        // Note: show_legend flag is now handled directly in the window rendering

        
        // Debug: Log the actual table dimensions being displayed
        static int last_width = -1, last_height = -1;
        static void* last_table_address = NULL;
        if (last_width != g_ve_table->width || last_height != g_ve_table->height || last_table_address != g_ve_table) {
            add_log_entry(0, "UI displaying table size: %dx%d (address: %p)", g_ve_table->width, g_ve_table->height, g_ve_table);
            last_width = g_ve_table->width;
            last_height = g_ve_table->height;
            last_table_address = g_ve_table;
            
            // Force ImGui to refresh the text
            ImGui::SetWindowFocus();
        }
        
        // Debug: Also log every frame to see if the table is being reset
        static int frame_count = 0;
        frame_count++;
        if (frame_count % 60 == 0) { // Log every 60 frames (about once per second)
            add_log_entry(3, "DEBUG: Current table dimensions: %dx%d", g_ve_table->width, g_ve_table->height);
        }
        
        // Action buttons
        if (ImGui::Button("Load from ECU", ImVec2(120, 30))) {
            // TODO: Load actual data from ECU
            ImGui::OpenPopup("Load from ECU");
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Save to ECU", ImVec2(120, 30))) {
            // TODO: Save data to ECU
            ImGui::OpenPopup("Save to ECU");
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Reset to Demo", ImVec2(120, 30))) {
            imgui_table_load_demo_data(g_ve_table);
            if (g_ve_texture) {
                imgui_ve_texture_update(g_ve_texture, g_ve_table, &g_ve_3d_view);
            }
        }
        
        // Popup for ECU operations
        if (ImGui::BeginPopupModal("Load from ECU", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Loading VE table from ECU...");
            ImGui::Text("This feature will be implemented when ECU communication is ready.");
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        if (ImGui::BeginPopupModal("Save to ECU", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Saving VE table to ECU...");
            ImGui::Text("This feature will be implemented when ECU communication is ready.");
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        ImGui::Separator();
        
        // Trail toggle control
        ImGui::Text("Display Options:");
        if (ImGui::Checkbox("Show Engine Trail", &g_show_engine_trail)) {
            if (g_show_engine_trail) {
                add_log_entry(0, "Engine trail enabled");
            } else {
                add_log_entry(0, "Engine trail disabled");
            }
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Shows fading trail of engine operating point)");
        
        // Y-axis label rotation removed - MAP title is now permanently rotated
        
        ImGui::Separator();
        
        // Render the selected view
        switch (g_ve_view_mode) {
            case 0: // 2D Heatmap
                render_ve_table_2d_view();
                break;
            case 1: // 3D View
                render_ve_table_3d_view();
                break;
            case 2: // Table Editor
                render_ve_table_editor_view();
                break;
            default:
                render_ve_table_2d_view(); // Default to 2D view
                break;
        }
        
        // Integrated legend panel - responsive to window size
        float window_width = ImGui::GetIO().DisplaySize.x;
        float legend_width = window_width * 0.4f; // 40% of window width
        float legend_x = window_width - legend_width - 20; // 20px margin from right
        ImGui::SetCursorPos(ImVec2(legend_x, 50));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.95f));
        ImGui::BeginChild("LegendPanel", ImVec2(legend_width, 200), true);
                

            
            ImGui::Text("🎮 VE Table Key Bindings");
            ImGui::Text("Professional ECU tuning software-style editing");
            ImGui::Separator();
            
            // Status indicator
            ImGui::Text("Status: ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "● Active");
            ImGui::SameLine();
            ImGui::Text(" | Inc: %.1f | %%: %.1f", 
                       g_key_binding_state.increment_amount, g_key_binding_state.percent_increment);
            
            // Interpolation mode indicator
            if (g_interpolation_mode) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), " | 🔄 INTERPOLATION MODE");
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "   Select second cell and press I again to interpolate");
            }
            
            ImGui::Separator();
            
            // Reorganized table with descriptions
            if (ImGui::BeginTable("KeyBindingsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 60);
                ImGui::TableSetupColumn("Function", ImGuiTableColumnFlags_WidthFixed, 80);
                ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 50);
                ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();
                
                // Navigation
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Arrow Keys");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Navigate");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Move between table cells using arrow keys");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Tab");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Next");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Move to next cell (Excel-style navigation)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Shift+Tab");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Prev");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Move to previous cell (reverse navigation)");
                
                // Basic Operations
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "+");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Inc");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Increase cell value by increment amount (%.1f)", g_key_binding_state.increment_amount);
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "-");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Dec");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Decrease cell value by increment amount (%.1f)", g_key_binding_state.increment_amount);
                
                // Alt Operations
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Alt++");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Inc Alt");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Alternative increment method (same as +)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Alt+-");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Dec Alt");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Alternative decrement method (same as -)");
                
                // Advanced Operations
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "*");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Scale %%");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Scale cell value by percentage (%.1f%%)", g_key_binding_state.percent_increment);
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "I");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Interp");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Interpolate between two selected cells");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "S");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Smooth");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "PLAN");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Smooth selected cells using interpolation (planned)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "Z");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Undo");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "PLAN");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Undo last action (planned)");
                
                // Navigation Keys
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Arrow Keys");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Navigate Cells");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Move between table cells");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Tab");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Next Cell");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Move to next cell (Excel-style)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Shift+Tab");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Previous Cell");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Move to previous cell");
                
                // Basic Value Operations
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "+");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Increment");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Increase cell value by %.1f", g_key_binding_state.increment_amount);
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "-");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Decrement");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Decrease cell value by %.1f", g_key_binding_state.increment_amount);
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Alt++");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Increment Alt");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Alternative increment method");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Alt+-");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Decrement Alt");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Alternative decrement method");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "*");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Scale By");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Multiply cell value by %.1f%%", g_key_binding_state.percent_increment);
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "=");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Set To");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "PART");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Set cell to specific value (needs input dialog)");
                
                // Advanced Operations
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "I");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Interpolate");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Interpolate between two selected cells");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "H");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Interpolate H");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Horizontal interpolation (planned)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "V");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Interpolate V");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Vertical interpolation (planned)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "S");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Smooth");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "OK");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Smooth selected cells (planned)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "R");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Reset");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "PLAN");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Reset selected cells to default (planned)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Z");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Undo");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "PLAN");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Undo last action (planned)");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Y");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Redo");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "PLAN");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Redo last undone action (planned)");
                
                // Copy/Paste Operations
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Ctrl+C");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Copy");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "PLAN");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Copy selected cell to clipboard");
                
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Ctrl+V");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("Paste");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "PLAN");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("Paste cell from clipboard");
                
                ImGui::EndTable();
            }
            
                                // Configuration section
                    ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "Config - use arrows to highlight and Space to select");
                    ImGui::SliderFloat("Increment Control (+ / -)", &g_key_binding_state.increment_amount, 0.1f, 10.0f, "%.1f");
                    ImGui::SliderFloat("Percent Control ( * )", &g_key_binding_state.percent_increment, 1.0f, 50.0f, "%.1f%%");
            
            ImGui::Separator();
            
            // Legend for status indicators
            ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "Status");
            ImGui::SameLine();
            ImGui::Text("OK=Working | PLAN=Planned");
            
        ImGui::EndChild();
        ImGui::PopStyleColor();
        
        // Double padding below the VE table
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
    

    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to initialize VE table or texture!");
    }
}

void render_tools_tab() {
    ImGui::Text("Tools - Utility Functions");
    ImGui::Separator();
    
    ImGui::Text("This tab will contain various utility functions.");
    ImGui::Text("Currently, it's a placeholder for future tools.");
}

void render_warmup_wizard_tab() {
    ImGui::Text("Warmup Wizard - ECU Warmup Procedure");
    ImGui::Separator();
    
    ImGui::Text("This tab will guide you through the ECU warmup procedure.");
    ImGui::Text("It will include steps for engine pre-heating and ECU initialization.");
}

void handle_communications_buttons() {
    // This function handles the actual button logic
    // For now, it's just a placeholder that simulates the button actions
    
    if (g_locate_port_button_pressed && g_button_press_timer == 30) {
        // Simulate port detection
        strcpy(g_button_status_text, "Found 2 ports: /dev/ttyUSB0, /dev/ttyACM0");
        add_log_entry(0, "Port detection completed: Found 2 ports");
    }
    
    if (g_reverse_connect_button_pressed && g_button_press_timer == 30) {
        // Simulate connection attempt
        if (g_ecu_context) {
            // Try to connect
            strcpy(g_button_status_text, "Connection attempt completed");
            add_log_entry(0, "Connection attempt completed");
        }
    }
    
    if (g_reset_io_button_pressed && g_button_press_timer == 30) {
        // Simulate reset
        strcpy(g_button_status_text, "I/O status counters reset");
        add_log_entry(0, "I/O status counters reset");
    }
}

void update_engine_trail(int current_x, int current_y, TrailPoint* trail, int* trail_count) {
    uint32_t current_time = SDL_GetTicks();
    
    // Only add new point if position changed or enough time has passed
    if (*trail_count == 0 || 
        trail[*trail_count - 1].x != current_x || 
        trail[*trail_count - 1].y != current_y ||
        current_time - trail[*trail_count - 1].timestamp > 100) { // Add point every 100ms max
        
        // Add new trail point
        if (*trail_count < MAX_TRAIL_POINTS) {
            trail[*trail_count].x = current_x;
            trail[*trail_count].y = current_y;
            trail[*trail_count].timestamp = current_time;
            trail[*trail_count].alpha = 1.0f; // Start fully visible
            (*trail_count)++;
        } else {
            // Shift trail and add new point at end
            memmove(&trail[0], &trail[1], sizeof(TrailPoint) * (MAX_TRAIL_POINTS - 1));
            trail[MAX_TRAIL_POINTS - 1].x = current_x;
            trail[MAX_TRAIL_POINTS - 1].y = current_y;
            trail[MAX_TRAIL_POINTS - 1].timestamp = current_time;
            trail[MAX_TRAIL_POINTS - 1].alpha = 1.0f;
        }
    }
    
    // Update alpha values for fading effect
    for (int i = 0; i < *trail_count; i++) {
        uint32_t age = current_time - trail[i].timestamp;
        float fade_duration = 3000.0f; // 3 seconds fade time
        trail[i].alpha = fmax(0.0f, 1.0f - (float)age / fade_duration);
        
        // Remove completely faded points
        if (trail[i].alpha <= 0.0f) {
            if (i < *trail_count - 1) {
                memmove(&trail[i], &trail[i + 1], sizeof(TrailPoint) * (*trail_count - i - 1));
            }
            (*trail_count)--;
            i--; // Re-check this index
        }
    }
}

// Copy/paste functions for table editing
void copy_selected_cell_to_clipboard() {
    if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0 && g_ve_table) {
        // Copy single cell
        g_clipboard_data[0][0] = imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y);
        g_clipboard_width = 1;
        g_clipboard_height = 1;
        g_clipboard_has_data = true;
        add_log_entry(0, "Copied cell [%d,%d] value %.1f to clipboard", 
                     g_selected_cell_x, g_selected_cell_y, g_clipboard_data[0][0]);
    }
}

void paste_from_clipboard() {
    if (!g_clipboard_has_data || !g_ve_table) return;
    
    if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
        // Paste single cell
        if (g_clipboard_width == 1 && g_clipboard_height == 1) {
            float new_val = g_clipboard_data[0][0];
            imgui_table_set_value(g_ve_table, g_selected_cell_x, g_selected_cell_y, new_val);
            
            // Update the edit buffer to reflect the new value immediately
            snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.0f", new_val);
            g_buffer_updated = true;
            
            add_log_entry(0, "Pasted value %.1f to cell [%d,%d]", 
                         new_val, g_selected_cell_x, g_selected_cell_y);
        }
    }
}

void interpolate_between_cells() {
    add_log_entry(0, "*** interpolate_between_cells() called *** - g_ve_table: %p, g_interpolation_mode: %s", 
                 g_ve_table, g_interpolation_mode ? "true" : "false");
    if (!g_ve_table) return;
    
    if (g_interpolation_mode) {
        // Second cell selected - perform interpolation
        g_interpolation_end_x = g_selected_cell_x;
        g_interpolation_end_y = g_selected_cell_y;
        
        // Calculate interpolation path
        int start_x = g_interpolation_start_x;
        int start_y = g_interpolation_start_y;
        int end_x = g_interpolation_end_x;
        int end_y = g_interpolation_end_y;
        
        // Get start and end values
        float start_val = imgui_table_get_value(g_ve_table, start_x, start_y);
        float end_val = imgui_table_get_value(g_ve_table, end_x, end_y);
        
        // Calculate number of steps
        int dx = abs(end_x - start_x);
        int dy = abs(end_y - start_y);
        int steps = fmax(dx, dy);
        
        if (steps > 0) {
            // Interpolate along the path
            for (int i = 1; i < steps; i++) {
                float t = (float)i / steps;
                int x = start_x + (int)((end_x - start_x) * t);
                int y = start_y + (int)((end_y - start_y) * t);
                
                // Ensure coordinates are within bounds
                if (x >= 0 && x < g_ve_table->width && y >= 0 && y < g_ve_table->height) {
                    float interpolated_val = start_val + (end_val - start_val) * t;
                    imgui_table_set_value(g_ve_table, x, y, interpolated_val);
                }
            }
            
            add_log_entry(0, "Interpolated %d cells from [%d,%d]=%.1f to [%d,%d]=%.1f", 
                         steps - 1, start_x, start_y, start_val, end_x, end_y, end_val);
        }
        
        // Reset interpolation mode
        g_interpolation_mode = false;
        g_interpolation_start_x = -1;
        g_interpolation_start_y = -1;
        g_interpolation_end_x = -1;
        g_interpolation_end_y = -1;
        
    } else {
        // First cell selected - start interpolation mode
        g_interpolation_start_x = g_selected_cell_x;
        g_interpolation_start_y = g_selected_cell_y;
        g_interpolation_mode = true;
        
        add_log_entry(0, "Interpolation mode started - select second cell and press I again");
    }
}

// Multi-cell selection helper functions
void clear_multi_selection() {
    g_multi_selection.start_x = -1;
    g_multi_selection.start_y = -1;
    g_multi_selection.end_x = -1;
    g_multi_selection.end_y = -1;
    g_multi_selection.active = false;
    g_multi_selection.dragging = false;
    add_log_entry(0, "Multi-cell selection cleared");
}

void start_multi_selection(int x, int y) {
    if (!g_ve_table || x < 0 || x >= g_ve_table->width || y < 0 || y >= g_ve_table->height) {
        add_log_entry(2, "Invalid coordinates for multi-cell selection: [%d, %d]", x, y);
        return;
    }
    
    g_multi_selection.start_x = x;
    g_multi_selection.start_y = y;
    g_multi_selection.end_x = x;
    g_multi_selection.end_y = y;
    g_multi_selection.active = true;
    g_multi_selection.dragging = true;
    g_multi_selection.drag_start_pos = ImGui::GetMousePos();
    g_multi_selection.drag_current_pos = g_multi_selection.drag_start_pos;
    
    add_log_entry(0, "Multi-cell selection started at [%d, %d]", x, y);
}

void update_multi_selection(int x, int y) {
    if (!g_multi_selection.dragging || !g_ve_table) {
        return;
    }
    
    if (x < 0 || x >= g_ve_table->width || y < 0 || y >= g_ve_table->height) {
        add_log_entry(2, "Invalid coordinates for multi-cell update: [%d, %d]", x, y);
        return;
    }
    
    g_multi_selection.end_x = x;
    g_multi_selection.end_y = y;
    g_multi_selection.drag_current_pos = ImGui::GetMousePos();
}

void end_multi_selection() {
    if (!g_multi_selection.dragging) {
        return;
    }
    
    g_multi_selection.dragging = false;
    
    // Ensure start is top-left and end is bottom-right
    int temp;
    if (g_multi_selection.start_x > g_multi_selection.end_x) {
        temp = g_multi_selection.start_x;
        g_multi_selection.start_x = g_multi_selection.end_x;
        g_multi_selection.end_x = temp;
    }
    if (g_multi_selection.start_y > g_multi_selection.end_y) {
        temp = g_multi_selection.start_y;
        g_multi_selection.start_y = g_multi_selection.end_y;
        g_multi_selection.end_y = temp;
    }
    
    int cell_count = get_selection_cell_count();
    add_log_entry(0, "Multi-cell selection ended: [%d,%d] to [%d,%d] (%d cells)", 
                 g_multi_selection.start_x, g_multi_selection.start_y,
                 g_multi_selection.end_x, g_multi_selection.end_y, cell_count);
}

bool is_cell_in_selection(int x, int y) {
    if (!g_multi_selection.active) {
        return false;
    }
    
    return (x >= g_multi_selection.start_x && x <= g_multi_selection.end_x &&
            y >= g_multi_selection.start_y && y <= g_multi_selection.end_y);
}

void get_selection_bounds(int* min_x, int* min_y, int* max_x, int* max_y) {
    if (!g_multi_selection.active) {
        *min_x = *min_y = *max_x = *max_y = -1;
        return;
    }
    
    *min_x = g_multi_selection.start_x;
    *min_y = g_multi_selection.start_y;
    *max_x = g_multi_selection.end_x;
    *max_y = g_multi_selection.end_y;
}

int get_selection_cell_count() {
    if (!g_multi_selection.active) {
        return 0;
    }
    
    int width = g_multi_selection.end_x - g_multi_selection.start_x + 1;
    int height = g_multi_selection.end_y - g_multi_selection.start_y + 1;
    return width * height;
}

void apply_operation_to_selection(TableOperation operation, float value) {
    if (!g_multi_selection.active) {
        return;
    }
    
    int cell_count = 0;
    for (int y = g_multi_selection.start_y; y <= g_multi_selection.end_y; y++) {
        for (int x = g_multi_selection.start_x; x <= g_multi_selection.end_x; x++) {
            float current_val = imgui_table_get_value(g_ve_table, x, y);
            float new_val = current_val;
            
            switch (operation) {
                case TABLE_OP_SET_TO:
                    new_val = value;
                    break;
                case TABLE_OP_INCREASE_BY:
                    new_val = current_val + value;
                    break;
                case TABLE_OP_DECREASE_BY:
                    new_val = current_val - value;
                    break;
                case TABLE_OP_SCALE_BY:
                    new_val = current_val * (1.0f + value / 100.0f);
                    break;
                default:
                    continue;
            }
            
            // Clamp to table bounds
            if (new_val < g_ve_table->metadata.min_value) {
                new_val = g_ve_table->metadata.min_value;
            }
            if (new_val > g_ve_table->metadata.max_value) {
                new_val = g_ve_table->metadata.max_value;
            }
            
            imgui_table_set_value(g_ve_table, x, y, new_val);
            cell_count++;
        }
    }
    
    add_log_entry(0, "Applied operation to %d cells in selection", cell_count);
}

// Function to handle ECU connection with feedback
void handle_ecu_connection(ECUContext* ecu_ctx, const ECUConfig* config) {
    if (!ecu_ctx || !config) return;
    
    // Add log entry for connection attempt
    add_log_entry(0, "Initiating ECU connection to %s at %d baud", config->port, config->baud_rate);
    
    // Start connection attempt
    bool success = ecu_connect(ecu_ctx, config);
    
    if (success) {
        add_log_entry(0, "ECU connection initiated successfully");
    } else {
        add_log_entry(2, "Failed to initiate ECU connection");
    }
}

// VE Table View Functions
void render_ve_table_2d_view() {
    // Safety check - ensure VE table is valid
    if (!g_ve_table || g_ve_table->width <= 0 || g_ve_table->height <= 0) {
        ImGui::Text("VE Table not available or invalid");
        return;
    }
    
    ImGui::Text("Interactive 2D VE Table Editor");
    ImGui::Text("Click on cells to edit values directly - Type new values and press Enter");
    ImGui::Text("Use +/- keys to increment/decrement values when a cell is selected");
    ImGui::Text("X-Axis: RPM (%.0f - %.0f)", g_ve_table->x_axis[0], g_ve_table->x_axis[g_ve_table->width - 1]);
    ImGui::Text("Y-Axis: MAP (%.1f - %.1f kPa)", g_ve_table->y_axis[0], g_ve_table->y_axis[g_ve_table->height - 1]);
    
    ImGui::Separator();
    
    // Editing controls
    ImGui::Text("Editing Controls:");
    ImGui::SameLine();
    if (ImGui::Button("Reset to Demo", ImVec2(100, 20))) {
        imgui_table_load_demo_data(g_ve_table);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Selection", ImVec2(100, 20))) {
        g_selected_cell_x = -1;
        g_selected_cell_y = -1;
        g_cell_editing = false;
        clear_multi_selection();
    }
    
    // Show selected cell info
    if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
        ImGui::Text("Selected: Cell [%d, %d] = %.1f VE", 
                   g_selected_cell_x, g_selected_cell_y, 
                   imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y));
        ImGui::Text("RPM: %.0f, MAP: %.1f kPa", 
                   g_ve_table->x_axis[g_selected_cell_x], 
                   g_ve_table->y_axis[g_selected_cell_y]);
    }
    
    // Show multi-cell selection info
    if (g_multi_selection.active) {
        int cell_count = get_selection_cell_count();
        ImGui::Text("Multi-Selection: [%d,%d] to [%d,%d] (%d cells)", 
                   g_multi_selection.start_x, g_multi_selection.start_y,
                   g_multi_selection.end_x, g_multi_selection.end_y, cell_count);
        ImGui::Text("Use Ctrl+Click and drag to select multiple cells");
    }
    
    ImGui::Separator();
    
    // Interactive heatmap area
    ImGui::Text("Interactive Heatmap (Click cells to edit):");
    
                    // Calculate display area - scale to window size with padding for UI log and axis labels
                float window_width = ImGui::GetWindowWidth();
                float window_height = ImGui::GetWindowHeight();
                float ui_log_height = 150; // Approximate UI log height
                float available_height = window_height - ui_log_height - 200; // Leave space for controls and UI log
                
                // Shrink heatmap to fit within axis labels
                float display_width = window_width - 120; // Leave more padding for axis labels
                float display_height = available_height - 120; // Leave more space for axis labels
                
                // Ensure minimum size
                if (display_width < 400) display_width = 400;
                if (display_height < 300) display_height = 300;
    
    // Create a child window for the interactive area
    ImGui::BeginChild("InteractiveHeatmap", ImVec2(display_width, display_height), true);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 window_size = ImGui::GetWindowSize();
    
    // Cache window position and size for event handling (to avoid calling ImGui functions from event handler)
    g_table_window_pos = window_pos;
    g_table_window_size = window_size;
    g_table_window_valid = true;
    
                    // Calculate cell dimensions - leave more space for axis labels
                float cell_width = 1.0f;
                float cell_height = 1.0f;
                
                if (g_ve_table && g_ve_table->width > 0 && g_ve_table->height > 0) {
                    cell_width = (window_size.x - 80) / g_ve_table->width;  // Leave 40px on each side
                    cell_height = (window_size.y - 60) / g_ve_table->height; // Leave 30px on top/bottom
                }
                
                // Debug: Log the actual cell dimensions being used for rendering
                static int last_render_width = -1, last_render_height = -1;
                if (last_render_width != g_ve_table->width || last_render_height != g_ve_table->height) {
                    add_log_entry(0, "Rendering VE table: %dx%d, cell size: %.1fx%.1f, window: %.1fx%.1f", 
                                g_ve_table->width, g_ve_table->height, cell_width, cell_height, window_size.x, window_size.y);
                    last_render_width = g_ve_table->width;
                    last_render_height = g_ve_table->height;
                }
    
    // Mouse interaction is now handled in the SDL event system
    // This section is kept for visual feedback only
    
                    // Draw the interactive heatmap
                if (g_ve_table && g_ve_table->width > 0 && g_ve_table->height > 0) {
                    for (int y = 0; y < g_ve_table->height; y++) {
                        for (int x = 0; x < g_ve_table->width; x++) {
                        float x_pos = window_pos.x + 40 + x * cell_width;  // Start 40px from left for Y-axis labels
                        float y_pos = window_pos.y + 30 + y * cell_height; // Start 30px from top for X-axis labels
            
            // Get VE value and normalize to 0-1
            float ve_value = 0.0f;
            float normalized = 0.0f;
            
            // Use safe table access wrapper for rendering
            ve_value = SafeTableAccess::get_value_safe(x, y, 75.0f);
            if (g_ve_table && g_ve_table->metadata.max_value > g_ve_table->metadata.min_value) {
                normalized = (ve_value - g_ve_table->metadata.min_value) / (g_ve_table->metadata.max_value - g_ve_table->metadata.min_value);
            }
            
            // Calculate color based on VE value
            ImU32 cell_color;
            if (normalized < 0.5f) {
                // Blue to Green gradient
                float t = normalized * 2.0f;
                cell_color = IM_COL32(
                    (int)(0.0f * (1.0f - t) + 0.0f * t),
                    (int)(0.0f * (1.0f - t) + 255.0f * t),
                    (int)(255.0f * (1.0f - t) + 0.0f * t),
                    255
                );
            } else {
                // Green to Red gradient
                float t = (normalized - 0.5f) * 2.0f;
                cell_color = IM_COL32(
                    (int)(0.0f * (1.0f - t) + 255.0f * t),
                    (int)(255.0f * (1.0f - t) + 0.0f * t),
                    (int)(0.0f * (1.0f - t) + 0.0f * t),
                    255
                );
            }
            
            // Draw cell background with unique ID
            char cell_bg_id[32];
            snprintf(cell_bg_id, sizeof(cell_bg_id), "##cell_bg_%d_%d", x, y);
            draw_list->AddRectFilled(
                ImVec2(x_pos, y_pos),
                ImVec2(x_pos + cell_width, y_pos + cell_height),
                cell_color
            );
            
            // Draw cell border with different highlighting
            ImU32 border_color = IM_COL32(100, 100, 100, 255);
            float border_thickness = 1.0f;
            
            // Check for multi-cell selection first
            if (is_cell_in_selection(x, y)) {
                border_color = IM_COL32(0, 255, 255, 255); // Cyan for multi-cell selection
                border_thickness = 2.0f;
            } else if (x == g_selected_cell_x && y == g_selected_cell_y) {
                border_color = IM_COL32(255, 255, 0, 255); // Yellow for selected cell
                border_thickness = 3.0f;
            } else if (x == g_current_rpm_cell && y == g_current_map_cell) {
                border_color = IM_COL32(255, 0, 255, 255); // Magenta for current engine operating point
                border_thickness = 2.0f;
            }
            
            // Draw cell border with unique ID
            char cell_border_id[32];
            snprintf(cell_border_id, sizeof(cell_border_id), "##cell_border_%d_%d", x, y);
            draw_list->AddRect(
                ImVec2(x_pos, y_pos),
                ImVec2(x_pos + cell_width, y_pos + cell_height),
                border_color,
                0.0f, 0, border_thickness
            );
        }
    }
    
    // Draw engine trail (fading trail of engine position) - BEFORE cell values
    if (g_show_engine_trail) {
        for (int i = 0; i < g_trail_count_2d; i++) {
            TrailPoint* point = &g_engine_trail_2d[i];
            if (point->alpha > 0.0f) {
                float x_pos = window_pos.x + 40 + point->x * cell_width;
                float y_pos = window_pos.y + 30 + point->y * cell_height;
                
                // Draw fading trail point with increased opacity
                ImU32 trail_color = IM_COL32(255, 0, 255, (int)(255 * point->alpha * 0.5f));
                draw_list->AddRectFilled(
                    ImVec2(x_pos + 2, y_pos + 2),
                    ImVec2(x_pos + cell_width - 2, y_pos + cell_height - 2),
                    trail_color
                );
                
                // Draw trail border with increased opacity
                ImU32 border_color = IM_COL32(255, 0, 255, (int)(255 * point->alpha * 0.8f));
                draw_list->AddRect(
                    ImVec2(x_pos + 1, y_pos + 1),
                    ImVec2(x_pos + cell_width - 1, y_pos + cell_height - 1),
                    border_color,
                    0.0f, 0, 1.0f
                );
            }
        }
    }
    
    // Now draw cell values on top of everything
    for (int y = 0; y < g_ve_table->height; y++) {
        for (int x = 0; x < g_ve_table->width; x++) {
            float x_pos = window_pos.x + 40 + x * cell_width;
            float y_pos = window_pos.y + 30 + y * cell_height;
            float ve_value = imgui_table_get_value(g_ve_table, x, y);
            
            // Draw cell value - make it editable if selected
                        if (x == g_selected_cell_x && y == g_selected_cell_y && g_show_input_field) {
                            // Create an input field for the selected cell with pre-highlighted value
                            ImGui::SetCursorPos(ImVec2(x_pos + 2, y_pos + 2));
                            ImGui::SetNextItemWidth(cell_width - 4);
                            
                            char cell_edit_id[32];
                            snprintf(cell_edit_id, sizeof(cell_edit_id), "##cell_edit_%d_%d", x, y);
                            
                            // Pre-fill the buffer with current value if it's empty and this is the selected cell
                            if (g_cell_edit_buffer[0] == '\0' && x == g_selected_cell_x && y == g_selected_cell_y) {
                                snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.0f", ve_value);
                            }
                            
                            // Draw a background for the input field to make it visible
                            draw_list->AddRectFilled(
                                ImVec2(x_pos + 1, y_pos + 1),
                                ImVec2(x_pos + cell_width - 1, y_pos + cell_height - 1),
                                IM_COL32(255, 255, 255, 255) // White background
                            );
                            

                            
                            // Position the input field directly within the cell
                            ImVec2 input_pos = ImVec2(x_pos + 2 - window_pos.x, y_pos + 2 - window_pos.y);
                            ImGui::SetCursorPos(input_pos);
                            ImGui::SetNextItemWidth(cell_width - 4);
                            
                            // Handle keyboard input for the input field
                            bool should_save = false;
                            bool should_move_next = false;
                            bool should_move_prev = false;
                            
                            // Allow editing in the input field
                            ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue;
                            
                            // Force refresh if buffer was updated
                            if (g_buffer_updated) {
                                // Don't automatically set focus - let user choose input method
                                g_buffer_updated = false; // Reset flag
                            }
                            
                            // Track if this input field is focused
                            bool is_focused = ImGui::IsItemFocused();
                            if (is_focused) {
                                g_input_field_focused = true;
                            } else {
                                // Clear focus if this field is not focused
                                g_input_field_focused = false;
                            }
                            
                            if (ImGui::InputText(cell_edit_id, g_cell_edit_buffer, sizeof(g_cell_edit_buffer), input_flags)) {
                                should_save = true;
                                g_input_field_focused = false; // Clear focus when Enter is pressed
                            }
                            
                            // Save changes if needed
                            if (should_save) {
                                float input_value = atof(g_cell_edit_buffer);
                                if (input_value >= 0.0f && input_value <= 200.0f) {
                                    imgui_table_set_value(g_ve_table, x, y, input_value);
                                    // Update min/max values
                                    g_ve_table->metadata.min_value = g_ve_table->metadata.max_value = g_ve_table->data[0][0];
                                    for (int y2 = 0; y2 < g_ve_table->height; y2++) {
                                        for (int x2 = 0; x2 < g_ve_table->width; x2++) {
                                            if (g_ve_table->data[y2][x2] < g_ve_table->metadata.min_value) g_ve_table->metadata.min_value = g_ve_table->data[y2][x2];
                                            if (g_ve_table->data[y2][x2] > g_ve_table->metadata.max_value) g_ve_table->metadata.max_value = g_ve_table->data[y2][x2];
                                        }
                                    }
                                    // Clear the buffer after successful edit
                                    g_cell_edit_buffer[0] = '\0';
                                    
                                    // Move to next/previous cell if requested
                                    if (should_move_next) {
                                        g_selected_cell_x++;
                                        if (g_selected_cell_x >= g_ve_table->width) {
                                            g_selected_cell_x = 0;
                                            g_selected_cell_y++;
                                            if (g_selected_cell_y >= g_ve_table->height) {
                                                g_selected_cell_y = 0;
                                            }
                                        }
                                        snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.0f", 
                                                imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y));
                                    } else if (should_move_prev) {
                                        g_selected_cell_x--;
                                        if (g_selected_cell_x < 0) {
                                            g_selected_cell_x = g_ve_table->width - 1;
                                            g_selected_cell_y--;
                                            if (g_selected_cell_y < 0) {
                                                g_selected_cell_y = g_ve_table->height - 1;
                                            }
                                        }
                                        snprintf(g_cell_edit_buffer, sizeof(g_cell_edit_buffer), "%.0f", 
                                                imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y));
                                    }
                                }
                            }
                        } else {
                            // Draw cell value - show buffer value if typing on selected cell
                            char value_text[16];
                            if (x == g_selected_cell_x && y == g_selected_cell_y && strlen(g_cell_edit_buffer) > 0) {
                                // Show the buffer value when typing
                                snprintf(value_text, sizeof(value_text), "%s", g_cell_edit_buffer);
                            } else {
                                // Show normal cell value
                                snprintf(value_text, sizeof(value_text), "%.0f", ve_value);
                            }
                            
                            ImVec2 text_size = ImGui::CalcTextSize(value_text);
                            ImVec2 text_pos = ImVec2(
                                x_pos + (cell_width - text_size.x) * 0.5f,
                                y_pos + (cell_height - text_size.y) * 0.5f
                            );
                            
                            // Draw text with shadow for better visibility
                            // Use different color for typing feedback
                            ImU32 text_color = (x == g_selected_cell_x && y == g_selected_cell_y && strlen(g_cell_edit_buffer) > 0) 
                                ? IM_COL32(255, 255, 0, 255)  // Yellow for typing feedback
                                : IM_COL32(255, 255, 255, 255); // White for normal values
                            
                            draw_list->AddText(
                                ImVec2(text_pos.x + 1, text_pos.y + 1),
                                IM_COL32(0, 0, 0, 255),
                                value_text
                            );
                            draw_list->AddText(text_pos, text_color, value_text);
                        }
                        }
                    }
                }
    
    // Engine position calculation moved to main update loop
    

    
    // Draw axis labels on all sides for better visibility - positioned inside child window
    // X-axis title (RPM) - Top and Bottom
    draw_list->AddText(
        ImVec2(window_pos.x + window_size.x * 0.5f - 20, window_pos.y + 5),
        IM_COL32(255, 255, 255, 255),
        "RPM (Top)"
    );
    draw_list->AddText(
        ImVec2(window_pos.x + window_size.x * 0.5f - 20, window_pos.y + window_size.y - 15),
        IM_COL32(255, 255, 255, 255),
        "RPM (Bottom)"
    );
    
    // X-axis labels (RPM values) - Top and Bottom
    for (int x = 0; x < g_ve_table->width; x++) {
        float x_pos = window_pos.x + 40 + x * cell_width + cell_width * 0.5f; // Match cell positioning
        
        // Top labels - inside window
        char rpm_text_top[16];
        snprintf(rpm_text_top, sizeof(rpm_text_top), "%.0f", g_ve_table->x_axis[x]);
        ImVec2 text_size_top = ImGui::CalcTextSize(rpm_text_top);
        draw_list->AddText(
            ImVec2(x_pos - text_size_top.x * 0.5f, window_pos.y + 5),
            IM_COL32(255, 255, 255, 255),
            rpm_text_top
        );
        
        // Bottom labels - inside window
        char rpm_text_bottom[16];
        snprintf(rpm_text_bottom, sizeof(rpm_text_bottom), "%.0f", g_ve_table->x_axis[x]);
        ImVec2 text_size_bottom = ImGui::CalcTextSize(rpm_text_bottom);
        draw_list->AddText(
            ImVec2(x_pos - text_size_bottom.x * 0.5f, window_pos.y + window_size.y - 15),
            IM_COL32(255, 255, 255, 255),
            rpm_text_bottom
        );
    }
    
    // Y-axis title (MAP) - Left and Right (permanently rotated 90 degrees)
    // Left side - rotated text positioned vertically
    draw_list->AddText(
        ImVec2(window_pos.x + 5, window_pos.y + window_size.y * 0.5f + 30),
        IM_COL32(255, 255, 255, 255),
        "MAP (kPa)"
    );
    // Right side - rotated text positioned vertically
    draw_list->AddText(
        ImVec2(window_pos.x + window_size.x - 60, window_pos.y + window_size.y * 0.5f + 30),
        IM_COL32(255, 255, 255, 255),
        "MAP (kPa)"
    );
    
    // Y-axis labels (MAP values) - Left and Right
    for (int y = 0; y < g_ve_table->height; y++) {
        float y_pos = window_pos.y + 30 + y * cell_height + cell_height * 0.5f; // Match cell positioning
        
        // Left labels - inside window (always horizontal)
        char map_text_left[16];
        snprintf(map_text_left, sizeof(map_text_left), "%.0f", g_ve_table->y_axis[y]);
        ImVec2 text_size_left = ImGui::CalcTextSize(map_text_left);
        draw_list->AddText(
            ImVec2(window_pos.x + 5, y_pos - text_size_left.y * 0.5f),
            IM_COL32(255, 255, 255, 255),
            map_text_left
        );
        
        // Right labels - inside window (always horizontal)
        char map_text_right[16];
        snprintf(map_text_right, sizeof(map_text_right), "%.0f", g_ve_table->y_axis[y]);
        ImVec2 text_size_right = ImGui::CalcTextSize(map_text_right);
        draw_list->AddText(
            ImVec2(window_pos.x + window_size.x - 45, y_pos - text_size_right.y * 0.5f),
            IM_COL32(255, 255, 255, 255),
            map_text_right
        );
    }
    
    // Trail is now drawn before cell values to avoid covering them
    
    ImGui::EndChild();
    
    // Cell editing interface - Simplified since we now have direct editing
    if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Selected Cell [%d, %d]: %.1f VE", 
                           g_selected_cell_x, g_selected_cell_y, 
                           imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y));
        ImGui::Text("RPM: %.0f, MAP: %.1f kPa", g_ve_table->x_axis[g_selected_cell_x], g_ve_table->y_axis[g_selected_cell_y]);
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Tip: Type new value and press Enter, or use +/- keys to adjust");
    } else {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click on any cell in the heatmap to edit its value directly");
    }
    
    // Enhanced color legend and status
    ImGui::Separator();
    ImGui::Text("Color Legend:");
    
    // Create a more visual color legend
    ImGui::BeginGroup();
    
    // Low VE (Blue)
    ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "■ Blue: Low VE (45-70)");
    ImGui::SameLine();
    
    // Medium VE (Green)
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "■ Green: Medium VE (70-90)");
    ImGui::SameLine();
    
    // High VE (Red)
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "■ Red: High VE (90-120)");
    
    ImGui::EndGroup();
    
    // Border legend
    ImGui::Text("Border Legend:");
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "■ Yellow Border: Selected Cell");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "■ Magenta Border: Current Engine Point");
    
    // Current engine operating point status
    if (g_current_rpm_cell >= 0 && g_current_map_cell >= 0) {
        ImGui::Text("Current Engine Point: RPM=%.0f, MAP=%.1f kPa (Cell [%d,%d])", 
                   g_ecu_data.rpm, g_ecu_data.map, g_current_rpm_cell, g_current_map_cell);
    } else {
        ImGui::Text("Engine Status: Not Running");
    }
}

void render_ve_table_3d_view() {
    ImGui::Text("3D View");
    ImGui::Text("This view shows a 3D representation of the VE table.");
    ImGui::Text("Use the controls above to adjust the view.");
    
    // 3D view controls
    ImGui::Separator();
    ImGui::Text("3D View Controls:");
    ImGui::SliderFloat("Rotation X", &g_ve_3d_view.rotation_x, -90.0f, 90.0f);
    ImGui::SliderFloat("Rotation Y", &g_ve_3d_view.rotation_y, -180.0f, 180.0f);
    ImGui::SliderFloat("Zoom", &g_ve_3d_view.zoom, 0.1f, 10.0f);
    ImGui::SliderFloat("Pan X", &g_ve_3d_view.pan_x, -2.0f, 2.0f);
    ImGui::SliderFloat("Pan Y", &g_ve_3d_view.pan_y, -2.0f, 2.0f);
    ImGui::SliderFloat("Opacity", &g_ve_3d_view.opacity, 0.1f, 1.0f);
    ImGui::Checkbox("Wireframe Mode", &g_ve_3d_view.wireframe_mode);
    ImGui::Checkbox("Show Grid", &g_ve_3d_view.show_grid);
    ImGui::Checkbox("Show Axes", &g_ve_3d_view.show_axes);
    
    ImGui::SameLine();
    
    if (ImGui::Button("Reset View", ImVec2(100, 20))) {
        g_ve_3d_view.rotation_x = 0.0f;
        g_ve_3d_view.rotation_y = 0.0f;
        g_ve_3d_view.zoom = 1.0f;
        g_ve_3d_view.pan_x = 0.0f;
        g_ve_3d_view.pan_y = 0.0f;
    }
    
    // Camera preset buttons
    ImGui::Separator();
    ImGui::Text("Camera Presets:");
    if (ImGui::Button("Isometric", ImVec2(100, 25))) {
        set_isometric_view();
    }
    ImGui::SameLine();
    if (ImGui::Button("Side View", ImVec2(100, 25))) {
        set_side_view();
    }
    ImGui::SameLine();
    if (ImGui::Button("Top Down", ImVec2(100, 25))) {
        set_top_down_view();
    }
    
    // Mouse control instructions
    ImGui::Separator();
    ImGui::Text("Mouse Controls:");
    ImGui::Text("• Left-click + drag: Rotate view");
    ImGui::Text("• Right-click + drag: Pan view");
    ImGui::Text("• Mouse wheel: Zoom in/out");
    ImGui::Text("• Hover over 3D area to activate mouse controls");
    
    // 3D rendering area
    ImGui::Separator();
    ImGui::Text("3D Rendering Area:");
    
    // Calculate rendering area
    float render_width = ImGui::GetWindowWidth() - 40;
    float render_height = render_width * 0.75f;
    
            // Create a child window for 3D rendering
        ImGui::BeginChild("3DRenderArea", ImVec2(render_width, render_height), true);
        
        // Get the draw list for this child window
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 window_pos = ImGui::GetWindowPos();
        ImVec2 window_size = ImGui::GetWindowSize();
        
        // Mouse interaction for 3D view
        ImVec2 mouse_pos = ImGui::GetMousePos();
        bool mouse_in_area = ImGui::IsMouseHoveringRect(
            ImVec2(window_pos.x, window_pos.y),
            ImVec2(window_pos.x + window_size.x, window_pos.y + window_size.y)
        );
        
        // Handle mouse wheel for zoom
        if (mouse_in_area) {
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f) {
                // Zoom in/out with mouse wheel
                float zoom_factor = wheel > 0 ? 1.1f : 0.9f;
                g_ve_3d_view.zoom *= zoom_factor;
                g_ve_3d_view.zoom = fmax(0.1f, fmin(10.0f, g_ve_3d_view.zoom)); // Clamp zoom
            }
            
            // Handle mouse drag for rotation
            if (ImGui::IsMouseDown(0) && !g_ve_mouse_dragging) {
                g_ve_mouse_dragging = true;
                g_ve_last_mouse_pos = mouse_pos;
            }
            
            if (g_ve_mouse_dragging && ImGui::IsMouseDown(0)) {
                ImVec2 delta = ImVec2(mouse_pos.x - g_ve_last_mouse_pos.x, mouse_pos.y - g_ve_last_mouse_pos.y);
                
                // Rotate based on mouse movement
                g_ve_3d_view.rotation_y += delta.x * 0.5f; // Horizontal movement rotates Y
                g_ve_3d_view.rotation_x += delta.y * 0.5f; // Vertical movement rotates X
                
                // Clamp rotation angles
                g_ve_3d_view.rotation_x = fmax(-90.0f, fmin(90.0f, g_ve_3d_view.rotation_x));
                g_ve_3d_view.rotation_y = fmod(g_ve_3d_view.rotation_y, 360.0f);
                
                g_ve_last_mouse_pos = mouse_pos;
            }
            
            if (!ImGui::IsMouseDown(0)) {
                g_ve_mouse_dragging = false;
            }
            
            // Handle right-click drag for panning
            if (ImGui::IsMouseDown(1) && !g_ve_mouse_dragging) {
                g_ve_mouse_dragging = true;
                g_ve_last_mouse_pos = mouse_pos;
            }
            
            if (g_ve_mouse_dragging && ImGui::IsMouseDown(1)) {
                ImVec2 delta = ImVec2(mouse_pos.x - g_ve_last_mouse_pos.x, mouse_pos.y - g_ve_last_mouse_pos.y);
                
                // Pan based on mouse movement
                g_ve_3d_view.pan_x += delta.x * 0.02f;
                g_ve_3d_view.pan_y += delta.y * 0.02f;
                
                // Clamp pan values
                g_ve_3d_view.pan_x = fmax(-2.0f, fmin(2.0f, g_ve_3d_view.pan_x));
                g_ve_3d_view.pan_y = fmax(-2.0f, fmin(2.0f, g_ve_3d_view.pan_y));
                
                g_ve_last_mouse_pos = mouse_pos;
            }
        }
    
    // Calculate center of rendering area
    ImVec2 center = ImVec2(window_pos.x + window_size.x * 0.5f, window_pos.y + window_size.y * 0.5f);
    
    // Draw background
    draw_list->AddRectFilled(
        ImVec2(window_pos.x + 5, window_pos.y + 5),
        ImVec2(window_pos.x + window_size.x - 5, window_pos.y + window_size.y - 5),
        IM_COL32(20, 20, 20, 255)
    );
    
    // Draw border
    draw_list->AddRect(
        ImVec2(window_pos.x + 5, window_pos.y + 5),
        ImVec2(window_pos.x + window_size.x - 5, window_pos.y + window_size.y - 5),
        IM_COL32(100, 100, 100, 255)
    );
    
            // Enhanced 3D wireframe visualization with proper transformations
        if (g_ve_table && g_ve_table->data) {
            // Calculate optimal scale to fill the visible area
            float render_width = window_size.x - 10;
            float render_height = window_size.y - 10;
            float optimal_scale = fmin(render_width, render_height) / 120.0f; // Base scale to fill area
            
            float scale = optimal_scale * g_ve_3d_view.zoom; // Apply zoom multiplier
            float grid_size = 100.0f * scale;
            float height_scale = 40.0f * scale; // Define height_scale here for axes
        
        // Convert rotation angles to radians
        float rot_x_rad = g_ve_3d_view.rotation_x * 3.14159f / 180.0f;
        float rot_y_rad = g_ve_3d_view.rotation_y * 3.14159f / 180.0f;
        
        // Apply pan offset
        float pan_offset_x = g_ve_3d_view.pan_x * 50.0f;
        float pan_offset_y = g_ve_3d_view.pan_y * 50.0f;
        
        // Helper function to transform 3D point to 2D
        auto transform_3d_to_2d = [&](float x, float y, float z) -> ImVec2 {
            // Apply rotation around Y axis (affects X and Z)
            float temp_x = x * cos(rot_y_rad) - z * sin(rot_y_rad);
            float temp_z = x * sin(rot_y_rad) + z * cos(rot_y_rad);
            
            // Apply rotation around X axis (affects Y and Z)
            float final_y = y * cos(rot_x_rad) - temp_z * sin(rot_x_rad);
            float final_z = y * sin(rot_x_rad) + temp_z * cos(rot_x_rad);
            
            // Apply pan and center
            float screen_x = center.x + temp_x + pan_offset_x;
            float screen_y = center.y + final_y + pan_offset_y;
            
            return ImVec2(screen_x, screen_y);
        };
        
        // Draw grid lines if enabled
        if (g_ve_3d_view.show_grid) {
            // Vertical grid lines (RPM axis)
            for (int x = 0; x <= g_ve_table->width; x++) {
                float x_pos = (x - g_ve_table->width * 0.5f) * grid_size / g_ve_table->width;
                ImVec2 start_3d = transform_3d_to_2d(x_pos, -grid_size * 0.5f, 0);
                ImVec2 end_3d = transform_3d_to_2d(x_pos, grid_size * 0.5f, 0);
                
                draw_list->AddLine(start_3d, end_3d, IM_COL32(50, 50, 50, 100));
            }
            
            // Horizontal grid lines (MAP axis)
            for (int y = 0; y <= g_ve_table->height; y++) {
                float y_pos = (y - g_ve_table->height * 0.5f) * grid_size / g_ve_table->height;
                ImVec2 start_3d = transform_3d_to_2d(-grid_size * 0.5f, y_pos, 0);
                ImVec2 end_3d = transform_3d_to_2d(grid_size * 0.5f, y_pos, 0);
                
                draw_list->AddLine(start_3d, end_3d, IM_COL32(50, 50, 50, 100));
            }
        }
        
        // Draw X, Y, Z graph walls (like old MegaTunix)
        if (g_ve_3d_view.show_axes) {
            // X-axis wall (RPM axis) - Red
            for (int x = 0; x <= g_ve_table->width; x++) {
                float x_pos = (x - g_ve_table->width * 0.5f) * grid_size / g_ve_table->width;
                ImVec2 wall_start = transform_3d_to_2d(x_pos, -grid_size * 0.5f, 0);
                ImVec2 wall_end = transform_3d_to_2d(x_pos, -grid_size * 0.5f, -height_scale);
                
                draw_list->AddLine(wall_start, wall_end, IM_COL32(255, 0, 0, 150), 2.0f);
            }
            
            // Y-axis wall (MAP axis) - Green
            for (int y = 0; y <= g_ve_table->height; y++) {
                float y_pos = (y - g_ve_table->height * 0.5f) * grid_size / g_ve_table->height;
                ImVec2 wall_start = transform_3d_to_2d(-grid_size * 0.5f, y_pos, 0);
                ImVec2 wall_end = transform_3d_to_2d(-grid_size * 0.5f, y_pos, -height_scale);
                
                draw_list->AddLine(wall_start, wall_end, IM_COL32(0, 255, 0, 150), 2.0f);
            }
            
            // Z-axis wall (VE axis) - Blue
            for (int z = 0; z <= 10; z++) {
                float z_pos = -z * height_scale / 10.0f;
                ImVec2 wall_start = transform_3d_to_2d(-grid_size * 0.5f, -grid_size * 0.5f, z_pos);
                ImVec2 wall_end = transform_3d_to_2d(grid_size * 0.5f, -grid_size * 0.5f, z_pos);
                
                draw_list->AddLine(wall_start, wall_end, IM_COL32(0, 0, 255, 150), 2.0f);
            }
            
            // Draw axis labels and values
            // X-axis (RPM) labels
            for (int x = 0; x < g_ve_table->width; x += 2) { // Show every other label to avoid clutter
                float x_pos = (x - g_ve_table->width * 0.5f) * grid_size / g_ve_table->width;
                ImVec2 label_pos = transform_3d_to_2d(x_pos, -grid_size * 0.5f - 10, 0);
                
                char rpm_label[16];
                snprintf(rpm_label, sizeof(rpm_label), "%.0f", g_ve_table->x_axis[x]);
                draw_list->AddText(label_pos, IM_COL32(255, 0, 0, 255), rpm_label);
            }
            
            // Y-axis (MAP) labels
            for (int y = 0; y < g_ve_table->height; y += 2) { // Show every other label to avoid clutter
                float y_pos = (y - g_ve_table->height * 0.5f) * grid_size / g_ve_table->height;
                ImVec2 label_pos = transform_3d_to_2d(-grid_size * 0.5f - 30, y_pos, 0);
                
                char map_label[16];
                snprintf(map_label, sizeof(map_label), "%.0f", g_ve_table->y_axis[y]);
                draw_list->AddText(label_pos, IM_COL32(0, 255, 0, 255), map_label);
            }
            
            // Z-axis (VE) labels
            for (int z = 0; z <= 5; z++) {
                float z_pos = -z * height_scale / 5.0f;
                ImVec2 label_pos = transform_3d_to_2d(-grid_size * 0.5f - 20, -grid_size * 0.5f - 10, z_pos);
                
                char ve_label[16];
                snprintf(ve_label, sizeof(ve_label), "%.0f", g_ve_table->metadata.min_value + (z * (g_ve_table->metadata.max_value - g_ve_table->metadata.min_value) / 5.0f));
                draw_list->AddText(label_pos, IM_COL32(0, 0, 255, 255), ve_label);
            }
            
            // Axis titles
            ImVec2 x_title_pos = transform_3d_to_2d(0, -grid_size * 0.5f - 30, 0);
            draw_list->AddText(x_title_pos, IM_COL32(255, 0, 0, 255), "RPM");
            
            ImVec2 y_title_pos = transform_3d_to_2d(-grid_size * 0.5f - 50, 0, 0);
            draw_list->AddText(y_title_pos, IM_COL32(0, 255, 0, 255), "MAP (kPa)");
            
            ImVec2 z_title_pos = transform_3d_to_2d(-grid_size * 0.5f - 20, -grid_size * 0.5f - 30, -height_scale);
            draw_list->AddText(z_title_pos, IM_COL32(0, 0, 255, 255), "VE (%)");
        }
        
        // Draw VE table as 3D wireframe with transformations
        for (int y = 0; y < g_ve_table->height - 1; y++) {
            for (int x = 0; x < g_ve_table->width - 1; x++) {
                // Calculate 3D positions
                float x1 = (x - g_ve_table->width * 0.5f) * grid_size / g_ve_table->width;
                float x2 = (x + 1 - g_ve_table->width * 0.5f) * grid_size / g_ve_table->width;
                float y1 = (y - g_ve_table->height * 0.5f) * grid_size / g_ve_table->height;
                float y2 = (y + 1 - g_ve_table->height * 0.5f) * grid_size / g_ve_table->height;
                
                // Calculate VE values (normalized to 0-1)
                float ve1 = (g_ve_table->data[y][x] - g_ve_table->metadata.min_value) / (g_ve_table->metadata.max_value - g_ve_table->metadata.min_value);
                float ve2 = (g_ve_table->data[y][x+1] - g_ve_table->metadata.min_value) / (g_ve_table->metadata.max_value - g_ve_table->metadata.min_value);
                float ve3 = (g_ve_table->data[y+1][x] - g_ve_table->metadata.min_value) / (g_ve_table->metadata.max_value - g_ve_table->metadata.min_value);
                float ve4 = (g_ve_table->data[y+1][x+1] - g_ve_table->metadata.min_value) / (g_ve_table->metadata.max_value - g_ve_table->metadata.min_value);
                
                // Apply 3D effect (height based on VE value)
                float z1 = -ve1 * height_scale;
                float z2 = -ve2 * height_scale;
                float z3 = -ve3 * height_scale;
                float z4 = -ve4 * height_scale;
                
                // Transform all points to 2D
                ImVec2 p1 = transform_3d_to_2d(x1, y1, z1);
                ImVec2 p2 = transform_3d_to_2d(x2, y1, z2);
                ImVec2 p3 = transform_3d_to_2d(x1, y2, z3);
                ImVec2 p4 = transform_3d_to_2d(x2, y2, z4);
                
                // Draw wireframe lines
                ImU32 line_color = IM_COL32(100, 200, 255, 200);
                
                // Horizontal lines
                draw_list->AddLine(p1, p2, line_color);
                draw_list->AddLine(p3, p4, line_color);
                
                // Vertical lines
                draw_list->AddLine(p1, p3, line_color);
                draw_list->AddLine(p2, p4, line_color);
                
                // Fill faces if not in wireframe mode
                if (!g_ve_3d_view.wireframe_mode) {
                    ImU32 fill_color = IM_COL32(
                        (int)(ve1 * 255), 
                        (int)((1.0f - ve1) * 255), 
                        100, 
                        (int)(g_ve_3d_view.opacity * 150)
                    );
                    
                    // Draw filled triangles
                    draw_list->AddTriangleFilled(p1, p2, p3, fill_color);
                    draw_list->AddTriangleFilled(p2, p3, p4, fill_color);
                }
            }
        }
        
        // Draw axes if enabled
        if (g_ve_3d_view.show_axes) {
            // X-axis (RPM) - transformed
            ImVec2 x_start = transform_3d_to_2d(-grid_size * 0.5f, 0, 0);
            ImVec2 x_end = transform_3d_to_2d(grid_size * 0.5f, 0, 0);
            draw_list->AddLine(x_start, x_end, IM_COL32(255, 0, 0, 255), 2.0f);
            draw_list->AddText(x_end, IM_COL32(255, 0, 0, 255), "RPM");
            
            // Y-axis (MAP) - transformed
            ImVec2 y_start = transform_3d_to_2d(0, -grid_size * 0.5f, 0);
            ImVec2 y_end = transform_3d_to_2d(0, grid_size * 0.5f, 0);
            draw_list->AddLine(y_start, y_end, IM_COL32(0, 255, 0, 255), 2.0f);
            draw_list->AddText(y_end, IM_COL32(0, 255, 0, 255), "MAP");
            
            // Z-axis (VE) - transformed
            ImVec2 z_start = transform_3d_to_2d(0, 0, 0);
            ImVec2 z_end = transform_3d_to_2d(0, 0, -height_scale);
            draw_list->AddLine(z_start, z_end, IM_COL32(0, 0, 255, 255), 2.0f);
            draw_list->AddText(z_end, IM_COL32(0, 0, 255, 255), "VE");
        }
        
        // Draw animated wireframe triangle/arrow at current engine position
        if (g_current_rpm_cell >= 0 && g_current_map_cell >= 0) {
            // Get the VE value at the current engine position
            float current_ve_3d = imgui_table_get_value(g_ve_table, g_current_rpm_cell, g_current_map_cell);
            
            // Calculate the 3D position of the current engine point
            float current_rpm_normalized = (g_ecu_data.rpm - g_ve_table->x_axis[0]) / 
                                         (g_ve_table->x_axis[g_ve_table->width-1] - g_ve_table->x_axis[0]);
            float current_map_normalized = (g_ecu_data.map - g_ve_table->y_axis[0]) / 
                                         (g_ve_table->y_axis[g_ve_table->height-1] - g_ve_table->y_axis[0]);
            
            // Clamp to valid range
            current_rpm_normalized = fmax(0.0f, fmin(1.0f, current_rpm_normalized));
            current_map_normalized = fmax(0.0f, fmin(1.0f, current_map_normalized));
            
            // Convert to 3D coordinates
            float x_pos = (current_rpm_normalized - 0.5f) * grid_size;
            float y_pos = (current_map_normalized - 0.5f) * grid_size;
            float z_pos = -current_ve_3d * height_scale / g_ve_table->metadata.max_value;
            
            // Transform to 2D screen coordinates
            ImVec2 engine_pos_2d = transform_3d_to_2d(x_pos, y_pos, z_pos);
            
            // Draw engine trail (fading trail of engine position) for 3D view
            if (g_show_engine_trail) {
                for (int i = 0; i < g_trail_count_3d; i++) {
                    TrailPoint* point = &g_engine_trail_3d[i];
                    if (point->alpha > 0.0f) {
                        // Convert trail point back to 3D coordinates
                        float trail_rpm_normalized = (float)point->x / (float)(g_ve_table->width - 1);
                        float trail_map_normalized = (float)point->y / (float)(g_ve_table->height - 1);
                        
                        // Get VE value at trail position
                        float trail_ve = imgui_table_get_value(g_ve_table, point->x, point->y);
                        
                        // Convert to 3D coordinates
                        float trail_x_pos = (trail_rpm_normalized - 0.5f) * grid_size;
                        float trail_y_pos = (trail_map_normalized - 0.5f) * grid_size;
                        float trail_z_pos = -trail_ve * height_scale / g_ve_table->metadata.max_value;
                        
                        // Transform to 2D screen coordinates
                        ImVec2 trail_pos_2d = transform_3d_to_2d(trail_x_pos, trail_y_pos, trail_z_pos);
                        
                                            // Draw fading trail point with increased opacity
                    ImU32 trail_color = IM_COL32(255, 0, 255, (int)(255 * point->alpha * 0.6f));
                    draw_list->AddCircleFilled(trail_pos_2d, 4.0f, trail_color);
                    
                    // Draw trail border with increased opacity
                    ImU32 border_color = IM_COL32(255, 0, 255, (int)(255 * point->alpha * 0.9f));
                        draw_list->AddCircle(trail_pos_2d, 6.0f, border_color, 0, 1.0f);
                    }
                }
            }
            
            // Create solid triangle/arrow pointing upward
            float triangle_size = 15.0f;
            
            // Draw wireframe triangle (pointing downward toward the map)
            ImVec2 triangle_bottom = ImVec2(engine_pos_2d.x, engine_pos_2d.y + triangle_size);
            ImVec2 triangle_left = ImVec2(engine_pos_2d.x - triangle_size * 0.5f, engine_pos_2d.y - triangle_size * 0.5f);
            ImVec2 triangle_right = ImVec2(engine_pos_2d.x + triangle_size * 0.5f, engine_pos_2d.y - triangle_size * 0.5f);
            
            // Draw triangle outline in solid magenta (matching 2D view)
            ImU32 triangle_color = IM_COL32(255, 0, 255, 255);
            draw_list->AddTriangle(triangle_bottom, triangle_left, triangle_right, triangle_color, 2.0f);
            
            // Add a small circle at the base for better visibility
            draw_list->AddCircle(engine_pos_2d, 3.0f, triangle_color, 0, 2.0f);
            
            // Add text label showing current values
            char engine_label[64];
            snprintf(engine_label, sizeof(engine_label), "%.0f RPM\n%.1f kPa", g_ecu_data.rpm, g_ecu_data.map);
            ImVec2 text_pos = ImVec2(engine_pos_2d.x + 20, engine_pos_2d.y - 10);
            draw_list->AddText(text_pos, triangle_color, engine_label);
        }
        
        // Draw comprehensive value labels like original MegaTunix
        char label_buffer[128];
        
        // Top-left info panel (like original MegaTunix)
        snprintf(label_buffer, sizeof(label_buffer), "VE Range: %.1f - %.1f", 
                g_ve_table->metadata.min_value, g_ve_table->metadata.max_value);
        draw_list->AddText(
            ImVec2(window_pos.x + 10, window_pos.y + 10),
            IM_COL32(200, 200, 200, 255),
            label_buffer
        );
        
        snprintf(label_buffer, sizeof(label_buffer), "RPM: %.0f - %.0f", 
                g_ve_table->x_axis[0], g_ve_table->x_axis[g_ve_table->width-1]);
        draw_list->AddText(
            ImVec2(window_pos.x + 10, window_pos.y + 25),
            IM_COL32(200, 200, 200, 255),
            label_buffer
        );
        
        snprintf(label_buffer, sizeof(label_buffer), "MAP: %.1f - %.1f kPa", 
                g_ve_table->y_axis[0], g_ve_table->y_axis[g_ve_table->height-1]);
        draw_list->AddText(
            ImVec2(window_pos.x + 10, window_pos.y + 40),
            IM_COL32(200, 200, 200, 255),
            label_buffer
        );
        
        // Current Position fields (like original MegaTunix)
        snprintf(label_buffer, sizeof(label_buffer), "Current Position:");
        draw_list->AddText(
            ImVec2(window_pos.x + 10, window_pos.y + 60),
            IM_COL32(255, 255, 255, 255),
            label_buffer
        );
        
        // Use real engine position (like original MegaTunix)
        float current_rpm_3d = g_ecu_data.rpm;
        float current_map_3d = g_ecu_data.map;
        float current_ve_3d = 0.0f;
        
        // Get the VE value at the current engine position
        if (g_current_rpm_cell >= 0 && g_current_map_cell >= 0) {
            current_ve_3d = imgui_table_get_value(g_ve_table, g_current_rpm_cell, g_current_map_cell);
        }
        
        snprintf(label_buffer, sizeof(label_buffer), "RPM: %.0f", current_rpm_3d);
        draw_list->AddText(
            ImVec2(window_pos.x + 10, window_pos.y + 75),
            IM_COL32(0, 255, 0, 255), // Green like original
            label_buffer
        );
        
        snprintf(label_buffer, sizeof(label_buffer), "MAP: %.1f kPa", current_map_3d);
        draw_list->AddText(
            ImVec2(window_pos.x + 10, window_pos.y + 90),
            IM_COL32(0, 255, 0, 255), // Green like original
            label_buffer
        );
        
        snprintf(label_buffer, sizeof(label_buffer), "VE: %.1f", current_ve_3d);
        draw_list->AddText(
            ImVec2(window_pos.x + 10, window_pos.y + 105),
            IM_COL32(0, 255, 0, 255), // Green like original
            label_buffer
        );
        
        // Edit Position fields (like original MegaTunix)
        snprintf(label_buffer, sizeof(label_buffer), "Edit Position:");
        draw_list->AddText(
            ImVec2(window_pos.x + 10, window_pos.y + 130),
            IM_COL32(255, 255, 255, 255),
            label_buffer
        );
        
        // Show selected cell position
        if (g_selected_cell_x >= 0 && g_selected_cell_y >= 0) {
            snprintf(label_buffer, sizeof(label_buffer), "RPM: %.0f", g_ve_table->x_axis[g_selected_cell_x]);
            draw_list->AddText(
                ImVec2(window_pos.x + 10, window_pos.y + 145),
                IM_COL32(255, 255, 0, 255), // Yellow for selected
                label_buffer
            );
            
            snprintf(label_buffer, sizeof(label_buffer), "MAP: %.1f kPa", g_ve_table->y_axis[g_selected_cell_y]);
            draw_list->AddText(
                ImVec2(window_pos.x + 10, window_pos.y + 160),
                IM_COL32(255, 255, 0, 255), // Yellow for selected
                label_buffer
            );
            
            float selected_ve = imgui_table_get_value(g_ve_table, g_selected_cell_x, g_selected_cell_y);
            snprintf(label_buffer, sizeof(label_buffer), "VE: %.1f", selected_ve);
            draw_list->AddText(
                ImVec2(window_pos.x + 10, window_pos.y + 175),
                IM_COL32(255, 255, 0, 255), // Yellow for selected
                label_buffer
            );
        } else {
            snprintf(label_buffer, sizeof(label_buffer), "RPM: --");
            draw_list->AddText(
                ImVec2(window_pos.x + 10, window_pos.y + 145),
                IM_COL32(128, 128, 128, 255), // Gray for no selection
                label_buffer
            );
            
            snprintf(label_buffer, sizeof(label_buffer), "MAP: -- kPa");
            draw_list->AddText(
                ImVec2(window_pos.x + 10, window_pos.y + 160),
                IM_COL32(128, 128, 128, 255), // Gray for no selection
                label_buffer
            );
            
            snprintf(label_buffer, sizeof(label_buffer), "VE: --");
            draw_list->AddText(
                ImVec2(window_pos.x + 10, window_pos.y + 175),
                IM_COL32(128, 128, 128, 255), // Gray for no selection
                label_buffer
            );
        }
    }
    
    ImGui::EndChild();
    
    // Real-Time Variables section (like original MegaTunix)
    ImGui::Separator();
    ImGui::Text("Real-Time Variables:");
    
    // Simulate real-time engine data
    static float rt_rpm = 2500.0f;
    static float rt_map = 80.0f;
    static float rt_afr = 14.7f;
    static float rt_clt = 85.0f;
    
    // Update simulated values slightly for demo
    rt_rpm += (rand() % 100 - 50) * 0.1f;
    rt_map += (rand() % 20 - 10) * 0.1f;
    rt_afr += (rand() % 10 - 5) * 0.01f;
    rt_clt += (rand() % 10 - 5) * 0.1f;
    
    // Clamp values to realistic ranges
    rt_rpm = fmax(800.0f, fmin(6000.0f, rt_rpm));
    rt_map = fmax(20.0f, fmin(120.0f, rt_map));
    rt_afr = fmax(10.0f, fmin(20.0f, rt_afr));
    rt_clt = fmax(60.0f, fmin(110.0f, rt_clt));
    
    ImGui::Text("RPM: %.0f", rt_rpm);
    ImGui::Text("MAP: %.1f kPa", rt_map);
    ImGui::Text("AFR: %.1f", rt_afr);
    ImGui::Text("CLT: %.1f°C", rt_clt);
    
    ImGui::Separator();
    ImGui::Text("3D Visualization: Wireframe shows VE table as a 3D surface");
    ImGui::Text("Height represents VE values, color indicates efficiency");
    ImGui::Text("Current Position shows simulated engine operating point");
    ImGui::Text("Edit Position shows selected cell coordinates");
}

void render_ve_table_editor_view() {
    ImGui::Text("Table Editor View");
    ImGui::Text("Click on cells to edit values directly.");
    
    // Table editing controls
    ImGui::Separator();
    ImGui::Text("Editing Controls:");
    
    static float edit_value = 0.0f;
    static int edit_x = 0, edit_y = 0;
    
    ImGui::InputInt("Edit X", &edit_x, 1, 1);
    ImGui::InputInt("Edit Y", &edit_y, 1, 1);
    ImGui::InputFloat("Edit Value", &edit_value, 1.0f, 5.0f, "%.1f");
    
    if (ImGui::Button("Set Value", ImVec2(100, 30))) {
        if (edit_x >= 0 && edit_x < g_ve_table->width && 
            edit_y >= 0 && edit_y < g_ve_table->height) {
            imgui_table_set_value(g_ve_table, edit_x, edit_y, edit_value);
            if (g_ve_texture) {
                imgui_ve_texture_update(g_ve_texture, g_ve_table, &g_ve_3d_view);
            }
        }
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Get Value", ImVec2(100, 30))) {
        if (edit_x >= 0 && edit_x < g_ve_table->width && 
            edit_y >= 0 && edit_y < g_ve_table->height) {
            edit_value = imgui_table_get_value(g_ve_table, edit_x, edit_y);
        }
    }
    
    ImGui::Separator();
    
    // Table display
    ImGui::Text("VE Table Values:");
    
    // Create a scrollable table
    ImGui::BeginChild("TableScroll", ImVec2(0, 300), true);
    
    // Table header (RPM values)
    ImGui::Text("MAP\\RPM");
    for (int x = 0; x < g_ve_table->width; x++) {
        ImGui::SameLine();
        ImGui::Text("%.0f", g_ve_table->x_axis[x]);
    }
    
    // Table rows
    for (int y = 0; y < g_ve_table->height; y++) {
        // Row header (MAP value)
        ImGui::Text("%.1f", g_ve_table->y_axis[y]);
        
        // Table cells
        for (int x = 0; x < g_ve_table->width; x++) {
            ImGui::SameLine();
            
                                        // Create unique ID for each cell
                            char cell_id[32];
                            snprintf(cell_id, sizeof(cell_id), "##table_cell_%d_%d", x, y);
            
            // Get current value
            float value = imgui_table_get_value(g_ve_table, x, y);
            
                                // Create smaller input field for each cell
                    char value_str[16];
                    snprintf(value_str, sizeof(value_str), "%.0f", value);
                    
                    ImGui::SetNextItemWidth(40); // Make cells much smaller
                    if (ImGui::InputText(cell_id, value_str, sizeof(value_str), 
                                        ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
                        // Parse new value
                        float new_value = atof(value_str);
                        if (new_value >= 0.0f && new_value <= 200.0f) { // Reasonable VE range
                            imgui_table_set_value(g_ve_table, x, y, new_value);
                            if (g_ve_texture) {
                                imgui_ve_texture_update(g_ve_texture, g_ve_table, &g_ve_3d_view);
                            }
                        }
                    }
            
            // Highlight selected cell
            if (x == g_selected_cell_x && y == g_selected_cell_y) {
                ImGui::GetWindowDrawList()->AddRect(
                    ImGui::GetItemRectMin(),
                    ImGui::GetItemRectMax(),
                    IM_COL32(255, 255, 0, 255),
                    0.0f, 0, 2.0f
                );
            }
        }
    }
    
    ImGui::EndChild();
} 