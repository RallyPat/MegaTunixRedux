/*
 * Window Manager - SDL2 Window and Event Management
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * Handles SDL2 window creation, event processing, and window management.
 */

#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "../megatunix_redux.h"
// Clay removed - will be replaced with ImGui
#include <SDL2/SDL.h>

// Window configuration
typedef struct {
    const char* title;
    int width;
    int height;
    int x;
    int y;
    Uint32 flags;
    bool fullscreen;
    bool resizable;
    bool vsync;
} WindowConfig;

// Event types
typedef enum {
    EVENT_NONE,
    EVENT_QUIT,
    EVENT_KEY_DOWN,
    EVENT_KEY_UP,
    EVENT_MOUSE_MOTION,
    EVENT_MOUSE_BUTTON_DOWN,
    EVENT_MOUSE_BUTTON_UP,
    EVENT_MOUSE_WHEEL,
    EVENT_WINDOW_RESIZE,
    EVENT_WINDOW_FOCUS
} EventType;

// Event data
typedef struct {
    EventType type;
    SDL_Event sdl_event;
    union {
        struct {
            SDL_Keycode key;
            SDL_Keymod mod;
        } key;
        struct {
            int x, y;
            int rel_x, rel_y;
        } mouse_motion;
        struct {
            int button;
            int x, y;
        } mouse_button;
        struct {
            int x, y;
        } mouse_wheel;
        struct {
            int width, height;
        } window_resize;
    } data;
} AppEvent;

// Function declarations
bool window_manager_init(const WindowConfig* config);
void window_manager_cleanup(void);
void window_manager_handle_events(void);
bool window_manager_poll_event(AppEvent* event);
void window_manager_present(void);

// Window management
void window_manager_set_title(const char* title);
void window_manager_set_size(int width, int height);
void window_manager_set_position(int x, int y);
void window_manager_toggle_fullscreen(void);
void window_manager_show(void);
void window_manager_hide(void);

// Event handling
void window_manager_process_event(const SDL_Event* sdl_event, AppEvent* app_event);
bool window_manager_should_quit(void);

// Utility functions
SDL_Window* window_manager_get_window(void);
SDL_Renderer* window_manager_get_renderer(void);
void window_manager_get_size(int* width, int* height);
void window_manager_get_position(int* x, int* y);

// Default configurations
extern const WindowConfig DEFAULT_WINDOW_CONFIG;

#endif // WINDOW_MANAGER_H 