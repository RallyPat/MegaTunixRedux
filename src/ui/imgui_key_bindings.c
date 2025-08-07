#include "../../include/ui/imgui_key_bindings.h"
#include <string.h>

// Global state for table focus detection
static bool g_table_focused = false;

void imgui_key_bindings_init(TableKeyBindingState* state) {
    if (!state) return;
    
    memset(state, 0, sizeof(TableKeyBindingState));
    state->increment_amount = 1.0f;
    state->number_of_increments = 1;
    state->percent_increment = 5.0f; // 5% default
    state->key_repeat_enabled = true;
}

void imgui_key_bindings_cleanup(TableKeyBindingState* state) {
    // Nothing to clean up for now
    (void)state;
}

void imgui_key_bindings_update_modifiers(TableKeyBindingState* state, SDL_Keymod mod) {
    if (!state) return;
    
    state->ctrl_pressed = (mod & KMOD_CTRL) != 0;
    state->shift_pressed = (mod & KMOD_SHIFT) != 0;
    state->alt_pressed = (mod & KMOD_ALT) != 0;
}

TableOperation imgui_key_bindings_process_key(TableKeyBindingState* state, SDL_Keycode key, SDL_Keymod mod) {
    if (!state || !imgui_key_bindings_is_table_focused()) {
        return TABLE_OP_NONE;
    }
    
    // Update modifier state
    imgui_key_bindings_update_modifiers(state, mod);
    
    // Handle key repeat timing
    uint32_t current_time = SDL_GetTicks();
    if (state->key_repeat_enabled && key == state->last_key_pressed) {
        if (current_time - state->last_key_time < 150) { // 150ms repeat delay
            return TABLE_OP_NONE;
        }
    }
    
    state->last_key_pressed = key;
    state->last_key_time = current_time;
    
    // Process key bindings (matching professional ECU tuning software muscle memory)
    // Handle SDL key codes for reliable key detection
    switch (key) {
        // Character-based keys (for direct character input)
        case SDLK_EQUALS:
            return TABLE_OP_SET_TO;
            
        case SDLK_COMMA:
        case SDLK_GREATER:
            return TABLE_OP_INCREMENT;
            
        case SDLK_PERIOD:
        case SDLK_LESS:
            return TABLE_OP_DECREMENT;
            
        case SDLK_PLUS:
        case SDLK_KP_PLUS:
            return TABLE_OP_INCREASE_BY;
            
        case SDLK_MINUS:
        case SDLK_KP_MINUS:
            return TABLE_OP_DECREASE_BY;
            
        case SDLK_ASTERISK:
        case SDLK_KP_MULTIPLY:
            return TABLE_OP_SCALE_BY;
            
        case SDLK_i:
            return TABLE_OP_INTERPOLATE;
            
        case SDLK_h:
            return TABLE_OP_INTERPOLATE_H;
            
        case SDLK_v:
            return TABLE_OP_INTERPOLATE_V;
            
        case SDLK_s:
            return TABLE_OP_SMOOTH_CELLS;
            
        case SDLK_f:
            return TABLE_OP_FILL_UP_RIGHT;
            
        // Copy/Paste with modifier keys
        case SDLK_c:
            if (state->ctrl_pressed) {
                return TABLE_OP_COPY;
            }
            break;
            
        // Note: SDLK_v is handled above for interpolation, so we need to handle Ctrl+V separately
        default:
            // Handle Ctrl+V for paste operation
            if (key == SDLK_v && state->ctrl_pressed) {
                return TABLE_OP_PASTE;
            }
            break;
    }
    
    return TABLE_OP_NONE;
}

bool imgui_key_bindings_is_table_focused(void) {
    return g_table_focused;
}

void imgui_key_bindings_set_table_focused(bool focused) {
    g_table_focused = focused;
}

void imgui_key_bindings_show_help_tooltip(void) {
    // This function is now a placeholder - tooltip rendering is handled in main.cpp
    // where ImGui is available
}

const char* imgui_key_bindings_get_operation_name(TableOperation op) {
    switch (op) {
        case TABLE_OP_SET_TO: return "Set to";
        case TABLE_OP_INCREMENT: return "Increment";
        case TABLE_OP_DECREMENT: return "Decrement";
        case TABLE_OP_INCREASE_BY: return "Increase by";
        case TABLE_OP_DECREASE_BY: return "Decrease by";
        case TABLE_OP_SCALE_BY: return "Scale by";
        case TABLE_OP_INTERPOLATE: return "Interpolate";
        case TABLE_OP_INTERPOLATE_H: return "Interpolate Horizontal";
        case TABLE_OP_INTERPOLATE_V: return "Interpolate Vertical";
        case TABLE_OP_SMOOTH_CELLS: return "Smooth Cells";
        case TABLE_OP_FILL_UP_RIGHT: return "Fill Up and Right";
        case TABLE_OP_COPY: return "Copy";
        case TABLE_OP_PASTE: return "Paste";
        case TABLE_OP_PASTE_SPECIAL_MULTIPLY_PERCENT: return "Paste Special - Multiply by Percent";
        case TABLE_OP_PASTE_SPECIAL_MULTIPLY_RAW: return "Paste Special - Multiply by Raw";
        case TABLE_OP_PASTE_SPECIAL_ADD: return "Paste Special - Add";
        case TABLE_OP_PASTE_SPECIAL_SUBTRACT: return "Paste Special - Subtract";
        case TABLE_OP_NONE: return "None";
        default: return "Unknown";
    }
} 