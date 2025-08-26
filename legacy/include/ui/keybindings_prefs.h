/*
 * Key Bindings Preferences - foundation for customizable shortcuts
 */

#ifndef KEYBINDINGS_PREFS_H
#define KEYBINDINGS_PREFS_H

#include <stdbool.h>
#include <SDL2/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	SDL_Keycode key;
	SDL_Keymod mods;
} KeyChord;

typedef struct {
	KeyChord increment;
	KeyChord decrement;
	KeyChord increase_by;
	KeyChord decrease_by;
	KeyChord scale_by;
	KeyChord interpolate;
	KeyChord interpolate_h;
	KeyChord interpolate_v;
} KeyBindingsPrefs;

bool keybindings_prefs_init(void);
void keybindings_prefs_shutdown(void);

void keybindings_prefs_get(KeyBindingsPrefs* out);
void keybindings_prefs_set(const KeyBindingsPrefs* in);

#ifdef __cplusplus
}
#endif

#endif // KEYBINDINGS_PREFS_H


