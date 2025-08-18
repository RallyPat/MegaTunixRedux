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
    
    // Ignore pure modifier keys
    if (key == SDLK_LCTRL || key == SDLK_RCTRL ||
        key == SDLK_LSHIFT || key == SDLK_RSHIFT ||
        key == SDLK_LALT || key == SDLK_RALT) {
        return TABLE_OP_NONE;
    }

    // Prioritize clipboard shortcuts before character mapping
    // Only process Ctrl+key combinations when both Ctrl and the key are pressed together
    if (state->ctrl_pressed) {
        if (key == SDLK_c) return TABLE_OP_COPY;
        if (key == SDLK_v) return TABLE_OP_PASTE;
        // Don't process other keys when Ctrl is held to avoid accidental operations
        return TABLE_OP_NONE;
    }

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
            if (state->alt_pressed) {
                return TABLE_OP_INCREMENT; // Alt++ for alternative increment
            }
            return TABLE_OP_INCREASE_BY;
            
        case SDLK_MINUS:
        case SDLK_KP_MINUS:
            if (state->alt_pressed) {
                return TABLE_OP_DECREMENT; // Alt+- for alternative decrement
            }
            return TABLE_OP_DECREASE_BY;
            
        case SDLK_ASTERISK:
        case SDLK_KP_MULTIPLY:
            return TABLE_OP_SCALE_BY;
            
        case SDLK_i:
            return TABLE_OP_INTERPOLATE;
            
        case SDLK_h:
            return TABLE_OP_INTERPOLATE_H;
            
        case SDLK_v:
            return TABLE_OP_INTERPOLATE_V; // ctrl+v handled above
            
        case SDLK_s:
            return TABLE_OP_SMOOTH_CELLS;
            
        case SDLK_g:
            return TABLE_OP_GAUSSIAN_SMOOTH;
            
        case SDLK_m:
            return TABLE_OP_MOVING_AVERAGE_SMOOTH;
            
        case SDLK_b:
            return TABLE_OP_BILATERAL_SMOOTH;
            
        case SDLK_f:
            return TABLE_OP_FILL_UP_RIGHT;
            
        // Additional professional key bindings
        case SDLK_r:
            return TABLE_OP_RESET; // Reset selected cells to default
            
        case SDLK_z:
            return TABLE_OP_UNDO; // Undo last action
            
        case SDLK_y:
            return TABLE_OP_REDO; // Redo last undone action
            
        // Function keys for quick operations
        case SDLK_F1:
            return TABLE_OP_HELP; // Show help/tooltip
            
        case SDLK_F2:
            return TABLE_OP_EDIT_MODE; // Toggle edit mode
            
        case SDLK_F3:
            return TABLE_OP_FIND; // Find/search functionality
            
        case SDLK_F4:
            return TABLE_OP_REPLACE; // Replace functionality
            
        case SDLK_F5:
            return TABLE_OP_REFRESH; // Refresh table data
            
        case SDLK_F6:
            return TABLE_OP_ZOOM_IN; // Zoom in on table
            
        case SDLK_F7:
            return TABLE_OP_ZOOM_OUT; // Zoom out on table
            
        case SDLK_F8:
            return TABLE_OP_FIT_VIEW; // Fit table to view
            
        // Space bar for quick selection toggle
        case SDLK_SPACE:
            if (!state->ctrl_pressed && !state->alt_pressed) {
                return TABLE_OP_TOGGLE_SELECTION; // Toggle cell selection
            }
            break;
            
        // Delete and Backspace for cell operations
        case SDLK_DELETE:
        case SDLK_BACKSPACE:
            if (!state->ctrl_pressed && !state->alt_pressed) {
                return TABLE_OP_CLEAR_CELL; // Clear cell value
            }
            break;
            
        // Enter key for applying changes
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            if (!state->ctrl_pressed && !state->alt_pressed) {
                return TABLE_OP_APPLY_CHANGES; // Apply current changes
            }
            break;
            
        // Escape key for canceling operations
        case SDLK_ESCAPE:
            if (!state->ctrl_pressed && !state->alt_pressed) {
                return TABLE_OP_CANCEL_OPERATION; // Cancel current operation
            }
            break;
            
        // Number keys for quick value setting (when not typing)
        case SDLK_1:
        case SDLK_2:
        case SDLK_3:
        case SDLK_4:
        case SDLK_5:
        case SDLK_6:
        case SDLK_7:
        case SDLK_8:
        case SDLK_9:
        case SDLK_0:
            if (!state->ctrl_pressed && !state->alt_pressed) {
                // Quick value setting - could be implemented as a special mode
                // For now, return NONE to let the main input system handle it
                return TABLE_OP_NONE;
            }
            break;
            
        default:
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
        case TABLE_OP_GAUSSIAN_SMOOTH: return "Gaussian Smooth";
        case TABLE_OP_MOVING_AVERAGE_SMOOTH: return "Moving Average Smooth";
        case TABLE_OP_BILATERAL_SMOOTH: return "Bilateral Smooth";
        case TABLE_OP_FILL_UP_RIGHT: return "Fill Up and Right";
        case TABLE_OP_RESET: return "Reset";
        case TABLE_OP_UNDO: return "Undo";
        case TABLE_OP_REDO: return "Redo";
        case TABLE_OP_HELP: return "Help";
        case TABLE_OP_EDIT_MODE: return "Edit Mode";
        case TABLE_OP_FIND: return "Find";
        case TABLE_OP_REPLACE: return "Replace";
        case TABLE_OP_REFRESH: return "Refresh";
        case TABLE_OP_ZOOM_IN: return "Zoom In";
        case TABLE_OP_ZOOM_OUT: return "Zoom Out";
        case TABLE_OP_FIT_VIEW: return "Fit View";
        case TABLE_OP_TOGGLE_SELECTION: return "Toggle Selection";
        case TABLE_OP_CLEAR_CELL: return "Clear Cell";
        case TABLE_OP_APPLY_CHANGES: return "Apply Changes";
        case TABLE_OP_CANCEL_OPERATION: return "Cancel Operation";
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