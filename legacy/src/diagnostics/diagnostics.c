/*
 * Diagnostics - Stub implementation
 */

#include "../../include/diagnostics/diagnostics.h"
#include <string.h>

static DiagnosticEntry g_last = {0};

bool diagnostics_init(void) {
	memset(&g_last, 0, sizeof(g_last));
	return true;
}

void diagnostics_shutdown(void) {
	memset(&g_last, 0, sizeof(g_last));
}

void diagnostics_report(DiagnosticLevel level, int code, const char* message) {
	g_last.level = level;
	g_last.code = code;
	if (message) {
		strncpy(g_last.message, message, sizeof(g_last.message) - 1);
	}
}

bool diagnostics_get_last(DiagnosticEntry* out) {
	if (!out) return false;
	*out = g_last;
	return true;
}


