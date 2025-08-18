/*
 * Key Bindings Preferences - Stub implementation wired to config
 */

#include "../../include/ui/keybindings_prefs.h"
#include "../../include/utils/config.h"

static KeyBindingsPrefs g_prefs = {0};

bool keybindings_prefs_init(void) {
	// Basic defaults mapping to current hardcoded keys
	g_prefs.increment.key = SDLK_COMMA;
	g_prefs.decrement.key = SDLK_PERIOD;
	g_prefs.increase_by.key = SDLK_PLUS;
	g_prefs.decrease_by.key = SDLK_MINUS;
	g_prefs.scale_by.key = SDLK_ASTERISK;
	g_prefs.interpolate.key = SDLK_i;
	g_prefs.interpolate_h.key = SDLK_h;
	g_prefs.interpolate_v.key = SDLK_v;
	return true;
}

void keybindings_prefs_shutdown(void) {
}

void keybindings_prefs_get(KeyBindingsPrefs* out) {
	if (!out) return;
	*out = g_prefs;
}

void keybindings_prefs_set(const KeyBindingsPrefs* in) {
	if (!in) return;
	g_prefs = *in;
}


