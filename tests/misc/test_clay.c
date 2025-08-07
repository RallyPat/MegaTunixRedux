#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define CLAY_IMPLEMENTATION
#include "include/ui/clay.h"

int main() {
    printf("Testing Clay library...\n");
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    
    if (TTF_Init() < 0) {
        printf("SDL_ttf initialization failed: %s\n", TTF_GetError());
        return 1;
    }
    
    // Create window and renderer
    SDL_Window* window = SDL_CreateWindow("Clay Test", 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
        800, 600, SDL_WINDOW_SHOWN);
    
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return 1;
    }
    
    // Load font
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);
    if (!font) {
        printf("Font loading failed: %s\n", TTF_GetError());
        return 1;
    }
    
    printf("SDL, SDL_ttf, window, renderer, and font initialized successfully\n");
    
    // Test Clay initialization
    printf("Testing Clay initialization...\n");
    
    // Set up Clay
    Clay_SetMaxElementCount(1000);
    Clay_SetMaxMeasureTextCacheWordCount(1000);
    
    // Allocate memory for Clay
    size_t memory_size = Clay_MinMemorySize();
    printf("Clay memory size: %zu\n", memory_size);
    
    void* arena_memory = malloc(memory_size);
    if (!arena_memory) {
        printf("Failed to allocate Clay memory\n");
        return 1;
    }
    
    // Create Clay arena
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(memory_size, arena_memory);
    printf("Clay arena created\n");
    
    // Initialize Clay context
    Clay_Dimensions layout_dimensions = {800.0f, 600.0f};
    Clay_ErrorHandler errorHandler = {
        .errorHandlerFunction = NULL,
        .userData = 0
    };
    
    Clay_Context* clay_ctx = Clay_Initialize(arena, layout_dimensions, errorHandler);
    if (!clay_ctx) {
        printf("Clay initialization failed\n");
        free(arena_memory);
        return 1;
    }
    
    printf("Clay initialized successfully\n");
    
    // Test Clay rendering
    printf("Testing Clay rendering...\n");
    
    Clay_SetCurrentContext(clay_ctx);
    Clay_SetLayoutDimensions(layout_dimensions);
    Clay_BeginLayout();
    
    // Render a simple rectangle
    CLAY({
        .id = CLAY_ID("test_rect"),
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_FIXED(200),
                .height = CLAY_SIZING_FIXED(100)
            }
        },
        .backgroundColor = (Clay_Color){255, 0, 0, 255}
    }) {
        // Empty
    }
    
    Clay_RenderCommandArray commands = Clay_EndLayout();
    printf("Clay rendering completed, got %d commands\n", commands.length);
    
    // Cleanup
    free(arena_memory);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    printf("Clay test completed successfully\n");
    return 0;
} 