/*
 * Diagnostics Foundation - error codes, health, and reporting
 */

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	DIAG_OK = 0,
	DIAG_WARNING = 1,
	DIAG_ERROR = 2
} DiagnosticLevel;

typedef struct {
	DiagnosticLevel level;
	int code;
	char message[128];
} DiagnosticEntry;

bool diagnostics_init(void);
void diagnostics_shutdown(void);

void diagnostics_report(DiagnosticLevel level, int code, const char* message);
bool diagnostics_get_last(DiagnosticEntry* out);

#ifdef __cplusplus
}
#endif

#endif // DIAGNOSTICS_H


