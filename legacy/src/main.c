/*
 * MegaTunix Redux - Main Application Entry Point
 * 
 * Copyright (C) 2025 Pat Burke
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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// ImGui includes
#include "../../external/imgui/imgui.h"
#include "../../external/imgui/imgui_impl_sdl2.h"
#include "../../external/imgui/imgui_impl_opengl3.h"

#include "../include/megatunix_redux.h"
#include "../include/ecu/ecu_communication.h"
#include "../include/dashboard/dashboard.h"
#include "../include/utils/config.h"
#include "../include/utils/logging.h"
#include "../include/ui/imgui_ve_table.h"

// Global variables
static SDL_Window* g_window = NULL;
static SDL_GLContext g_gl_context = NULL;
static TTF_Font* g_font = NULL;
static bool g_running = true;
static int g_selected_tab = 0;
static const char* g_tab_names[] = {
    "Dashboard",
    "Communications", 
    "Engine Vitals",
    "Fuel & Spark",
    "VE Table Editor",
    "Settings"
};

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

// VE Table Editor state
static ImGuiVETable* g_ve_table = NULL;
static ImGuiVE3DView g_ve_3d_view = {0};
static ImGuiVETexture* g_ve_texture = NULL;
static bool g_ve_table_initialized = false;
static bool g_show_2d_view = true;
static bool g_show_3d_view = false;
static bool g_show_table_editor = false;
static int g_selected_cell_x = -1;
static int g_selected_cell_y = -1;
static char g_cell_edit_buffer[32] = "";
static bool g_cell_editing = false;

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
void render_dashboard_tab();
void render_communications_tab();
void render_engine_vitals_tab();
void render_fuel_spark_tab();
void render_ve_table_tab();
void render_settings_tab();
void handle_communications_buttons();
void init_ve_table_editor();
void cleanup_ve_table_editor();
void render_ve_table_2d_view();
void render_ve_table_3d_view();
void render_ve_table_editor_view();

int main(int argc, char* argv[]) {
    printf("MegaTunix Redux - ImGui Version\n");
    printf("By J. Andruczyk\n");
    printf("(ImGui Version)\n\n");

    // Initialize SDL
    if (!init_sdl()) {
        fprintf(stderr, "Failed to initialize SDL\n");
        return 1;
    }

    // Initialize OpenGL
    if (!init_opengl()) {
        fprintf(stderr, "Failed to initialize OpenGL\n");
        cleanup_sdl();
        return 1;
    }

    // Initialize TTF
    if (!init_ttf()) {
        fprintf(stderr, "Failed to initialize TTF\n");
        cleanup_opengl();
        cleanup_sdl();
        return 1;
    }

    // Initialize ImGui
    if (!init_imgui()) {
        fprintf(stderr, "Failed to initialize ImGui\n");
        cleanup_ttf();
        cleanup_opengl();
        cleanup_sdl();
        return 1;
    }

    // Initialize ECU communication
    if (!init_ecu_communication()) {
        fprintf(stderr, "Failed to initialize ECU communication\n");
        cleanup_imgui();
        cleanup_ttf();
        cleanup_opengl();
        cleanup_sdl();
        return 1;
    }

    // Initialize VE Table Editor
    init_ve_table_editor();

    printf("Initialization successful!\n");

    // Main loop
    while (g_running) {
        handle_events();
        update();
        render();
    }

    // Cleanup
    cleanup_ve_table_editor();
    cleanup_ecu_communication();
    cleanup_imgui();
    cleanup_ttf();
    cleanup_opengl();
    cleanup_sdl();

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
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    if (!ImGui_ImplSDL2_InitForOpenGL(g_window, g_gl_context)) {
        fprintf(stderr, "ImGui_ImplSDL2_InitForOpenGL failed\n");
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 130")) {
        fprintf(stderr, "ImGui_ImplOpenGL3_Init failed\n");
        return false;
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
}

void handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        
        switch (event.type) {
            case SDL_QUIT:
                g_running = false;
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    g_running = false;
                }
                break;
        }
    }
}

void update() {
    // Update ECU status
    if (g_ecu_context) {
        g_ecu_connected = ecu_is_connected(g_ecu_context);
        ecu_get_status(g_ecu_context, g_ecu_status, sizeof(g_ecu_status));
        ecu_get_data(g_ecu_context, &g_ecu_data);
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
}

void render() {
    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Render main window
    render_main_window();

    // Render
    ImGui::Render();
    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }

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
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "MEGATUNIX REDUX %s", g_ecu_status);
    
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
        case 0: render_dashboard_tab(); break;
        case 1: render_communications_tab(); break;
        case 2: render_engine_vitals_tab(); break;
        case 3: render_fuel_spark_tab(); break;
        case 4: render_ve_table_tab(); break;
        case 5: render_settings_tab(); break;
    }

    ImGui::EndChild();
    ImGui::End();
}

void render_dashboard_tab() {
    ImGui::Text("Dashboard - Engine Monitoring");
    ImGui::Separator();
    
    // Engine RPM
    ImGui::Text("Engine RPM: %.0f", g_ecu_data.rpm);
    
    // Coolant Temperature
    ImGui::Text("Coolant Temp: %.1f°C", g_ecu_data.coolant_temp);
    
    // Intake Air Temperature
    ImGui::Text("Intake Air Temp: %.1f°C", g_ecu_data.intake_temp);
    
    // Manifold Absolute Pressure
    ImGui::Text("MAP: %.1f kPa", g_ecu_data.map);
    
    // Throttle Position
    ImGui::Text("Throttle: %.1f%%", g_ecu_data.tps);
    
    // Battery Voltage
    ImGui::Text("Battery: %.1fV", g_ecu_data.battery_voltage);
}

void render_communications_tab() {
    ImGui::Text("Communications - ECU Connection");
    ImGui::Separator();
    
    // Connection status
    ImGui::Text("Status: %s", g_ecu_status);
    ImGui::Text("Connected: %s", g_ecu_connected ? "Yes" : "No");
    
    ImGui::Separator();
    
    // Buttons
    if (ImGui::Button(g_locate_port_button_pressed ? "Scanning..." : "Locate Port", ImVec2(120, 30))) {
        g_locate_port_button_pressed = true;
        g_button_press_timer = 60; // 1 second at 60 FPS
        strcpy(g_button_status_text, "Scanning for available ports...");
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button(g_reverse_connect_button_pressed ? "Connecting..." : "Reverse Connect", ImVec2(120, 30))) {
        g_reverse_connect_button_pressed = true;
        g_button_press_timer = 60;
        strcpy(g_button_status_text, "Attempting reverse connection...");
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button(g_reset_io_button_pressed ? "Resetting..." : "Reset I/O Status Counters", ImVec2(150, 30))) {
        g_reset_io_button_pressed = true;
        g_button_press_timer = 60;
        strcpy(g_button_status_text, "Resetting I/O status counters...");
    }
    
    // Status text
    if (strlen(g_button_status_text) > 0) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", g_button_status_text);
    }
    
    // Detected ports (placeholder)
    ImGui::Separator();
    ImGui::Text("Detected Ports:");
    ImGui::Text("  /dev/ttyUSB0 - FTDI Serial");
    ImGui::Text("  /dev/ttyACM0 - Arduino");
}

void render_engine_vitals_tab() {
    ImGui::Text("Engine Vitals - Real-time Monitoring");
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
    ImGui::Text("Barometric: %.1f kPa", g_ecu_data.barometric_pressure);
    ImGui::Text("Fuel Pressure: %.1f kPa", g_ecu_data.fuel_pressure);
}

void render_fuel_spark_tab() {
    ImGui::Text("Fuel & Spark - Tuning Parameters");
    ImGui::Separator();
    
    ImGui::Text("Fuel System");
    ImGui::Text("Injector Pulse Width: %.2f ms", g_ecu_data.injector_pulse_width);
    ImGui::Text("Fuel Pressure: %.1f kPa", g_ecu_data.fuel_pressure);
    ImGui::Text("AFR: %.1f", g_ecu_data.afr);
    
    ImGui::Separator();
    
    ImGui::Text("Ignition System");
    ImGui::Text("Advance: %.1f°", g_ecu_data.ignition_advance);
    ImGui::Text("Dwell: %.2f ms", g_ecu_data.dwell);
}

void render_settings_tab() {
    ImGui::Text("Settings - Application Configuration");
    ImGui::Separator();
    
    static char port_name[64] = "/dev/ttyUSB0";
    static int baud_rate = 115200;
    static bool auto_connect = false;
    
    ImGui::Text("Serial Port Settings");
    ImGui::InputText("Port", port_name, sizeof(port_name));
    ImGui::Combo("Baud Rate", &baud_rate, "9600\0 19200\0 38400\0 57600\0 115200\0\0");
    ImGui::Checkbox("Auto-connect on startup", &auto_connect);
    
    ImGui::Separator();
    
    if (ImGui::Button("Save Settings", ImVec2(120, 30))) {
        // TODO: Save settings
        ImGui::OpenPopup("Settings Saved");
    }
    
    if (ImGui::BeginPopupModal("Settings Saved", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Settings have been saved successfully!");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void handle_communications_buttons() {
    // This function handles the actual button logic
    // For now, it's just a placeholder that simulates the button actions
    
    if (g_locate_port_button_pressed && g_button_press_timer == 30) {
        // Simulate port detection
        strcpy(g_button_status_text, "Found 2 ports: /dev/ttyUSB0, /dev/ttyACM0");
    }
    
    if (g_reverse_connect_button_pressed && g_button_press_timer == 30) {
        // Simulate connection attempt
        if (g_ecu_context) {
            // Try to connect
            strcpy(g_button_status_text, "Connection attempt completed");
        }
    }
    
    if (g_reset_io_button_pressed && g_button_press_timer == 30) {
        // Simulate reset
        strcpy(g_button_status_text, "I/O status counters reset");
    }
}

// VE Table Editor Functions
void init_ve_table_editor() {
    // Create VE table with 12x12 size (typical for Speeduino)
    g_ve_table = imgui_ve_table_create(12, 12);
    if (!g_ve_table) {
        fprintf(stderr, "Failed to create VE table\n");
        return;
    }
    
    // Load demo data for testing
    imgui_ve_table_load_demo_data(g_ve_table);
    
    // Initialize 3D view
    g_ve_3d_view.rotation_x = 30.0f;
    g_ve_3d_view.rotation_y = 45.0f;
    g_ve_3d_view.zoom = 3.0f;
    g_ve_3d_view.pan_x = 0.0f;
    g_ve_3d_view.pan_y = 0.0f;
    g_ve_3d_view.wireframe_mode = false;
    g_ve_3d_view.show_grid = true;
    g_ve_3d_view.show_axes = true;
    g_ve_3d_view.opacity = 0.8f;
    
    // Create texture for 2D view
    g_ve_texture = imgui_ve_texture_create(512, 512);
    if (g_ve_texture) {
        imgui_ve_texture_update(g_ve_texture, g_ve_table, &g_ve_3d_view);
    }
    
    g_ve_table_initialized = true;
    printf("VE Table Editor initialized successfully\n");
}

void cleanup_ve_table_editor() {
    if (g_ve_texture) {
        imgui_ve_texture_destroy(g_ve_texture);
        g_ve_texture = NULL;
    }
    
    if (g_ve_table) {
        imgui_ve_table_destroy(g_ve_table);
        g_ve_table = NULL;
    }
    
    g_ve_table_initialized = false;
}

void render_ve_table_tab() {
    if (!g_ve_table_initialized || !g_ve_table) {
        ImGui::Text("VE Table Editor - Initializing...");
        return;
    }
    
    ImGui::Text("VE Table Editor - Volumetric Efficiency Tuning");
    ImGui::Separator();
    
    // View mode selection
    ImGui::Text("View Mode:");
    ImGui::SameLine();
    if (ImGui::RadioButton("2D Heatmap", g_show_2d_view)) {
        g_show_2d_view = true;
        g_show_3d_view = false;
        g_show_table_editor = false;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("3D View", g_show_3d_view)) {
        g_show_2d_view = false;
        g_show_3d_view = true;
        g_show_table_editor = false;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Table Editor", g_show_table_editor)) {
        g_show_2d_view = false;
        g_show_3d_view = false;
        g_show_table_editor = true;
    }
    
    ImGui::Separator();
    
    // Table information
    ImGui::Text("Table Size: %dx%d", g_ve_table->width, g_ve_table->height);
    ImGui::Text("Value Range: %.1f - %.1f", g_ve_table->min_value, g_ve_table->max_value);
    
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
        imgui_ve_table_load_demo_data(g_ve_table);
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
    
    // Render the selected view
    if (g_show_2d_view) {
        render_ve_table_2d_view();
    } else if (g_show_3d_view) {
        render_ve_table_3d_view();
    } else if (g_show_table_editor) {
        render_ve_table_editor_view();
    }
}

void render_ve_table_2d_view() {
    ImGui::Text("2D Heatmap View");
    ImGui::Text("X-Axis: RPM (%.0f - %.0f)", g_ve_table->x_axis[0], g_ve_table->x_axis[g_ve_table->width - 1]);
    ImGui::Text("Y-Axis: MAP (%.1f - %.1f kPa)", g_ve_table->y_axis[0], g_ve_table->y_axis[g_ve_table->height - 1]);
    
    // 3D view controls
    ImGui::Separator();
    ImGui::Text("3D View Controls:");
    ImGui::SliderFloat("Rotation X", &g_ve_3d_view.rotation_x, -90.0f, 90.0f);
    ImGui::SliderFloat("Rotation Y", &g_ve_3d_view.rotation_y, -180.0f, 180.0f);
    ImGui::SliderFloat("Zoom", &g_ve_3d_view.zoom, 1.0f, 10.0f);
    ImGui::SliderFloat("Opacity", &g_ve_3d_view.opacity, 0.1f, 1.0f);
    ImGui::Checkbox("Wireframe Mode", &g_ve_3d_view.wireframe_mode);
    ImGui::Checkbox("Show Grid", &g_ve_3d_view.show_grid);
    ImGui::Checkbox("Show Axes", &g_ve_3d_view.show_axes);
    
    // Update texture if needed
    if (g_ve_texture) {
        imgui_ve_texture_update(g_ve_texture, g_ve_table, &g_ve_3d_view);
    }
    
    // Display the texture
    if (g_ve_texture && g_ve_texture->initialized) {
        ImGui::Separator();
        ImGui::Text("Heatmap Visualization:");
        
        // Calculate display size
        float display_width = ImGui::GetWindowWidth() - 40;
        float display_height = display_width * 0.75f; // 4:3 aspect ratio
        
        // Display the texture
        ImGui::Image((void*)(intptr_t)g_ve_texture->texture_id, 
                    ImVec2(display_width, display_height));
        
        // Color legend
        ImGui::Text("Color Legend:");
        ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "Blue: Low VE");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Green: Medium VE");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Red: High VE");
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
    ImGui::SliderFloat("Zoom", &g_ve_3d_view.zoom, 1.0f, 10.0f);
    ImGui::SliderFloat("Pan X", &g_ve_3d_view.pan_x, -2.0f, 2.0f);
    ImGui::SliderFloat("Pan Y", &g_ve_3d_view.pan_y, -2.0f, 2.0f);
    ImGui::SliderFloat("Opacity", &g_ve_3d_view.opacity, 0.1f, 1.0f);
    ImGui::Checkbox("Wireframe Mode", &g_ve_3d_view.wireframe_mode);
    ImGui::Checkbox("Show Grid", &g_ve_3d_view.show_grid);
    ImGui::Checkbox("Show Axes", &g_ve_3d_view.show_axes);
    
    // 3D rendering area
    ImGui::Separator();
    ImGui::Text("3D Rendering Area:");
    
    // Calculate rendering area
    float render_width = ImGui::GetWindowWidth() - 40;
    float render_height = render_width * 0.75f;
    
    // Create a child window for OpenGL rendering
    ImGui::BeginChild("3DRenderArea", ImVec2(render_width, render_height), true);
    
    // Get the draw list for this child window
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 window_size = ImGui::GetWindowSize();
    
    // For now, draw a placeholder rectangle
    // TODO: Implement actual OpenGL rendering in this area
    draw_list->AddRectFilled(
        ImVec2(window_pos.x + 10, window_pos.y + 10),
        ImVec2(window_pos.x + window_size.x - 10, window_pos.y + window_size.y - 10),
        IM_COL32(50, 50, 50, 255)
    );
    
    draw_list->AddText(
        ImVec2(window_pos.x + window_size.x * 0.5f - 50, window_pos.y + window_size.y * 0.5f - 10),
        IM_COL32(200, 200, 200, 255),
        "3D View Placeholder"
    );
    
    ImGui::EndChild();
    
    ImGui::Text("Note: Full 3D rendering will be implemented in a future update.");
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
            imgui_ve_table_set_value(g_ve_table, edit_x, edit_y, edit_value);
            if (g_ve_texture) {
                imgui_ve_texture_update(g_ve_texture, g_ve_table, &g_ve_3d_view);
            }
        }
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Get Value", ImVec2(100, 30))) {
        if (edit_x >= 0 && edit_x < g_ve_table->width && 
            edit_y >= 0 && edit_y < g_ve_table->height) {
            edit_value = imgui_ve_table_get_value(g_ve_table, edit_x, edit_y);
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
            snprintf(cell_id, sizeof(cell_id), "##cell_%d_%d", x, y);
            
            // Get current value
            float value = imgui_ve_table_get_value(g_ve_table, x, y);
            
            // Create input field for each cell
            char value_str[16];
            snprintf(value_str, sizeof(value_str), "%.1f", value);
            
            if (ImGui::InputText(cell_id, value_str, sizeof(value_str), 
                                ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
                // Parse new value
                float new_value = atof(value_str);
                if (new_value >= 0.0f && new_value <= 200.0f) { // Reasonable VE range
                    imgui_ve_table_set_value(g_ve_table, x, y, new_value);
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