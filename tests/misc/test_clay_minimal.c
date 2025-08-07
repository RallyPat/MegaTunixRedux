#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "include/ui/clay.h"
#include "include/ui/clay_ui.h"

int main() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    
    if (TTF_Init() < 0) {
        printf("TTF initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "Clay Test - Single Window",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Load font
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);
    if (!font) {
        printf("Font loading failed: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Initialize Clay UI
    ClayUIContext* ui_ctx = clay_ui_init(renderer, font, 800, 600);
    if (!ui_ctx) {
        printf("Clay UI initialization failed\n");
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    printf("Clay test initialized successfully\n");
    
    // Main loop
    SDL_Event event;
    bool running = true;
    
    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
        
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
        SDL_RenderClear(renderer);
        
        // Begin Clay frame
        clay_ui_begin_frame(ui_ctx);
        
        // Simple Clay content
        CLAY({
            .id = CLAY_ID("test_container"),
            .layout = {
                .sizing = { .width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW() },
                .padding = { 20, 20, 20, 20 }
            },
            .backgroundColor = (Clay_Color){0.1f, 0.1f, 0.15f, 1.0f}
        }) {
            CLAY_TEXT(CLAY_STRING("Clay Test - Single Window"), CLAY_TEXT_CONFIG({
                .fontSize = 24,
                .textColor = (Clay_Color){1.0f, 1.0f, 1.0f, 1.0f},
                .textAlignment = CLAY_TEXT_ALIGN_CENTER
            }));
            
            CLAY_TEXT(CLAY_STRING("This should be in the main window, not a separate modal"), CLAY_TEXT_CONFIG({
                .fontSize = 16,
                .textColor = (Clay_Color){0.8f, 0.8f, 0.9f, 1.0f},
                .textAlignment = CLAY_TEXT_ALIGN_CENTER
            }));
        }
        
        // End Clay frame and render
        clay_ui_end_frame(ui_ctx);
        clay_ui_render(ui_ctx);
        
        // Present
        SDL_RenderPresent(renderer);
    }
    
    // Cleanup
    clay_ui_cleanup(ui_ctx);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    printf("Clay test completed\n");
    return 0;
} 