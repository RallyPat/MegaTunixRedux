/*
 * Window Manager - SDL2 Window and Event Management Implementation
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * Handles SDL2 window creation, event processing, and window management.
 */

#include "../include/app/window_manager.h"
#include <stdio.h>
#include <stdlib.h>

// Global window manager state
static SDL_Window* g_window = NULL;
static SDL_Renderer* g_renderer = NULL;
static bool g_should_quit = false;

bool window_manager_init(const WindowConfig* config) {
    if (!config) {
        return false;
    }
    
    printf("[DEBUG] Window manager: Creating window with title: %s\n", config->title);
    printf("[DEBUG] Window manager: Window dimensions: %dx%d\n", config->width, config->height);
    
    // Create window
    g_window = SDL_CreateWindow(
        config->title,
        config->x,
        config->y,
        config->width,
        config->height,
        config->flags
    );
    
    if (!g_window) {
        printf("Failed to create window: %s\n", SDL_GetError());
        return false;
    }
    
    printf("[DEBUG] Window manager: Window created successfully: %p\n", (void*)g_window);
    
    // Create renderer
    Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
    if (config->vsync) {
        renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
    }
    
    g_renderer = SDL_CreateRenderer(g_window, -1, renderer_flags);
    if (!g_renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_window);
        g_window = NULL;
        return false;
    }
    
    printf("[DEBUG] Window manager: Renderer created successfully: %p\n", (void*)g_renderer);
    
    // Set renderer properties
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
    
    printf("Window manager initialized successfully\n");
    return true;
}

void window_manager_cleanup(void) {
    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = NULL;
    }
    
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
    }
    
    printf("Window manager cleaned up\n");
}

void window_manager_handle_events(ClayUIContext* ui_ctx) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                g_should_quit = true;
                break;
            case SDL_MOUSEMOTION:
                if (ui_ctx) {
                    clay_ui_handle_mouse_motion(ui_ctx, event.motion.x, event.motion.y);
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (ui_ctx) {
                    clay_ui_handle_mouse_button(ui_ctx, event.button.button, true);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (ui_ctx) {
                    clay_ui_handle_mouse_button(ui_ctx, event.button.button, false);
                }
                break;
        }
    }
}

bool window_manager_poll_event(AppEvent* event) {
    if (!event) {
        return false;
    }
    
    SDL_Event sdl_event;
    if (SDL_PollEvent(&sdl_event)) {
        window_manager_process_event(&sdl_event, event);
        return true;
    }
    
    event->type = EVENT_NONE;
    return false;
}

void window_manager_present(void) {
    if (g_renderer) {
        SDL_RenderPresent(g_renderer);
    }
}

void window_manager_set_title(const char* title) {
    if (g_window && title) {
        SDL_SetWindowTitle(g_window, title);
    }
}

void window_manager_set_size(int width, int height) {
    if (g_window) {
        SDL_SetWindowSize(g_window, width, height);
    }
}

void window_manager_set_position(int x, int y) {
    if (g_window) {
        SDL_SetWindowPosition(g_window, x, y);
    }
}

void window_manager_toggle_fullscreen(void) {
    if (g_window) {
        Uint32 flags = SDL_GetWindowFlags(g_window);
        if (flags & SDL_WINDOW_FULLSCREEN) {
            SDL_SetWindowFullscreen(g_window, 0);
        } else {
            SDL_SetWindowFullscreen(g_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        }
    }
}

void window_manager_show(void) {
    if (g_window) {
        SDL_ShowWindow(g_window);
    }
}

void window_manager_hide(void) {
    if (g_window) {
        SDL_HideWindow(g_window);
    }
}

void window_manager_process_event(const SDL_Event* sdl_event, AppEvent* app_event) {
    if (!sdl_event || !app_event) {
        return;
    }
    
    app_event->sdl_event = *sdl_event;
    
    switch (sdl_event->type) {
        case SDL_QUIT:
            app_event->type = EVENT_QUIT;
            break;
        case SDL_KEYDOWN:
            app_event->type = EVENT_KEY_DOWN;
            app_event->data.key.key = sdl_event->key.keysym.sym;
            app_event->data.key.mod = sdl_event->key.keysym.mod;
            break;
        case SDL_KEYUP:
            app_event->type = EVENT_KEY_UP;
            app_event->data.key.key = sdl_event->key.keysym.sym;
            app_event->data.key.mod = sdl_event->key.keysym.mod;
            break;
        case SDL_MOUSEMOTION:
            app_event->type = EVENT_MOUSE_MOTION;
            app_event->data.mouse_motion.x = sdl_event->motion.x;
            app_event->data.mouse_motion.y = sdl_event->motion.y;
            app_event->data.mouse_motion.rel_x = sdl_event->motion.xrel;
            app_event->data.mouse_motion.rel_y = sdl_event->motion.yrel;
            break;
        case SDL_MOUSEBUTTONDOWN:
            app_event->type = EVENT_MOUSE_BUTTON_DOWN;
            app_event->data.mouse_button.button = sdl_event->button.button;
            app_event->data.mouse_button.x = sdl_event->button.x;
            app_event->data.mouse_button.y = sdl_event->button.y;
            break;
        case SDL_MOUSEBUTTONUP:
            app_event->type = EVENT_MOUSE_BUTTON_UP;
            app_event->data.mouse_button.button = sdl_event->button.button;
            app_event->data.mouse_button.x = sdl_event->button.x;
            app_event->data.mouse_button.y = sdl_event->button.y;
            break;
        case SDL_MOUSEWHEEL:
            app_event->type = EVENT_MOUSE_WHEEL;
            app_event->data.mouse_wheel.x = sdl_event->wheel.x;
            app_event->data.mouse_wheel.y = sdl_event->wheel.y;
            break;
        case SDL_WINDOWEVENT:
            if (sdl_event->window.event == SDL_WINDOWEVENT_RESIZED) {
                app_event->type = EVENT_WINDOW_RESIZE;
                app_event->data.window_resize.width = sdl_event->window.data1;
                app_event->data.window_resize.height = sdl_event->window.data2;
            }
            break;
        default:
            app_event->type = EVENT_NONE;
            break;
    }
}

bool window_manager_should_quit(void) {
    return g_should_quit;
}

SDL_Window* window_manager_get_window(void) {
    return g_window;
}

SDL_Renderer* window_manager_get_renderer(void) {
    return g_renderer;
}

void window_manager_get_size(int* width, int* height) {
    if (g_window && width && height) {
        SDL_GetWindowSize(g_window, width, height);
    }
}

void window_manager_get_position(int* x, int* y) {
    if (g_window && x && y) {
        SDL_GetWindowPosition(g_window, x, y);
    }
} 