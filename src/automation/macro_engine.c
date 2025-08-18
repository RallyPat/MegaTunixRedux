/*
 * Macro Engine - Stub implementation
 */

#include "../../include/automation/macro_engine.h"
#include <string.h>

typedef struct {
	char name[128];
	MacroEvent events[1024];
	unsigned int num_events;
} MacroBuffer;

static MacroBuffer g_recording = {0};
static bool g_is_recording = false;
static bool g_is_playing = false;

bool macro_engine_init(void) {
	g_is_recording = false;
	g_is_playing = false;
	memset(&g_recording, 0, sizeof(g_recording));
	return true;
}

void macro_engine_shutdown(void) {
	g_is_recording = false;
	g_is_playing = false;
}

void macro_begin_recording(const char* name) {
	g_is_recording = true;
	memset(&g_recording, 0, sizeof(g_recording));
	if (name) {
		strncpy(g_recording.name, name, sizeof(g_recording.name) - 1);
	}
}

bool macro_is_recording(void) {
	return g_is_recording;
}

void macro_stop_recording(void) {
	g_is_recording = false;
}

void macro_record_event(const MacroEvent* evt) {
	if (!g_is_recording || !evt) return;
	if (g_recording.num_events >= (sizeof(g_recording.events)/sizeof(g_recording.events[0]))) return;
	g_recording.events[g_recording.num_events++] = *evt;
}

bool macro_play(const char* name) {
	(void)name; // Single-buffer stub for now
	if (g_recording.num_events == 0) return false;
	g_is_playing = true;
	// Playback logic will be integrated with event dispatch; stub returns immediately
	g_is_playing = false;
	return true;
}

bool macro_is_playing(void) {
	return g_is_playing;
}

void macro_stop_playback(void) {
	g_is_playing = false;
}


