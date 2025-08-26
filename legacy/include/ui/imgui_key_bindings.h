#ifndef IMGUI_KEY_BINDINGS_H
#define IMGUI_KEY_BINDINGS_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Key binding definitions (matching professional ECU tuning software muscle memory)
#define KEY_SET_TO           '='
#define KEY_INCREMENT        '>'  // or ','
#define KEY_DECREMENT        '<'  // or '.'
#define KEY_INCREASE_BY      '+'
#define KEY_DECREASE_BY      '-'
#define KEY_SCALE_BY         '*'
#define KEY_INTERPOLATE      'I'
#define KEY_INTERPOLATE_H    'H'
#define KEY_INTERPOLATE_V    'V'
#define KEY_SMOOTH_CELLS     's'
#define KEY_GAUSSIAN_SMOOTH  'g'
#define KEY_MOVING_AVERAGE_SMOOTH 'm'
#define KEY_BILATERAL_SMOOTH 'b'
#define KEY_FILL_UP_RIGHT    'f'
#define KEY_COPY            SDLK_c  // CTRL-C
#define KEY_PASTE           SDLK_v  // CTRL-V

// Modifier combinations
#define MODIFIER_CTRL        KMOD_CTRL
#define MODIFIER_SHIFT       KMOD_SHIFT
#define MODIFIER_ALT         KMOD_ALT

// Table operation types
typedef enum {
    TABLE_OP_SET_TO,
    TABLE_OP_INCREMENT,
    TABLE_OP_DECREMENT,
    TABLE_OP_INCREASE_BY,
    TABLE_OP_DECREASE_BY,
    TABLE_OP_SCALE_BY,
    TABLE_OP_INTERPOLATE,
    TABLE_OP_INTERPOLATE_H,
    TABLE_OP_INTERPOLATE_V,
    TABLE_OP_SMOOTH_CELLS,
    TABLE_OP_GAUSSIAN_SMOOTH,
    TABLE_OP_MOVING_AVERAGE_SMOOTH,
    TABLE_OP_BILATERAL_SMOOTH,
    TABLE_OP_FILL_UP_RIGHT,
    TABLE_OP_RESET,
    TABLE_OP_UNDO,
    TABLE_OP_REDO,
    TABLE_OP_HELP,
    TABLE_OP_EDIT_MODE,
    TABLE_OP_FIND,
    TABLE_OP_REPLACE,
    TABLE_OP_REFRESH,
    TABLE_OP_ZOOM_IN,
    TABLE_OP_ZOOM_OUT,
    TABLE_OP_FIT_VIEW,
    TABLE_OP_TOGGLE_SELECTION,
    TABLE_OP_CLEAR_CELL,
    TABLE_OP_APPLY_CHANGES,
    TABLE_OP_CANCEL_OPERATION,
    TABLE_OP_COPY,
    TABLE_OP_PASTE,
    TABLE_OP_PASTE_SPECIAL_MULTIPLY_PERCENT,
    TABLE_OP_PASTE_SPECIAL_MULTIPLY_RAW,
    TABLE_OP_PASTE_SPECIAL_ADD,
    TABLE_OP_PASTE_SPECIAL_SUBTRACT,
    TABLE_OP_NONE
} TableOperation;

// Key binding state
typedef struct {
    bool ctrl_pressed;
    bool shift_pressed;
    bool alt_pressed;
    char last_key_pressed;
    uint32_t last_key_time;
    bool key_repeat_enabled;
    float increment_amount;
    int number_of_increments;
    float percent_increment;
    bool context_menu_open;
} TableKeyBindingState;

// Function declarations
void imgui_key_bindings_init(TableKeyBindingState* state);
void imgui_key_bindings_cleanup(TableKeyBindingState* state);
TableOperation imgui_key_bindings_process_key(TableKeyBindingState* state, SDL_Keycode key, SDL_Keymod mod);
void imgui_key_bindings_update_modifiers(TableKeyBindingState* state, SDL_Keymod mod);
bool imgui_key_bindings_is_table_focused(void);
void imgui_key_bindings_show_help_tooltip(void);
void imgui_key_bindings_set_table_focused(bool focused);
const char* imgui_key_bindings_get_operation_name(TableOperation op);

#ifdef __cplusplus
}
#endif

#endif // IMGUI_KEY_BINDINGS_H 