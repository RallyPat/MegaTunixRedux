/*
 * Macro Engine - Foundation for recording and playing back editor actions
 */

#ifndef MACRO_ENGINE_H
#define MACRO_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	MACRO_ACTION_UNKNOWN = 0,
	MACRO_ACTION_TABLE_OP = 1,
	MACRO_ACTION_KEYPRESS = 2,
	MACRO_ACTION_SELECTION = 3
} MacroActionType;

typedef struct {
	MacroActionType type;
	uint32_t data_u32[4];
	float data_f32[4];
} MacroEvent;

bool macro_engine_init(void);
void macro_engine_shutdown(void);

void macro_begin_recording(const char* name);
bool macro_is_recording(void);
void macro_stop_recording(void);

void macro_record_event(const MacroEvent* evt);

bool macro_play(const char* name);
bool macro_is_playing(void);
void macro_stop_playback(void);

#ifdef __cplusplus
}
#endif

#endif // MACRO_ENGINE_H


