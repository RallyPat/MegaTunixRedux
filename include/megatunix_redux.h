/*
 * MegaTunix Redux - Modern ECU Tuning Software
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

#ifndef MEGATUNIX_REDUX_H
#define MEGATUNIX_REDUX_H

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
    #define PATH_SEPARATOR "\\"
    #define PATH_SEPARATOR_CHAR '\\'
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS
    #define PATH_SEPARATOR "/"
    #define PATH_SEPARATOR_CHAR '/'
#else
    #define PLATFORM_LINUX
    #define PATH_SEPARATOR "/"
    #define PATH_SEPARATOR_CHAR '/'
    #ifdef WAYLAND_SUPPORT
        #define DISPLAY_WAYLAND
    #else
        #define DISPLAY_X11
    #endif
#endif

// Platform-specific includes
#ifdef PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_ttf.h>
#else
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_ttf.h>
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Version information
#define MEGATUNIX_REDUX_VERSION_MAJOR 2
#define MEGATUNIX_REDUX_VERSION_MINOR 0
#define MEGATUNIX_REDUX_VERSION_PATCH 0
#define MEGATUNIX_REDUX_VERSION_STRING "2.0.0"

// Application configuration
#define APP_NAME "MegaTunix Redux"
#define APP_TITLE "MegaTunix Redux - Modern ECU Tuning Software"
#define DEFAULT_WINDOW_WIDTH 1280
#define DEFAULT_WINDOW_HEIGHT 720
#define DEFAULT_FPS 60

// Platform-specific paths
#ifdef PLATFORM_WINDOWS
    #define ASSETS_PATH "assets"
    #define CONFIG_PATH "config"
    #define LOG_PATH "logs"
#elif defined(PLATFORM_MACOS)
    #define ASSETS_PATH "MegaTunixRedux.app/Contents/Resources/assets"
    #define CONFIG_PATH "Library/Application Support/MegaTunixRedux"
    #define LOG_PATH "Library/Logs/MegaTunixRedux"
#else
    #define ASSETS_PATH "/usr/share/megatunix-redux/assets"
    #define CONFIG_PATH ".config/megatunix-redux"
    #define LOG_PATH ".local/share/megatunix-redux/logs"
#endif

// Color definitions
typedef struct {
    uint8_t r, g, b, a;
} Color;

#define COLOR_BLACK   (Color){0, 0, 0, 255}
#define COLOR_WHITE   (Color){255, 255, 255, 255}
#define COLOR_RED     (Color){255, 0, 0, 255}
#define COLOR_GREEN   (Color){0, 255, 0, 255}
#define COLOR_BLUE    (Color){0, 0, 255, 255}
#define COLOR_YELLOW  (Color){255, 255, 0, 255}
#define COLOR_CYAN    (Color){0, 255, 255, 255}
#define COLOR_MAGENTA (Color){255, 0, 255, 255}

// Application state
typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    bool running;
    bool demo_mode;
    bool debug_mode;
    int window_width;
    int window_height;
    uint32_t frame_count;
    uint32_t last_frame_time;
    char* config_path;
    char* assets_path;
    char* log_path;
} AppState;

// Global application state
extern AppState g_app;

// Function declarations
bool app_init(void);
void app_cleanup(void);
void app_run(void);
void app_handle_events(void);
void app_update(void);
void app_render(void);

// Cross-platform path functions
char* get_config_path(void);
char* get_assets_path(void);
char* get_log_path(void);
bool create_directories(const char* path);
char* path_join(const char* path1, const char* path2);

// Error handling
void log_error(const char* format, ...);
void log_info(const char* format, ...);
void log_debug(const char* format, ...);

// Platform-specific functions
bool init_sdl(void);
bool init_ttf(void);
void cleanup_sdl(void);
void cleanup_ttf(void);

// File system functions
bool file_exists(const char* path);
bool directory_exists(const char* path);
char* get_executable_path(void);
char* get_user_home_path(void);

#endif // MEGATUNIX_REDUX_H 